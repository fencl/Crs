#include "IR.h"
#include <iostream>
#include "../Error.h"

namespace Corrosive {

	IRFunction* IRModule::create_function(IRDataType returns) {
		std::unique_ptr<IRFunction> function = std::make_unique<IRFunction>();
		IRFunction* function_ptr = function.get();
		function_ptr->id = (unsigned int)functions.size();
		function_ptr->parent = this;
		function_ptr->yields = returns;
		functions.push_back(std::move(function));
		return function_ptr;
	}

	IRBlock* IRFunction::create_block(IRDataType accepts) {
		std::unique_ptr<IRBlock> block = std::make_unique<IRBlock>();
		IRBlock* block_ptr = block.get();
		block->id = (unsigned int)blocks_memory.size();
		block->parent = this;
		block->accepts = accepts;
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
		data_pool.back()->size += (unsigned int)size;
		return r;
	}

	void IRBlock::write_instruction(IRInstruction instruction) {
		IRInstruction* w = (IRInstruction*)reserve_data(sizeof(instruction));
		(*w) = instruction;
	}

	void IRBlock::write_const_type(IRDataType type) {
		IRDataType* w = (IRDataType*)reserve_data(sizeof(type));
		(*w) = type;
	}

	void IRBlock::write_value(size_t size, unsigned char* value) {
		unsigned char* w = reserve_data(size);
		memcpy(w, value, size);
	}



	void IRFunction::dump() {
		std::cout << "function " << id << " -> ";
		IRBlock::dump_data_type(yields);
		std::cout << "\n";

		for (auto b = blocks.begin(); b != blocks.end(); b++) {
			(*b)->dump();
		}
	}

	void IRBlock::memmove(std::list<std::unique_ptr<IRBlockData>>::iterator& pool, size_t& memoff, size_t off) {
		memoff += off;
		if ((*pool)->size <= memoff) {
			memoff = 0;
			pool++;
		}
	}

	void IRBlock::dump_data_type(IRDataType dt) {
		switch (dt) {
			case IRDataType::ibool:  std::cout << "bool"; break;
			case IRDataType::u8:  std::cout << "u8"; break;
			case IRDataType::u16: std::cout << "u16"; break;
			case IRDataType::u32: std::cout << "u32"; break;
			case IRDataType::u64: std::cout << "u64"; break;
			case IRDataType::i8:  std::cout << "i8"; break;
			case IRDataType::i16: std::cout << "i16"; break;
			case IRDataType::i32: std::cout << "i32"; break;
			case IRDataType::i64: std::cout << "i64"; break;
			case IRDataType::f32: std::cout << "f32"; break;
			case IRDataType::f64: std::cout << "f64"; break;
			case IRDataType::ptr:  std::cout << "ptr"; break;
			case IRDataType::none: std::cout << "none"; break;
			default: std::cout << "error";
		}
	}


	unsigned char* IRBlock::read_data(size_t s, std::list<std::unique_ptr<IRBlockData>>::iterator& pool, size_t& memoff) {
		unsigned char* r = &((*pool)->data[memoff]);
		memmove(pool, memoff, s);
		return r;
	}

#define read_data_type(T) ((T*)read_data(sizeof(T),mempool,memoff))

	void IRBlock::dump() {
		std::cout << " " << id << " [";
		dump_data_type(accepts);
		std::cout << "] -> ";
		dump_data_type(yields);
		std::cout << "\n";

		std::list<std::unique_ptr<IRBlockData>>::iterator mempool = data_pool.begin();
		size_t memoff = 0;
		while (mempool != data_pool.end()) {

			auto inst = read_data_type(IRInstruction);

			switch (*inst) {
			case IRInstruction::ret:
				std::cout << "   ret\n";
				break;
			case IRInstruction::sub:
				std::cout << "   sub\n";
				break;
			case IRInstruction::div:
				std::cout << "   div\n";
				break;
			case IRInstruction::rem:
				std::cout << "   rem\n";
				break;
			case IRInstruction::mul:
				std::cout << "   mul\n";
				break;
			case IRInstruction::add:
				std::cout << "   add\n";
				break;
			case IRInstruction::o_and:
				std::cout << "   and\n";
				break;
			case IRInstruction::o_or:
				std::cout << "   or\n";
				break;
			case IRInstruction::o_xor:
				std::cout << "   xor\n";
				break;

			case IRInstruction::eq:
				std::cout << "   eq\n";
				break;
			case IRInstruction::ne:
				std::cout << "   ne\n";
				break;
			case IRInstruction::gt:
				std::cout << "   gt\n";
				break;
			case IRInstruction::lt:
				std::cout << "   lt\n";
				break;
			case IRInstruction::ge:
				std::cout << "   ge\n";
				break;
			case IRInstruction::le:
				std::cout << "   le\n";
				break;
			case IRInstruction::accept:
				std::cout << "   accept\n";
				break;
			case IRInstruction::discard:
				std::cout << "   discard\n";
				break;
			case IRInstruction::jmp: {
				std::cout << "   jmp ";
				auto address = read_data_type(unsigned int);
				std::cout << *address << "\n";
				break;
			}
			case IRInstruction::load: {
				std::cout << "   load [";
				auto type = read_data_type(IRDataType);
				dump_data_type(*type);
				std::cout << "]\n";
				break;
			}
			case IRInstruction::jmpz: {
				std::cout << "   jmpz ";
				auto address = read_data_type(unsigned int);
				std::cout << *address << " ";
				address = read_data_type(unsigned int);
				std::cout << *address << "\n";
				break;
			}
			case IRInstruction::yield:
				std::cout << "   yield\n";
				break;
			case IRInstruction::value: {
				std::cout << "   const [";

				auto type = read_data_type(IRDataType);
				dump_data_type(*type);
				std::cout << "] ";

				switch (*type) {
				case IRDataType::ibool:  std::cout << ((*read_data_type(uint8_t))?"true":"false"); break;
				case IRDataType::u8:  std::cout << *read_data_type(uint8_t); break;
				case IRDataType::u16: std::cout << *read_data_type(uint16_t); break;
				case IRDataType::u32: std::cout << *read_data_type(uint32_t); break;
				case IRDataType::u64: std::cout << *read_data_type(uint64_t); break;

				case IRDataType::i8:  std::cout << *read_data_type(int8_t); break;
				case IRDataType::i16: std::cout << *read_data_type(int16_t); break;
				case IRDataType::i32: std::cout << *read_data_type(int32_t); break;
				case IRDataType::i64: std::cout << *read_data_type(int64_t); break;

				case IRDataType::f32: std::cout << *read_data_type(float); break;
				case IRDataType::f64: std::cout << *read_data_type(double); break;
				}
				std::cout << "\n";

				break;
			}
			}
		}
	}

#undef read_data_type



	void IRFunction :: assert_flow() {
		for (auto b = blocks.begin(); b != blocks.end(); b++) {
			(*b)->assert_flow();
		}

		for (auto b = return_blocks.begin(); b != return_blocks.end(); b++) {
			if ((*b)->yields != yields) {
				throw_ir_wrong_data_flow_error();
			}
		}
	}

	void IRBlock::assert_flow() {
		for (auto b = predecessors.begin(); b != predecessors.end(); b++) {
			if ((*b)->yields != accepts) {
				throw_ir_wrong_data_flow_error();
			}
		}
	}
}