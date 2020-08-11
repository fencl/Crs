
#include "IL.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <fstream>

namespace Corrosive {

	thread_local std::vector<ILModule*> ILModule::current;

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

	size_t align_up(size_t value, size_t alignment) {
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

	ILNativeFunction* ILModule::create_native_function(std::string alias) {
		std::unique_ptr<ILNativeFunction> function = std::make_unique<ILNativeFunction>();
		ILNativeFunction* function_ptr = function.get();
		function_ptr->id = (uint32_t)functions.size();
		function_ptr->parent = this;
		external_functions[alias] = function_ptr->id;
		function_ptr->alias = std::move(alias);
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

						stack_size = align_up(stack_size, elem_align);
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
		current.push_back(this);
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
		current.pop_back();
	}

	
	bool operator < (const ILSize& l,const ILSize& r) {
		if (l.type < r.type) return true;
		return l.value < r.value;
	}


	uint32_t ILModule::register_structure_table() {
		structure_tables.push_back(ILStructTable());
		return (uint32_t)(structure_tables.size() - 1);
	}
	uint32_t ILModule::register_array_table(ILSize s, tableelement_t count) {
		std::pair<ILSize, tableelement_t> key(s,count);
		auto ins = array_tables_map.insert(std::make_pair(key,(tableid_t)0));
		if (ins.second) {
			ILArrayTable t;
			t.count = count;
			t.element = s;
			ins.first->second = (tableid_t)(array_tables.size() - 1);
			array_tables.push_back(t);
		}

		return ins.first->second;
	}

	size_t ILSize::eval(ILModule* mod, ILArchitecture arch) const {
		switch (type) {
			case ILSizeType::_0: return 0;

			case ILSizeType::abs8: {
				return (size_t)value;
			}
			case ILSizeType::abs16: {
				return (size_t)value*2;
			}
			
			case ILSizeType::absf32:
			case ILSizeType::abs32: {
				return (size_t)value*4;
			}
			
			case ILSizeType::absf64:
			case ILSizeType::abs64: {
				return (size_t)value*8;
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
			case ILSizeType::_0:
			case ILSizeType::abs8: return 1;

			case ILSizeType::abs16: return 2;

			case ILSizeType::absf32:
			case ILSizeType::abs32: return 4;

			
			case ILSizeType::absf64: return 8;

			case ILSizeType::abs64: {
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
				calculated_size = align_up(calculated_size, elem_align);
				calculated_offsets[id++] = calculated_size;
				calculated_size += elem->eval(mod, arch);
				calculated_alignment = std::max(calculated_alignment, elem_align);
			}

			calculated_size = align_up(calculated_size, calculated_alignment);
			calculated_for = arch;
		}
	}

	void ILArrayTable::calculate(ILModule* mod, ILArchitecture arch) {
		if (arch != calculated_for) {
			calculated_alignment = element.alignment(mod, arch);
			calculated_size = (size_t)(align_up(element.eval(mod, arch), calculated_alignment) * count);
			calculated_for = arch;
		}
	}

	void ILSize::print(ILModule* mod) {
		switch (type) {
			case ILSizeType::_0: {
				std::cout<<"[0]";
			}
			case ILSizeType::abs8: {
				std::cout << "["<<value << " x i8]";
			}break;
			case ILSizeType::abs16: {
				std::cout << "["<<value << " x i16]";
			}break;
			case ILSizeType::abs32: {
				std::cout << "["<<value << " x i32]";
			}break;
			case ILSizeType::abs64: {
				std::cout << "["<<value << " x i64]";
			}break;
			
			case ILSizeType::absf32: {
				std::cout << "["<<value << " x f32]";
			}break;
			
			case ILSizeType::absf64: {
				std::cout << "["<<value << " x f64]";
			}break;

			case ILSizeType::word: {
				std::cout << "["<<value << " x word]";
			}break;

			case ILSizeType::table: {
				ILStructTable& table = mod->structure_tables[value];
				std::cout << "{";
				for (size_t i=0; i<table.elements.size(); ++i) {
					if (i>0) std::cout<<",";
					std::cout<<" ";
					table.elements[i].print(mod);
				}
				
				std::cout << " }";
			}break;

			case ILSizeType::array: {
				ILArrayTable& table = mod->array_tables[value];
				std::cout<<"["<<table.count<<" x ";
				table.element.print(mod);
				std::cout<<"]";
			} break;
		}
	}



	ILSize::ILSize() : type(ILSizeType::_0), value(0) {}
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
					ILSize(t,off).print(parent->parent);
					std::cout << "\n";
					break;
				}
				case ILInstruction::offset16: {
					std::cout << "   offset ";
					auto t = ILBlock::read_data<ILSizeType>(it);
					auto off = ILBlock::read_data<uint16_t>(it);
					ILSize(t, off).print(parent->parent);
					std::cout << "\n";
					break;
				}
				case ILInstruction::offset8: {
					std::cout << "   offset ";
					auto t = ILBlock::read_data<ILSizeType>(it);
					auto off = ILBlock::read_data<uint8_t>(it);
					ILSize(t, off).print(parent->parent);
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
					off.print(parent->parent);
					std::cout << "\n";
					break;
				}
				case ILInstruction::memcpy2: {
					std::cout << "   memcpy rev ";
					auto off = ILBlock::read_data<ILSize>(it);
					off.print(parent->parent);
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

					ILSize(t,off).print(parent->parent);
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

					ILSize(t, off).print(parent->parent);
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

					ILSize(t, off).print(parent->parent);
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
					size.print(parent->parent);
					std::cout << "\n";
				} break;

				case ILInstruction::memcmp2: {
					std::cout << "   memcmp rev ";
					auto size = ILBlock::read_data<ILSize>(it);
					size.print(parent->parent);
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
					ILSize(t,(tableid_t)v).print(parent->parent);
					std::cout << "\n";
				}break;
				case ILInstruction::size16: {
					std::cout << "   size ";
					auto t = ILBlock::read_data<ILSizeType>(it);
					auto v = ILBlock::read_data<uint16_t>(it);
					ILSize(t, (tableid_t)v).print(parent->parent);
					std::cout << "\n";
				}break;
				case ILInstruction::size32: {
					std::cout << "   size ";
					auto t = ILBlock::read_data<ILSizeType>(it);
					auto v = ILBlock::read_data<uint32_t>(it);
					ILSize(t, (tableid_t)v).print(parent->parent);
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

	
	void ILModule::try_link(std::string name,void* ptr){
		auto f = external_functions.find(name);
		if (f != external_functions.end()) {
			((ILNativeFunction*)functions[f->second].get())->ptr = ptr;
		}
	}

	void ILBytecodeFunction::save(ILOutputStream& file) {

		file.u32(local_stack_lifetime.id);
		file.s( local_stack_lifetime.lifetime.size());
		file.write(local_stack_lifetime.lifetime.data(), local_stack_lifetime.lifetime.size());
		file.s(blocks.size());
		for (auto&& block : blocks) {
			file.s(block->data_pool.size());
			file.write(block->data_pool.data(), block->data_pool.size());
		}
	}

	void ILBytecodeFunction::load(ILInputStream& file) {
		local_stack_lifetime.id = file.u32();
		local_stack_lifetime.lifetime = std::vector<uint8_t>(file.s());
		file.read(local_stack_lifetime.lifetime.data(), local_stack_lifetime.lifetime.size());


		blocks_memory = std::vector<std::unique_ptr<ILBlock>>(file.s());
		blocks = std::vector<ILBlock*>(blocks_memory.size());

		for (size_t i=0; i<blocks_memory.size(); ++i) {
			std::unique_ptr<ILBlock> block = std::make_unique<ILBlock>();
			block->parent = this;

			size_t datapool_size = file.s();
			block->data_pool = std::vector<uint8_t>(datapool_size);
			file.read((char*)block->data_pool.data(), datapool_size);

			blocks[i] = block.get();
			blocks_memory[i] = std::move(block);
		}

	}


	void ILModule::save(ILOutputStream& file) {
		bool has_entry_point = entry_point != nullptr;
		file.b(has_entry_point);
		file.u32(entry_point->id);

		file.s(functions.size());
		for (auto&& func: functions) {
			if (auto bfunc = dynamic_cast<ILBytecodeFunction*>(func.get())) {
				file.u8(1);
				file.u32(func->decl_id);
				file.s(func->alias.size());
				file.write(func->alias.data(), func->alias.size());
				bfunc->save(file);
			}else {
				file.u8(2);
				file.u32(func->decl_id);
				file.s(func->alias.size());
				file.write(func->alias.data(), func->alias.size());
			}
		}

		file.s(function_decl.size());
		for (auto&& decl : function_decl) {
			file.u8((uint8_t)std::get<0>(decl));
			file.dt(std::get<1>(decl));
			file.s(std::get<2>(decl).size());
			file.write(std::get<2>(decl).data(), std::get<2>(decl).size());
		}

		
		file.s(structure_tables.size());
		for (auto&& table : structure_tables) {
			file.s(table.elements.size());
			for (auto && elem : table.elements) {
				file.u8((uint8_t)elem.type);
				file.u32(elem.value);
			}
		}

		file.s(array_tables.size());
		for (auto&& table : array_tables) {
			file.u32(table.count);
			file.u8((uint8_t)table.element.type);
			file.u32(table.element.value);
		}

		
		file.s(vtable_data.size());
		for (auto&& vtable : vtable_data) {
			file.u32(vtable.first);
			for (uint32_t i=0; i<vtable.first; ++i) {
				ILFunction* fun = (ILFunction*)vtable.second[i];
				file.u32(fun->id);
			}
		}

		file.s(constant_memory.size());
		for (auto&& con_mem: constant_memory) {
			file.u8((uint8_t)con_mem.first.type);
			file.u32((uint8_t)con_mem.first.value);
			con_mem.first.save(this, file, con_mem.second.get());
		}

		
		file.s(static_memory.size());
		for (auto&& static_mem: static_memory) {
			file.u8((uint8_t)static_mem.first.type);
			file.u32((uint8_t)static_mem.first.value);
			static_mem.first.save(this, file, static_mem.second.get());
		}

	}

	void ILModule::load(ILInputStream& file) {
		external_functions.clear();

		bool has_entry_point = file.b();
		uint32_t e_point_id = file.u32();


		size_t read_size = file.s();
		functions = std::vector<std::unique_ptr<ILFunction>>(read_size);
		for (size_t funcid = 0; funcid < read_size; ++funcid) {
			uint8_t ftype = file.u8();			
			if (ftype == 1) {
				auto fun = std::make_unique<ILBytecodeFunction>();
				fun->parent = this;
				fun->id = funcid;
				fun->decl_id = file.u32();
				size_t alias_size = file.s();
				fun->alias = std::string(alias_size,'\0');
				file.read(fun->alias.data(), alias_size);
				fun->load(file);
				functions[funcid] = std::move(fun);
			} else if (ftype == 2) {
				auto fun = std::make_unique<ILNativeFunction>();
				fun->parent = this;
				fun->id = funcid;
				fun->decl_id = file.u32();
				size_t alias_size = file.s();
				fun->alias = std::string(alias_size,'\0');
				file.read(fun->alias.data(), alias_size);
				external_functions[fun->alias] = fun->id;
				functions[funcid] = std::move(fun);
			}
		}

		read_size = file.s();
		function_decl = std::vector<std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>>>(read_size);
		for (size_t declid = 0; declid < read_size; ++declid) {
			ILCallingConvention cconv = (ILCallingConvention)file.u8();
			ILDataType ret = file.dt();
			size_t asize = file.s();
			std::vector<ILDataType> args(asize);
			file.read(args.data(),args.size());
			function_decl[declid] = std::make_tuple(cconv, ret, std::move(args));
		}


		read_size = file.s();
		structure_tables = std::vector<ILStructTable>(read_size);
		for (size_t tid = 0; tid < read_size; ++tid) {
			ILStructTable table;
			size_t elem_size = file.s();
			table.elements = std::vector<ILSize>(elem_size);
			for (size_t elem_id = 0; elem_id<elem_size; ++elem_id) {
				table.elements[elem_id].type = (ILSizeType)file.u8();
				table.elements[elem_id].value = file.u32();
			}
			structure_tables[tid] = std::move(table);
		}

		read_size = file.s();
		array_tables = std::vector<ILArrayTable>(read_size);
		for (size_t tid = 0; tid < read_size; ++tid) {
			ILArrayTable table;
			table.count = file.u32();
			table.element.type = (ILSizeType)file.u8();
			table.element.value = file.u32();
			array_tables_map[std::make_pair(table.element, table.count)] = tid;
			array_tables[tid] = std::move(table);
		}

		read_size = file.s();
		vtable_data = std::vector<std::pair<uint32_t,std::unique_ptr<void*[]>>>(read_size);
		for (size_t tid = 0; tid < read_size; ++tid) {
			std::pair<uint32_t, std::unique_ptr<void*[]>> vtable;
			vtable.first = file.u32();
			vtable.second = std::make_unique<void*[]>(vtable.first);
			for(uint32_t i=0;i<vtable.first;++i) {
				vtable.second[i] = functions[file.u32()].get();
			}
			vtable_data[tid] = std::move(vtable);
		}

		read_size = file.s();
		constant_memory = std::vector<std::pair<ILSize,std::unique_ptr<unsigned char[]>>>(read_size);
		for (size_t cid = 0; cid < read_size; ++cid) {
			ILSize s;
			s.type = (ILSizeType)file.u8();
			s.value = file.u32();
			size_t es = s.eval(this, compiler_arch);
			auto mem = std::make_unique<unsigned char[]>(es);
			s.load(this, file, mem.get());
			constant_memory[cid] = std::make_pair(s, std::move(mem));
		}

		read_size = file.s();
		static_memory = std::vector<std::pair<ILSize,std::unique_ptr<unsigned char[]>>>(read_size);
		for (size_t sid = 0; sid < read_size; ++sid) {
			ILSize s;
			s.type = (ILSizeType)file.u8();
			s.value = file.u32();
			size_t es = s.eval(this, compiler_arch);
			auto mem = std::make_unique<unsigned char[]>(es);
			s.load(this, file, mem.get());
			static_memory[sid] = std::make_pair(s, std::move(mem));
		}

		entry_point = has_entry_point?functions[e_point_id].get() : nullptr;
	}


	void ILOutputStream::u8(uint8_t v) {
		target.put(v);
	}
	void ILOutputStream::i8(int8_t v) {
		target.put(v);
	}
	void ILOutputStream::u16(uint16_t v){
		target.put((uint8_t)((v)&0x00ff));
		target.put((uint8_t)((v>>8)&0x00ff));
	}
	void ILOutputStream::i16(int16_t v){
		u16(*(uint16_t*)&v);
	}
	void ILOutputStream::u32(uint32_t v){
		target.put((uint8_t)((v)&0x000000ff));
		target.put((uint8_t)((v>>8)&0x000000ff));
		target.put((uint8_t)((v>>16)&0x000000ff));
		target.put((uint8_t)((v>>24)&0x000000ff));
	}
	void ILOutputStream::i32(int32_t v){
		u32(*(uint32_t*)&v);
	}
	void ILOutputStream::u64(uint64_t v){
		target.put((uint8_t)((v)&0x00000000000000ff));
		target.put((uint8_t)((v>>8)&0x00000000000000ff));
		target.put((uint8_t)((v>>16)&0x00000000000000ff));
		target.put((uint8_t)((v>>24)&0x00000000000000ff));
		target.put((uint8_t)((v>>32)&0x00000000000000ff));
		target.put((uint8_t)((v>>40)&0x00000000000000ff));
		target.put((uint8_t)((v>>48)&0x00000000000000ff));
		target.put((uint8_t)((v>>56)&0x00000000000000ff));
	}
	void ILOutputStream::i64(int64_t v){
		u64(*(uint64_t*)&v);
	}
	void ILOutputStream::s(size_t v){
		u64((uint64_t)v);
	}
	
	void ILOutputStream::dt(ILDataType v){
		u8((uint8_t)v);
	}
	
	void ILOutputStream::b(bool v){
		u8((uint8_t)(v?1:0));
	}

	
	void ILOutputStream::write(void* v, size_t s) {
		if (s>0) {
			uint8_t* ptr = (uint8_t*)v;
			for (size_t i=0; i<s; ++i) {
				target.put(*ptr++);
			}
		}
	}


	uint8_t ILInputStream::u8(){
		return (uint8_t)target.get();
	}
	int8_t ILInputStream::i8() {
		uint8_t v= u8();
		return *(int8_t*)&v;
	}
	
	uint16_t ILInputStream::u16(){
		uint16_t v = 0;
		uint8_t r;
		r = (uint8_t)target.get();
		v |= (((uint16_t)r))     & 0x00ff;
		r = (uint8_t)target.get();
		v |= (((uint16_t)r)<<8)  & 0xff00;
		return v;
	}
	int16_t ILInputStream::i16() {
		uint16_t v = u16();
		return *(int16_t*)&v;
	}

	uint32_t ILInputStream::u32(){
		uint32_t v = 0;
		uint8_t r;
		r = (uint8_t)target.get();
		v |= (((uint32_t)r))     & 0x000000ff;
		r = (uint8_t)target.get();
		v |= (((uint32_t)r)<<8)  & 0x0000ff00;
		r = (uint8_t)target.get();
		v |= (((uint32_t)r)<<16) & 0x00ff0000;
		r = (uint8_t)target.get();
		v |= (((uint32_t)r)<<24) & 0xff000000;
		return v;
	}
	int32_t ILInputStream::i32() {
		uint32_t v = u32();
		return *(int32_t*)&v;
	}
	uint64_t ILInputStream::u64() {
		uint64_t v = 0;
		uint8_t r;
		r = (uint8_t)target.get();
		v |= (((uint64_t)r))     & 0x00000000000000ff;
		r = (uint8_t)target.get();
		v |= (((uint64_t)r)<<8)  & 0x000000000000ff00;
		r = (uint8_t)target.get();
		v |= (((uint64_t)r)<<16) & 0x0000000000ff0000;
		r = (uint8_t)target.get();
		v |= (((uint64_t)r)<<24) & 0x00000000ff000000;
		r = (uint8_t)target.get();
		v |= (((uint64_t)r)<<32) & 0x000000ff00000000;
		r = (uint8_t)target.get();
		v |= (((uint64_t)r)<<40) & 0x0000ff0000000000;
		r = (uint8_t)target.get();
		v |= (((uint64_t)r)<<48) & 0x00ff000000000000;
		r = (uint8_t)target.get();
		v |= (((uint64_t)r)<<56) & 0xff00000000000000;
		return v;
	}
	int64_t ILInputStream::i64() {
		uint64_t v = u64();
		return *(int64_t*)&v;
	}
	size_t ILInputStream::s() {
		return (size_t)u64();
	}
	ILDataType ILInputStream::dt() {
		return (ILDataType)u8();
	}
	bool ILInputStream::b() {
		return (bool)u8();
	}
	void ILInputStream::read(void* v, size_t s) {
		if (s>0) {
			uint8_t* ptr = (uint8_t*)v;
			for (size_t i=0; i<s; ++i) {
				uint8_t vb = (uint8_t)target.get();
				*ptr++ = vb;
			}
		}
	}

	
	void ILSize::load(ILModule* mod, ILInputStream& stream, unsigned char* ptr) {
		switch (type)
		{
			case ILSizeType::abs8:
				for (uint32_t i=0;i<value;++i) { *(uint8_t*)ptr = stream.u8(); ptr+=1; }
				break;

			case ILSizeType::abs16:
				for (uint32_t i=0;i<value;++i) { *(uint16_t*)ptr = stream.u16(); ptr+=2; }
				break;

			case ILSizeType::absf32:
			case ILSizeType::abs32:
				for (uint32_t i=0;i<value;++i) { *(uint32_t*)ptr = stream.u32(); ptr+=4; }
				break;

			case ILSizeType::absf64:
			case ILSizeType::abs64:
				for (uint32_t i=0;i<value;++i) { *(uint64_t*)ptr = stream.u64(); ptr+=8; }
				break;

			case ILSizeType::word:
				switch (compiler_arch)
				{
					case ILArchitecture::bit64:
						ptr+=8;
						break;

					case ILArchitecture::bit32:
						ptr+=4;
						break;
				}
				break;
			case ILSizeType::table: {
				size_t s = eval(mod, compiler_arch);
				auto& table = mod->structure_tables[value];
				table.calculate(mod, compiler_arch);
				for (size_t e = 0; e<table.elements.size(); ++e) {
					unsigned char* elem_off = ptr + table.calculated_offsets[e];
					table.elements[e].load(mod,stream, elem_off);
				}
				ptr+=s;
			}
			case ILSizeType::array: {
				size_t s = eval(mod, compiler_arch);
				auto& table = mod->array_tables[value];
				table.calculate(mod, compiler_arch);
				table.element.load(mod, stream, ptr);
				ptr+=s;
			}
		}
	}

	void ILSize::save(ILModule* mod, ILOutputStream& stream, unsigned char* ptr) {
		switch (type)
		{
			case ILSizeType::abs8:
				for (uint32_t i=0;i<value;++i) { stream.u8(*(uint8_t*)ptr); ptr+=1; }
				break;

			case ILSizeType::abs16:
				for (uint32_t i=0;i<value;++i) { stream.u16(*(uint16_t*)ptr); ptr+=2; }
				break;

			case ILSizeType::abs32:
			case ILSizeType::absf32:
				for (uint32_t i=0;i<value;++i) { stream.u32(*(uint32_t*)ptr); ptr+=4; }
				break;

			case ILSizeType::abs64:
			case ILSizeType::absf64:
				for (uint32_t i=0;i<value;++i) { stream.u64(*(uint64_t*)ptr); ptr+=8; }
				break;

			case ILSizeType::word:
				switch (compiler_arch)
				{
					case ILArchitecture::bit64:
						ptr+=8;
						break;

					case ILArchitecture::bit32:
						ptr+=4;
						break;
				}
				break;
			case ILSizeType::table: {
				size_t s = eval(mod, compiler_arch);
				auto& table = mod->structure_tables[value];
				table.calculate(mod, compiler_arch);
				for (size_t e = 0; e<table.elements.size(); ++e) {
					unsigned char* elem_off = ptr + table.calculated_offsets[e];
					table.elements[e].save(mod,stream, elem_off);
				}
				ptr+=s;
			}
			case ILSizeType::array: {
				size_t s = eval(mod, compiler_arch);
				auto& table = mod->array_tables[value];
				table.calculate(mod, compiler_arch);
				table.element.save(mod, stream, ptr);
				ptr+=s;
			}
		}
	}

}