
#include "IL.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <unordered_map>

namespace Corrosive {
	void throw_il_wrong_data_flow_error() {
		throw std::exception("Compiler Error, Wrong data flow inside compiler IL");
	}

	void throw_il_nothing_on_stack_error() {
		throw std::exception("Compiler Error, Instruction requires more argumens than the number of arguments on the stack");
	}

	void throw_il_wrong_type_error() {
		throw std::exception("Compiler Error, Passed broken type");
	}


	void throw_il_remaining_stack_error() {
		throw std::exception("Compiler Error, Stack is not empty after terminator instruction");
	}

	void throw_il_wrong_arguments_error() {
		throw std::exception("Compiler Error, Instruction cannot use argument(s) on the stack");
	}

	void throw_runtime_exception_header(const ILEvaluator* eval, std::stringstream& cerr) {
		if (eval->debug_file < eval->debug_file_names.size()) {
			cerr << "\n | Error (" << eval->debug_file_names[eval->debug_file] << ": " << (eval->debug_line + 1) << "):\n | \t";
		}
		else {
			cerr << "\n | Error (?):\n | \t";
		}
	}

	void throw_runtime_exception_footer(const ILEvaluator* eval, std::stringstream& cerr) {
		cerr << "\n |\n";
		for (auto t = eval->callstack_debug.rbegin(); t != eval->callstack_debug.rend(); t++) {
			if (std::get<1>(*t) < eval->debug_file_names.size()) {
				cerr << "\n | At (" << eval->debug_file_names[std::get<1>(*t)] << ": " << (std::get<0>(*t) + 1) << ") " << std::get<2>(*t);
			}
			else {
				cerr << "\n | At (?) " << std::get<2>(*t);
			}
		}
	}

	size_t _align_up(size_t value, size_t alignment) {
		return alignment == 0 ? value : ((value % alignment == 0) ? value : value + (alignment - (value % alignment)));
	}

	void throw_runtime_exception(const ILEvaluator* eval, std::string_view message) {
		std::stringstream cerr;
		throw_runtime_exception_header(eval, cerr);


		cerr << message;
		throw_runtime_exception_footer(eval, cerr);
		throw string_exception(std::move(cerr.str()));
	}

	void throw_segfault_exception(const ILEvaluator* eval, int signal) {
		std::stringstream cerr;
		throw_runtime_exception_header(eval, cerr);

		cerr << "Attempt to access protected memory range (Segmentation fault [" << signal << "])";
		throw_runtime_exception_footer(eval, cerr);
		throw string_exception(std::move(cerr.str()));
	}

	void throw_interrupt_exception(const ILEvaluator* eval, int signal) {
		std::stringstream cerr;
		throw_runtime_exception_header(eval, cerr);

		cerr << "Interrupt exception (Interrupt [" << signal << "])";
		throw_runtime_exception_footer(eval, cerr);
		throw string_exception(std::move(cerr.str()));
	}


	uint16_t ILEvaluator::register_debug_source(std::string name) {
		debug_file_names.push_back(name);
		return (uint16_t)debug_file_names.size() - 1;
	}

	ILFunction::~ILFunction() {}

	ILBytecodeFunction* ILModule::create_function(ILContext context) {
		std::unique_ptr<ILBytecodeFunction> function = std::make_unique<ILBytecodeFunction>();
		ILBytecodeFunction* function_ptr = function.get();
		function_ptr->id = (uint32_t)functions.size();
		function_ptr->parent = this;
		function_ptr->context = context;
		functions.push_back(std::move(function));
		return function_ptr;
	}

	ILNativeFunction* ILModule::create_native_function() {
		std::unique_ptr<ILNativeFunction> function = std::make_unique<ILNativeFunction>();
		ILNativeFunction* function_ptr = function.get();
		function_ptr->id = (uint32_t)functions.size();
		function_ptr->parent = this;
		functions.push_back(std::move(function));
		return function_ptr;
	}

	ILBlock* ILBytecodeFunction::create_and_append_block() {
		ILBlock* b = create_block();
		append_block(b);
		return b;
	}

	ILBlock* ILBytecodeFunction::create_block() {
		std::unique_ptr<ILBlock> block = std::make_unique<ILBlock>();
		ILBlock* block_ptr = block.get();
		block->id = (uint32_t)blocks_memory.size();
		block->parent = this;
		blocks_memory.push_back(std::move(block));
		return block_ptr;
	}


	uint32_t ILModule::register_vtable(uint32_t size, std::unique_ptr<void* []> table) {
		vtable_data.push_back(std::make_pair(size,std::move(table)));
		return (uint32_t)vtable_data.size() - 1;
	}

	void ILBytecodeFunction::append_block(ILBlock* block) {
		blocks.push_back(block);
	}

	uint8_t* ILBlock::reserve_data(size_t size) {
		data_pool.resize(data_pool.size() + size);
		return &data_pool[data_pool.size() - size];
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



	void ILBytecodeFunction::dump() {
		std::cout << "function " << id << " -> ";
		ILBlock::dump_data_type((*return_blocks.begin())->yields);
		std::cout << " \"" << alias << "\"\n";

		for (auto b = blocks.begin(); b != blocks.end(); b++) {
			(*b)->dump();
		}
	}

	void ILBlock::dump_data_type(ILDataType dt) {
		switch (dt) {
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
			case ILDataType::word:  std::cout << "w"; break;
			case ILDataType::dword:  std::cout << "dw"; break;
			case ILDataType::none: std::cout << "none"; break;
			default: std::cout << "error";
		}
	}


	void ILLifetime::push() {
		lifetime.push_back((unsigned char)ILLifetimeEvent::push);
	}

	void ILLifetime::pop() {
		lifetime.push_back((unsigned char)ILLifetimeEvent::pop);
	}

	void ILLifetime::discard_push() {
		lifetime.pop_back();
	}

	stackid_t ILLifetime::append(ILSize s) {
		lifetime.push_back((unsigned char)ILLifetimeEvent::append);


		lifetime.push_back((unsigned char)s.type);
		lifetime.push_back((s.value >> 24) & 0xFF);
		lifetime.push_back((s.value >> 16) & 0xFF);
		lifetime.push_back((s.value >> 8) & 0xFF);
		lifetime.push_back((s.value) & 0xFF);

		return id++;
	}


	stackid_t ILLifetime::append_unknown(size_t& holder) {
		lifetime.push_back((unsigned char)ILLifetimeEvent::append);
		holder = lifetime.size();
		lifetime.push_back(0);
		lifetime.push_back(0);
		lifetime.push_back(0);
		lifetime.push_back(0);
		lifetime.push_back(0);
		return id++;
	}

	void ILLifetime::resolve_unknown(size_t holder, ILSize s) {

		lifetime[holder] = (unsigned char)s.type;
		lifetime[holder + 1] = (s.value >> 24) & (unsigned char)0xFF;
		lifetime[holder + 2] = (s.value >> 16) & (unsigned char)0xFF;
		lifetime[holder + 3] = (s.value >> 8) & (unsigned char)0xFF;
		lifetime[holder + 4] = (s.value) & (unsigned char)0xFF;
	}

	uint32_t ILModule::register_constant(unsigned char* memory, ILSize size) {
		size_t compile_size = size.eval(this, compiler_arch);
		auto data = std::make_unique<unsigned char[]>(compile_size);
		memcpy(data.get(), memory, compile_size);
		constant_memory.push_back(std::make_pair(size,std::move(data)));
		return (uint32_t)constant_memory.size() - 1;
	}

	uint32_t ILModule::register_static(unsigned char* memory, ILSize size) {
		size_t compile_size = size.eval(this, compiler_arch);
		auto data = std::make_unique<unsigned char[]>(compile_size);
		if (memory != nullptr) {
			memcpy(data.get(), memory, compile_size);
		}
		static_memory.push_back(std::make_pair(size, std::move(data)));
		return (uint32_t)static_memory.size() - 1;
	}

	void ILBytecodeFunction::calculate_stack(ILArchitecture arch) {
		if (arch != calculated_for) {
			calculated_local_stack_size = 0;
			calculated_local_stack_alignment = 0;
			size_t stack_size = 0;
			std::vector<size_t> stack_sizes;
			stack_sizes.push_back(0);
			calculated_local_offsets.resize(local_stack_lifetime.id);

			size_t lid = 0;
			unsigned char* ptr = local_stack_lifetime.lifetime.data();
			unsigned char* end = ptr + local_stack_lifetime.lifetime.size();
			while (ptr != end) {
				switch (*(ILLifetimeEvent*)(ptr++))
				{
					case ILLifetimeEvent::push: {
						stack_sizes.push_back(stack_size);
					}break;
					case ILLifetimeEvent::pop: {
						stack_size = stack_sizes.back();
						stack_sizes.pop_back();
					}break;
					case ILLifetimeEvent::append: {
						ILSizeType ptr_t = *(ILSizeType*)(ptr++);
						uint32_t ptr_val = (((uint32_t) * (ptr++)) << 24) | (((uint32_t) * (ptr++)) << 16) | (((uint32_t) * (ptr++)) << 8) | (((uint32_t) * (ptr++)));

						ILSize ptr_s = ILSize(ptr_t, ptr_val);
						size_t elem_align = ptr_s.alignment(parent, arch);
						calculated_local_stack_alignment = std::max(elem_align, calculated_local_stack_alignment);

						stack_size = _align_up(stack_size, elem_align);
						calculated_local_offsets[lid++] = stack_size;
						size_t sz = ptr_s.eval(parent, arch);
						stack_size += sz;
						calculated_local_stack_size = std::max(calculated_local_stack_size, stack_size);
					}break;
				}
			}



			calculated_for = arch;
		}
	}


	void ILModule::run(ILFunction* func) {
		auto eval = std::make_unique<ILEvaluator>();
		eval->parent = this;
		ILBuilder::eval_fncall(eval.get(), func);

		auto lr1b = (size_t)(eval->register_stack_pointer_1b - eval->register_stack_1b);
		if (lr1b > 0) { std::cout << "leaked 1 byte registers: " << lr1b << "\n"; }
		auto lr2b = (size_t)(eval->register_stack_pointer_2b - eval->register_stack_2b);
		if (lr2b) { std::cout << "leaked 2 byte registers: " << lr2b << "\n"; }
		auto lr4b = (size_t)(eval->register_stack_pointer_4b - eval->register_stack_4b);
		if (lr4b) { std::cout << "leaked 4 byte registers: " << lr4b << "\n"; }
		auto lr8b = (size_t)(eval->register_stack_pointer_8b - eval->register_stack_8b);
		if (lr8b) { std::cout << "leaked 8 byte registers: " << lr8b << "\n"; }
	}


	uint32_t ILModule::register_structure_table() {
		structure_tables.push_back(ILStructTable());
		return (uint32_t)(structure_tables.size() - 1);
	}
	uint32_t ILModule::register_array_table() {
		array_tables.push_back(ILArrayTable());
		return (uint32_t)(array_tables.size() - 1);
	}

	size_t ILSize::eval(ILModule* mod, ILArchitecture arch) const {
		switch (type) {
			case ILSizeType::absolute: {
				return (size_t)value;
			}

			case ILSizeType::word: {
				switch (arch)
				{
					case ILArchitecture::bit32:
						return (size_t)value * 4;
					case ILArchitecture::bit64:
						return (size_t)value * 8;
					default:
						return 0;
				}
			}

			case ILSizeType::table: {
				auto& stable = mod->structure_tables[value];
				stable.calculate(mod, arch);
				return stable.calculated_size;
			}

			case ILSizeType::array: {
				auto& stable = mod->array_tables[value];
				stable.calculate(mod, arch);
				return stable.calculated_size;
			}
		}

		return 0;
	}

	uint32_t _upper_power_of_two(uint32_t v)
	{
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		v++;
		return v;
	}

	size_t ILSize::alignment(ILModule* mod, ILArchitecture arch) const {
		switch (type) {
			case ILSizeType::absolute: {
				switch (arch)
				{
					case ILArchitecture::bit32:
						return (size_t)_upper_power_of_two((uint32_t)std::min<size_t>((size_t)value, 4));
					case ILArchitecture::bit64:
						return (size_t)_upper_power_of_two((uint32_t)std::min<size_t>((size_t)value, 8));
					default:
						return 0;
				}
			}

			case ILSizeType::word: {
				switch (arch)
				{
					case ILArchitecture::bit32:
						return 4;
					case ILArchitecture::bit64:
						return 8;
					default:
						return 0;
				}
			}

			case ILSizeType::table: {
				auto& stable = mod->structure_tables[value];
				stable.calculate(mod, arch);
				return stable.calculated_alignment;
			}

			case ILSizeType::array: {
				auto& stable = mod->array_tables[value];
				stable.calculate(mod, arch);
				return stable.calculated_alignment;
			}
		}

		return 0;
	}

	const ILSize ILSize::single_ptr = { ILSizeType::word,1 };
	const ILSize ILSize::double_ptr = { ILSizeType::word,2 };

	void ILStructTable::calculate(ILModule* mod, ILArchitecture arch) {
		if (arch != calculated_for) {
			calculated_size = 0;
			calculated_alignment = 1;
			calculated_offsets.resize(elements.size());

			size_t id = 0;
			for (auto elem = elements.begin(); elem != elements.end(); elem++) {
				size_t elem_align = elem->alignment(mod, arch);
				calculated_size = _align_up(calculated_size, elem_align);
				calculated_offsets[id++] = calculated_size;
				calculated_size += elem->eval(mod, arch);
				calculated_alignment = std::max(calculated_alignment, elem_align);
			}

			calculated_size = _align_up(calculated_size, calculated_alignment);
			calculated_for = arch;
		}
	}

	void ILArrayTable::calculate(ILModule* mod, ILArchitecture arch) {
		if (arch != calculated_for) {
			calculated_alignment = element.alignment(mod, arch);
			calculated_size = (size_t)(_align_up(element.eval(mod, arch), calculated_alignment) * count);
			calculated_for = arch;
		}
	}

	void ILSize::print() {
		switch (type) {
			case ILSizeType::absolute: {
				std::cout << value;
			}break;

			case ILSizeType::word: {
				std::cout << value << "w";
			}break;

			case ILSizeType::table: {
				std::cout << "[T " << value << "]";
			}break;
		}
	}



	ILSize::ILSize() : type(ILSizeType::absolute), value(0) {}
	ILSize::ILSize(ILSizeType t, tableid_t v) : type(t), value(v) {}


	uint32_t ILModule::register_function_decl(std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>> decl) {
		function_decl.push_back(std::move(decl));
		return (uint32_t)(function_decl.size() - 1);
	}

	void ILModule::dump_function_decl(uint32_t id) {
		auto& decl = function_decl[id];
		std::cout << "[";
		for (size_t i = 0; i < std::get<2>(decl).size(); ++i) {
			if (i > 0) std::cout << ", ";

			ILBlock::dump_data_type(std::get<2>(decl)[i]);
		}
		std::cout << "] -> ";
		ILBlock::dump_data_type(std::get<1>(decl));
	}


	void ILBlock::dump() {
		std::cout << " " << id << " [";
		dump_data_type(accepts);
		std::cout << "] -> ";
		dump_data_type(yields);
		std::cout << " \"" << alias << "\"\n";

		std::vector<uint8_t>::iterator it = data_pool.begin();
		size_t memoff = 0;
		while (it != data_pool.end()) {

			auto inst = ILBlock::read_data<ILInstruction>(it);

			switch (inst) {
				case ILInstruction::ret: {
					std::cout << "   ret [";
					auto type = ILBlock::read_data<ILDataType>(it);
					dump_data_type(type);
					std::cout << "]\n";
				} break;
				case ILInstruction::negative: {
					std::cout << "   negative [";
					auto type = ILBlock::read_data<ILDataType>(it);
					dump_data_type(type);
					std::cout << "]\n";
				} break;
				case ILInstruction::call: {
					std::cout << "   call ";
					auto id = ILBlock::read_data<uint32_t>(it);
					parent->parent->dump_function_decl(id);
					std::cout << "\n";
				} break;

				case ILInstruction::fnptr: {
					std::cout << "   fnptr ";
					auto ind = ILBlock::read_data<uint32_t>(it);
					ILFunction* fn = parent->parent->functions[ind].get();
					std::cout << ind << " \"" << fn->alias << "\"\n";
				} break;

				case ILInstruction::fncall: {
					std::cout << "   fncall ";
					auto ind = ILBlock::read_data<uint32_t>(it);
					ILFunction* fn = parent->parent->functions[ind].get();
					std::cout << ind << " \"" << fn->alias << "\"\n";
				} break;

				case ILInstruction::vtable: {
					std::cout << "   vtable ";
					auto ind = ILBlock::read_data<uint32_t>(it);
					std::cout << ind << "\n";
				} break;
				case ILInstruction::constref: {
					std::cout << "   constref ";
					auto ind = ILBlock::read_data<uint32_t>(it);
					std::cout << ind << "\n";
				} break;
				case ILInstruction::staticref: {
					std::cout << "   staticref ";
					auto ind = ILBlock::read_data<uint32_t>(it);
					std::cout << ind << "\n";
				} break;

				case ILInstruction::table8roffset8: {
					std::cout << "   tableroffset [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << "] -> ["; dump_data_type(pair.second()); std::cout << "] (table ";
					std::cout << (uint16_t)ILBlock::read_data<uint8_t>(it) << "):" << (uint16_t)ILBlock::read_data<uint8_t>(it) << "\n";
				} break;
				case ILInstruction::table8roffset16: {
					std::cout << "   tableroffset [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << "] -> ["; dump_data_type(pair.second()); std::cout << "] (table ";
					std::cout << (uint16_t)ILBlock::read_data<uint8_t>(it) << "):" << ILBlock::read_data<uint16_t>(it) << "\n";
				} break;
				case ILInstruction::table8roffset32: {
					std::cout << "   tableroffset [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << "] -> ["; dump_data_type(pair.second()); std::cout << "] (table ";
					std::cout << (uint16_t)ILBlock::read_data<uint8_t>(it) << "):" << ILBlock::read_data<uint32_t>(it) << "\n";
				} break;
				case ILInstruction::table16roffset8: {
					std::cout << "   tableroffset [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << "] -> ["; dump_data_type(pair.second()); std::cout << "] (table ";
					std::cout << ILBlock::read_data<uint16_t>(it) << "):" << (uint16_t)ILBlock::read_data<uint8_t>(it) << "\n";
				} break;
				case ILInstruction::table16roffset16: {
					std::cout << "   tableroffset [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << "] -> ["; dump_data_type(pair.second()); std::cout << "] (table ";
					std::cout << ILBlock::read_data<uint16_t>(it) << "):" << ILBlock::read_data<uint16_t>(it) << "\n";
				} break;
				case ILInstruction::table16roffset32: {
					std::cout << "   tableroffset [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << "] -> ["; dump_data_type(pair.second()); std::cout << "] (table ";
					std::cout << ILBlock::read_data<uint16_t>(it) << "):" << ILBlock::read_data<uint32_t>(it) << "\n";
				} break;
				case ILInstruction::table32roffset8: {
					std::cout << "   tableroffset [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << "] -> ["; dump_data_type(pair.second()); std::cout << "] (table ";
					std::cout << ILBlock::read_data<uint32_t>(it) << "):" << (uint16_t)ILBlock::read_data<uint8_t>(it) << "\n";
				} break;
				case ILInstruction::table32roffset16: {
					std::cout << "   tableroffset [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << "] -> ["; dump_data_type(pair.second()); std::cout << "] (table ";
					std::cout << ILBlock::read_data<uint32_t>(it) << "):" << ILBlock::read_data<uint16_t>(it) << "\n";
				} break;
				case ILInstruction::table32roffset32: {
					std::cout << "   tableroffset [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << "] -> ["; dump_data_type(pair.second()); std::cout << "] (table ";
					std::cout << ILBlock::read_data<uint32_t>(it) << "):" << ILBlock::read_data<uint32_t>(it) << "\n";
				} break;

				case ILInstruction::table8offset8: {
					std::cout << "   tableoffset (table " << (uint16_t)ILBlock::read_data<uint8_t>(it) << "): " << (uint16_t)ILBlock::read_data<uint8_t>(it) << "\n";
				} break;
				case ILInstruction::table8offset16: {
					std::cout << "   tableoffset (table " << (uint16_t)ILBlock::read_data<uint8_t>(it) << "): " << ILBlock::read_data<uint16_t>(it) << "\n";
				} break;
				case ILInstruction::table8offset32: {
					std::cout << "   tableoffset (table " << (uint16_t)ILBlock::read_data<uint8_t>(it) << "): " << ILBlock::read_data<uint32_t>(it) << "\n";
				} break;
				case ILInstruction::table16offset8: {
					std::cout << "   tableoffset (table " << ILBlock::read_data<uint16_t>(it) << "): " << (uint16_t)ILBlock::read_data<uint8_t>(it) << "\n";
				} break;
				case ILInstruction::table16offset16: {
					std::cout << "   tableoffset (table " << ILBlock::read_data<uint16_t>(it) << "): " << ILBlock::read_data<uint16_t>(it) << "\n";
				} break;
				case ILInstruction::table16offset32: {
					std::cout << "   tableoffset (table " << ILBlock::read_data<uint16_t>(it) << "): " << ILBlock::read_data<uint32_t>(it) << "\n";
				} break;
				case ILInstruction::table32offset8: {
					std::cout << "   tableoffset (table " << ILBlock::read_data<uint32_t>(it) << "): " << (uint16_t)ILBlock::read_data<uint8_t>(it) << "\n";
				} break;
				case ILInstruction::table32offset16: {
					std::cout << "   tableoffset (table " << ILBlock::read_data<uint32_t>(it) << "): " << ILBlock::read_data<uint16_t>(it) << "\n";
				} break;
				case ILInstruction::table32offset32: {
					std::cout << "   tableoffset (table " << ILBlock::read_data<uint32_t>(it) << "): " << ILBlock::read_data<uint32_t>(it) << "\n";
				} break;

				case ILInstruction::duplicate: {
					std::cout << "   duplicate [";
					dump_data_type(ILBlock::read_data<ILDataType>(it));
					std::cout << "]\n";
				} break;
				case ILInstruction::clone: {
					std::cout << "   clone [";
					dump_data_type(ILBlock::read_data<ILDataType>(it));
					std::cout << "] " << ILBlock::read_data<uint16_t>(it) << "\n";
				} break;
				case ILInstruction::insintric: {
					std::cout << "   insintric \"";
					auto type = ILBlock::read_data<uint8_t>(it);
					std::cout << parent->parent->insintric_function_name[type] << "\"\n";
				} break;
				

				case ILInstruction::sub: {
					std::cout << "   sub [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << " -> "; dump_data_type(pair.second()); std::cout << "]\n";
					} break;
				case ILInstruction::div: {
					std::cout << "   div [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << " -> "; dump_data_type(pair.second()); std::cout << "]\n";
					} break;
				case ILInstruction::rem: {
					std::cout << "   rem [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << " -> "; dump_data_type(pair.second()); std::cout << "]\n";
					} break;
				case ILInstruction::mul: {
					std::cout << "   mul [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << " -> "; dump_data_type(pair.second()); std::cout << "]\n";
					} break;
				case ILInstruction::add: {
					std::cout << "   add [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << " -> "; dump_data_type(pair.second()); std::cout << "]\n";
					} break;
				case ILInstruction::bit_and: {
					std::cout << "   and [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << " -> "; dump_data_type(pair.second()); std::cout << "]\n";
					} break;
				case ILInstruction::bit_or: {
					std::cout << "   or [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << " -> "; dump_data_type(pair.second()); std::cout << "]\n";
					} break;
				case ILInstruction::bit_xor: {
					std::cout << "   xor [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << " -> "; dump_data_type(pair.second()); std::cout << "]\n";
					} break;
				case ILInstruction::eq: {
					std::cout << "   eq [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << " -> "; dump_data_type(pair.second()); std::cout << "]\n";
					} break;
				case ILInstruction::ne: {
					std::cout << "   ne [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << " -> "; dump_data_type(pair.second()); std::cout << "]\n";
					} break;
				case ILInstruction::gt: {
					std::cout << "   gt [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << " -> "; dump_data_type(pair.second()); std::cout << "]\n";
					} break;
				case ILInstruction::lt: {
					std::cout << "   lt [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << " -> "; dump_data_type(pair.second()); std::cout << "]\n";
					} break;
				case ILInstruction::ge: {
					std::cout << "   ge [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << " -> "; dump_data_type(pair.second()); std::cout << "]\n";
					} break;
				case ILInstruction::le: {
					std::cout << "   le [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << " -> "; dump_data_type(pair.second()); std::cout << "]\n";
					} break;
				case ILInstruction::cast: {
					std::cout << "   cast ";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << " -> "; dump_data_type(pair.second()); std::cout << "\n";
				} break;
				case ILInstruction::bitcast: {
					std::cout << "   bitcast ";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(pair.first()); std::cout << " -> "; dump_data_type(pair.second()); std::cout << "\n";
				} break;
				case ILInstruction::store:
					std::cout << "   store [";
					dump_data_type(ILBlock::read_data<ILDataType>(it)); std::cout << "]\n";
					break;
				case ILInstruction::store2:
					std::cout << "   store rev [";
					dump_data_type(ILBlock::read_data<ILDataType>(it)); std::cout << "]\n";
					break;
				case ILInstruction::start: {
					std::cout << "   start\n";
				} break;

				case ILInstruction::rtoffset: {
					std::cout << "   rtoffset\n";
				} break;

				case ILInstruction::rtoffset2: {
					std::cout << "   rtoffset2\n";
				} break;

				case ILInstruction::negate: {
					std::cout << "   negate\n";
				} break;

				case ILInstruction::null: {
					std::cout << "   null\n";
				} break;

				case ILInstruction::combinedw: {
					std::cout << "   combine [dword]\n";
				} break;

				case ILInstruction::highdw: {
					std::cout << "   high [word]\n";
				} break;

				case ILInstruction::splitdw: {
					std::cout << "   split [dword]\n";
				} break;

				case ILInstruction::isnotzero: {
					std::cout << "   isnotzero [";
					dump_data_type(ILBlock::read_data<ILDataType>(it)); std::cout << "]\n";
				} break;

				case ILInstruction::accept: {
					std::cout << "   accept [";
					dump_data_type(ILBlock::read_data<ILDataType>(it)); std::cout << "]\n";
				} break;

				case ILInstruction::yield: {
					std::cout << "   yield [";
					dump_data_type(ILBlock::read_data<ILDataType>(it)); std::cout << "]\n";
				} break;

				case ILInstruction::discard: {
					std::cout << "   discard [";
					dump_data_type(ILBlock::read_data<ILDataType>(it)); std::cout << "]\n";
				} break;

				case ILInstruction::jmp: {
					std::cout << "   jmp ";
					auto address = ILBlock::read_data<uint32_t>(it);
					std::cout << address << " \"" << parent->blocks_memory[address]->alias << "\"\n";
					break;
				}
				case ILInstruction::local8: {
					std::cout << "   local ";
					auto offset = ILBlock::read_data<uint8_t>(it);
					std::cout << (uint16_t)offset << "\n";
					break;
				}
				case ILInstruction::local16: {
					std::cout << "   local ";
					auto offset = ILBlock::read_data<uint16_t>(it);
					std::cout << offset << "\n";
					break;
				}
				case ILInstruction::local32: {
					std::cout << "   local ";
					auto offset = ILBlock::read_data<uint32_t>(it);
					std::cout << offset << "\n";
					break;
				}
				case ILInstruction::debug: {
					// do not print
					ILBlock::read_data<uint16_t>(it);
					ILBlock::read_data<uint16_t>(it);
					break;
				}
				case ILInstruction::offset32: {
					std::cout << "   offset ";
					auto t = ILBlock::read_data<ILSizeType>(it);
					auto off = ILBlock::read_data<uint32_t>(it);
					ILSize(t,off).print();
					std::cout << "\n";
					break;
				}
				case ILInstruction::offset16: {
					std::cout << "   offset ";
					auto t = ILBlock::read_data<ILSizeType>(it);
					auto off = ILBlock::read_data<uint16_t>(it);
					ILSize(t, off).print();
					std::cout << "\n";
					break;
				}
				case ILInstruction::offset8: {
					std::cout << "   offset ";
					auto t = ILBlock::read_data<ILSizeType>(it);
					auto off = ILBlock::read_data<uint8_t>(it);
					ILSize(t, off).print();
					std::cout << "\n";
					break;
				}
				case ILInstruction::aoffset8: {
					std::cout << "   aoffset " << (uint16_t)ILBlock::read_data<uint8_t>(it) << "\n";
					break;
				}
				case ILInstruction::aoffset16: {
					std::cout << "   aoffset " << ILBlock::read_data<uint16_t>(it) << "\n";
					break;
				}
				case ILInstruction::aoffset32: {
					std::cout << "   aoffset " << ILBlock::read_data<uint32_t>(it) << "\n";
					break;
				}
				case ILInstruction::woffset8: {
					std::cout << "   woffset " << (uint16_t)ILBlock::read_data<uint8_t>(it) << "\n";
					break;
				}
				case ILInstruction::woffset16: {
					std::cout << "   woffset " << ILBlock::read_data<uint16_t>(it) << "\n";
					break;
				}
				case ILInstruction::woffset32: {
					std::cout << "   woffset " << ILBlock::read_data<uint32_t>(it) << "\n";
					break;
				}

				case ILInstruction::memcpy: {
					std::cout << "   memcpy ";
					auto off = ILBlock::read_data<ILSize>(it);
					off.print();
					std::cout << "\n";
					break;
				}
				case ILInstruction::memcpy2: {
					std::cout << "   memcpy rev ";
					auto off = ILBlock::read_data<ILSize>(it);
					off.print();
					std::cout << "\n";
					break;
				}

				case ILInstruction::swap: {
					std::cout << "   swap [";
					auto type = ILBlock::read_data<ILDataType>(it);
					dump_data_type(type);
					std::cout << "]\n";
					break;
				}

				case ILInstruction::swap2: {
					std::cout << "   swap2 [";
					auto p = ILBlock::read_data<ILDataTypePair>(it);
					dump_data_type(p.first());
					std::cout << ", ";
					dump_data_type(p.second());
					std::cout << "]\n";
					break;
				}

				case ILInstruction::roffset32: {
					std::cout << "   roffset [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					auto t = ILBlock::read_data<ILSizeType>(it);
					auto off = ILBlock::read_data<uint32_t>(it);

					dump_data_type(pair.first());
					std::cout << "] -> [";
					dump_data_type(pair.second());
					std::cout << "]\n";

					ILSize(t,off).print();
					break;
				}
				case ILInstruction::roffset16: {
					std::cout << "   roffset [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					auto t = ILBlock::read_data<ILSizeType>(it);
					auto off = ILBlock::read_data<uint16_t>(it);

					dump_data_type(pair.first());
					std::cout << "] -> [";
					dump_data_type(pair.second());
					std::cout << "]\n";

					ILSize(t, off).print();
					break;
				}
				case ILInstruction::roffset8: {
					std::cout << "   roffset [";
					auto pair = ILBlock::read_data<ILDataTypePair>(it);
					auto t = ILBlock::read_data<ILSizeType>(it);
					auto off = ILBlock::read_data<uint8_t>(it);

					dump_data_type(pair.first());
					std::cout << "] -> [";
					dump_data_type(pair.second());
					std::cout << "]\n";

					ILSize(t, off).print();
					break;
				}
				case ILInstruction::aroffset: {
					std::cout << "   aroffset [";

					auto p = ILBlock::read_data<ILDataTypePair>(it);
					auto off = ILBlock::read_data<uint8_t>(it);

					dump_data_type(p.first());
					std::cout << "] -> [";
					dump_data_type(p.second());
					std::cout << "] " << (uint32_t)off << "\n";

					break;
				}

				case ILInstruction::wroffset: {
					std::cout << "   wroffset [";
					auto p = ILBlock::read_data<ILDataTypePair>(it);
					auto off = ILBlock::read_data<uint8_t>(it);

					dump_data_type(p.first());
					std::cout << "] -> [";
					dump_data_type(p.second());
					std::cout << "] " << (uint32_t)off << "\n";

					break;
				}

				case ILInstruction::memcmp: {
					std::cout << "   memcmp ";
					auto size = ILBlock::read_data<ILSize>(it);
					size.print();
					std::cout << "\n";
				} break;

				case ILInstruction::memcmp2: {
					std::cout << "   memcmp rev ";
					auto size = ILBlock::read_data<ILSize>(it);
					size.print();
					std::cout << "\n";
				} break;

				case ILInstruction::rmemcmp: {
					std::cout << "   rmemcmp [";
					auto t = ILBlock::read_data<ILDataType>(it);
					dump_data_type(t);
					std::cout << "]\n";
				} break;

				case ILInstruction::rmemcmp2: {
					std::cout << "   rmemcmp rev [";
					auto t = ILBlock::read_data<ILDataType>(it);
					dump_data_type(t);
					std::cout << "]\n";
				} break;

				case ILInstruction::load: {
					std::cout << "   load [";
					auto type = ILBlock::read_data<ILDataType>(it);
					dump_data_type(type);
					std::cout << "]\n";
					break;
				}

				case ILInstruction::forget: {
					std::cout << "   forget [";
					auto type = ILBlock::read_data<ILDataType>(it);
					dump_data_type(type);
					std::cout << "]\n";
					break;
				}
				case ILInstruction::jmpz: {
					std::cout << "   jmpz ";
					auto address = ILBlock::read_data<uint32_t>(it);

					std::cout << address << " \"" << parent->blocks_memory[address]->alias << "\" : ";
					address = ILBlock::read_data<uint32_t>(it);
					std::cout << address << " \"" << parent->blocks_memory[address]->alias << "\"\n";
					break;
				}

				case ILInstruction::slice: std::cout << "   slice constref " << ILBlock::read_data<uint32_t>(it) <<", " << ILBlock::read_data<uint64_t>(it) << "\n"; break;

				case ILInstruction::u8:  std::cout << "   u8 " << (uint16_t)ILBlock::read_data<uint8_t>(it) << "\n"; break;
				case ILInstruction::u16: std::cout << "   u16 " << ILBlock::read_data<uint16_t>(it) << "\n"; break;
				case ILInstruction::u32: std::cout << "   u32 " << ILBlock::read_data<uint32_t>(it) << "\n"; break;
				case ILInstruction::u64: std::cout << "   u64 " << ILBlock::read_data<uint64_t>(it) << "\n"; break;
				case ILInstruction::i8:  std::cout << "   i8 " << (int16_t)ILBlock::read_data<int8_t>(it) << "\n"; break;
				case ILInstruction::i16: std::cout << "   i16 " << ILBlock::read_data<int16_t>(it) << "\n"; break;
				case ILInstruction::i32: std::cout << "   i32 " << ILBlock::read_data<int32_t>(it) << "\n"; break;
				case ILInstruction::i64: std::cout << "   i64 " << ILBlock::read_data<int64_t>(it) << "\n"; break;
				case ILInstruction::f32: std::cout << "   f32 " << ILBlock::read_data<float>(it) << "\n"; break;
				case ILInstruction::f64: std::cout << "   f64 " << ILBlock::read_data<double>(it) << "\n"; break;
				case ILInstruction::word: std::cout << "   word " << ILBlock::read_data<void*>(it) << "\n"; break;
				case ILInstruction::size8: {
					std::cout << "   size ";
					auto t = ILBlock::read_data<ILSizeType>(it);
					auto v = ILBlock::read_data<uint8_t>(it);
					ILSize(t,(tableid_t)v).print();
					std::cout << "\n";
				}break;
				case ILInstruction::size16: {
					std::cout << "   size ";
					auto t = ILBlock::read_data<ILSizeType>(it);
					auto v = ILBlock::read_data<uint16_t>(it);
					ILSize(t, (tableid_t)v).print();
					std::cout << "\n";
				}break;
				case ILInstruction::size32: {
					std::cout << "   size ";
					auto t = ILBlock::read_data<ILSizeType>(it);
					auto v = ILBlock::read_data<uint32_t>(it);
					ILSize(t, (tableid_t)v).print();
					std::cout << "\n";
				}break;

			}
		}
	}

	bool ILBytecodeFunction::assert_flow() {
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



	void* ILEvaluator::read_last_register_value_indirect(ILDataType rs) {
		size_t ctrs = compile_time_register_size(rs);
		switch (ctrs) {
			case 1:
				return (void*)(register_stack_pointer_1b - 1);
			case 2:
				return (void*)(register_stack_pointer_2b - 1);
			case 3:
			case 4:
				return (void*)(register_stack_pointer_4b - 1);
			default:
				return (void*)(register_stack_pointer_8b - 1);
		}
	}
}