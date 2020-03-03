#include "IR.h"

namespace Corrosive {

	IRFunction* IRModule::create_function() {
		std::unique_ptr<IRFunction> function = std::make_unique<IRFunction>();
		IRFunction* function_ptr = function.get();
		function_ptr->id = functions.size();
		function_ptr->parent = this;
		functions.push_back(std::move(function));
		return function_ptr;
	}

	IRBlock* IRFunction::create_block() {
		std::unique_ptr<IRBlock> block = std::make_unique<IRBlock>();
		IRBlock* block_ptr = block.get();
		block->id = blocks_memory.size();
		block->parent = this;
		blocks_memory.push_back(std::move(block));
		return block_ptr;
	}


	void IRFunction::append_block(IRBlock* block) {
		blocks.push_back(block);
	}

	unsigned char* IRBlock::reserve_data(size_t size) {
		if (data_pool.size() == 0) {
			data_pool.push_back(std::make_unique<IRBlockData>());
		}

		if (data_pool.back()->size + size >= 1024) {
			data_pool.push_back(std::make_unique<IRBlockData>());
		}

		unsigned char* r = &data_pool.back()->data[data_pool.back()->size];
		data_pool.back()->size += size;
		return r;
	}

	void IRBlock::write_instruction(IRInstruction instruction) {
		IRInstruction* w = (IRInstruction*)reserve_data(sizeof(instruction));
		(*w) = instruction;
	}

	void IRBlock::write_const_type(IRConstType type) {
		IRConstType* w = (IRConstType*)reserve_data(sizeof(type));
		(*w) = type;
	}

	void IRBlock::write_value(size_t size, unsigned char* value) {
		unsigned char* w = reserve_data(size);
		memcpy(w, value, size);
	}


}