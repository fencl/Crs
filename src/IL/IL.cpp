
#include "IL.h"
#include <iostream>
#include <algorithm>

namespace Corrosive {
	void throw_il_wrong_data_flow_error() {
		std::cerr << "Compiler Error:\n\tWrong data flow inside compiler IL" << std::endl;
	}

	void throw_il_nothing_on_stack_error() {
		std::cerr << "Compiler Error:\n\tInstruction requires more argumens than the number of arguments on the stack" << std::endl;
	}

	void throw_il_wrong_type_error() {
		std::cerr << "Compiler Error:\n\tPassed broken type" << std::endl;
	}


	void throw_il_remaining_stack_error() {
		std::cerr << "Compiler Error:\n\tStack is not empty after terminator instruction" << std::endl;
	}

	void throw_il_wrong_arguments_error() {
		std::cerr << "Compiler Error:\n\tInstruction cannot use argument(s) on the stack" << std::endl;
	}

	ILFunction::~ILFunction() {}

	ILFunction* ILModule::create_function() {
		std::unique_ptr<ILFunction> function = std::make_unique<ILFunction>();
		ILFunction* function_ptr = function.get();
		function_ptr->id = (uint32_t)functions.size();
		function_ptr->parent = this;
		functions.push_back(std::move(function));
		return function_ptr;
	}


	ILBlock* ILFunction::create_and_append_block() {
		ILBlock* b = create_block();
		append_block(b);
		return b;
	}

	ILBlock* ILFunction::create_block() {
		std::unique_ptr<ILBlock> block = std::make_unique<ILBlock>();
		ILBlock* block_ptr = block.get();
		block->id = (uint32_t)blocks_memory.size();
		block->parent = this;
		blocks_memory.push_back(std::move(block));
		return block_ptr;
	}


	uint32_t ILModule::register_vtable(std::unique_ptr<void* []> table) {
		vtable_data.push_back(std::move(table));
		return (uint32_t)vtable_data.size() - 1;
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
		ILBlock::dump_data_type((*return_blocks.begin())->yields);
		std::cout << " \"" << alias << "\"\n";

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
			case ILDataType::size: std::cout << "size"; break;
			default: std::cout << "error";
		}
	}


	unsigned char* ILBlock::read_data(size_t s, std::list<std::unique_ptr<ILBlockData>>::iterator& pool, size_t& memoff) {
		unsigned char* r = &((*pool)->data[memoff]);
		memmove(pool, memoff, s);
		return r;
	}


	uint16_t ILFunction::register_local(ILSize size) {
		local_offsets.push_back(stack_size);
		stack_size = stack_size+  size;
		return (uint16_t)(local_offsets.size() - 1);
	}


	uint16_t ILEvaluator::mask_local(unsigned char* ptr) {
		auto& ls = local_stack_offsets.back();
		ls.push_back(ptr);
		return (uint16_t)(ls.size() - 1);
	}

	void ILEvaluator::pop_mask_local() {
		local_stack_offsets.pop_back();
	}

	uint16_t ILEvaluator::push_local(ILSize size) {
		auto& lss = local_stack_size.back();
		auto& lsb = local_stack_base.back();
		auto& ls = local_stack_offsets.back();
		
		size_t sz = size.eval(compiler_arch);

		ls.push_back(lsb + lss);
		lss += sz;

		return (uint16_t)(ls.size() - 1);
	}

	void ILEvaluator::pop_local(ILSize size) {
		auto& lss = local_stack_size.back();
		size_t sz = size.eval(compiler_arch);
		lss -= sz;
		local_stack_offsets.pop_back();
	}


	void ILEvaluator::stack_push() {
		if (local_stack_base.size() == 0) {
			local_stack_base.push_back(memory_stack);
		}
		else {
			local_stack_base.push_back(local_stack_base.back() + local_stack_size.back());
		}

		local_stack_size.push_back(0);
		local_stack_offsets.push_back(std::move(decltype(local_stack_offsets)::value_type()));
	}

	void ILEvaluator::stack_pop() {
		local_stack_base.pop_back();
		local_stack_size.pop_back();
		local_stack_offsets.pop_back();
	}

	unsigned char* ILEvaluator::stack_ptr(uint16_t id) {
		return local_stack_offsets.back()[id];
	}

	size_t _align_up(size_t value, size_t alignment) {
		return alignment == 0 ? value : ((value % alignment == 0) ? value : value + (alignment - (value % alignment)));
	}

	size_t alignment_value(ILArchitecture arch, ILAlignment align) {
		ILAlignment al = ILAlignment::none;
		switch (arch)
		{
			case Corrosive::ILArchitecture::x86_64: al = std::max(align, ILAlignment::__4word__);
				break;
			case Corrosive::ILArchitecture::i386: al = std::max(align, ILAlignment::__8word__);
				break;
		}
		if (al != ILAlignment::none) {
			switch (al)
			{
				case Corrosive::ILAlignment::none:
					return 1;
				case Corrosive::ILAlignment::two:
					return 2;
				case Corrosive::ILAlignment::four:
					return 4;
				case Corrosive::ILAlignment::__4word__:
					return 4;
				case Corrosive::ILAlignment::eight:
					return 8;
				case Corrosive::ILAlignment::__8word__:
					return 8;
				default:
					return 0;
			}
		}
		else {
			return 1;
		}
	}

	size_t ILSize::eval(ILArchitecture arch, ILAlignment align) const {

		size_t align_val = alignment_value(arch, align);

		switch (arch)
		{
			case ILArchitecture::i386:
				return _align_up((size_t)absolute + (size_t)pointers * 4, align_val);
			case ILArchitecture::x86_64:
				return _align_up((size_t)absolute + (size_t)pointers * 8, align_val);
			default:
				return 0;
		}
	}


	const ILSize ILSize::single_ptr = { 0,1 };
	const ILSize ILSize::double_ptr = { 0,2 };


	ILSize operator* (const ILSize& l, const uint32_t& r) {
		return { l.absolute * r, (uint16_t)(l.pointers * r) };
	}

	ILSize operator+ (const ILSize& l, const uint32_t& r) {
		return { l.absolute + r, l.pointers };
	}

	ILSize operator+ (const ILSize& l, const ILSize& r) {
		return { l.absolute + r.absolute, (uint16_t)(l.pointers+r.pointers) };
	}

	ILSize operator- (const ILSize& l, const uint32_t& r) {
		return { l.absolute - r, l.pointers };
	}

	ILSize operator- (const ILSize& l, const ILSize& r) {
		return { l.absolute - r.absolute, (uint16_t)(l.pointers-r.pointers) };
	}



	size_t ILSmallSize::eval(ILArchitecture arch) const {
		switch (arch)
		{
			case ILArchitecture::i386:
				return (combined & 0x0f) + (combined >> 4) * 4;
			case ILArchitecture::x86_64:
				return (combined & 0x0f) + (combined >> 4) * 8;
			default:
				return 0;
		}
	}


	ILSize::ILSize() : absolute(0), pointers(0) {}
	ILSize::ILSize(uint32_t a, uint16_t p) : absolute(a), pointers(p){}

	ILSmallSize::ILSmallSize() : combined(0) {}
	ILSmallSize::ILSmallSize(uint8_t a, uint8_t p) : combined((a&0x0f) + (p<<4)){}


#define read_data_type(T) ((T*)read_data(sizeof(T),mempool,memoff))
#define read_data_size(S) (read_data((S),mempool,memoff))

	void ILBlock::dump() {
		std::cout << " " << id << " [";
		dump_data_type(accepts);
		std::cout << "] -> ";
		dump_data_type(yields);
		std::cout << " \"" << alias << "\"\n";

		std::list<std::unique_ptr<ILBlockData>>::iterator mempool = data_pool.begin();
		size_t memoff = 0;
		while (mempool != data_pool.end()) {

			auto inst = read_data_type(ILInstruction);

			switch (*inst) {
				case ILInstruction::ret: {
					std::cout << "   ret [";
					auto type = read_data_type(ILDataType);
					dump_data_type(*type);
					std::cout << "]\n";
				} break;
				case ILInstruction::call: {
					std::cout << "   call [";
					auto type = read_data_type(ILDataType);
					dump_data_type(*type);
					auto argc = read_data_type(uint16_t);
					std::cout << "] (" << *argc << ")\n";
				} break;
				case ILInstruction::fnptr: {
					std::cout << "   fnptr ";
					auto ind = read_data_type(uint32_t);
					ILFunction* fn = parent->parent->functions[*ind].get();
					std::cout << *ind << " \"" << fn->alias << "\"\n";
				} break;
				case ILInstruction::vtable: {
					std::cout << "   vtable ";
					auto ind = read_data_type(uint32_t);
					std::cout << *ind << "\n";
				} break;
				case ILInstruction::duplicate: {
					std::cout << "   duplicate ";
					auto type = read_data_type(ILDataType);
					dump_data_type(*type);
					std::cout << "\n";
				} break;
				case ILInstruction::insintric: {
					std::cout << "   insintric \"";
					auto type = read_data_type(uint8_t);
					std::cout << parent->parent->insintric_function_name[*type] << "\"\n";
				} break;
				case ILInstruction::sub:
					std::cout << "   sub [";
					dump_data_type(*read_data_type(ILDataType)); std::cout << ", "; dump_data_type(*read_data_type(ILDataType)); std::cout << "]\n";
					break;
				case ILInstruction::div:
					std::cout << "   div [";
					dump_data_type(*read_data_type(ILDataType)); std::cout << ", "; dump_data_type(*read_data_type(ILDataType)); std::cout << "]\n";
					break;
				case ILInstruction::rem:
					std::cout << "   rem [";
					dump_data_type(*read_data_type(ILDataType)); std::cout << ", "; dump_data_type(*read_data_type(ILDataType)); std::cout << "]\n";
					break;
				case ILInstruction::mul:
					std::cout << "   mul [";
					dump_data_type(*read_data_type(ILDataType)); std::cout << ", "; dump_data_type(*read_data_type(ILDataType)); std::cout << "]\n";
					break;
				case ILInstruction::add:
					std::cout << "   add [";
					dump_data_type(*read_data_type(ILDataType)); std::cout << ", "; dump_data_type(*read_data_type(ILDataType)); std::cout << "]\n";
					break;
				case ILInstruction::bit_and:
					std::cout << "   and [";
					dump_data_type(*read_data_type(ILDataType)); std::cout << ", "; dump_data_type(*read_data_type(ILDataType)); std::cout << "]\n";
					break;
				case ILInstruction::bit_or:
					std::cout << "   or [";
					dump_data_type(*read_data_type(ILDataType)); std::cout << ", "; dump_data_type(*read_data_type(ILDataType)); std::cout << "]\n";
					break;
				case ILInstruction::bit_xor:
					std::cout << "   xor [";
					dump_data_type(*read_data_type(ILDataType)); std::cout << ", "; dump_data_type(*read_data_type(ILDataType)); std::cout << "]\n";
					break;
				case ILInstruction::eq:
					std::cout << "   eq [";
					dump_data_type(*read_data_type(ILDataType)); std::cout << ", "; dump_data_type(*read_data_type(ILDataType)); std::cout << "]\n";
					break;
				case ILInstruction::ne:
					std::cout << "   ne [";
					dump_data_type(*read_data_type(ILDataType)); std::cout << ", "; dump_data_type(*read_data_type(ILDataType)); std::cout << "]\n";
					break;
				case ILInstruction::gt:
					std::cout << "   gt [";
					dump_data_type(*read_data_type(ILDataType)); std::cout << ", "; dump_data_type(*read_data_type(ILDataType)); std::cout << "]\n";
					break;
				case ILInstruction::lt:
					std::cout << "   lt [";
					dump_data_type(*read_data_type(ILDataType)); std::cout << ", "; dump_data_type(*read_data_type(ILDataType)); std::cout << "]\n";
					break;
				case ILInstruction::ge:
					std::cout << "   ge [";
					dump_data_type(*read_data_type(ILDataType)); std::cout << ", "; dump_data_type(*read_data_type(ILDataType)); std::cout << "]\n";
					break;
				case ILInstruction::le:
					std::cout << "   le [";
					dump_data_type(*read_data_type(ILDataType)); std::cout << ", "; dump_data_type(*read_data_type(ILDataType)); std::cout << "]\n";
					break;
				case ILInstruction::cast:
					std::cout << "   cast ";
					dump_data_type(*read_data_type(ILDataType)); std::cout << " -> "; dump_data_type(*read_data_type(ILDataType)); std::cout << "\n";
					break;
				case ILInstruction::store:
					std::cout << "   store [";
					dump_data_type(*read_data_type(ILDataType)); std::cout << "]\n";
					break;
				case ILInstruction::store2:
					std::cout << "   store2 [";
					dump_data_type(*read_data_type(ILDataType)); std::cout << "]\n";
					break;
				case ILInstruction::start: {
					std::cout << "   start\n";
				} break;

				case ILInstruction::malloc: {
					std::cout << "   malloc\n";
				} break;

				case ILInstruction::rtoffset: {
					std::cout << "   rtoffset\n";
				} break;

				case ILInstruction::rtoffset2: {
					std::cout << "   rtoffset2\n";
				} break;
					
				case ILInstruction::null: {
					std::cout << "   null\n";
				} break;

				case ILInstruction::jmp: {
					std::cout << "   jmp ";
					auto address = read_data_type(uint32_t);
					std::cout << *address << " \"" << parent->blocks_memory[*address]->alias << "\"\n";
					break;
				}
				case ILInstruction::local: {
					std::cout << "   local ";
					auto offset = *read_data_type(uint16_t);
					std::cout << offset << "\n";
					break;
				}
				case ILInstruction::offset: {
					std::cout << "   offset ";
					auto off = read_data_type(ILSize);
					std::cout << off->absolute << " + " << off->pointers << "p\n";
					break;
				}
				case ILInstruction::memcpy: {
					std::cout << "   memcpy ";
					auto off = read_data_type(ILSize);
					std::cout << off->absolute << " + " << off->pointers << "p\n";
					break;
				}
				case ILInstruction::memcpy2: {
					std::cout << "   memcpy2 ";
					auto off = read_data_type(ILSize);
					std::cout << off->absolute << " + " << off->pointers << "p\n";
					break;
				}

				case ILInstruction::swap: {
					std::cout << "   swap [";
					auto type = read_data_type(ILDataType);
					dump_data_type(*type);
					std::cout << "]\n";
					break;
				}

				case ILInstruction::swap2: {
					std::cout << "   swap2 [";
					auto type1 = read_data_type(ILDataType);
					auto type2 = read_data_type(ILDataType);
					dump_data_type(*type1);
					std::cout << ", ";
					dump_data_type(*type2);
					std::cout << "]\n";
					break;
				}

				case ILInstruction::roffset: {
					std::cout << "   R offset ";
					auto from_t = *read_data_type(ILDataType);
					auto to_t = *read_data_type(ILDataType);
					auto off = read_data_type(ILSmallSize);

					std::cout << (uint16_t)(off->combined & 0x0f) << " + " << (uint16_t)(off->combined >> 4) << "p\n";

					dump_data_type(from_t);
					std::cout << " -> ";
					dump_data_type(to_t);
					std::cout << "]\n";
					break;
				}
				case ILInstruction::load: {
					std::cout << "   load [";
					auto type = read_data_type(ILDataType);
					dump_data_type(*type);
					std::cout << "]\n";
					break;
				}

				case ILInstruction::forget: {
					std::cout << "   forget [";
					auto type = read_data_type(ILDataType);
					dump_data_type(*type);
					std::cout << "]\n";
					break;
				}
				case ILInstruction::jmpz: {
					std::cout << "   jmpz ";
					auto address = read_data_type(uint32_t);
					std::cout << *address << " \"" << parent->blocks_memory[*address]->alias << "\" : ";
					address = read_data_type(uint32_t);
					std::cout << *address << " \"" << parent->blocks_memory[*address]->alias << "\"\n";
					break;
				}
				case ILInstruction::value: {
					std::cout << "   const [";

					auto type = read_data_type(ILDataType);
					dump_data_type(*type);
					std::cout << "] ";

					switch (*type) {
						case ILDataType::ibool:  std::cout << ((*read_data_type(uint8_t)) ? "true" : "false"); break;
						case ILDataType::u8:  std::cout << (uint16_t)*read_data_type(uint8_t); break;
						case ILDataType::u16: std::cout << *read_data_type(uint16_t); break;
						case ILDataType::u32: std::cout << *read_data_type(uint32_t); break;
						case ILDataType::u64: std::cout << *read_data_type(uint64_t); break;

						case ILDataType::i8:  std::cout << (int16_t)*read_data_type(int8_t); break;
						case ILDataType::i16: std::cout << *read_data_type(int16_t); break;
						case ILDataType::i32: std::cout << *read_data_type(int32_t); break;
						case ILDataType::i64: std::cout << *read_data_type(int64_t); break;

						case ILDataType::f32: std::cout << *read_data_type(float); break;
						case ILDataType::f64: std::cout << *read_data_type(double); break;

						case ILDataType::ptr: std::cout << *read_data_type(void*); break;
						case ILDataType::size: {
							auto off = read_data_type(ILSize);
							std::cout << off->absolute << " + " << off->pointers;
						}break;
					}
					std::cout << "\n";

					break;
				}
			}
		}
	}

#undef read_data_size
#undef read_data_type

	bool ILFunction::assert_flow() {
		if (return_blocks.size() == 0) {
			throw_il_wrong_data_flow_error();
			return false;
		}

		for (auto b = blocks.begin(); b != blocks.end(); b++) {
			if (!(*b)->assert_flow()) return false;
		}

		auto assert_ret_type = (*return_blocks.begin())->yields;

		for (auto b = return_blocks.begin(); b != return_blocks.end(); b++) {
			if ((*b)->yields != assert_ret_type) {
				throw_il_wrong_data_flow_error();
				return false;
			}
		}

		return true;
	}

	bool ILBlock::assert_flow() {
		for (auto b = predecessors.begin(); b != predecessors.end(); b++) {
			if ((*b)->yields != accepts) {
				throw_il_wrong_data_flow_error();
				return false;
			}
		}

		return true;
	}


	/*uint16_t ILFunction::register_local(uint32_t type_compile_size, uint32_t type_runtime_size) {
		local_offsets.push_back(std::make_pair(compile_stack_size,runtime_stack_size));
		compile_stack_size += type_compile_size;
		runtime_stack_size += type_runtime_size;
		return (uint16_t)(local_offsets.size()-1);
	}*/

	/*
	unsigned int ILType::runtime_size() { return 0; }
	unsigned int ILType::runtime_alignment() { return 0; }

	void ILStruct::add_member(ILType* type) {
		unsigned int n_size = _align_up(size_in_bytes, type->runtime_alignment());
		member_vars.push_back(std::make_tuple(n_size,compile_time_size_in_bytes, type));
		size_in_bytes = n_size + type->runtime_size();
		compile_time_size_in_bytes += type->compile_time_size();

		alignment_in_bytes = std::max(alignment_in_bytes, type->runtime_alignment());
	}

	void ILType::compile_time_move(void* src, void* dst) {

	}

	void ILStruct::compile_time_move(void* src, void* dst) {
		for (auto&& m : member_vars) {
			void* src_o = (char*)src + std::get<1>(m);
			void* dst_o = (char*)dst + std::get<1>(m);
			std::get<2>(m)->compile_time_move(src_o, dst_o);
		}
	}

	size_t ILType::compile_time_size() {
		return 0;
	}

	size_t ILStruct::compile_time_size() {
		return compile_time_size_in_bytes;
	}

	unsigned int ILStruct::runtime_size() { return size_in_bytes; }
	unsigned int ILStruct::runtime_alignment() { return alignment_in_bytes; }


	unsigned int ILArray::runtime_size() { return base->runtime_size()*count; }
	unsigned int ILArray::runtime_alignment() { return base->runtime_alignment(); }


	size_t ILArray::compile_time_size() {
		return base->compile_time_size()*count;
	}

	int ILType::compile_time_compare(void* p1, void* p2) {
		return memcmp(p1, p2, compile_time_size());
	}

	int ILStruct::compile_time_compare(void* p1, void* p2) {
		for (auto&& m : member_vars) {
			void* p1_o = (char*)p1 + std::get<1>(m);
			void* p2_o = (char*)p2 + std::get<1>(m);
			int r = std::get<2>(m)->compile_time_compare(p1_o, p2_o);
			if (r == 0)
				continue;
			else if (r > 0)
				return 1;
			else if (r < 0)
				return -1;
		}

		return 0;
	}

	int ILArray::compile_time_compare(void* p1, void* p2) {
		size_t es = base->compile_time_size();
		unsigned char* pd1 = (unsigned char*)p1;
		unsigned char* pd2 = (unsigned char*)p2;

		for (int i = 0; i < count; i++) {
			int c = base->compile_time_compare(pd1, pd2);
			if (c != 0) return c;
			pd1 += es;
			pd2 += es;
		}

		return 0;
	}

	void ILArray::compile_time_move(void* p1, void* p2) {
		size_t es = base->compile_time_size();
		unsigned char* pd1 = (unsigned char*)p1;
		unsigned char* pd2 = (unsigned char*)p2;

		for (int i = 0; i < count; i++) {
			base->compile_time_move(pd1, pd2);
			pd1 += es;
			pd2 += es;
		}
	}*/

	void* ILEvaluator::read_last_register_value_indirect(ILDataType rs) {
		return register_stack_pointer - compile_time_register_size(rs);
	}


	void ILEvaluator::discard_last_register_type(ILDataType rs) {
		register_stack_pointer -= compile_time_register_size(rs);
	}

	/*
	void ILStruct::align_size() {
		size_in_bytes = _align_up(size_in_bytes, alignment_in_bytes);
	}


	ILType::~ILType() {}
	ILType::ILType() : rvalue(ILDataType::undefined) {}
	ILType::ILType(ILDataType rv) : rvalue(rv) {}

	ILStruct::ILStruct() : ILType(ILDataType::ptr) {}
	ILStruct::ILStruct(ILDataType rv, unsigned int sz, unsigned int ct, unsigned int alg) : ILType(rv), size_in_bytes(sz),alignment_in_bytes(alg),compile_time_size_in_bytes(ct) {}


	ILType* ILModule::create_primitive_type(ILDataType rv, unsigned int sz, unsigned int cs, unsigned int alg) {
		std::unique_ptr<ILType> t = std::make_unique<ILStruct>(rv, sz,cs, alg);
		ILType* rt = t.get();
		types.push_back(std::move(t));
		return rt;
	}


	ILArray* ILModule::create_array_type() {
		std::unique_ptr<ILArray> t = std::make_unique<ILArray>();
		ILArray* rt = t.get();
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

		t_void = create_primitive_type(ILDataType::none, 0, 0,0);

		t_i8 = create_primitive_type(ILDataType::i8, 1,1, 1);
		t_u8 = create_primitive_type(ILDataType::u8, 1,1, 1);
		t_bool = create_primitive_type(ILDataType::ibool, 1,1, 1);

		t_i16 = create_primitive_type(ILDataType::i16, 2,2, 2);
		t_u16 = create_primitive_type(ILDataType::u16, 2,2, 2);

		t_i32 = create_primitive_type(ILDataType::i32, 4,4, 4);
		t_u32 = create_primitive_type(ILDataType::u32, 4,4, 4);

		t_f32 = create_primitive_type(ILDataType::f32, 4,sizeof(float), 4);
		t_f64 = create_primitive_type(ILDataType::f64, 8,sizeof(double), 8);

		t_type = create_primitive_type(ILDataType::ptr,sizeof(void*), sizeof(void*), sizeof(void*));

		if (architecture == ILArchitecture::i386) {
			t_i64 = create_primitive_type(ILDataType::i64, 8,8, 4);
			t_u64 = create_primitive_type(ILDataType::u64, 8,8, 4);
			t_ptr = create_primitive_type(ILDataType::ptr, 4,sizeof(void*), 4);
		}
		else if (architecture == ILArchitecture::x86_64) {
			t_i64 = create_primitive_type(ILDataType::i64, 8,8, 8);
			t_u64 = create_primitive_type(ILDataType::u64, 8,8, 8);
			t_ptr = create_primitive_type(ILDataType::ptr, 8,sizeof(void*), 8);
		}
	}*/

}