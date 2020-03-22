#ifndef _il_crs_h
#define _il_crs_h

#include <vector>
#include <memory>
#include <set>
#include <list>
#include <string_view>

namespace Corrosive {

	void throw_il_wrong_data_flow_error();
	void throw_il_nothing_on_stack_error();
	void throw_il_remaining_stack_error();
	void throw_il_wrong_arguments_error();
	void throw_il_wrong_type_error();

	class ILFunction;
	class ILModule;
	
	struct ILCtype {
		void* type;
		uint32_t ptr;
	};

	enum class ILInstruction : unsigned char {
		value, add, sub, div, mul, rem, o_and, o_or, o_xor, load, store, accept, discard, yield, ret, jmp, jmpz, eq, ne, gt, ge, lt, le, local, member, yield_type
	};

	enum class ILDataType : unsigned char {
		ibool, u8, i8, u16, i16, u32, i32, u64, i64, f32, f64, ptr, none, undefined, ctype
	};

	enum class ILArchitecture {
		x86_64,i386
	};

	struct ILBlockData {
		unsigned int size = 0;
		unsigned char data[1024];
	};

	class ILBlock {
	public:
		std::vector<bool> is_const;
		void push_const(bool c);
		bool test_const();
		void pop_const();

		unsigned int id;
		ILDataType yields = ILDataType::none;
		ILDataType accepts = ILDataType::none;
		ILFunction* parent;
		std::list<std::unique_ptr<ILBlockData>> data_pool;
		std::set<ILBlock*> predecessors;

		void write_instruction(ILInstruction instruction);
		void write_value(size_t size,unsigned char* value);
		void write_const_type(ILDataType type);

		template<typename T> inline T pop() {
			ILBlockData* bd = data_pool.back().get();
			if (bd->size == 0) {
				if (data_pool.size() > 1) {
					data_pool.pop_back();
					bd = data_pool.back().get();
				}
			}
			T res = *(T*)&bd->data[bd->size - sizeof(T)];
			bd->size -= sizeof(T);
			return res;
		}

		void dump();
		static void dump_data_type(ILDataType dt);
		bool assert_flow();
	private:
		unsigned char* reserve_data(size_t size);
		void memmove(std::list<std::unique_ptr<ILBlockData>>::iterator& pool, size_t& memoff, size_t off);
		unsigned char* read_data(size_t, std::list<std::unique_ptr<ILBlockData>>::iterator& pool, size_t& memoff);
	};

	class ILType {
	public:
		~ILType();
		ILType();
		ILType(ILDataType rv,unsigned int sz, unsigned int ct, unsigned int alg);
		ILDataType rvalue;
		unsigned int size_in_bytes = 0;
		unsigned int alignment_in_bytes = 0;

		unsigned int compile_time_size_in_bytes = 0;

		virtual int auto_compare(void* p1, void* p2);
		virtual void auto_move(void* src, void* dst);
	};

	class ILStruct : public ILType {
	public:
		ILStruct();
		std::vector<std::tuple<unsigned int /*runtime_offset*/, unsigned int /*compile_time_offset*/, ILType*>> member_vars;

		void			add_member(ILType* type);
		void			align_size();

		virtual int		compile_time_compare(void* p1, void* p2);
		virtual void	compile_time_move(void* src, void* dst);
	};

	class ILFunction {
	public:
		~ILFunction();
		unsigned int	id;
		ILModule*		parent;
		ILType*			returns = nullptr;
		bool			is_const = false;

		std::vector<ILType*>					locals;
		std::vector<ILBlock*>					blocks;
		std::vector<std::unique_ptr<ILBlock>>	blocks_memory;
		std::set<ILBlock*>						return_blocks;

		ILBlock*	 create_block(ILDataType accepts);
		void		 append_block(ILBlock* block);
		void		 dump();
		bool		 assert_flow();
		unsigned int register_local(ILType* type);
	};

	class ILEvaluator {
	public:
		using register_value = uint64_t;

		static const inline size_t stack_size = 1024*4;
		unsigned char memory_stack[stack_size];
		unsigned char register_stack[stack_size];
		unsigned char* memory_stack_pointer = memory_stack;
		unsigned char* register_stack_pointer = register_stack;

		std::vector<unsigned char*> local_ids;

		register_value yield;
		ILDataType yield_type;

		void	write_register_value_indirect(size_t size, void* value);
		void	pop_register_value_indirect(size_t size, void* into);
		void*	read_last_register_value_indirect(ILDataType rs);

		size_t	compile_time_register_size(ILDataType t);
		void	discard_last_register_type(ILDataType rs);
		
		std::vector<std::vector<void*>> on_stack;
		unsigned char*	stack_push();
		void			stack_pop(unsigned char* stack_pointer);
		void			stack_write(size_t size, void* from);
		unsigned char*	stack_reserve(size_t size);
		void			stack_push_pointer(void* ptr);

		template<typename T> inline T read_register_value() {
			return *(((T*)register_stack_pointer)-1);
		}

		template<typename T> inline T pop_register_value() {
			register_stack_pointer -= sizeof(T);
			return *((T*)register_stack_pointer);
		}

		template<typename T> inline void write_register_value(T v) {
			*((T*)register_stack_pointer) = v;
			register_stack_pointer += sizeof(T);
		}
	};

	class ILModule {
	public:
		std::vector<std::unique_ptr<ILFunction>> functions;
		std::vector<std::unique_ptr<ILType>> types;
		ILArchitecture architecture = ILArchitecture::x86_64;
		ILFunction* create_function(ILType* returns);
		ILType* create_primitive_type(ILDataType rv, unsigned int sz, unsigned int cs, unsigned int alg);
		ILStruct* create_struct_type();

		ILType* t_i8;
		ILType* t_u8;
		ILType* t_i16;
		ILType* t_u16;
		ILType* t_i32;
		ILType* t_u32;
		ILType* t_i64;
		ILType* t_u64;
		ILType* t_f32;
		ILType* t_f64;
		ILType* t_bool;
		ILType* t_ptr;
		ILType* t_void;

		ILType* t_type;

		void build_default_types();
	};

	class ILBuilder {
	public:

		static bool eval_const_ibool (ILEvaluator* eval_ctx, int8_t   value);
		static bool eval_const_i8    (ILEvaluator* eval_ctx, int8_t   value);
		static bool eval_const_i16   (ILEvaluator* eval_ctx, int16_t  value);
		static bool eval_const_i32   (ILEvaluator* eval_ctx, int32_t  value);
		static bool eval_const_i64   (ILEvaluator* eval_ctx, int64_t  value);
		static bool eval_const_u8    (ILEvaluator* eval_ctx, uint8_t  value);
		static bool eval_const_u16   (ILEvaluator* eval_ctx, uint16_t value);
		static bool eval_const_u32   (ILEvaluator* eval_ctx, uint32_t value);
		static bool eval_const_u64   (ILEvaluator* eval_ctx, uint64_t value);
		static bool eval_const_f32   (ILEvaluator* eval_ctx, float    value);
		static bool eval_const_f64   (ILEvaluator* eval_ctx, double   value);
		static bool eval_const_ptr   (ILEvaluator* eval_ctx, void*    value);
		static bool eval_const_ctype (ILEvaluator* eval_ctx, ILCtype value);

		static bool build_const_ibool (ILBlock* block, int8_t   value);
		static bool build_const_i8	  (ILBlock* block, int8_t   value);
		static bool build_const_i16	  (ILBlock* block, int16_t  value);
		static bool build_const_i32	  (ILBlock* block, int32_t  value);
		static bool build_const_i64	  (ILBlock* block, int64_t  value);
		static bool build_const_u8	  (ILBlock* block, uint8_t  value);
		static bool build_const_u16	  (ILBlock* block, uint16_t value);
		static bool build_const_u32	  (ILBlock* block, uint32_t value);
		static bool build_const_u64	  (ILBlock* block, uint64_t value);
		static bool build_const_f32	  (ILBlock* block, float    value);
		static bool build_const_f64	  (ILBlock* block, double   value);
		static bool build_const_ctype (ILBlock* block, ILCtype value);

		static bool eval_add(ILEvaluator* eval_ctx,ILDataType tl,ILDataType tr);
		static bool eval_load(ILEvaluator* eval_ctx, ILDataType type);
		static bool eval_store(ILEvaluator* eval_ctx, ILDataType type);
		static bool eval_local(ILEvaluator* eval_ctx, unsigned int id);
		static bool eval_member(ILEvaluator* eval_ctx, ILStruct* type, unsigned int id);
		static bool eval_and(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static bool eval_or(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static bool eval_xor(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static bool eval_eq(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static bool eval_ne(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static bool eval_gt(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static bool eval_lt(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static bool eval_ge(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static bool eval_le(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static bool eval_sub(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static bool eval_div(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static bool eval_rem(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static bool eval_mul(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static bool eval_accept(ILEvaluator* eval_ctx);
		static bool eval_discard(ILEvaluator* eval_ctx);
		static bool eval_yield(ILEvaluator* eval_ctx, ILDataType yt);

		static ILDataType arith_result(ILDataType l,ILDataType r);
		static bool build_add(ILBlock* block, ILDataType tl, ILDataType tr);
		static bool build_load(ILBlock* block, ILDataType type);
		static bool build_store(ILBlock* block, ILDataType type);
		static bool build_local(ILBlock* block,unsigned int id);
		static bool build_member(ILBlock* block,ILStruct* type,unsigned int id);
		static bool build_and(ILBlock* block, ILDataType tl, ILDataType tr);
		static bool build_or(ILBlock* block, ILDataType tl, ILDataType tr);
		static bool build_xor(ILBlock* block, ILDataType tl, ILDataType tr);
		static bool build_eq(ILBlock* block, ILDataType tl, ILDataType tr);
		static bool build_ne(ILBlock* block, ILDataType tl, ILDataType tr);
		static bool build_gt(ILBlock* block, ILDataType tl, ILDataType tr);
		static bool build_lt(ILBlock* block, ILDataType tl, ILDataType tr);
		static bool build_ge(ILBlock* block, ILDataType tl, ILDataType tr);
		static bool build_le(ILBlock* block, ILDataType tl, ILDataType tr);
		static bool build_sub(ILBlock* block, ILDataType tl, ILDataType tr);
		static bool build_div(ILBlock* block, ILDataType tl, ILDataType tr);
		static bool build_rem(ILBlock* block, ILDataType tl, ILDataType tr);
		static bool build_mul(ILBlock* block, ILDataType tl, ILDataType tr);
		static bool build_accept(ILBlock* block);
		static bool build_discard(ILBlock* block);
		static bool build_yield(ILBlock* block, ILDataType type);
		static bool build_yield_type(ILBlock* block,std::string_view name);
		static bool build_ret(ILBlock* block);
		static bool build_jmp(ILBlock* block,ILBlock* address);
		static bool build_jmpz(ILBlock* block,ILBlock* ifz, ILBlock* ifnz);
	};
}

#endif
