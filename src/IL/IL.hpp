#ifndef _il_crs_h
#define _il_crs_h

#include <vector>
#include <memory>
#include <set>
#include <list>
#include <string>
#include <string_view>
#include <map>
#include <variant>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <exception>

namespace Corrosive {

	struct string_exception : public std::exception
	{
		std::string s;
		inline string_exception(const std::string& msg) { s = std::move(msg); }
		inline ~string_exception() noexcept {}
		const char* what() const noexcept { return s.c_str(); }
	};

	struct err_undef_tag {};
	struct err_undef_true_tag {};
	struct err_undef_false_tag {};

	class err_handler {
		public:
			err_handler() = default;
			err_handler(const err_handler& h) = default;
			err_handler(err_undef_tag && h) : state(0) {};
			err_handler(err_undef_true_tag&& h) : state(3) {};
			err_handler(err_undef_false_tag&& h) : state(4) {};
			err_handler(bool b) : state(b?1:2) {  }
			err_handler(err_handler&& h) : state(((bool)h)?1:2) { if (h.state <=2) h.state = 0; }

			~err_handler() {
				switch(state) {
					case 1:
					case 2:
						std::cout << "Error: exception not handled properly"; exit(1); break;

					default:
						break;
				}
			}

			bool operator = (const bool& r) {
				state = r?1:2;
				return r;
			}

			operator bool() {
				switch(state) {
					case 0: return true;
					case 1: state = 0; return true;
					case 2: state = 0; return false;
					case 3: return true;
					case 4: return false;
					default: return false;
				}
			}
		private:
			std::uint8_t state = 1;
	};

	using errvoid = err_handler;
	
	struct err {
		static errvoid ok;
		static errvoid fail;
	};

	inline bool operator!(err_handler& h) { return !((bool)h); }


	
	inline errvoid pass(){
		return false;
	}

	using stackid_t = std::uint32_t;
	using tableid_t = std::uint32_t;
	using tableelement_t = std::uint32_t;


	void throw_il_wrong_data_flow_error();
	void throw_il_nothing_on_stack_error();
	void throw_il_remaining_stack_error();
	void throw_il_wrong_arguments_error();
	void throw_il_wrong_type_error();

	std::size_t align_up(std::size_t value, std::size_t alignment);

	class ILBytecodeFunction;
	class ILFunction;
	class ILModule;
	class ILEvaluator;

	enum class ILContext {
		compile, runtime, both
	};

	enum class ILInstruction : unsigned char {
		u8, i8, u16, i16, u32, i32, u64, i64, f32, f64, word, dword, slice,
		size8, size16, size32,

		add, sub, div, mul, rem,
		bit_and, bit_or, bit_xor,
		eq, ne, gt, ge, lt, le,

		load, store, store2, ret, jmp, jmpz,
		local8,local16,local32,
		forget, fncall,
		fnptr, call, start, insintric, cast, bitcast,
		roffset8, roffset16, roffset32,
		offset8, offset16, offset32,
		vtable, duplicate, swap, swap2,
		memcpy, memcpy2, memcmp, memcmp2, rmemcmp, rmemcmp2,
		rtoffset, rtoffset2, null, isnotzero, yield, discard, accept,
		debug, constref, staticref, negative, negate, 
		table8offset8, table8offset16, table8offset32,
		table16offset8, table16offset16, table16offset32,
		table32offset8, table32offset16, table32offset32,
		table8roffset8, table8roffset16, table8roffset32,
		table16roffset8, table16roffset16, table16roffset32,
		table32roffset8, table32roffset16, table32roffset32,
		aoffset8, aoffset16, aoffset32,
		woffset8, woffset16, woffset32, 
		aroffset, wroffset, clone, combinedw, splitdw, highdw, lowdw,
		extract8, extract16, extract32,
		cut8, cut16, cut32,
	};

	enum class ILDataType : unsigned char {
		i8, u8, i16, u16, i32, u32, i64, u64, f32, f64, word, none, dword, undefined
	};

	class ILDataTypePair {
	public:
		std::uint8_t value;
		ILDataTypePair(ILDataType f, ILDataType s) : value((((std::uint8_t)f) & 0x0f) | (((std::uint8_t)(s)<<4) & 0xf0)) {}
		ILDataType first() { return (ILDataType)(value & 0x0f); }
		ILDataType second() { return (ILDataType)((value >> 4) & 0x0f); }
	};

	enum class ILArchitecture : unsigned char {
		none, bit64, bit32
	};

	enum class ILSizeType : unsigned char {
		array, table, abs8, abs16, abs32, abs64, absf32, absf64, ptr, word, slice, _0
	};

	enum class ILCallingConvention : std::uint8_t {
		bytecode, native, stdcall, __max
	};


	class ILOutputStream {
	public:
		ILOutputStream(std::variant<std::ostream*, std::vector<std::uint8_t>*> tg): target(tg) {}
		void u8(std::uint8_t v);
		void i8(std::int8_t v);
		void u16(std::uint16_t v);
		void i16(std::int16_t v);
		void u32(std::uint32_t v);
		void i32(std::int32_t v);
		void u64(std::uint64_t v);
		void i64(std::int64_t v);
		void s(std::size_t v);
		void dt(ILDataType v);
		void b(bool v);
		void write(void* v, std::size_t s);
		std::size_t offset();
	private:
		std::variant<std::ostream*, std::vector<std::uint8_t>*> target;
	};

	class ILInputStream {
	public:
		ILInputStream(std::istream& tg): target(tg) {}
		std::uint8_t u8();
		std::int8_t i8();
		std::uint16_t u16();
		std::int16_t i16();
		std::uint32_t u32();
		std::int32_t i32();
		std::uint64_t u64();
		std::int64_t i64();
		std::size_t s();
		ILDataType dt();
		bool b();
		void read(void* v, std::size_t s);
	private:
		std::istream& target;
	};

	struct ILSize {
		ILSize();
		ILSize(ILSizeType type, tableid_t value);

		tableid_t value;
		ILSizeType type;

		std::size_t eval(ILModule* mod) const;
		std::size_t alignment(ILModule* mod) const;

		static const ILSize single_ptr;
		static const ILSize double_ptr;
		static const ILSize slice;
		static const ILSize single_word;

		void print(ILModule* mod);

		void clean_prepass(ILModule* mod, std::set<void*>& pointer_offsets, unsigned char* ptr);

		void load(ILModule* mod, ILInputStream& stream, unsigned char* ptr);
		void save(ILModule* mod, ILOutputStream& stream, unsigned char* ptr);
	};

	bool operator < (const ILSize& l,const ILSize& r);

	struct ILStructTable {
		std::vector<ILSize> elements;
		std::vector<std::size_t> calculated_offsets;
		std::size_t calculated_size;
		std::size_t calculated_alignment = 0;
		void calculate(ILModule* mod);

		void clean_prepass(ILModule& mod, std::unordered_set<std::size_t>& used_tables,
			std::unordered_set<std::size_t>& used_arrays);

		void clean_pass(ILModule& mod, std::unordered_map<std::size_t, std::size_t>& map_tables,
			std::unordered_map<std::size_t, std::size_t>& map_arrays);
	};

	struct ILArrayTable {
		ILSize element;
		std::uint32_t count;
		std::size_t calculated_size;
		std::size_t calculated_alignment = 0;
		void calculate(ILModule* mod);

		void clean_prepass(ILModule& mod, std::unordered_set<std::size_t>& used_tables,
			std::unordered_set<std::size_t>& used_arrays);

		void clean_pass(ILModule& mod, std::unordered_map<std::size_t, std::size_t>& map_tables,
			std::unordered_map<std::size_t, std::size_t>& map_arrays);
	};

	class ILBlock {
	public:
		//std::string alias;

		std::uint32_t id;
		ILBytecodeFunction* parent;
		std::vector<std::uint8_t> data_pool;

		void write_instruction(ILInstruction instruction);
		void write_const_type(ILDataType type);


		template<typename T> inline void write_value(T av) {
			switch(sizeof(T)) {
				case 1: data_pool.push_back(*(std::uint8_t*)&av); break;
				case 2: {
					std::uint16_t v = *(std::uint16_t*)&av;
					data_pool.push_back((std::uint8_t)(v & 0x00ff));
					data_pool.push_back((std::uint8_t)((v>>8) & 0x00ff)); 
				} break;
				case 3:{
					std::uint32_t v = *(std::uint32_t*)&av;
					data_pool.push_back((std::uint8_t)(v & 0x000000ff));
					data_pool.push_back((std::uint8_t)((v>>8) & 0x000000ff));
					data_pool.push_back((std::uint8_t)((v>>16) & 0x000000ff));
				} break;
				case 4: {
					std::uint32_t v = *(std::uint32_t*)&av;
					data_pool.push_back((std::uint8_t)(v & 0x000000ff));
					data_pool.push_back((std::uint8_t)((v>>8) & 0x000000ff));
					data_pool.push_back((std::uint8_t)((v>>16) & 0x000000ff));
					data_pool.push_back((std::uint8_t)((v>>24) & 0x000000ff));
				} break;
				case 5:{
					std::uint64_t v = *(std::uint64_t*)&av;
					data_pool.push_back((std::uint8_t)(v & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>8) & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>16) & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>24) & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>32) & 0x00000000000000ff));
				} break;
				case 6:{
					std::uint64_t v = *(std::uint64_t*)&av;
					data_pool.push_back((std::uint8_t)(v & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>8) & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>16) & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>24) & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>32) & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>40) & 0x00000000000000ff));
				} break;
				case 7:{
					std::uint64_t v = *(std::uint64_t*)&av;
					data_pool.push_back((std::uint8_t)(v & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>8) & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>16) & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>24) & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>32) & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>40) & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>48) & 0x00000000000000ff));
				} break;
				case 8: {
					std::uint64_t v = *(std::uint64_t*)&av;
					data_pool.push_back((std::uint8_t)(v & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>8) & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>16) & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>24) & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>32) & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>40) & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>48) & 0x00000000000000ff));
					data_pool.push_back((std::uint8_t)((v>>56) & 0x00000000000000ff));
				} break;
				default: throw string_exception("data too large");
			}
		}

		template<typename T> inline static T read_data(std::vector<std::uint8_t>::iterator& it) {
			
			std::uint64_t v = 0;

			switch(sizeof(T)) {
				case 1: v=*it++; break;
				case 2: {
					v |= ((std::uint16_t)*it++) & 0x00ff;
					v |= ((std::uint16_t)*it++ << 8) & 0xff00;
				} break;
				case 3: {
					v |= ((std::uint32_t)*it++)       & 0x000000ff;
					v |= ((std::uint32_t)*it++ << 8)  & 0x0000ff00;
					v |= ((std::uint32_t)*it++ << 16) & 0x00ff0000;
				} break;
				case 4: {
					v |= ((std::uint32_t)*it++)       & 0x000000ff;
					v |= ((std::uint32_t)*it++ << 8)  & 0x0000ff00;
					v |= ((std::uint32_t)*it++ << 16) & 0x00ff0000;
					v |= ((std::uint32_t)*it++ << 24) & 0xff000000;
				} break;
				case 5:{
					v |= ((std::uint64_t)*it++)     & 0x00000000000000ff;
					v |= ((std::uint64_t)*it++<<8)  & 0x000000000000ff00;
					v |= ((std::uint64_t)*it++<<16) & 0x0000000000ff0000;
					v |= ((std::uint64_t)*it++<<24) & 0x00000000ff000000;
					v |= ((std::uint64_t)*it++<<32) & 0x000000ff00000000;
				} break;
				case 6:{
					v |= ((std::uint64_t)*it++)     & 0x00000000000000ff;
					v |= ((std::uint64_t)*it++<<8)  & 0x000000000000ff00;
					v |= ((std::uint64_t)*it++<<16) & 0x0000000000ff0000;
					v |= ((std::uint64_t)*it++<<24) & 0x00000000ff000000;
					v |= ((std::uint64_t)*it++<<32) & 0x000000ff00000000;
					v |= ((std::uint64_t)*it++<<40) & 0x0000ff0000000000;
				} break;
				case 7:{
					v |= ((std::uint64_t)*it++)     & 0x00000000000000ff;
					v |= ((std::uint64_t)*it++<<8)  & 0x000000000000ff00;
					v |= ((std::uint64_t)*it++<<16) & 0x0000000000ff0000;
					v |= ((std::uint64_t)*it++<<24) & 0x00000000ff000000;
					v |= ((std::uint64_t)*it++<<32) & 0x000000ff00000000;
					v |= ((std::uint64_t)*it++<<40) & 0x0000ff0000000000;
					v |= ((std::uint64_t)*it++<<48) & 0x00ff000000000000;
				} break;
				case 8: {
					v |= ((std::uint64_t)*it++)     & 0x00000000000000ff;
					v |= ((std::uint64_t)*it++<<8)  & 0x000000000000ff00;
					v |= ((std::uint64_t)*it++<<16) & 0x0000000000ff0000;
					v |= ((std::uint64_t)*it++<<24) & 0x00000000ff000000;
					v |= ((std::uint64_t)*it++<<32) & 0x000000ff00000000;
					v |= ((std::uint64_t)*it++<<40) & 0x0000ff0000000000;
					v |= ((std::uint64_t)*it++<<48) & 0x00ff000000000000;
					v |= ((std::uint64_t)*it++<<56) & 0xff00000000000000;
				} break;
				default: throw string_exception("data too large");
			}

			return *(T*)&v;
		}

		void dump();
		static void dump_data_type(ILDataType dt);
		
		unsigned char* reserve_data(std::size_t size);
	};

	struct dword_t {
		dword_t() = default;
		dword_t(const dword_t&) = default;
		dword_t(size_t s):p1(nullptr),p2((void*)s) {}
		dword_t(size_t s1,size_t s2) : p1((void*)s1),  p2((void*)s2) {}
		dword_t(void* s1,void* s2) : p1(s1),  p2(s2) {}
		void* p1;
		void* p2;
	};

	class uint128_t {
	private:
		std::uint64_t a;
		std::uint64_t b;
	public:
		uint128_t() {}
		uint128_t(const uint128_t& v): a(v.a), b(v.b) {}
		uint128_t(int v) : a(v), b(0) {};
		bool operator == (const int& v) { return a == v && b == 0; }
		uint128_t operator << (std::size_t v) { uint128_t r; r.a = (a << v); r.b = (b << v) | (a >> (64 - v)); return r; }
		uint128_t operator >> (std::size_t v) { uint128_t r; r.b = (b >> v); r.a = (a >> v) | (b << (64 - v)); return r; }
	};

	enum class ILLifetimeEvent : unsigned char {
		push=1, pop=2, append=3
	};

	enum class ILBitWidth {
		b8,b16,b32
	};

	inline static ILBitWidth bit(std::uint32_t v) {
		return (v <= UINT8_MAX) ? ILBitWidth::b8 : ((v <= UINT16_MAX) ? ILBitWidth::b16 : ILBitWidth::b32);
	}

	
	void build_sandbox();
	void invalidate_sandbox();

	extern void* sandbox;
	extern int (*wrap)(void*);
	extern void (*longjmp_func)(void*, int);
	

	struct ILLifetime {
		stackid_t id = 0;
		std::vector<unsigned char> lifetime;
		void push();
		void pop();
		stackid_t append(ILSize s);
		stackid_t append_unknown(std::size_t& holder);
		void resolve_unknown(std::size_t holder, ILSize s);
		void discard_push();


		void clean_prepass(std::unordered_set<std::size_t>& used_tables,
			std::unordered_set<std::size_t>& used_arrays);


		void clean_pass(std::unordered_map<std::size_t, std::size_t>& map_tables,
			std::unordered_map<std::size_t, std::size_t>& map_arrays);
	};

	class ILFunction {
	public:
		virtual ~ILFunction();
		std::uint32_t id;
		ILModule* parent;
		std::uint32_t decl_id = UINT32_MAX;
	};

	class ILBytecodeFunction : public ILFunction {
	public:
		ILContext context;

		std::vector<std::size_t>	calculated_local_offsets;
		std::size_t					calculated_local_stack_size;
		std::size_t					calculated_local_stack_alignment = 0;
		ILLifetime					local_stack_lifetime;

		void calculate_stack();

		std::vector<ILBlock*>						blocks;
		std::vector<std::unique_ptr<ILBlock>>		blocks_memory;

		ILBlock*	create_block();
		ILBlock*	create_and_append_block();
		void		append_block(ILBlock* block);
		void		dump();

		void clean_prepass(std::unordered_set<std::size_t>& used_functions,
			std::unordered_set<std::size_t>& used_constants,
			std::unordered_set<std::size_t>& used_statics,
			std::unordered_set<std::size_t>& used_vtables,
			std::unordered_set<std::size_t>& used_decls,
			std::unordered_set<std::size_t>& used_tables,
			std::unordered_set<std::size_t>& used_arrays);


		void clean_pass(std::unordered_map<std::size_t, std::size_t>& map_functions,
			std::unordered_map<std::size_t, std::size_t>& map_constants,
			std::unordered_map<std::size_t, std::size_t>& map_statics,
			std::unordered_map<std::size_t, std::size_t>& map_vtables,
			std::unordered_map<std::size_t, std::size_t>& map_decls,
			std::unordered_map<std::size_t, std::size_t>& map_tables,
			std::unordered_map<std::size_t, std::size_t>& map_arrays);

			
		void load(ILInputStream& stream);
		void save(ILOutputStream& stream);
	};

	class ILNativeFunction : public ILFunction {
	public:
		std::string name;
		void* ptr = nullptr;
	};

	using ilsize_t = uint128_t; // max register size for all architectures (temp storage)

	enum class ILInsintric : unsigned char {
		push_template, build_template, type_dynamic_cast
	};
	
	errvoid throw_runtime_exception(const ILEvaluator* eval, std::string_view message);
	errvoid throw_segfault_exception(const ILEvaluator* eval, int signal);
	errvoid throw_interrupt_exception(const ILEvaluator* eval, int signal);
	errvoid throw_runtime_handler_exception(const ILEvaluator* eval);

	class ILEvaluator {
	public:
		static void ex_throw();

		ILModule* parent = nullptr;

		static const inline std::size_t stack_size = 1024 * 4;
		unsigned char memory_stack[stack_size];
		unsigned char* stack_pointer = memory_stack;
		unsigned char* stack_base = memory_stack;
		unsigned char* stack_base_aligned = memory_stack;

		std::uint8_t register_stack_1b[stack_size];
		std::uint8_t* register_stack_pointer_1b = register_stack_1b;

		std::uint16_t register_stack_2b[stack_size];
		std::uint16_t* register_stack_pointer_2b = register_stack_2b;

		std::uint32_t register_stack_4b[stack_size];
		std::uint32_t* register_stack_pointer_4b = register_stack_4b;

		std::uint64_t register_stack_8b[stack_size];
		std::uint64_t* register_stack_pointer_8b = register_stack_8b;

		ilsize_t yield_storage;

		std::vector<void*> callstack;
		//std::vector<std::tuple<std::uint16_t, std::uint16_t, std::string_view>> callstack_debug;

		std::uint16_t debug_line = 0;
		std::uint16_t debug_file = 0;

		std::vector<std::string> debug_file_names;
		std::uint16_t register_debug_source(std::string name);

		static void sandbox_begin();
		static void sandbox_end();
		static bool sandboxed();

		errvoid	write_register_value_indirect(std::size_t size, void* value);
		errvoid	pop_register_value_indirect(std::size_t size, void* into);
		void*	read_last_register_value_indirect(ILDataType rs);
		std::size_t	compile_time_register_size(ILDataType t);


		template<typename T> inline errvoid read_register_value(T& r) {
			if (!wrap || wrap(sandbox) == 0) {
				switch (sizeof(T)) {
					case 1:
						if (register_stack_pointer_1b <= register_stack_1b) return err::fail;
						r = *(T*)(register_stack_pointer_1b - 1); return err::ok;
					case 2:
						if (register_stack_pointer_2b <= register_stack_2b) return err::fail;
						r = *(T*)(register_stack_pointer_2b - 1); return err::ok;
					case 3:
					case 4:
						if (register_stack_pointer_4b <= register_stack_4b) return err::fail;
						r = *(T*)(register_stack_pointer_4b - 1); return err::ok;
					case 5:
					case 6:
					case 7:
					case 8:
						if (register_stack_pointer_8b <= register_stack_8b) return err::fail;
						r = *(T*)(register_stack_pointer_8b - 1); return err::ok;
					default:
						if (register_stack_pointer_8b <= register_stack_8b+1) return err::fail;
						r = *(T*)(register_stack_pointer_8b - 2); return err::ok;
				}
			}
			else {
				return throw_runtime_handler_exception(this);
			}
			
			return err::ok;
		}

		template<typename T> inline errvoid pop_register_value(T& r) {
			if (!wrap || wrap(sandbox) == 0) {
				switch (sizeof(T)) {
					case 1:
						if (register_stack_pointer_1b <= register_stack_1b) return err::fail;
						r = *(T*)(--register_stack_pointer_1b); return err::ok;
					case 2:
						if (register_stack_pointer_2b <= register_stack_2b) return err::fail;
						r = *(T*)(--register_stack_pointer_2b); return err::ok;
					case 3:
					case 4:
						if (register_stack_pointer_4b <= register_stack_4b) return err::fail;
						r = *(T*)(--register_stack_pointer_4b); return err::ok;
					case 5:
					case 6:
					case 7:
					case 8:
						if (register_stack_pointer_8b <= register_stack_8b) return err::fail;
						r = *(T*)(--register_stack_pointer_8b); return err::ok;
					default:
						if (register_stack_pointer_8b <= register_stack_8b + 1) return err::fail;
						--register_stack_pointer_8b;
						r = *(T*)(--register_stack_pointer_8b); return err::ok;
				}
			}
			else {
				return throw_runtime_handler_exception(this);
			}
			
			return err::ok;
		}

		template<typename T> inline void write_register_value(T v) {
			if (!wrap || wrap(sandbox) == 0) {
				switch (sizeof(T)) {
					case 0: return;
					case 1:
						*(T*)(register_stack_pointer_1b++) = v; break;
					case 2:
						*(T*)(register_stack_pointer_2b++) = v; break;
					case 3:
					case 4:
						*(T*)(register_stack_pointer_4b++) = v; break;
					case 5:
					case 6:
					case 7:
					case 8:
						*(T*)(register_stack_pointer_8b++) = v; break;
					default:
						*(T*)(register_stack_pointer_8b++) = v; register_stack_pointer_8b++; break;
				}
			}
			else {
				throw_runtime_handler_exception(this);
			}
		}
	};

	void release_jit_code();


	enum class ILPointerType {
		function, vtable, constant, staticstorage
	};

	class ILModule {
	private:
		static thread_local std::vector<ILModule*> current;
	public:
		static ILModule* active() { return current.back(); }

		std::map<std::string, std::uint32_t> external_functions;
		std::vector<std::pair<ILSize,std::unique_ptr<unsigned char[]>>> constant_memory;
		std::vector<std::pair<ILSize,std::unique_ptr<unsigned char[]>>> static_memory;

		std::vector<std::unique_ptr<ILFunction>> functions;
		std::vector<std::pair<std::uint32_t,std::unique_ptr<void* []>>> vtable_data;
		std::vector<ILStructTable> structure_tables;
		std::vector<ILArrayTable> array_tables;
		std::map<std::pair<ILSize, tableelement_t>, tableid_t> array_tables_map;
		std::vector<std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>>> function_decl;

		void try_link(std::string name,void* ptr);

		tableid_t register_structure_table();
		tableid_t register_array_table(ILSize type,tableelement_t count);
		std::uint32_t register_vtable(std::uint32_t size, std::unique_ptr<void* []> table);
		ILBytecodeFunction* create_function(ILContext context);
		ILNativeFunction* create_native_function(std::string alias);
		std::uint32_t register_constant(unsigned char* memory, ILSize size);
		std::uint32_t register_static(unsigned char* memory, ILSize size);
		std::uint32_t register_function_decl(std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>> decl);

		void strip_unused_content();

		errvoid (*insintric_function[256])(ILEvaluator*);
		std::string insintric_function_name[256];

		void dump_function_decl(std::uint32_t id);
		ILFunction* entry_point = nullptr;
		std::vector<ILFunction*> exported_functions;

		void run(ILFunction* func);
		void load(ILInputStream& stream);
		void save(ILOutputStream& stream);

		struct MemoryMapCompare {
			bool operator()(const std::pair<void*,void*>& l, const std::pair<void*,void*>& r) const;
		};
	};

	errvoid abi_dynamic_call(ILEvaluator* eval, ILCallingConvention conv, void* ptr, std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>>& decl);

	class ILBuilder {
	public:

		static errvoid eval_const_i8(ILEvaluator* eval_ctx, std::int8_t   value);
		static errvoid eval_const_i16(ILEvaluator* eval_ctx, std::int16_t  value);
		static errvoid eval_const_i32(ILEvaluator* eval_ctx, std::int32_t  value);
		static errvoid eval_const_i64(ILEvaluator* eval_ctx, std::int64_t  value);
		static errvoid eval_const_u8(ILEvaluator* eval_ctx, std::uint8_t  value);
		static errvoid eval_const_u16(ILEvaluator* eval_ctx, std::uint16_t value);
		static errvoid eval_const_u32(ILEvaluator* eval_ctx, std::uint32_t value);
		static errvoid eval_const_u64(ILEvaluator* eval_ctx, std::uint64_t value);
		static errvoid eval_const_f32(ILEvaluator* eval_ctx, float    value);
		static errvoid eval_const_f64(ILEvaluator* eval_ctx, double   value);
		static errvoid eval_const_word(ILEvaluator* eval_ctx, void* value);
		static errvoid eval_const_dword(ILEvaluator* eval_ctx, dword_t value);
		static errvoid eval_const_size(ILEvaluator* eval_ctx, std::size_t   value);
		static errvoid eval_const_slice(ILEvaluator* eval_ctx, std::uint32_t constid, ILSize size);

		static errvoid build_const_i8(ILBlock* block, std::int8_t   value);
		static errvoid build_const_i16(ILBlock* block, std::int16_t  value);
		static errvoid build_const_i32(ILBlock* block, std::int32_t  value);
		static errvoid build_const_i64(ILBlock* block, std::int64_t  value);
		static errvoid build_const_u8(ILBlock* block, std::uint8_t  value);
		static errvoid build_const_u16(ILBlock* block, std::uint16_t value);
		static errvoid build_const_u32(ILBlock* block, std::uint32_t value);
		static errvoid build_const_u64(ILBlock* block, std::uint64_t value);
		static errvoid build_const_f32(ILBlock* block, float    value);
		static errvoid build_const_f64(ILBlock* block, double   value);
		static errvoid build_const_word(ILBlock* block, void* value);
		static errvoid build_const_dword(ILBlock* block, dword_t value);
		static errvoid build_const_size(ILBlock* block, ILSize value);
		static errvoid build_const_slice(ILBlock* block, std::uint32_t constid, ILSize size);

		static errvoid eval_combine_dword(ILEvaluator* eval_ctx);
		static errvoid eval_high_word(ILEvaluator* eval_ctx);
		static errvoid eval_low_word(ILEvaluator* eval_ctx);
		static errvoid eval_extract(ILEvaluator* eval_ctx, ILSize s);
		static errvoid eval_cut(ILEvaluator* eval_ctx, ILSize s);
		static errvoid eval_split_dword(ILEvaluator* eval_ctx);
		static errvoid eval_tableoffset(ILEvaluator* eval_ctx, tableid_t tableid, tableelement_t itemid);
		static errvoid eval_tableroffset(ILEvaluator* eval_ctx, ILDataType src, ILDataType dst, tableid_t tableid, tableelement_t itemid);
		static errvoid eval_constref(ILEvaluator* eval_ctx, std::uint32_t constid);
		static errvoid eval_staticref(ILEvaluator* eval_ctx, std::uint32_t constid);
		static errvoid eval_debug(ILEvaluator* eval_ctx, std::uint16_t file, std::uint16_t line);
		static errvoid eval_load(ILEvaluator* eval_ctx, ILDataType type);
		static errvoid eval_store(ILEvaluator* eval_ctx, ILDataType type);
		static errvoid eval_store_rev(ILEvaluator* eval_ctx, ILDataType type);
		static errvoid eval_negative(ILEvaluator* eval_ctx, ILDataType type);
		static errvoid eval_vtable(ILEvaluator* eval_ctx, std::uint32_t id);
		static errvoid eval_memcpy(ILEvaluator* eval_ctx, std::size_t size);
		static errvoid eval_memcpy_rev(ILEvaluator* eval_ctx, std::size_t size);
		static errvoid eval_memcmp(ILEvaluator* eval_ctx, std::size_t size);
		static errvoid eval_memcmp_rev(ILEvaluator* eval_ctx, std::size_t size);
		static errvoid eval_null(ILEvaluator* eval_ctx);
		static errvoid eval_negate(ILEvaluator* eval_ctx);
		static errvoid eval_rmemcmp(ILEvaluator* eval_ctx, ILDataType type);
		static errvoid eval_rmemcmp_rev(ILEvaluator* eval_ctx, ILDataType type);
		static errvoid eval_isnotzero(ILEvaluator* eval_ctx, ILDataType type);
		static errvoid eval_rtoffset(ILEvaluator* eval_ctx);
		static errvoid eval_rtoffset_rev(ILEvaluator* eval_ctx);
		static errvoid eval_offset(ILEvaluator* eval_ctx, ILSize offset);
		static errvoid eval_aoffset(ILEvaluator* eval_ctx, std::uint32_t offset);
		static errvoid eval_woffset(ILEvaluator* eval_ctx, std::uint32_t offset);
		static errvoid eval_aroffset(ILEvaluator* eval_ctx, ILDataType from, ILDataType to, std::uint32_t offset);
		static errvoid eval_wroffset(ILEvaluator* eval_ctx, ILDataType from, ILDataType to, std::uint32_t offset);
		static errvoid eval_roffset(ILEvaluator* eval_ctx, ILDataType src, ILDataType dst, std::size_t offset);
		static errvoid eval_add(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static errvoid eval_and(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static errvoid eval_or(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static errvoid eval_xor(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static errvoid eval_eq(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static errvoid eval_ne(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static errvoid eval_gt(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static errvoid eval_lt(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static errvoid eval_ge(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static errvoid eval_le(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static errvoid eval_sub(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static errvoid eval_div(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static errvoid eval_rem(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static errvoid eval_mul(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static errvoid eval_cast(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static errvoid eval_bitcast(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static errvoid eval_forget(ILEvaluator* eval_ctx, ILDataType type);
		static errvoid eval_fnptr(ILEvaluator* eval_ctx, ILFunction* fun);
		static errvoid eval_fncall(ILEvaluator* eval_ctx, ILFunction* fun);
		static errvoid eval_insintric(ILEvaluator* eval_ctx, ILInsintric fun);
		static errvoid eval_discard(ILEvaluator* eval_ctx, ILDataType type);
		static errvoid eval_yield(ILEvaluator* eval_ctx, ILDataType type);
		static errvoid eval_accept(ILEvaluator* eval_ctx, ILDataType type);
		static errvoid eval_callstart(ILEvaluator* eval_ctx);
		static errvoid eval_call(ILEvaluator* eval_ctx, std::uint32_t decl);
		static errvoid eval_duplicate(ILEvaluator* eval_ctx, ILDataType type);
		static errvoid eval_clone(ILEvaluator* eval_ctx, ILDataType type, std::uint16_t times);
		static errvoid eval_swap(ILEvaluator* eval_ctx, ILDataType type);
		static errvoid eval_swap2(ILEvaluator* eval_ctx, ILDataType type1, ILDataType type2);

		static ILDataType arith_result(ILDataType l, ILDataType r);


		static errvoid build_combine_dword(ILBlock* block);
		static errvoid build_high_word(ILBlock* block);
		static errvoid build_low_word(ILBlock* block);
		static errvoid build_extract(ILBlock* block, ILSize s);
		static errvoid build_cut(ILBlock* block, ILSize s);
		static errvoid build_split_dword(ILBlock* block);
		static errvoid build_duplicate(ILBlock* block, ILDataType type);
		static errvoid build_clone(ILBlock* block, ILDataType type, std::uint16_t times);
		static errvoid build_swap(ILBlock* block, ILDataType type);
		static errvoid build_swap2(ILBlock* block, ILDataType type1, ILDataType type2);
		static errvoid build_tableroffset(ILBlock* block, ILDataType src, ILDataType dst, tableid_t tableid, tableelement_t itemid);
		static errvoid build_tableoffset(ILBlock* block, tableid_t tableid, tableelement_t itemid);
		static errvoid build_constref(ILBlock* block, std::uint32_t constid);
		static errvoid build_staticref(ILBlock* block, std::uint32_t constid);
		static errvoid build_debug(ILBlock* block, std::uint16_t file, std::uint16_t line);
		static errvoid build_load(ILBlock* block, ILDataType type);
		static errvoid build_store(ILBlock* block, ILDataType type);
		static errvoid build_store_rev(ILBlock* block, ILDataType type);
		static errvoid build_negative(ILBlock* block, ILDataType type);
		static errvoid build_memcpy(ILBlock* block, ILSize size);
		static errvoid build_memcpy_rev(ILBlock* block, ILSize size);
		static errvoid build_memcmp(ILBlock* block, ILSize size);
		static errvoid build_memcmp_rev(ILBlock* block, ILSize size);
		static errvoid build_offset(ILBlock* block, ILSize offest);
		static errvoid build_aoffset(ILBlock* block, std::uint32_t offest);
		static errvoid build_woffset(ILBlock* block, std::uint32_t offest);
		static errvoid build_aroffset(ILBlock* block, ILDataType from, ILDataType to, std::uint32_t offest);
		static errvoid build_wroffset(ILBlock* block, ILDataType from, ILDataType to, std::uint32_t offest);
		static errvoid build_roffset(ILBlock* block, ILDataType src, ILDataType dst, ILSize offset);
		static errvoid build_rtoffset(ILBlock* block);
		static errvoid build_rtoffset_rev(ILBlock* block);
		static errvoid build_rmemcmp(ILBlock* block, ILDataType type);
		static errvoid build_rmemcmp2(ILBlock* block, ILDataType type);
		static errvoid build_null(ILBlock* block);
		static errvoid build_negate(ILBlock* block);
		static errvoid build_isnotzero(ILBlock* block, ILDataType type);
		static errvoid build_local(ILBlock* block, stackid_t id);
		static errvoid build_vtable(ILBlock* block, std::uint32_t id);
		static errvoid build_add(ILBlock* block, ILDataType tl, ILDataType tr);
		static errvoid build_and(ILBlock* block, ILDataType tl, ILDataType tr);
		static errvoid build_or(ILBlock* block, ILDataType tl, ILDataType tr);
		static errvoid build_xor(ILBlock* block, ILDataType tl, ILDataType tr);
		static errvoid build_eq(ILBlock* block, ILDataType tl, ILDataType tr);
		static errvoid build_ne(ILBlock* block, ILDataType tl, ILDataType tr);
		static errvoid build_gt(ILBlock* block, ILDataType tl, ILDataType tr);
		static errvoid build_lt(ILBlock* block, ILDataType tl, ILDataType tr);
		static errvoid build_ge(ILBlock* block, ILDataType tl, ILDataType tr);
		static errvoid build_le(ILBlock* block, ILDataType tl, ILDataType tr);
		static errvoid build_sub(ILBlock* block, ILDataType tl, ILDataType tr);
		static errvoid build_div(ILBlock* block, ILDataType tl, ILDataType tr);
		static errvoid build_rem(ILBlock* block, ILDataType tl, ILDataType tr);
		static errvoid build_mul(ILBlock* block, ILDataType tl, ILDataType tr);
		static errvoid build_cast(ILBlock* block, ILDataType tl, ILDataType tr);
		static errvoid build_bitcast(ILBlock* block, ILDataType tl, ILDataType tr);
		static errvoid build_accept(ILBlock* block, ILDataType type);
		static errvoid build_discard(ILBlock* block, ILDataType type);
		static errvoid build_yield(ILBlock* block, ILDataType type);
		static errvoid build_forget(ILBlock* block, ILDataType type);
		static errvoid build_ret(ILBlock* block, ILDataType type);
		static errvoid build_fnptr(ILBlock* block, ILFunction* fun);
		static errvoid build_fnptr_id(ILBlock* block, std::uint32_t id);
		static errvoid build_fncall(ILBlock* eval_ctx, ILFunction* fun);
		static errvoid build_call(ILBlock* block, std::uint32_t decl);
		static errvoid build_callstart(ILBlock* block);
		static errvoid build_jmp(ILBlock* block, ILBlock* address);
		static errvoid build_jmpz(ILBlock* block, ILBlock* ifz, ILBlock* ifnz);
		static errvoid build_insintric(ILBlock* block, ILInsintric fun);
	};
}

#endif