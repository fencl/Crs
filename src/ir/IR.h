#ifndef _ir_crs_h
#define _ir_crs_h

#include <vector>
#include <memory>

namespace Corrosive {
	class IRFunction;
	class IRModule;

	enum class IRInstruction : unsigned char {
		Clear, Const, Add, Sub, Div, Mul, Load, Store
	};

	enum class IRConstType : unsigned char {
		u8,i8,u16,i16,u32,i32,u64,i64
	};

	struct IRBlockData {
		unsigned int size = 0;
		unsigned char data[1024];
	};

	class IRBlock {
	public:
		unsigned int id;
		IRFunction* parent;
		std::vector<std::unique_ptr<IRBlockData>> data_pool;

		void write_instruction(IRInstruction instruction);
		void write_value(size_t size,unsigned char* value);
		void write_const_type(IRConstType type);
	private:
		unsigned char* reserve_data(size_t size);
	};


	class IRFunction {
	public:
		unsigned int id;
		IRModule* parent;
		std::vector<IRBlock*> blocks;
		std::vector<std::unique_ptr<IRBlock>> blocks_memory;

		IRBlock* create_block();
		void append_block(IRBlock* block);
	};

	class IRModule {
	public:
		std::vector<std::unique_ptr<IRFunction>> functions;

		IRFunction* create_function();
	};

	class IRBuilder {
	public:
		static void build_clear(IRBlock* block);

		static void build_const_i8(IRBlock* block, int8_t   value);
		static void build_const_i16(IRBlock* block, int16_t  value);
		static void build_const_i32(IRBlock* block, int32_t  value);
		static void build_const_i64(IRBlock* block, int64_t  value);
		static void build_const_u8(IRBlock* block, uint8_t  value);
		static void build_const_u16(IRBlock* block, uint16_t value);
		static void build_const_u32(IRBlock* block, uint32_t value);
		static void build_const_u64(IRBlock* block, uint64_t value);
	};
}

#endif
