#ifndef _il_crs_h
#define _il_crs_h

#include <vector>
#include <memory>
#include <set>
#include <list>
namespace Corrosive {

	void throw_il_wrong_data_flow_error();
	void throw_il_nothing_on_stack_error();
	void throw_il_remaining_stack_error();
	void throw_il_wrong_arguments_error();
	void throw_il_wrong_type_error();

	class ILFunction;
	class ILModule;
	
	enum class ILInstruction : unsigned char {
		value, add, sub, div, mul, rem, o_and, o_or, o_xor, load, store, accept, discard, yield, ret, jmp, jmpz, eq, ne, gt, ge, lt, le, local, member
	};

	enum class ILDataType : unsigned char {
		ibool,u8,i8,u16,i16,u32,i32,u64,i64,f32,f64,ptr,none,undefined
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
		unsigned int id;
		ILDataType yields = ILDataType::none;
		ILDataType accepts = ILDataType::none;
		ILFunction* parent;
		std::list<std::unique_ptr<ILBlockData>> data_pool;
		std::set<ILBlock*> predecessors;
		std::vector<std::pair<bool,ILDataType>> stack;

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
		ILType(ILDataType rv,unsigned int sz, unsigned int alg);
		ILDataType rvalue;
		unsigned int size_in_bytes = 0;
		unsigned int alignment_in_bytes = 0;
	};

	class ILStruct : public ILType {
	public:
		ILStruct();
		std::vector<std::pair<unsigned int, ILType*>> members;
		void add_member(ILType* type);
		void align_size();
	};

	class ILFunction {
	public:
		unsigned int id;
		ILModule* parent;
		ILType* returns = nullptr;

		std::vector<ILType*> locals;
		std::vector<ILBlock*> blocks;
		std::vector<std::unique_ptr<ILBlock>> blocks_memory;
		std::set<ILBlock*> return_blocks;

		ILBlock* create_block(ILDataType accepts);
		void append_block(ILBlock* block);
		void dump();
		bool assert_flow();
		unsigned int register_local(ILType* type);
	};

	class ILModule {
	public:
		std::vector<std::unique_ptr<ILFunction>> functions;
		std::vector<std::unique_ptr<ILType>> types;
		ILArchitecture architecture = ILArchitecture::x86_64;
		ILFunction* create_function(ILType* returns);
		ILType* create_primitive_type(ILDataType rv, unsigned int sz, unsigned int alg);
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

		void build_default_types();
	};

	class ILBuilder {
	public:
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

		static ILDataType arith_result(ILDataType l,ILDataType r);
		static bool build_add(ILBlock* block);
		static bool build_load(ILBlock* block, ILDataType type);
		static bool build_store(ILBlock* block);
		static bool build_local(ILBlock* block,unsigned int id);
		static bool build_member(ILBlock* block,ILStruct* type,unsigned int id);
		static bool build_and(ILBlock* block);
		static bool build_or(ILBlock* block);
		static bool build_xor(ILBlock* block);
		static bool build_eq(ILBlock* block);
		static bool build_ne(ILBlock* block);
		static bool build_gt(ILBlock* block);
		static bool build_lt(ILBlock* block);
		static bool build_ge(ILBlock* block);
		static bool build_le(ILBlock* block);
		static bool build_sub(ILBlock* block);
		static bool build_div(ILBlock* block);
		static bool build_rem(ILBlock* block);
		static bool build_mul(ILBlock* block);
		static bool build_accept(ILBlock* block);
		static bool build_discard(ILBlock* block);
		static bool build_yield(ILBlock* block);
		static bool build_ret(ILBlock* block);
		static bool build_jmp(ILBlock* block,ILBlock* address);
		static bool build_jmpz(ILBlock* block,ILBlock* ifz, ILBlock* ifnz);
	};
}

#endif
