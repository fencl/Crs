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

namespace Corrosive {

	struct string_exception : public std::exception
	{
		std::string s;
		inline string_exception(std::string msg) { s = std::move(msg); }
		inline ~string_exception() throw () {}
		const char* what() const throw() { return s.c_str(); }
	};

	void throw_il_wrong_data_flow_error();
	void throw_il_nothing_on_stack_error();
	void throw_il_remaining_stack_error();
	void throw_il_wrong_arguments_error();
	void throw_il_wrong_type_error();

	size_t _align_up(size_t value, size_t alignment);

	class ILBytecodeFunction;
	class ILFunction;
	class ILModule;
	class ILEvaluator;

	enum class ILContext {
		compile, runtime, both
	};

	enum class ILInstruction : unsigned char {
		u8, i8, u16, i16, u32, i32, u64, i64, f32, f64, word, size,

		add, sub, div, mul, rem,
		bit_and, bit_or, bit_xor,
		eq, ne, gt, ge, lt, le,

		add2, sub2, div2, mul2, rem2,
		bit_and2, bit_or2, bit_xor2,
		eq2, ne2, gt2, ge2, lt2, le2,

		load, store, store2, ret, jmp, jmpz,
		local, forget, fncall,
		fnptr, call, start, insintric, cast,
		roffset, offset, vtable, duplicate, duplicate2, swap, swap2,
		memcpy, memcpy2, memcmp, memcmp2, rmemcmp, rmemcmp2,
		rtoffset, rtoffset2, null, isnotzero, yield, discard, accept,
		debug, constref, negative, negate, tableoffset, tableroffset, tableoffset2,
		aoffset, aoffset2, woffset, woffset2, aroffset, wroffset, clone, clone2
	};

	enum class ILDataType : unsigned char {
		i8, u8, i16, u16, i32, u32, i64, u64, f32, f64, word, none, undefined
	};

	enum class ILArchitecture : unsigned char {
		none, bit64, bit32
	};

	enum class ILSizeType : unsigned char {
		array, table, absolute, word
	};

	enum class ILCallingConvention : uint8_t {
		bytecode, win_x64, x86_cdecl, x86_stdcall, __max
	};

	struct ILSize {
		ILSize();
		ILSize(ILSizeType type, uint32_t value);

		ILSizeType type;
		uint32_t value;

		size_t eval(ILModule* mod, ILArchitecture arch) const;
		size_t alignment(ILModule* mod, ILArchitecture arch) const;

		static const ILSize single_ptr;
		static const ILSize double_ptr;

		void print();
	};

	struct ILStructTable {
		std::vector<ILSize> elements;
		std::vector<size_t> calculated_offsets;
		size_t calculated_size;
		size_t calculated_alignment;
		ILArchitecture calculated_for = ILArchitecture::none;
		void calculate(ILModule* mod, ILArchitecture arch);
	};

	struct ILArrayTable {
		ILSize element;
		uint32_t count;
		size_t calculated_size;
		size_t calculated_alignment;
		ILArchitecture calculated_for = ILArchitecture::none;
		void calculate(ILModule* mod, ILArchitecture arch);
	};


	struct ILBlockData {
		unsigned int size = 0;
		unsigned char data[1024];
	};

	class ILBlock {
	public:
		std::string alias;

		uint32_t id;
		ILDataType yields = ILDataType::none;
		ILDataType accepts = ILDataType::none;
		ILBytecodeFunction* parent;
		std::list<std::unique_ptr<ILBlockData>> data_pool;
		std::set<ILBlock*> predecessors;

		void write_instruction(ILInstruction instruction);
		void write_value(size_t size, unsigned char* value);
		void write_const_type(ILDataType type);

		template<typename T> inline T pop() {
			ILBlockData* bd = data_pool.back().get();
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
		static void dump_data_type(ILDataType dt);
		bool assert_flow();

		unsigned char* read_data(size_t, std::list<std::unique_ptr<ILBlockData>>::iterator& pool, size_t& memoff);
		unsigned char* reserve_data(size_t size);
		void memmove(std::list<std::unique_ptr<ILBlockData>>::iterator& pool, size_t& memoff, size_t off);
	};



	enum class ILLifetimeEvent : unsigned char {
		push, pop, append
	};

	struct ILLifetime {
		uint32_t id = 0;
		std::vector<unsigned char> lifetime;
		void push();
		void pop();
		uint32_t append(ILSize s);
		uint32_t append_unknown(size_t& holder);
		void resolve_unknown(size_t holder, ILSize s);
		void discard_push();
	};

	class ILFunction {
	public:
		virtual ~ILFunction();
		std::string alias;
		unsigned int id;
		ILModule* parent;

		uint32_t decl_id;
	};

	class ILBytecodeFunction : public ILFunction {
	public:
		bool is_const = false;

		std::vector<size_t>			calculated_local_offsets;
		size_t						calculated_local_stack_size;
		size_t						calculated_local_stack_alignment;
		ILArchitecture				calculated_for = ILArchitecture::none;
		ILLifetime					local_stack_lifetime;

		void calculate_stack(ILArchitecture arch);

		std::vector<ILBlock*>						blocks;
		std::vector<std::unique_ptr<ILBlock>>		blocks_memory;
		std::set<ILBlock*>							return_blocks;

		ILBlock* create_block();
		ILBlock* create_and_append_block();
		void		 append_block(ILBlock* block);
		void		 dump();
		bool		 assert_flow();
	};

	class ILExtFunction : public ILFunction {
	public:
		void (*ptr)(ILEvaluator*) = nullptr;
	};

	using ilsize_t = uint64_t; // max register size for all architectures (temp storage)

	enum class ILInsintric : unsigned char {
		push_template, build_template, type_dynamic_cast
	};

	extern const ILArchitecture compiler_arch;

	class ILEvaluator {
	public:
		using register_value = uint64_t;

		ILModule* parent = nullptr;

		static const inline size_t stack_size = 1024 * 4;
		unsigned char memory_stack[stack_size];

		uint8_t register_stack_1b[stack_size];
		uint8_t* register_stack_pointer_1b = register_stack_1b;

		uint16_t register_stack_2b[stack_size];
		uint16_t* register_stack_pointer_2b = register_stack_2b;

		uint32_t register_stack_4b[stack_size];
		uint32_t* register_stack_pointer_4b = register_stack_4b;

		uint64_t register_stack_8b[stack_size];
		uint64_t* register_stack_pointer_8b = register_stack_8b;

		ilsize_t yield_storage;

		std::vector<void*> callstack;
		std::vector<std::tuple<uint16_t, uint16_t, std::string_view>> callstack_debug;

		uint16_t debug_line = 0;
		uint16_t debug_file = 0;

		std::vector<std::string> debug_file_names;
		uint16_t register_debug_source(std::string name);

		std::vector<size_t> local_stack_size;
		std::vector<unsigned char*> local_stack_base;
		std::vector<std::vector<unsigned char*>> local_stack_offsets;

		uint16_t		mask_local(unsigned char* ptr);
		void			pop_mask_local();
		uint16_t		push_local(ILSize size);
		void			pop_local(ILSize size);
		void			stack_push(size_t align = 1);
		void			stack_pop();
		unsigned char*	stack_ptr(uint16_t id);

		static void sandbox_begin();
		static void sandbox_end();

		void	write_register_value_indirect(size_t size, void* value);
		void	pop_register_value_indirect(size_t size, void* into);
		void*	read_last_register_value_indirect(ILDataType rs);
		size_t	compile_time_register_size(ILDataType t);


		template<typename T> inline T read_register_value() {
			switch (sizeof(T)) {
				case 1:
					return *(T*)(register_stack_pointer_1b - 1);
				case 2:
					return *(T*)(register_stack_pointer_2b - 1);
				case 3:
				case 4:
					return *(T*)(register_stack_pointer_4b - 1);
				default:
					return *(T*)(register_stack_pointer_8b - 1);
			}
		}

		template<typename T> inline T pop_register_value() {
			switch (sizeof(T)) {
				case 1:
					return *(T*)(--register_stack_pointer_1b);
				case 2:
					return *(T*)(--register_stack_pointer_2b);
				case 3:
				case 4:
					return *(T*)(--register_stack_pointer_4b);
				default:
					return *(T*)(--register_stack_pointer_8b );
			}
		}

		template<typename T> inline void write_register_value(T v) {
			
			switch (sizeof(T)) {
				case 1:
					*(T*)(register_stack_pointer_1b++) = v; break;
				case 2:
					*(T*)(register_stack_pointer_2b++) = v; break;
				case 3:
				case 4:
					*(T*)(register_stack_pointer_4b++) = v; break;
				default:
					*(T*)(register_stack_pointer_8b++) = v; break;
			}
		}
	};


	void throw_runtime_exception(const ILEvaluator* eval, std::string_view message);
	void throw_segfault_exception(const ILEvaluator* eval, int signal);
	void throw_interrupt_exception(const ILEvaluator* eval, int signal);
	void throw_runtime_handler_exception(const ILEvaluator* eval);

	class ILModule {
	public:
		std::vector<std::unique_ptr<unsigned char[]>> constant_memory;
		std::vector<std::unique_ptr<ILFunction>> functions;
		std::vector<std::unique_ptr<void* []>> vtable_data;
		std::vector<ILStructTable> structure_tables;
		std::vector<ILArrayTable> array_tables;
		std::vector<std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>>> function_decl;

		uint32_t register_structure_table();
		uint32_t register_array_table();

		uint32_t register_vtable(std::unique_ptr<void* []> table);

		void (*insintric_function[256])(ILEvaluator*);
		std::string insintric_function_name[256];

		ILBytecodeFunction* create_function();
		ILExtFunction* create_ext_function();

		uint32_t register_constant(unsigned char* memory, size_t size);
		uint32_t register_function_decl(std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>> decl);
		void dump_function_decl(uint32_t id);
	};

	void abi_dynamic_call(ILEvaluator* eval, ILCallingConvention conv, void* ptr, std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>>& decl);

	class ILBuilder {
	public:

		static void eval_const_i8(ILEvaluator* eval_ctx, int8_t   value);
		static void eval_const_i16(ILEvaluator* eval_ctx, int16_t  value);
		static void eval_const_i32(ILEvaluator* eval_ctx, int32_t  value);
		static void eval_const_i64(ILEvaluator* eval_ctx, int64_t  value);
		static void eval_const_u8(ILEvaluator* eval_ctx, uint8_t  value);
		static void eval_const_u16(ILEvaluator* eval_ctx, uint16_t value);
		static void eval_const_u32(ILEvaluator* eval_ctx, uint32_t value);
		static void eval_const_u64(ILEvaluator* eval_ctx, uint64_t value);
		static void eval_const_f32(ILEvaluator* eval_ctx, float    value);
		static void eval_const_f64(ILEvaluator* eval_ctx, double   value);
		static void eval_const_type(ILEvaluator* eval_ctx, void* value);
		static void eval_const_ptr(ILEvaluator* eval_ctx, void* value);
		static void eval_const_size(ILEvaluator* eval_ctx, size_t   value);

		static void build_const_i8(ILBlock* block, int8_t   value);
		static void build_const_i16(ILBlock* block, int16_t  value);
		static void build_const_i32(ILBlock* block, int32_t  value);
		static void build_const_i64(ILBlock* block, int64_t  value);
		static void build_const_u8(ILBlock* block, uint8_t  value);
		static void build_const_u16(ILBlock* block, uint16_t value);
		static void build_const_u32(ILBlock* block, uint32_t value);
		static void build_const_u64(ILBlock* block, uint64_t value);
		static void build_const_f32(ILBlock* block, float    value);
		static void build_const_f64(ILBlock* block, double   value);
		static void build_const_type(ILBlock* block, void* value);
		static void build_const_size(ILBlock* block, ILSize   value);

		static void eval_tableoffset(ILEvaluator* eval_ctx, uint32_t tableid, uint16_t itemid);
		static void eval_tableoffset_pair(ILEvaluator* eval_ctx, uint32_t tableid, uint16_t itemid);
		static void eval_tableroffset(ILEvaluator* eval_ctx, ILDataType src, ILDataType dst, uint32_t tableid, uint16_t itemid);
		static void eval_constref(ILEvaluator* eval_ctx, uint32_t constid);
		static void eval_debug(ILEvaluator* eval_ctx, uint16_t file, uint16_t line);
		static void eval_load(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_store(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_store_rev(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_negative(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_vtable(ILEvaluator* eval_ctx, uint32_t id);
		static void eval_memcpy(ILEvaluator* eval_ctx, size_t size);
		static void eval_memcpy_rev(ILEvaluator* eval_ctx, size_t size);
		static void eval_memcmp(ILEvaluator* eval_ctx, size_t size);
		static void eval_memcmp_rev(ILEvaluator* eval_ctx, size_t size);
		static void eval_null(ILEvaluator* eval_ctx);
		static void eval_negate(ILEvaluator* eval_ctx);
		static void eval_rmemcmp(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_rmemcmp_rev(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_local(ILEvaluator* eval_ctx, uint16_t id);
		static void eval_isnotzero(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_rtoffset(ILEvaluator* eval_ctx);
		static void eval_rtoffset_rev(ILEvaluator* eval_ctx);
		static void eval_offset(ILEvaluator* eval_ctx, ILSize offset);
		static void eval_aoffset(ILEvaluator* eval_ctx, uint32_t offset);
		static void eval_aoffset_pair(ILEvaluator* eval_ctx, uint32_t offset);
		static void eval_woffset(ILEvaluator* eval_ctx, uint32_t offset);
		static void eval_woffset_pair(ILEvaluator* eval_ctx, uint32_t offset);
		static void eval_aroffset(ILEvaluator* eval_ctx, ILDataType from, ILDataType to, uint8_t offset);
		static void eval_wroffset(ILEvaluator* eval_ctx, ILDataType from, ILDataType to, uint8_t offset);
		static void eval_roffset(ILEvaluator* eval_ctx, ILDataType src, ILDataType dst, size_t offset);
		static void eval_add(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static void eval_and(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static void eval_or(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static void eval_xor(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static void eval_eq(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static void eval_ne(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static void eval_gt(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static void eval_lt(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static void eval_ge(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static void eval_le(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static void eval_sub(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static void eval_div(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static void eval_rem(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static void eval_mul(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static void eval_cast(ILEvaluator* eval_ctx, ILDataType tl, ILDataType tr);
		static void eval_forget(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_fnptr(ILEvaluator* eval_ctx, ILFunction* fun);
		static void eval_fncall(ILEvaluator* eval_ctx, ILFunction* fun);
		static void eval_insintric(ILEvaluator* eval_ctx, ILInsintric fun);
		static void eval_discard(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_yield(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_accept(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_callstart(ILEvaluator* eval_ctx);
		static void eval_call(ILEvaluator* eval_ctx, uint32_t decl);
		static void eval_duplicate(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_clone(ILEvaluator* eval_ctx, ILDataType type, uint16_t times);
		static void eval_duplicate_pair(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_clone_pair(ILEvaluator* eval_ctx, ILDataType type, uint16_t times);
		static void eval_swap(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_swap2(ILEvaluator* eval_ctx, ILDataType type1, ILDataType type2);

		static ILDataType arith_result(ILDataType l, ILDataType r);

		static void build_duplicate(ILBlock* block, ILDataType type);
		static void build_clone(ILBlock* block, ILDataType type, uint16_t times);
		static void build_duplicate_pair(ILBlock* block, ILDataType type);
		static void build_clone_pair(ILBlock* block, ILDataType type, uint16_t times);
		static void build_swap(ILBlock* block, ILDataType type);
		static void build_swap2(ILBlock* block, ILDataType type1, ILDataType type2);
		static void build_tableroffset(ILBlock* block, ILDataType src, ILDataType dst, uint32_t tableid, uint16_t itemid);
		static void build_tableoffset(ILBlock* block, uint32_t tableid, uint16_t itemid);
		static void build_tableoffset_pair(ILBlock* block, uint32_t tableid, uint16_t itemid);
		static void build_constref(ILBlock* block, uint32_t constid);
		static void build_debug(ILBlock* block, uint16_t file, uint16_t line);
		static void build_load(ILBlock* block, ILDataType type);
		static void build_store(ILBlock* block, ILDataType type);
		static void build_store_rev(ILBlock* block, ILDataType type);
		static void build_negative(ILBlock* block, ILDataType type);
		static void build_memcpy(ILBlock* block, ILSize size);
		static void build_memcpy_rev(ILBlock* block, ILSize size);
		static void build_memcmp(ILBlock* block, ILSize size);
		static void build_memcmp_rev(ILBlock* block, ILSize size);
		static void build_offset(ILBlock* block, ILSize offest);
		static void build_aoffset(ILBlock* block, uint32_t offest);
		static void build_aoffset_pair(ILBlock* block, uint32_t offest);
		static void build_woffset(ILBlock* block, uint32_t offest);
		static void build_woffset_pair(ILBlock* block, uint32_t offest);
		static void build_aroffset(ILBlock* block, ILDataType from, ILDataType to, uint8_t offest);
		static void build_wroffset(ILBlock* block, ILDataType from, ILDataType to, uint8_t offest);
		static void build_roffset(ILBlock* block, ILDataType src, ILDataType dst, ILSize offset);
		static void build_rtoffset(ILBlock* block);
		static void build_rtoffset_rev(ILBlock* block);
		static void build_rmemcmp(ILBlock* block, ILDataType type);
		static void build_rmemcmp2(ILBlock* block, ILDataType type);
		static void build_null(ILBlock* block);
		static void build_negate(ILBlock* block);
		static void build_isnotzero(ILBlock* block, ILDataType type);
		static void build_local(ILBlock* block, uint16_t id);
		static void build_vtable(ILBlock* block, uint32_t id);
		static void build_add(ILBlock* block, ILDataType tl, ILDataType tr);
		static void build_and(ILBlock* block, ILDataType tl, ILDataType tr);
		static void build_or(ILBlock* block, ILDataType tl, ILDataType tr);
		static void build_xor(ILBlock* block, ILDataType tl, ILDataType tr);
		static void build_eq(ILBlock* block, ILDataType tl, ILDataType tr);
		static void build_ne(ILBlock* block, ILDataType tl, ILDataType tr);
		static void build_gt(ILBlock* block, ILDataType tl, ILDataType tr);
		static void build_lt(ILBlock* block, ILDataType tl, ILDataType tr);
		static void build_ge(ILBlock* block, ILDataType tl, ILDataType tr);
		static void build_le(ILBlock* block, ILDataType tl, ILDataType tr);
		static void build_sub(ILBlock* block, ILDataType tl, ILDataType tr);
		static void build_div(ILBlock* block, ILDataType tl, ILDataType tr);
		static void build_rem(ILBlock* block, ILDataType tl, ILDataType tr);
		static void build_mul(ILBlock* block, ILDataType tl, ILDataType tr);
		static void build_cast(ILBlock* block, ILDataType tl, ILDataType tr);
		static void build_accept(ILBlock* block, ILDataType type);
		static void build_discard(ILBlock* block, ILDataType type);
		static void build_yield(ILBlock* block, ILDataType type);
		static void build_forget(ILBlock* block, ILDataType type);
		static void build_ret(ILBlock* block, ILDataType type);
		static void build_fnptr(ILBlock* block, ILFunction* fun);
		static void build_fncall(ILBlock* eval_ctx, ILFunction* fun);
		static void build_call(ILBlock* block, uint32_t decl);
		static void build_callstart(ILBlock* block);
		static void build_jmp(ILBlock* block, ILBlock* address);
		static void build_jmpz(ILBlock* block, ILBlock* ifz, ILBlock* ifnz);
		static void build_insintric(ILBlock* block, ILInsintric fun);
	};
}

#endif