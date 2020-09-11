
#include "IL.hpp"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <unordered_map>
#include <fstream>
#include <cstring>

namespace Crs {

#ifdef DEBUG
	errvoid err::ok = (err_undef_true_tag());
	errvoid err::fail = (err_undef_false_tag());
#else
	errvoid err::ok = true;
	errvoid err::fail = false;
#endif


	
	thread_local ILEvaluator* ILEvaluator::active = nullptr;

	void throw_il_wrong_data_flow_error() {
		throw string_exception("Compiler Error, Wrong data flow inside compiler IL");
	}

	void throw_il_nothing_on_stack_error() {
		throw string_exception("Compiler Error, Instruction requires more argumens than the number of arguments on the stack");
	}

	void throw_il_wrong_type_error() {
		throw string_exception("Compiler Error, Passed broken type");
	}

	void throw_il_remaining_stack_error() {
		throw string_exception("Compiler Error, Stack is not empty after terminator instruction");
	}

	void throw_il_wrong_arguments_error() {
		throw string_exception("Compiler Error, Instruction cannot use argument(s) on the stack");
	}

	void throw_runtime_exception_header(const ILEvaluator* eval, std::stringstream& cerr) {
		if (eval->debug_file < eval->parent->debug_file_names.size()) {
			cerr << "\n | Error (" << eval->parent->debug_file_names[eval->debug_file] << ": " << (eval->debug_line + 1) << "):\n | \t";
		}
		else {
			cerr << "\n | Error (external):\n | \t";
		}
	}

	void throw_runtime_exception_footer(const ILEvaluator* eval, std::stringstream& cerr) {
		cerr << "\n |";
		for (auto t = eval->debug_callstack.rbegin(); t != eval->debug_callstack.rend(); t++) {
			if (std::get<1>(*t) < eval->parent->debug_file_names.size()) {
				cerr << "\n | At (" << eval->parent->debug_file_names[std::get<1>(*t)] << ": " << (std::get<0>(*t) + 1) << ") called " << std::get<2>(*t);
			}
			else {
				cerr << "\n | At (external) called " << std::get<2>(*t);
			}
		}
		cerr << "\n\n";
	}

	void ILEvaluator::reset_debug() {
		debug_file = UINT16_MAX;
		debug_line = 0;
		debug_callstack.clear();
	}

	std::size_t align_up(std::size_t value, std::size_t alignment) {
		return alignment == 0 ? value : ((value % alignment == 0) ? value : value + (alignment - (value % alignment)));
	}

	errvoid throw_runtime_exception(const ILEvaluator* eval, std::string_view message) {
		std::stringstream cerr;
		throw_runtime_exception_header(eval, cerr);
		cerr << message;
		throw_runtime_exception_footer(eval, cerr);
		std::cerr << cerr.str();
		return err::fail;
	}

	errvoid throw_segfault_exception(const ILEvaluator* eval, int signal) {
		std::stringstream cerr;
		throw_runtime_exception_header(eval, cerr);

		cerr << "Attempt to access protected memory range (Segmentation fault [" << signal << "])";
		throw_runtime_exception_footer(eval, cerr);
		std::cerr << cerr.str();
		return err::fail;
	}

	errvoid throw_interrupt_exception(const ILEvaluator* eval, int signal) {
		std::stringstream cerr;
		throw_runtime_exception_header(eval, cerr);

		cerr << "Interrupt exception (Interrupt [" << signal << "])";
		throw_runtime_exception_footer(eval, cerr);
		std::cerr << cerr.str();
		return err::fail;
	}


	bool ILModule::MemoryMapCompare::operator()(const std::pair<void*,void*>& l, const std::pair<void*,void*>& r) const {
		return l.second <= r.first;
	}
	
	std::uint16_t ILModule::register_debug_source(std::string name) {
		debug_file_names.push_back(name);
		return (std::uint16_t)debug_file_names.size() - 1;
	}

	ILFunction::~ILFunction() {}

	ILBytecodeFunction* ILModule::create_function(ILContext context) {
		std::unique_ptr<ILBytecodeFunction> function = std::make_unique<ILBytecodeFunction>();
		ILBytecodeFunction* function_ptr = function.get();
		function_ptr->id = (std::uint32_t)functions.size();
		function_ptr->parent = this;
		function_ptr->context = context;
		functions.push_back(std::move(function));
		return function_ptr;
	}

	ILNativeFunction* ILModule::create_native_function(std::string alias) {
		std::unique_ptr<ILNativeFunction> function = std::make_unique<ILNativeFunction>();
		ILNativeFunction* function_ptr = function.get();
		function_ptr->id = (std::uint32_t)functions.size();
		function_ptr->parent = this;
		external_functions[alias] = function_ptr->id;
		function_ptr->name = std::move(alias);
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
		block->id = (std::uint32_t)blocks_memory.size();
		block->parent = this;
		blocks_memory.push_back(std::move(block));
		return block_ptr;
	}


	std::uint32_t ILModule::register_vtable(std::uint32_t size, std::unique_ptr<void* []> table) {
		vtable_data.push_back(std::make_pair(size,std::move(table)));

		unsigned char* ptr1 = (unsigned char*)vtable_data.back().second.get();
		unsigned char* ptr2 = ptr1 + vtable_data.back().first*sizeof(void*);
		return (std::uint32_t)vtable_data.size() - 1;
	}

	void ILBytecodeFunction::append_block(ILBlock* block) {
		blocks.push_back(block);
	}

	std::uint8_t* ILBlock::reserve_data(std::size_t size) {
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
		if (!ILBytecodeFunction::print_function(this)) return;
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


	stackid_t ILLifetime::append_unknown(std::size_t& holder) {
		lifetime.push_back((unsigned char)ILLifetimeEvent::append);
		holder = lifetime.size();
		lifetime.push_back(0);
		lifetime.push_back(0);
		lifetime.push_back(0);
		lifetime.push_back(0);
		lifetime.push_back(0);
		return id++;
	}

	void ILLifetime::resolve_unknown(std::size_t holder, ILSize s) {

		lifetime[holder] = (unsigned char)s.type;
		lifetime[holder + 1] = (s.value >> 24) & (unsigned char)0xFF;
		lifetime[holder + 2] = (s.value >> 16) & (unsigned char)0xFF;
		lifetime[holder + 3] = (s.value >> 8) & (unsigned char)0xFF;
		lifetime[holder + 4] = (s.value) & (unsigned char)0xFF;
	}

	std::uint32_t ILModule::register_constant(unsigned char* memory, ILSize size) {
		std::size_t compile_size = size.eval(this);
		auto data = std::make_unique<unsigned char[]>(compile_size);
		std::memcpy(data.get(), memory, compile_size);
		constant_memory.push_back(std::make_pair(size,std::move(data)));

		unsigned char* ptr1 = (unsigned char*)constant_memory.back().second.get();
		unsigned char* ptr2 = ptr1 + compile_size;
		return (std::uint32_t)constant_memory.size() - 1;
	}

	std::uint32_t ILModule::register_static(unsigned char* memory, ILSize size, ILBytecodeFunction* init) {
		std::size_t compile_size = size.eval(this);
		auto data = std::make_unique<unsigned char[]>(compile_size);
		if (memory != nullptr) {
			std::memcpy(data.get(), memory, compile_size);
		}
		std::uint32_t fid = UINT32_MAX;
		if (init) { fid = init->id; }
		static_memory.push_back(std::make_tuple(size, std::move(data), fid, static_memory.size()));

		unsigned char* ptr1 = (unsigned char*)std::get<1>(static_memory.back()).get();
		unsigned char* ptr2 = ptr1 + compile_size;
		return (std::uint32_t)static_memory.size() - 1;
	}

	void ILBytecodeFunction::calculate_stack() {
		if (calculated_local_stack_alignment == 0) {
			calculated_local_stack_alignment = 1;
			calculated_local_stack_size = 0;
			std::size_t stack_size = 0;
			std::vector<std::size_t> stack_sizes;
			stack_sizes.push_back(0);
			calculated_local_offsets.resize(local_stack_lifetime.id);

			std::size_t lid = 0;
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
						std::uint32_t ptr_val = (((std::uint32_t) * (ptr++)) << 24) | (((std::uint32_t) * (ptr++)) << 16) | (((std::uint32_t) * (ptr++)) << 8) | (((std::uint32_t) * (ptr++)));

						ILSize ptr_s = ILSize(ptr_t, ptr_val);
						std::size_t elem_align = ptr_s.alignment(parent);
						calculated_local_stack_alignment = std::max(elem_align, calculated_local_stack_alignment);

						stack_size = align_up(stack_size, elem_align);
						calculated_local_offsets[lid++] = stack_size;
						std::size_t sz = ptr_s.eval(parent);
						stack_size += sz;
						calculated_local_stack_size = std::max(calculated_local_stack_size, stack_size);
					}break;
				}
			}
		}
	}


	void ILModule::run(ILFunction* func) {
		auto eval = std::make_unique<ILEvaluator>();
		eval->parent = this;
		ILEvaluator::active = eval.get();

		if (ILBuilder::eval_fncall(func)) {
			auto lr1b = (std::size_t)(eval->register_stack_pointer_1b - eval->register_stack_1b);
			if (lr1b > 0) { std::cout << "leaked 1 byte registers: " << lr1b << "\n"; }
			auto lr2b = (std::size_t)(eval->register_stack_pointer_2b - eval->register_stack_2b);
			if (lr2b) { std::cout << "leaked 2 byte registers: " << lr2b << "\n"; }
			auto lr4b = (std::size_t)(eval->register_stack_pointer_4b - eval->register_stack_4b);
			if (lr4b) { std::cout << "leaked 4 byte registers: " << lr4b << "\n"; }
			auto lr8b = (std::size_t)(eval->register_stack_pointer_8b - eval->register_stack_8b);
			if (lr8b) { std::cout << "leaked 8 byte registers: " << lr8b << "\n"; }
		}
	}

	errvoid ILModule::initialize_statics() {
		auto eval = std::make_unique<ILEvaluator>();
		eval->parent = this;
		ILEvaluator::active = eval.get();

		for (auto&& s : static_memory) {
			if (std::get<2>(s) != UINT32_MAX) {
				if (ILBuilder::eval_fncall(functions[std::get<2>(s)].get())) return err::fail;
			}
		}
		return err::ok;
	}

	
	bool operator < (const ILSize& l,const ILSize& r) {
		if (l.type < r.type) return true;
		return l.value < r.value;
	}


	std::uint32_t ILModule::register_structure_table() {
		structure_tables.push_back(ILStructTable());
		return (std::uint32_t)(structure_tables.size() - 1);
	}
	std::uint32_t ILModule::register_array_table(ILSize s, tableelement_t count) {
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

	std::size_t ILSize::eval(ILModule* mod) const {
		switch (type) {
			case ILSizeType::_0: return 0;

			case ILSizeType::abs8: {
				return (std::size_t)value;
			}
			case ILSizeType::abs16: {
				return (std::size_t)value*2;
			}
			
			case ILSizeType::absf32:
			case ILSizeType::abs32: {
				return (std::size_t)value*4;
			}
			
			case ILSizeType::absf64:
			case ILSizeType::abs64: {
				return (std::size_t)value*8;
			}

			case ILSizeType::word: 
			case ILSizeType::ptr: {
				return (std::size_t)value * sizeof(void*);
			}

			case ILSizeType::slice: {
				return (std::size_t)value * sizeof(void*) * 2;
			}

			case ILSizeType::table: {
				auto& stable = mod->structure_tables[value];
				stable.calculate(mod);
				return stable.calculated_size;
			}

			case ILSizeType::array: {
				auto& stable = mod->array_tables[value];
				stable.calculate(mod);
				return stable.calculated_size;
			}
		}

		return 0;
	}

	std::uint32_t _upper_power_of_two(std::uint32_t v)
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

	std::size_t ILSize::alignment(ILModule* mod) const {
		switch (type) {
			case ILSizeType::_0:
			case ILSizeType::abs8: return alignof(std::uint8_t);
			case ILSizeType::abs16: return alignof(std::uint16_t);
			case ILSizeType::absf32:
			case ILSizeType::abs32: return alignof(std::uint32_t);
			case ILSizeType::absf64: return alignof(double);
			case ILSizeType::abs64:	return alignof(std::uint64_t);

			case ILSizeType::slice:
			case ILSizeType::word:
			case ILSizeType::ptr: {
				return sizeof(void*);
			}

			case ILSizeType::table: {
				auto& stable = mod->structure_tables[value];
				stable.calculate(mod);
				return stable.calculated_alignment;
			}

			case ILSizeType::array: {
				auto& stable = mod->array_tables[value];
				stable.calculate(mod);
				return stable.calculated_alignment;
			}
		}

		return 0;
	}

	const ILSize ILSize::single_ptr = { ILSizeType::ptr,1 };
	const ILSize ILSize::double_ptr = { ILSizeType::ptr,2 };
	const ILSize ILSize::slice = { ILSizeType::slice, 1};
	const ILSize ILSize::single_word = { ILSizeType::word, 1};

	void ILStructTable::calculate(ILModule* mod) {
		if (calculated_alignment == 0) {
			calculated_size = 0;
			calculated_alignment = 1;
			calculated_offsets.resize(elements.size());

			std::size_t id = 0;
			for (auto elem = elements.begin(); elem != elements.end(); elem++) {
				std::size_t elem_align = elem->alignment(mod);
				calculated_size = align_up(calculated_size, elem_align);
				calculated_offsets[id++] = calculated_size;
				calculated_size += elem->eval(mod);
				calculated_alignment = std::max(calculated_alignment, elem_align);
			}

			calculated_size = align_up(calculated_size, calculated_alignment);
		}
	}

	void ILArrayTable::calculate(ILModule* mod) {
		if (calculated_alignment == 0) {
			calculated_alignment = element.alignment(mod);
			calculated_size = (std::size_t)(align_up(element.eval(mod), calculated_alignment) * count);
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

			case ILSizeType::ptr: {
				std::cout << "["<<value << " x word]";
			}break;

			case ILSizeType::table: {
				ILStructTable& table = mod->structure_tables[value];
				std::cout << "{";
				for (std::size_t i=0; i<table.elements.size(); ++i) {
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


	std::uint32_t ILModule::register_function_decl(std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>> decl) {
		function_decl.push_back(std::move(decl));
		return (std::uint32_t)(function_decl.size() - 1);
	}

	void ILModule::dump_function_decl(std::uint32_t id) {
		auto& decl = function_decl[id];
		std::cout << "[";
		for (std::size_t i = 0; i < std::get<2>(decl).size(); ++i) {
			if (i > 0) std::cout << ", ";

			ILBlock::dump_data_type(std::get<2>(decl)[i]);
		}
		std::cout << "] -> ";
		ILBlock::dump_data_type(std::get<1>(decl));
	}



	void* ILEvaluator::read_last_register_value_indirect(ILDataType rs) {
		std::size_t ctrs = compile_time_register_size(rs);
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
			ILNativeFunction* nfunc = ((ILNativeFunction*)functions[f->second].get());
			if (nfunc->ptr == nullptr) {
				//memory_map_table[std::make_pair(nfunc->ptr,(unsigned char*)nfunc->ptr+1)] = std::make_pair(ILPointerType::function, nfunc->id);
				nfunc->ptr = ptr;
			}
		}
	}

	void ILBytecodeFunction::save(ILOutputStream& file) {

		file.u32(local_stack_lifetime.id);
		file.s( local_stack_lifetime.lifetime.size());
		file.write(local_stack_lifetime.lifetime.data(), local_stack_lifetime.lifetime.size());
		file.s(blocks.size());
		for (auto&& block : blocks_memory) {
			file.s(block->data_pool.size());
			file.write(block->data_pool.data(), block->data_pool.size());
		}
	}

	void ILBytecodeFunction::load(ILInputStream& file) {
		local_stack_lifetime.id = file.u32();
		local_stack_lifetime.lifetime = std::vector<std::uint8_t>(file.s());
		file.read(local_stack_lifetime.lifetime.data(), local_stack_lifetime.lifetime.size());


		blocks_memory = std::vector<std::unique_ptr<ILBlock>>(file.s());
		blocks = std::vector<ILBlock*>(blocks_memory.size());

		for (std::uint32_t i=0; i<(std::uint32_t)blocks_memory.size(); ++i) {
			std::unique_ptr<ILBlock> block = std::make_unique<ILBlock>();
			block->parent = this;
			block->id = i;

			std::size_t datapool_size = file.s();
			block->data_pool = std::vector<std::uint8_t>(datapool_size);
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
				file.s(bfunc->name.size());
				file.write(bfunc->name.data(), bfunc->name.size());
				bfunc->save(file);
			}else {
				ILNativeFunction* nfun = (ILNativeFunction*)func.get();
				file.u8(2);
				file.u32(func->decl_id);
				file.s(nfun->name.size());
				file.write(nfun->name.data(), nfun->name.size());
			}
		}

		file.s(function_decl.size());
		for (auto&& decl : function_decl) {
			file.u8((std::uint8_t)std::get<0>(decl));
			file.dt(std::get<1>(decl));
			file.s(std::get<2>(decl).size());
			file.write(std::get<2>(decl).data(), std::get<2>(decl).size());
		}

		
		file.s(structure_tables.size());
		for (auto&& table : structure_tables) {
			file.s(table.elements.size());
			for (auto && elem : table.elements) {
				file.u8((std::uint8_t)elem.type);
				file.u32(elem.value);
			}
		}

		file.s(array_tables.size());
		for (auto&& table : array_tables) {
			file.u32(table.count);
			file.u8((std::uint8_t)table.element.type);
			file.u32(table.element.value);
		}

		
		file.s(vtable_data.size());
		for (auto&& vtable : vtable_data) {
			file.u32(vtable.first);
			for (std::uint32_t i=0; i<vtable.first; ++i) {
				ILFunction* fun = (ILFunction*)vtable.second[i];
				file.u32(fun->id);
			}
		}

		file.s(constant_memory.size());
		for (auto&& con_mem: constant_memory) {
			file.u8((std::uint8_t)con_mem.first.type);
			file.u32((std::uint8_t)con_mem.first.value);
			con_mem.first.save(this, file, con_mem.second.get());
		}

		
		file.s(static_memory.size());
		for (auto&& static_mem: static_memory) {
			file.u8((std::uint8_t)std::get<0>(static_mem).type);
			file.u32((std::uint8_t)std::get<0>(static_mem).value);
			file.u32((std::uint8_t)std::get<2>(static_mem));
		}
	}

	void ILModule::load(ILInputStream& file) {
		external_functions.clear();

		bool has_entry_point = file.b();
		std::uint32_t e_point_id = file.u32();


		std::size_t read_size = file.s();
		functions = std::vector<std::unique_ptr<ILFunction>>(read_size);
		for (std::uint32_t funcid = 0; funcid < (std::uint32_t)read_size; ++funcid) {
			std::uint8_t ftype = file.u8();			
			if (ftype == 1) {
				auto fun = std::make_unique<ILBytecodeFunction>();
				fun->parent = this;
				fun->id = funcid;
				fun->decl_id = file.u32();

				std::size_t alias_size = file.s();
				fun->name = std::string(alias_size,'\0');
				file.read(fun->name.data(), alias_size);

				fun->load(file);
				functions[funcid] = std::move(fun);
			} else if (ftype == 2) {
				auto fun = std::make_unique<ILNativeFunction>();
				fun->parent = this;
				fun->id = funcid;
				fun->decl_id = file.u32();

				std::size_t alias_size = file.s();
				fun->name = std::string(alias_size,'\0');
				file.read(fun->name.data(), alias_size);

				external_functions[fun->name] = fun->id;
				functions[funcid] = std::move(fun);
			}
		}

		read_size = file.s();
		function_decl = std::vector<std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>>>(read_size);
		for (std::size_t declid = 0; declid < read_size; ++declid) {
			ILCallingConvention cconv = (ILCallingConvention)file.u8();
			ILDataType ret = file.dt();
			std::size_t asize = file.s();
			std::vector<ILDataType> args(asize);
			file.read(args.data(),args.size());
			function_decl[declid] = std::make_tuple(cconv, ret, std::move(args));
		}


		read_size = file.s();
		structure_tables = std::vector<ILStructTable>(read_size);
		for (std::size_t tid = 0; tid < read_size; ++tid) {
			ILStructTable table;
			std::size_t elem_size = file.s();
			table.elements = std::vector<ILSize>(elem_size);
			for (std::size_t elem_id = 0; elem_id<elem_size; ++elem_id) {
				table.elements[elem_id].type = (ILSizeType)file.u8();
				table.elements[elem_id].value = file.u32();
			}
			structure_tables[tid] = std::move(table);
		}

		read_size = file.s();
		array_tables = std::vector<ILArrayTable>(read_size);
		for (std::uint32_t tid = 0; tid < (std::uint32_t)read_size; ++tid) {
			ILArrayTable table;
			table.count = file.u32();
			table.element.type = (ILSizeType)file.u8();
			table.element.value = file.u32();
			array_tables_map[std::make_pair(table.element, table.count)] = tid;
			array_tables[tid] = std::move(table);
		}

		read_size = file.s();
		vtable_data = std::vector<std::pair<std::uint32_t,std::unique_ptr<void*[]>>>(read_size);
		for (std::size_t tid = 0; tid < read_size; ++tid) {
			std::pair<std::uint32_t, std::unique_ptr<void*[]>> vtable;
			vtable.first = file.u32();
			vtable.second = std::make_unique<void*[]>(vtable.first);
			for(std::uint32_t i=0;i<vtable.first;++i) {
				vtable.second[i] = functions[file.u32()].get();
			}
			vtable_data[tid] = std::move(vtable);
		}

		read_size = file.s();
		constant_memory = std::vector<std::pair<ILSize,std::unique_ptr<unsigned char[]>>>(read_size);
		for (std::size_t cid = 0; cid < read_size; ++cid) {
			ILSize s;
			s.type = (ILSizeType)file.u8();
			s.value = file.u32();
			std::size_t es = s.eval(this);
			auto mem = std::make_unique<unsigned char[]>(es);
			s.load(this, file, mem.get());
			constant_memory[cid] = std::make_pair(s, std::move(mem));
		}

		read_size = file.s();
		static_memory = std::vector<std::tuple<ILSize,std::unique_ptr<unsigned char[]>, std::uint32_t, std::size_t>>(read_size);
		for (std::size_t sid = 0; sid < read_size; ++sid) {
			ILSize s;
			s.type = (ILSizeType)file.u8();
			s.value = file.u32();
			std::uint32_t fid = file.u32();
			std::size_t es = s.eval(this);
			auto mem = std::make_unique<unsigned char[]>(es);
			static_memory[sid] = std::make_tuple(s, std::move(mem), fid, static_memory.size());
		}

		entry_point = has_entry_point?functions[e_point_id].get() : nullptr;
	}


	void ILOutputStream::u8(std::uint8_t v) {
		if (target.index() == 0) {
			std::get<0>(target)->put(v);
		} else {
			std::get<1>(target)->push_back(v);
		}
	}
	void ILOutputStream::i8(std::int8_t v) {
		u8(*(std::uint8_t*)&v);
	}
	void ILOutputStream::u16(std::uint16_t v) {
		if (target.index() == 0) {
			std::get<0>(target)->put((std::uint8_t)((v)&0x00ff))
			.put((std::uint8_t)((v>>8)&0x00ff));
		} else {
			std::get<1>(target)->push_back((std::uint8_t)((v)&0x00ff));
			std::get<1>(target)->push_back((std::uint8_t)((v>>8)&0x00ff));
		}
	}
	void ILOutputStream::i16(std::int16_t v) {
		u16(*(std::uint16_t*)&v);
	}
	void ILOutputStream::u32(std::uint32_t v) {
		if (target.index() == 0) {
			std::get<0>(target)->put((std::uint8_t)((v)&0x000000ff))
			.put((std::uint8_t)((v>>8)&0x000000ff))
			.put((std::uint8_t)((v>>16)&0x000000ff))
			.put((std::uint8_t)((v>>24)&0x000000ff));
		} else {
			std::get<1>(target)->push_back((std::uint8_t)((v)&0x000000ff));
			std::get<1>(target)->push_back((std::uint8_t)((v>>8)&0x000000ff));
			std::get<1>(target)->push_back((std::uint8_t)((v>>16)&0x000000ff));
			std::get<1>(target)->push_back((std::uint8_t)((v>>24)&0x000000ff));			
		}
	}
	void ILOutputStream::i32(std::int32_t v){
		u32(*(std::uint32_t*)&v);
	}
	void ILOutputStream::u64(std::uint64_t v){
		
		if (target.index() == 0) {
			std::get<0>(target)->put((std::uint8_t)((v)&0x00000000000000ff))
			.put((std::uint8_t)((v>>8)&0x00000000000000ff))
			.put((std::uint8_t)((v>>16)&0x00000000000000ff))
			.put((std::uint8_t)((v>>24)&0x00000000000000ff))
			.put((std::uint8_t)((v>>32)&0x00000000000000ff))
			.put((std::uint8_t)((v>>40)&0x00000000000000ff))
			.put((std::uint8_t)((v>>48)&0x00000000000000ff))
			.put((std::uint8_t)((v>>56)&0x00000000000000ff));
		} else {
			std::get<1>(target)->push_back((std::uint8_t)((v)&0x00000000000000ff));
			std::get<1>(target)->push_back((std::uint8_t)((v>>8)&0x00000000000000ff));
			std::get<1>(target)->push_back((std::uint8_t)((v>>16)&0x00000000000000ff));
			std::get<1>(target)->push_back((std::uint8_t)((v>>24)&0x00000000000000ff));
			std::get<1>(target)->push_back((std::uint8_t)((v>>32)&0x00000000000000ff));
			std::get<1>(target)->push_back((std::uint8_t)((v>>40)&0x00000000000000ff));
			std::get<1>(target)->push_back((std::uint8_t)((v>>48)&0x00000000000000ff));
			std::get<1>(target)->push_back((std::uint8_t)((v>>56)&0x00000000000000ff));
		}
	}
	void ILOutputStream::i64(std::int64_t v){
		u64(*(std::uint64_t*)&v);
	}
	void ILOutputStream::s(std::size_t v){
		u64((std::uint64_t)v);
	}
	
	void ILOutputStream::dt(ILDataType v){
		u8((std::uint8_t)v);
	}
	
	void ILOutputStream::b(bool v){
		u8((std::uint8_t)(v?1:0));
	}

	
	void ILOutputStream::write(void* v, std::size_t s) {
		if (s>0) {
			std::uint8_t* ptr = (std::uint8_t*)v;
			
			if (target.index() == 0) {
				for (std::size_t i=0; i<s; ++i) {
					std::get<0>(target)->put(*ptr++);
				}
			} else {
				for (std::size_t i=0; i<s; ++i) {
					std::get<1>(target)->push_back(*ptr++);
				}
			}
		}
	}


	std::uint8_t ILInputStream::u8(){
		return (std::uint8_t)target.get();
	}
	std::int8_t ILInputStream::i8() {
		std::uint8_t v= u8();
		return *(std::int8_t*)&v;
	}
	
	std::uint16_t ILInputStream::u16(){
		std::uint16_t v = 0;
		std::uint8_t r;
		r = (std::uint8_t)target.get();
		v |= (((std::uint16_t)r))     & 0x00ff;
		r = (std::uint8_t)target.get();
		v |= (((std::uint16_t)r)<<8)  & 0xff00;
		return v;
	}
	std::int16_t ILInputStream::i16() {
		std::uint16_t v = u16();
		return *(std::int16_t*)&v;
	}

	std::uint32_t ILInputStream::u32(){
		std::uint32_t v = 0;
		std::uint8_t r;
		r = (std::uint8_t)target.get();
		v |= (((std::uint32_t)r))     & 0x000000ff;
		r = (std::uint8_t)target.get();
		v |= (((std::uint32_t)r)<<8)  & 0x0000ff00;
		r = (std::uint8_t)target.get();
		v |= (((std::uint32_t)r)<<16) & 0x00ff0000;
		r = (std::uint8_t)target.get();
		v |= (((std::uint32_t)r)<<24) & 0xff000000;
		return v;
	}
	std::int32_t ILInputStream::i32() {
		std::uint32_t v = u32();
		return *(std::int32_t*)&v;
	}
	std::uint64_t ILInputStream::u64() {
		std::uint64_t v = 0;
		std::uint8_t r;
		r = (std::uint8_t)target.get();
		v |= (((std::uint64_t)r))     & 0x00000000000000ff;
		r = (std::uint8_t)target.get();
		v |= (((std::uint64_t)r)<<8)  & 0x000000000000ff00;
		r = (std::uint8_t)target.get();
		v |= (((std::uint64_t)r)<<16) & 0x0000000000ff0000;
		r = (std::uint8_t)target.get();
		v |= (((std::uint64_t)r)<<24) & 0x00000000ff000000;
		r = (std::uint8_t)target.get();
		v |= (((std::uint64_t)r)<<32) & 0x000000ff00000000;
		r = (std::uint8_t)target.get();
		v |= (((std::uint64_t)r)<<40) & 0x0000ff0000000000;
		r = (std::uint8_t)target.get();
		v |= (((std::uint64_t)r)<<48) & 0x00ff000000000000;
		r = (std::uint8_t)target.get();
		v |= (((std::uint64_t)r)<<56) & 0xff00000000000000;
		return v;
	}
	std::int64_t ILInputStream::i64() {
		std::uint64_t v = u64();
		return *(std::int64_t*)&v;
	}
	std::size_t ILInputStream::s() {
		return (std::size_t)u64();
	}
	ILDataType ILInputStream::dt() {
		return (ILDataType)u8();
	}
	bool ILInputStream::b() {
		return (bool)u8();
	}
	void ILInputStream::read(void* v, std::size_t s) {
		if (s>0) {
			std::uint8_t* ptr = (std::uint8_t*)v;
			for (std::size_t i=0; i<s; ++i) {
				std::uint8_t vb = (std::uint8_t)target.get();
				*ptr++ = vb;
			}
		}
	}

	
	void ILSize::load(ILModule* mod, ILInputStream& stream, unsigned char* ptr) {
		switch (type)
		{
			case ILSizeType::abs8:
				for (std::uint32_t i=0;i<value;++i) { *(std::uint8_t*)ptr = stream.u8(); ptr+=1; }
				break;

			case ILSizeType::abs16:
				for (std::uint32_t i=0;i<value;++i) { *(std::uint16_t*)ptr = stream.u16(); ptr+=2; }
				break;

			case ILSizeType::absf32:
			case ILSizeType::abs32:
				for (std::uint32_t i=0;i<value;++i) { *(std::uint32_t*)ptr = stream.u32(); ptr+=4; }
				break;

			case ILSizeType::absf64:
			case ILSizeType::abs64:
				for (std::uint32_t i=0;i<value;++i) { *(std::uint64_t*)ptr = stream.u64(); ptr+=8; }
				break;

			case ILSizeType::word: {
				throw string_exception("Compiler error: found size value inside imported data");
			}break;

			case ILSizeType::ptr: {				
				throw string_exception("Compiler error: found pointer value inside imported data");
			} break;

			case ILSizeType::slice: {				
				throw string_exception("Compiler error: found pointer value inside imported data");
			} break;

			case ILSizeType::table: {
				std::size_t s = eval(mod);
				auto& table = mod->structure_tables[value];
				table.calculate(mod);
				for (std::size_t e = 0; e<table.elements.size(); ++e) {
					unsigned char* elem_off = ptr + table.calculated_offsets[e];
					table.elements[e].load(mod,stream, elem_off);
				}
				ptr+=s;
			}
			case ILSizeType::array: {
				std::size_t s = eval(mod);
				auto& table = mod->array_tables[value];
				table.calculate(mod);
				table.element.load(mod, stream, ptr);
				ptr+=s;
			}
		}
	}

	
	std::size_t ILOutputStream::offset() {
		if (target.index() == 1) {
			return std::get<1>(target)->size();
		}
		return 0;
	}

	void ILSize::save(ILModule* mod, ILOutputStream& stream, unsigned char* ptr) {
		switch (type)
		{
			case ILSizeType::abs8:
				for (std::uint32_t i=0;i<value;++i) { stream.u8(*(std::uint8_t*)ptr); ptr+=1; }
				break;

			case ILSizeType::abs16:
				for (std::uint32_t i=0;i<value;++i) { stream.u16(*(std::uint16_t*)ptr); ptr+=2; }
				break;

			case ILSizeType::abs32:
			case ILSizeType::absf32:
				for (std::uint32_t i=0;i<value;++i) { stream.u32(*(std::uint32_t*)ptr); ptr+=4; }
				break;

			case ILSizeType::abs64:
			case ILSizeType::absf64:
				for (std::uint32_t i=0;i<value;++i) { stream.u64(*(std::uint64_t*)ptr); ptr+=8; }
				break;

			case ILSizeType::word: {
				throw string_exception("Compiler error: there are size values inside data export");
			} break;
			case ILSizeType::ptr: {
				throw string_exception("Compiler error: there are pointer values inside data export");
			} break;
			case ILSizeType::slice: {
				throw string_exception("Compiler error: there are pointers values inside data export");
			}break;
			case ILSizeType::table: {
				std::size_t s = eval(mod);
				auto& table = mod->structure_tables[value];
				table.calculate(mod);
				for (std::size_t e = 0; e<table.elements.size(); ++e) {
					unsigned char* elem_off = ptr + table.calculated_offsets[e];
					table.elements[e].save(mod,stream, elem_off);
				}
				ptr+=s;
			}
			case ILSizeType::array: {
				std::size_t s = eval(mod);
				auto& table = mod->array_tables[value];
				table.calculate(mod);
				table.element.save(mod, stream, ptr);
				ptr+=s;
			}
		}
	}
}