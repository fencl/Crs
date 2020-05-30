#ifndef _il_crs_h
#define _il_crs_h

#include <vector>
#include <memory>
#include <set>
#include <list>
#include <string>
#include <string_view>
#include <map>
#include <variant>

namespace Corrosive {

	void throw_il_wrong_data_flow_error();
	void throw_il_nothing_on_stack_error();
	void throw_il_remaining_stack_error();
	void throw_il_wrong_arguments_error();
	void throw_il_wrong_type_error();

	class ILFunction;
	class ILModule;

	enum class ILContext {
		compile, runtime, both
	};

	enum class ILInstruction : unsigned char {
		value, add, sub, div, mul, rem, bit_and, bit_or, bit_xor,
		load, store, store2, accept, discard, yield, ret, jmp, jmpz,
		eq, ne, gt, ge, lt, le, 
		local, forget, 
		fnptr, call, start, insintric, cast, 
		roffset, offset, vtable, duplicate, swap, swap2,
		memcpy, memcpy2, memcmp, memcmp2, rmemcmp, rmemcmp2
	};

	enum class ILDataType : unsigned char {
		ibool, u8, i8, u16, i16, u32, i32, u64, i64, size, f32, f64, ptr, none, undefined
	};

	enum class ILArchitecture : unsigned char {
		none,x86_64,i386
	};

	enum class ILAlignment : unsigned char {
		none,two,four,__4word__,eight,__8word__,word
	};

	
	struct ILSize {
		ILSize();
		ILSize(uint32_t a, uint16_t p);
		uint32_t absolute;
		uint16_t pointers;

		size_t eval(ILArchitecture arch, ILAlignment align= ILAlignment::none) const;
		static const ILSize single_ptr;
		static const ILSize double_ptr;
	};

	ILSize operator* (const ILSize& l, const uint32_t& r);
	ILSize operator+ (const ILSize& l, const uint32_t& r);
	ILSize operator- (const ILSize& l, const uint32_t& r);
	ILSize operator+ (const ILSize& l, const ILSize& r);
	ILSize operator- (const ILSize& l, const ILSize& r);

	struct ILSmallSize {
		ILSmallSize();
		ILSmallSize(uint8_t a, uint8_t p);
		uint8_t combined;
		size_t eval(ILArchitecture arch) const;
	};

	struct ILBlockData {
		unsigned int size = 0;
		unsigned char data[1024];
	};

	class ILBlock {
	public:
		std::string alias;

		uint32_t id;
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

		unsigned char* read_data(size_t, std::list<std::unique_ptr<ILBlockData>>::iterator& pool, size_t& memoff);
		unsigned char* reserve_data(size_t size);
		void memmove(std::list<std::unique_ptr<ILBlockData>>::iterator& pool, size_t& memoff, size_t off);
	};


	class ILFunction {
	public:
		~ILFunction();
		unsigned int	id;
		ILModule*		parent;
		bool			is_const = false;

		std::string alias;

		std::vector<ILDataType>						arguments;
		ILDataType									returns;
		std::vector<ILSize>							local_offsets;
		ILSize stack_size;

		std::vector<ILBlock*>						blocks;
		std::vector<std::unique_ptr<ILBlock>>		blocks_memory;
		std::set<ILBlock*>							return_blocks;

		ILBlock*	 create_block(ILDataType accepts);
		ILBlock*	 create_and_append_block(ILDataType accepts);
		void		 append_block(ILBlock* block);
		void		 dump();
		bool		 assert_flow();

		uint16_t register_local(ILSize size);
		uint32_t compile_time_stack=0; uint32_t runtime_stack=0;
	};

	using ilsize_t = uint64_t; // max size for all architectures

	enum class ILInsintric : unsigned char {
		build_array, build_reference, push_template, build_template, malloc, build_slice, debug_cursor, template_cast
	};

	class ILEvaluator {
	public:
		using register_value = uint64_t;

		ILModule* parent = nullptr;

		static const inline size_t stack_size = 1024*4;
		unsigned char memory_stack[stack_size];

		unsigned char register_stack[stack_size];
		unsigned char* register_stack_pointer = register_stack;

		unsigned char* memory_stack_pointer = memory_stack;
		unsigned char* memory_stack_base_pointer = memory_stack;

		ILFunction* callstack[1024];

		uint16_t callstack_depth = 0;

		register_value yield;

		void	write_register_value_indirect(size_t size, void* value);
		void	pop_register_value_indirect(size_t size, void* into);
		void*	read_last_register_value_indirect(ILDataType rs);

		size_t	compile_time_register_size(ILDataType t);
		void	discard_last_register_type(ILDataType rs);
		
		std::pair<unsigned char*, unsigned char*>	stack_push();
		void										stack_pop(std::pair<unsigned char*, unsigned char*> stack_pointer);
		unsigned char*								stack_reserve(size_t size);

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
		std::vector<std::unique_ptr<void*[]>> vtable_data;


		uint32_t register_vtable(std::unique_ptr<void* []> table);

		bool (*insintric_function[256])(ILEvaluator*);
		std::string insintric_function_name[256];

		ILArchitecture architecture = ILArchitecture::x86_64;
		ILFunction* create_function();
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
		static bool eval_const_type  (ILEvaluator* eval_ctx, void*    value);
		static bool eval_const_ptr   (ILEvaluator* eval_ctx, void*    value);
		static bool eval_const_size  (ILEvaluator* eval_ctx, size_t   value);

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
		static bool build_const_type  (ILBlock* block, void*    value);
		static bool build_const_size  (ILBlock* block, ILSize   value);

		static bool eval_load(ILEvaluator* eval_ctx, ILDataType type);
		static bool eval_store(ILEvaluator* eval_ctx, ILDataType type);
		static bool eval_store2(ILEvaluator* eval_ctx, ILDataType type);

		static bool eval_vtable(ILEvaluator* eval_ctx, uint32_t id);

		static bool eval_memcpy(ILEvaluator* eval_ctx, size_t size);
		static bool eval_memcpy2(ILEvaluator* eval_ctx, size_t size);
		static bool eval_memcmp(ILEvaluator* eval_ctx, size_t size);
		static bool eval_memcmp2(ILEvaluator* eval_ctx, size_t size);


		static bool eval_rmemcmp(ILEvaluator* eval_ctx, ILDataType type);
		static bool eval_rmemcmp2(ILEvaluator* eval_ctx, ILDataType type);

		static bool eval_local(ILEvaluator* eval_ctx, size_t offset);

		static bool eval_offset(ILEvaluator* eval_ctx, size_t offset);
		static bool eval_roffset(ILEvaluator* eval_ctx, ILDataType src, ILDataType dst, size_t offset);

		static bool eval_add(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
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
		static bool eval_cast(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static bool eval_accept(ILEvaluator* eval_ctx, ILDataType type);
		static bool eval_discard(ILEvaluator* eval_ctx, ILDataType type);
		static bool eval_yield(ILEvaluator* eval_ctx, ILDataType type);
		static bool eval_forget(ILEvaluator* eval_ctx, ILDataType type);
		static bool eval_fnptr(ILEvaluator* eval_ctx, ILFunction* fun);
		static bool eval_insintric(ILEvaluator* eval_ctx, ILInsintric fun);

		static bool eval_callstart(ILEvaluator* eval_ctx);
		static bool eval_call(ILEvaluator* eval_ctx, ILDataType rett, uint16_t argc);

		static bool eval_duplicate(ILEvaluator* eval_ctx, ILDataType type);

		static bool eval_swap(ILEvaluator* eval_ctx, ILDataType type);
		static bool eval_swap2(ILEvaluator* eval_ctx, ILDataType type1, ILDataType type2);

		static bool build_duplicate(ILBlock* block, ILDataType type);

		static bool build_swap(ILBlock* block, ILDataType type);
		static bool build_swap2(ILBlock* block, ILDataType type1, ILDataType type2);

		static ILDataType arith_result(ILDataType l,ILDataType r);

		static bool build_load(ILBlock* block, ILDataType type);
		static bool build_store(ILBlock* block, ILDataType type);
		static bool build_store2(ILBlock* block, ILDataType type);


		static bool build_memcpy(ILBlock* block, ILSize size);
		static bool build_memcpy2(ILBlock* block, ILSize size);

		static bool build_memcmp(ILBlock* block, ILSize size);
		static bool build_memcmp2(ILBlock* block, ILSize size);

		static bool build_offset(ILBlock* block, ILSize offest);
		static bool build_roffset(ILBlock* block, ILDataType src, ILDataType dst, ILSmallSize offset);

		static bool build_rmemcmp(ILBlock* block, ILDataType type);
		static bool build_rmemcmp2(ILBlock* block, ILDataType type);


		static bool build_local(ILBlock* block, uint16_t id);
		static bool build_vtable(ILBlock* block, uint32_t id);

		static bool build_add(ILBlock* block, ILDataType tl, ILDataType tr);
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
		static bool build_cast(ILBlock* block, ILDataType tl, ILDataType tr);
		static bool build_accept(ILBlock* block, ILDataType type);
		static bool build_discard(ILBlock* block, ILDataType type);
		static bool build_yield(ILBlock* block, ILDataType type);
		static bool build_forget(ILBlock* block, ILDataType type);
		static bool build_ret(ILBlock* block, ILDataType type);
		static bool build_fnptr(ILBlock* block, ILFunction* fun);
		static bool build_call(ILBlock* block, ILDataType type, uint16_t argc);
		static bool build_callstart(ILBlock* block);
		static bool build_jmp(ILBlock* block,ILBlock* address);
		static bool build_jmpz(ILBlock* block,ILBlock* ifz, ILBlock* ifnz);
		static bool build_insintric(ILBlock* block, ILInsintric fun);
	};
}

#endif
