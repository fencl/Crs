
#include "IL.h"
#include <iostream>
#include <algorithm>

namespace Corrosive {
	void throw_il_wrong_data_flow_error() {
		std::cerr << "Compiler Error:\n\tWrong data flow inside compiler IL";
		exit(1);
	}

	void throw_il_nothing_on_stack_error() {
		std::cerr << "Compiler Error:\n\tInstruction requires more argumens than the number of arguments on the stack";
		exit(1);
	}

	void throw_il_wrong_type_error() {
		std::cerr << "Compiler Error:\n\tPassed broken type";
		exit(1);
	}


	void throw_il_remaining_stack_error() {
		std::cerr << "Compiler Error:\n\tStack is not empty after terminator instruction";
		exit(1);
	}

	void throw_il_wrong_arguments_error() {
		std::cerr << "Compiler Error:\n\tInstruction cannot use argument(s) on the stack";
		exit(1);
	}

	ILFunction* ILModule::create_function(ILType* returns) {
		std::unique_ptr<ILFunction> function = std::make_unique<ILFunction>();
		ILFunction* function_ptr = function.get();
		function_ptr->id = (unsigned int)functions.size();
		function_ptr->parent = this;
		function_ptr->returns = returns;
		functions.push_back(std::move(function));
		return function_ptr;
	}

	ILBlock* ILFunction::create_block(ILDataType accepts) {
		std::unique_ptr<ILBlock> block = std::make_unique<ILBlock>();
		ILBlock* block_ptr = block.get();
		block->id = (unsigned int)blocks_memory.size();
		block->parent = this;
		block->accepts = accepts;
		blocks_memory.push_back(std::move(block));
		return block_ptr;
	}


	void ILFunction::append_block(ILBlock* block) {
		blocks.push_back(block);
	}

	unsigned char* ILBlock::reserve_data(size_t size) {
		if (data_pool.size() == 0) {
			data_pool.push_back(std::make_unique<ILBlockData>());
		}

		if (data_pool.back()->size + size >= 1024) {
			data_pool.push_back(std::make_unique<ILBlockData>());
		}

		unsigned char* r = &data_pool.back()->data[data_pool.back()->size];
		data_pool.back()->size += (unsigned int)size;
		return r;
	}

	void ILBlock::write_instruction(ILInstruction instruction) {
		ILInstruction* w = (ILInstruction*)reserve_data(sizeof(instruction));
		(*w) = instruction;
	}

	void ILBlock::write_const_type(ILDataType type) {
		ILDataType* w = (ILDataType*)reserve_data(sizeof(type));
		(*w) = type;
	}

	void ILBlock::write_value(size_t size, unsigned char* value) {
		unsigned char* w = reserve_data(size);
		memcpy(w, value, size);
	}



	void ILFunction::dump() {
		std::cout << "function " << id << " -> ";
		ILBlock::dump_data_type(returns->rvalue);
		std::cout << "\n";

		for (auto b = blocks.begin(); b != blocks.end(); b++) {
			(*b)->dump();
		}
	}

	void ILBlock::memmove(std::list<std::unique_ptr<ILBlockData>>::iterator& pool, size_t& memoff, size_t off) {
		memoff += off;
		if ((*pool)->size <= memoff) {
			memoff = 0;
			pool++;
		}
	}

	void ILBlock::dump_data_type(ILDataType dt) {
		switch (dt) {
			case ILDataType::ibool:  std::cout << "bool"; break;
			case ILDataType::u8:  std::cout << "u8"; break;
			case ILDataType::u16: std::cout << "u16"; break;
			case ILDataType::u32: std::cout << "u32"; break;
			case ILDataType::u64: std::cout << "u64"; break;
			case ILDataType::i8:  std::cout << "i8"; break;
			case ILDataType::i16: std::cout << "i16"; break;
			case ILDataType::i32: std::cout << "i32"; break;
			case ILDataType::i64: std::cout << "i64"; break;
			case ILDataType::f32: std::cout << "f32"; break;
			case ILDataType::f64: std::cout << "f64"; break;
			case ILDataType::ptr:  std::cout << "ptr"; break;
			case ILDataType::none: std::cout << "none"; break;
			default: std::cout << "error";
		}
	}


	unsigned char* ILBlock::read_data(size_t s, std::list<std::unique_ptr<ILBlockData>>::iterator& pool, size_t& memoff) {
		unsigned char* r = &((*pool)->data[memoff]);
		memmove(pool, memoff, s);
		return r;
	}

#define read_data_type(T) ((T*)read_data(sizeof(T),mempool,memoff))

	void ILBlock::dump() {
		std::cout << " " << id << " [";
		dump_data_type(accepts);
		std::cout << "] -> ";
		dump_data_type(yields);
		std::cout << "\n";

		std::list<std::unique_ptr<ILBlockData>>::iterator mempool = data_pool.begin();
		size_t memoff = 0;
		while (mempool != data_pool.end()) {

			auto inst = read_data_type(ILInstruction);

			switch (*inst) {
			case ILInstruction::ret:
				std::cout << "   ret\n";
				break;
			case ILInstruction::sub:
				std::cout << "   sub\n";
				break;
			case ILInstruction::div:
				std::cout << "   div\n";
				break;
			case ILInstruction::rem:
				std::cout << "   rem\n";
				break;
			case ILInstruction::mul:
				std::cout << "   mul\n";
				break;
			case ILInstruction::add:
				std::cout << "   add\n";
				break;
			case ILInstruction::o_and:
				std::cout << "   and\n";
				break;
			case ILInstruction::o_or:
				std::cout << "   or\n";
				break;
			case ILInstruction::o_xor:
				std::cout << "   xor\n";
				break;
			case ILInstruction::eq:
				std::cout << "   eq\n";
				break;
			case ILInstruction::ne:
				std::cout << "   ne\n";
				break;
			case ILInstruction::gt:
				std::cout << "   gt\n";
				break;
			case ILInstruction::lt:
				std::cout << "   lt\n";
				break;
			case ILInstruction::ge:
				std::cout << "   ge\n";
				break;
			case ILInstruction::le:
				std::cout << "   le\n";
				break;
			case ILInstruction::store:
				std::cout << "   store\n";
				break;
			case ILInstruction::accept:
				std::cout << "   accept\n";
				break;
			case ILInstruction::discard:
				std::cout << "   discard\n";
				break;
			case ILInstruction::jmp: {
				std::cout << "   jmp ";
				auto address = read_data_type(unsigned int);
				std::cout << *address << "\n";
				break;
			}
			case ILInstruction::local: {
				std::cout << "   local ";
				auto address = read_data_type(unsigned int);
				std::cout << *address << "\n";
				break;
			}
			case ILInstruction::member: {
				std::cout << "   member ";
				auto type = read_data_type(ILDataType*);
				auto address = read_data_type(unsigned int);
				std::cout << *address << "\n";
				break;
			}
			case ILInstruction::load: {
				std::cout << "   load [";
				auto type = read_data_type(ILDataType);
				dump_data_type(*type);
				std::cout << "]\n";
				break;
			}
			case ILInstruction::jmpz: {
				std::cout << "   jmpz ";
				auto address = read_data_type(unsigned int);
				std::cout << *address << " ";
				address = read_data_type(unsigned int);
				std::cout << *address << "\n";
				break;
			}
			case ILInstruction::yield:
				std::cout << "   yield\n";
				break;
			case ILInstruction::value: {
				std::cout << "   const [";

				auto type = read_data_type(ILDataType);
				dump_data_type(*type);
				std::cout << "] ";

				switch (*type) {
				case ILDataType::ibool:  std::cout << ((*read_data_type(uint8_t))?"true":"false"); break;
				case ILDataType::u8:  std::cout << *read_data_type(uint8_t); break;
				case ILDataType::u16: std::cout << *read_data_type(uint16_t); break;
				case ILDataType::u32: std::cout << *read_data_type(uint32_t); break;
				case ILDataType::u64: std::cout << *read_data_type(uint64_t); break;

				case ILDataType::i8:  std::cout << *read_data_type(int8_t); break;
				case ILDataType::i16: std::cout << *read_data_type(int16_t); break;
				case ILDataType::i32: std::cout << *read_data_type(int32_t); break;
				case ILDataType::i64: std::cout << *read_data_type(int64_t); break;

				case ILDataType::f32: std::cout << *read_data_type(float); break;
				case ILDataType::f64: std::cout << *read_data_type(double); break;
				}
				std::cout << "\n";

				break;
			}
			}
		}
	}

#undef read_data_type

	void ILFunction :: assert_flow() {
		for (auto b = blocks.begin(); b != blocks.end(); b++) {
			(*b)->assert_flow();
		}

		for (auto b = return_blocks.begin(); b != return_blocks.end(); b++) {
			if ((*b)->yields != returns->rvalue) {
				throw_il_wrong_data_flow_error();
			}
		}
	}

	void ILBlock::assert_flow() {
		for (auto b = predecessors.begin(); b != predecessors.end(); b++) {
			if ((*b)->yields != accepts) {
				throw_il_wrong_data_flow_error();
			}
		}
	}


	unsigned int ILFunction::register_local(ILType* type) {
		unsigned int r = (unsigned int)locals.size();
		locals.push_back(type);
		return r;
	}

	unsigned int _align_up(unsigned int value, unsigned int alignment) {
		return value + (alignment - (value % alignment));
	}

	void ILStruct::add_member(ILType* type) {
		unsigned int n_size = _align_up(size_in_bytes, type->alignment_in_bytes);
		members.push_back(std::make_pair(n_size, type));
		size_in_bytes = n_size + type->size_in_bytes;
		alignment_in_bytes = std::max(alignment_in_bytes, type->alignment_in_bytes);
	}


	void ILStruct::align_size() {
		size_in_bytes = _align_up(size_in_bytes, alignment_in_bytes);
	}


	ILType::~ILType() {}
	ILType::ILType() : rvalue(ILDataType::undefined), size_in_bytes(0), alignment_in_bytes(0){}
	ILType::ILType(ILDataType rv, unsigned int sz, unsigned int alg) : rvalue(rv), size_in_bytes(sz), alignment_in_bytes(alg) {}

	ILStruct::ILStruct() : ILType(ILDataType::ptr,0,0) {}


	ILType* ILModule::create_primitive_type(ILDataType rv, unsigned int sz, unsigned int alg) {
		std::unique_ptr<ILType> t = std::make_unique<ILType>(rv, sz, alg);
		ILType* rt = t.get();
		types.push_back(std::move(t));
		return rt;
	}

	ILStruct* ILModule::create_struct_type() {
		std::unique_ptr<ILStruct> t = std::make_unique<ILStruct>();
		ILStruct* rt = t.get();
		types.push_back(std::move(t));
		return rt;
	}

	void ILModule::build_default_types() {

		t_void = create_primitive_type(ILDataType::none, 0, 0);

		t_i8 = create_primitive_type(ILDataType::i8, 1, 1);
		t_u8 = create_primitive_type(ILDataType::u8, 1, 1);
		t_bool = create_primitive_type(ILDataType::ibool, 1, 1);

		t_i16 = create_primitive_type(ILDataType::i16, 2, 2);
		t_u16 = create_primitive_type(ILDataType::u16, 2, 2);

		t_i32 = create_primitive_type(ILDataType::i32, 4, 4);
		t_u32 = create_primitive_type(ILDataType::u32, 4, 4);

		t_f32 = create_primitive_type(ILDataType::f32, 4, 4);
		t_f64 = create_primitive_type(ILDataType::f64, 8, 8);

		if (architecture == ILArchitecture::i386) {
			t_i64 = create_primitive_type(ILDataType::i64, 8, 4);
			t_u64 = create_primitive_type(ILDataType::u64, 8, 4);
			t_ptr = create_primitive_type(ILDataType::ptr, 4, 4);
		}
		else if (architecture == ILArchitecture::x86_64) {
			t_i64 = create_primitive_type(ILDataType::i64, 8, 8);
			t_u64 = create_primitive_type(ILDataType::u64, 8, 8);
			t_ptr = create_primitive_type(ILDataType::ptr, 8, 8);
		}
	}

}