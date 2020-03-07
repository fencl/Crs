#ifndef _ir_crs_h
#define _ir_crs_h

#include <vector>
#include <memory>
#include <set>
#include <list>
namespace Corrosive {

	void throw_ir_wrong_data_flow_error();
	void throw_ir_nothing_on_stack_error();
	void throw_ir_remaining_stack_error();
	void throw_ir_wrong_arguments_error();
	void throw_ir_wrong_type_error();

	class IRFunction;
	class IRModule;
	
	enum class IRInstruction : unsigned char {
		value, add, sub, div, mul, rem, o_and, o_or, o_xor, load, store, accept, discard, yield, ret, jmp, jmpz, eq, ne, gt, ge, lt, le, local, member
	};

	enum class IRDataType : unsigned char {
		ibool,u8,i8,u16,i16,u32,i32,u64,i64,f32,f64,ptr,none,undefined
	};

	enum class IRArchitecture {
		x86_64,i386
	};

	struct IRBlockData {
		unsigned int size = 0;
		unsigned char data[1024];
	};

	class IRBlock {
	public:
		unsigned int id;
		IRDataType yields = IRDataType::none;
		IRDataType accepts = IRDataType::none;
		IRFunction* parent;
		std::list<std::unique_ptr<IRBlockData>> data_pool;
		std::set<IRBlock*> predecessors;
		std::vector<std::pair<bool,IRDataType>> stack;

		void write_instruction(IRInstruction instruction);
		void write_value(size_t size,unsigned char* value);
		void write_const_type(IRDataType type);

		template<typename T> inline T pop() {
			IRBlockData* bd = data_pool.back().get();
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
		static void dump_data_type(IRDataType dt);
		void assert_flow();
	private:
		unsigned char* reserve_data(size_t size);
		void memmove(std::list<std::unique_ptr<IRBlockData>>::iterator& pool, size_t& memoff, size_t off);
		unsigned char* read_data(size_t, std::list<std::unique_ptr<IRBlockData>>::iterator& pool, size_t& memoff);
	};

	class IRType {
	public:
		~IRType();
		IRType();
		IRType(IRDataType rv,unsigned int sz, unsigned int alg);
		IRDataType rvalue;
		unsigned int size_in_bytes = 0;
		unsigned int alignment_in_bytes = 0;
	};

	class IRStruct : public IRType {
	public:
		IRStruct();
		std::vector<std::pair<unsigned int, IRType*>> members;
		void add_member(IRType* type);
		void align_size();
	};

	class IRFunction {
	public:
		unsigned int id;
		IRModule* parent;
		IRType* returns = nullptr;

		std::vector<IRType*> locals;
		std::vector<IRBlock*> blocks;
		std::vector<std::unique_ptr<IRBlock>> blocks_memory;
		std::set<IRBlock*> return_blocks;

		IRBlock* create_block(IRDataType accepts);
		void append_block(IRBlock* block);
		void dump();
		void assert_flow();
		unsigned int register_local(IRType* type);
	};

	class IRModule {
	public:
		std::vector<std::unique_ptr<IRFunction>> functions;
		std::vector<std::unique_ptr<IRType>> types;
		IRArchitecture architecture = IRArchitecture::x86_64;
		IRFunction* create_function(IRType* returns);
		IRType* create_primitive_type(IRDataType rv, unsigned int sz, unsigned int alg);
		IRStruct* create_struct_type();

		IRType* t_i8;
		IRType* t_u8;
		IRType* t_i16;
		IRType* t_u16;
		IRType* t_i32;
		IRType* t_u32;
		IRType* t_i64;
		IRType* t_u64;
		IRType* t_f32;
		IRType* t_f64;
		IRType* t_bool;
		IRType* t_ptr;
		IRType* t_void;

		void build_default_types();
	};

	class IRBuilder {
	public:
		static void build_const_ibool (IRBlock* block, int8_t   value);
		static void build_const_i8	  (IRBlock* block, int8_t   value);
		static void build_const_i16	  (IRBlock* block, int16_t  value);
		static void build_const_i32	  (IRBlock* block, int32_t  value);
		static void build_const_i64	  (IRBlock* block, int64_t  value);
		static void build_const_u8	  (IRBlock* block, uint8_t  value);
		static void build_const_u16	  (IRBlock* block, uint16_t value);
		static void build_const_u32	  (IRBlock* block, uint32_t value);
		static void build_const_u64	  (IRBlock* block, uint64_t value);
		static void build_const_f32	  (IRBlock* block, float    value);
		static void build_const_f64	  (IRBlock* block, double   value);

		static IRDataType arith_result(IRDataType l,IRDataType r);
		static void build_add(IRBlock* block);
		static void build_load(IRBlock* block, IRDataType type);
		static void build_store(IRBlock* block);
		static void build_local(IRBlock* block,unsigned int id);
		static void build_member(IRBlock* block,IRStruct* type,unsigned int id);
		static void build_and(IRBlock* block);
		static void build_or(IRBlock* block);
		static void build_xor(IRBlock* block);
		static void build_eq(IRBlock* block);
		static void build_ne(IRBlock* block);
		static void build_gt(IRBlock* block);
		static void build_lt(IRBlock* block);
		static void build_ge(IRBlock* block);
		static void build_le(IRBlock* block);
		static void build_sub(IRBlock* block);
		static void build_div(IRBlock* block);
		static void build_rem(IRBlock* block);
		static void build_mul(IRBlock* block);
		static void build_accept(IRBlock* block);
		static void build_discard(IRBlock* block);
		static void build_yield(IRBlock* block);
		static void build_ret(IRBlock* block);
		static void build_jmp(IRBlock* block,IRBlock* address);
		static void build_jmpz(IRBlock* block,IRBlock* ifz, IRBlock* ifnz);
	};
}

#endif
