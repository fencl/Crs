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


	class ILFunction;
	class ILModule;

	enum class ILContext {
		compile, runtime, both
	};

	enum class ILInstruction : unsigned char {
		value, add, sub, div, mul, rem, bit_and, bit_or, bit_xor,
		load, store, store2, ret, jmp, jmpz,
		eq, ne, gt, ge, lt, le, 
		local, forget, 
		fnptr, call, start, insintric, cast, 
		roffset, offset, vtable, duplicate, swap, swap2,
		memcpy, memcpy2, memcmp, memcmp2, rmemcmp, rmemcmp2,
		malloc, free,
		rtoffset, rtoffset2, null, isnotzero, yield, discard, accept,
		debug, constref, negative, negate, tableoffset, tableroffset
	};

	enum class ILDataType : unsigned char {
		ibool, u8, i8, u16, i16, u32, i32, u64, i64, size, f32, f64, ptr, none, undefined
	};

	enum class ILArchitecture : unsigned char {
		none,x86_64,i386
	};
	
	enum class ILSizeType : unsigned char {
		array, table, absolute, word
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
		uint64_t count;
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
		ILFunction* parent;
		std::list<std::unique_ptr<ILBlockData>> data_pool;
		std::set<ILBlock*> predecessors;

		void write_instruction(ILInstruction instruction);
		void write_value(size_t size,unsigned char* value);
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
		push,pop,append
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
		~ILFunction();
		unsigned int	id;
		ILModule*		parent;
		bool			is_const = false;

		std::string alias;

		std::vector<ILDataType>		arguments;
		ILDataType					returns;

		std::vector<size_t>			calculated_local_offsets;
		size_t						calculated_local_stack_size;
		size_t						calculated_local_stack_alignment;
		ILArchitecture				calculated_for = ILArchitecture::none;
		ILLifetime					local_stack_lifetime;

		void calculate_stack(ILArchitecture arch);


		std::vector<ILBlock*>						blocks;
		std::vector<std::unique_ptr<ILBlock>>		blocks_memory;
		std::set<ILBlock*>							return_blocks;

		ILBlock*	 create_block();
		ILBlock*	 create_and_append_block();
		void		 append_block(ILBlock* block);
		void		 dump();
		bool		 assert_flow();
	};

	using ilsize_t = uint64_t; // max size for all architectures

	enum class ILInsintric : unsigned char {
		build_array, build_reference, push_template, build_template, build_slice, template_cast, type_size
	};

	extern const ILArchitecture compiler_arch;

	class ILEvaluator {
	public:
		using register_value = uint64_t;

		ILModule* parent = nullptr;

		static const inline size_t stack_size = 1024*4;
		unsigned char memory_stack[stack_size];

		unsigned char register_stack[stack_size];
		unsigned char* register_stack_pointer = register_stack;

		ilsize_t yield_storage;

		std::vector<ILFunction*> callstack;
		std::vector<std::tuple<uint16_t, uint16_t,std::string_view>> callstack_debug;
		
		uint16_t debug_line = 0;
		uint16_t debug_file = 0;

		std::vector<std::string> debug_file_names;
		uint16_t register_debug_source(std::string name);

		std::vector<size_t> local_stack_size;
		std::vector<unsigned char*> local_stack_base;
		std::vector<std::vector<unsigned char*>> local_stack_offsets;

		uint16_t mask_local(unsigned char* ptr);
		void pop_mask_local();
		uint16_t push_local(ILSize size);
		void pop_local(ILSize size);
		void stack_push(size_t align = 1);
		void stack_pop();
		unsigned char* stack_ptr(uint16_t id);

		static void sandbox_begin();
		static void sandbox_end();

		void	write_register_value_indirect(size_t size, void* value);
		void	pop_register_value_indirect(size_t size, void* into);
		void*	read_last_register_value_indirect(ILDataType rs);

		size_t	compile_time_register_size(ILDataType t);
		void	discard_last_register_type(ILDataType rs);
		

		template<typename T> inline T read_register_value() {
			if (register_stack_pointer - register_stack < sizeof(T)) { 
				throw std::exception("Compiler error, register stack smaller than requested data"); 
			}
			return *(((T*)register_stack_pointer)-1);
		}

		template<typename T> inline T pop_register_value() {
			if (register_stack_pointer - register_stack < sizeof(T)) { 
				throw std::exception("Compiler error, register stack smaller than requested data"); 
			}
			register_stack_pointer -= sizeof(T);

			//std::cout << "-" << sizeof(T) << "\n";
			return *((T*)register_stack_pointer);
		}

		template<typename T> inline void write_register_value(T v) {
			if (register_stack_pointer - register_stack + sizeof(T) > stack_size) { throw std::exception("Register stack overflow"); }
			*((T*)register_stack_pointer) = v;
			register_stack_pointer += sizeof(T);
			//std::cout << "+" << sizeof(T) << "\n";
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
		std::vector<std::unique_ptr<void*[]>> vtable_data;
		std::vector<ILStructTable> structure_tables;
		std::vector<ILArrayTable> array_tables;

		uint32_t register_structure_table();
		uint32_t register_array_table();

		uint32_t register_vtable(std::unique_ptr<void* []> table);

		void (*insintric_function[256])(ILEvaluator*);
		std::string insintric_function_name[256];

		ILArchitecture architecture = ILArchitecture::x86_64;
		ILFunction* create_function();
		
		uint32_t register_constant(unsigned char* memory, size_t size);
	};


	class ILBuilder {
	public:

		static void eval_const_ibool (ILEvaluator* eval_ctx, uint8_t   value);
		static void eval_const_i8    (ILEvaluator* eval_ctx, int8_t   value);
		static void eval_const_i16   (ILEvaluator* eval_ctx, int16_t  value);
		static void eval_const_i32   (ILEvaluator* eval_ctx, int32_t  value);
		static void eval_const_i64   (ILEvaluator* eval_ctx, int64_t  value);
		static void eval_const_u8    (ILEvaluator* eval_ctx, uint8_t  value);
		static void eval_const_u16   (ILEvaluator* eval_ctx, uint16_t value);
		static void eval_const_u32   (ILEvaluator* eval_ctx, uint32_t value);
		static void eval_const_u64   (ILEvaluator* eval_ctx, uint64_t value);
		static void eval_const_f32   (ILEvaluator* eval_ctx, float    value);
		static void eval_const_f64   (ILEvaluator* eval_ctx, double   value);
		static void eval_const_type  (ILEvaluator* eval_ctx, void*    value);
		static void eval_const_ptr   (ILEvaluator* eval_ctx, void*    value);
		static void eval_const_size  (ILEvaluator* eval_ctx, size_t   value);

		static void build_const_ibool (ILBlock* block, uint8_t  value);
		static void build_const_i8	  (ILBlock* block, int8_t   value);
		static void build_const_i16	  (ILBlock* block, int16_t  value);
		static void build_const_i32	  (ILBlock* block, int32_t  value);
		static void build_const_i64	  (ILBlock* block, int64_t  value);
		static void build_const_u8	  (ILBlock* block, uint8_t  value);
		static void build_const_u16	  (ILBlock* block, uint16_t value);
		static void build_const_u32	  (ILBlock* block, uint32_t value);
		static void build_const_u64	  (ILBlock* block, uint64_t value);
		static void build_const_f32	  (ILBlock* block, float    value);
		static void build_const_f64	  (ILBlock* block, double   value);
		static void build_const_type  (ILBlock* block, void*    value);
		static void build_const_size  (ILBlock* block, ILSize   value);

		static void eval_tableoffset(ILEvaluator* eval_ctx, uint32_t tableid, uint16_t itemid);
		static void eval_tableroffset(ILEvaluator* eval_ctx, ILDataType src, ILDataType dst, uint32_t tableid, uint16_t itemid);
		static void eval_constref(ILEvaluator* eval_ctx, uint32_t constid);
		static void eval_debug(ILEvaluator* eval_ctx, uint16_t file, uint16_t line);
		static void eval_load(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_store(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_store2(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_negative(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_vtable(ILEvaluator* eval_ctx, uint32_t id);
		static void eval_memcpy(ILEvaluator* eval_ctx, size_t size);
		static void eval_memcpy2(ILEvaluator* eval_ctx, size_t size);
		static void eval_memcmp(ILEvaluator* eval_ctx, size_t size);
		static void eval_memcmp2(ILEvaluator* eval_ctx, size_t size);
		static void eval_malloc(ILEvaluator* eval_ctx);
		static void eval_free(ILEvaluator* eval_ctx);
		static void eval_null(ILEvaluator* eval_ctx);
		static void eval_negate(ILEvaluator* eval_ctx);
		static void eval_rmemcmp(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_rmemcmp2(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_local(ILEvaluator* eval_ctx, uint16_t id);
		static void eval_isnotzero(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_rtoffset(ILEvaluator* eval_ctx);
		static void eval_rtoffset2(ILEvaluator* eval_ctx);
		static void eval_offset(ILEvaluator* eval_ctx, size_t offset);
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
		static void eval_insintric(ILEvaluator* eval_ctx, ILInsintric fun);
		static void eval_discard(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_yield(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_accept(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_callstart(ILEvaluator* eval_ctx);
		static void eval_call(ILEvaluator* eval_ctx, ILDataType rett, uint16_t argc);
		static void eval_duplicate(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_swap(ILEvaluator* eval_ctx, ILDataType type);
		static void eval_swap2(ILEvaluator* eval_ctx, ILDataType type1, ILDataType type2);
		static void build_duplicate(ILBlock* block, ILDataType type);
		static void build_swap(ILBlock* block, ILDataType type);
		static void build_swap2(ILBlock* block, ILDataType type1, ILDataType type2);

		static ILDataType arith_result(ILDataType l,ILDataType r);

		static void build_tableroffset(ILBlock* block, ILDataType src, ILDataType dst, uint32_t tableid, uint16_t itemid);
		static void build_tableoffset(ILBlock* block, uint32_t tableid, uint16_t itemid);
		static void build_constref(ILBlock* block, uint32_t constid);
		static void build_debug(ILBlock* block, uint16_t file, uint16_t line);
		static void build_load(ILBlock* block, ILDataType type);
		static void build_store(ILBlock* block, ILDataType type);
		static void build_store2(ILBlock* block, ILDataType type);
		static void build_negative(ILBlock* block, ILDataType type);
		static void build_memcpy(ILBlock* block, ILSize size);
		static void build_memcpy2(ILBlock* block, ILSize size);
		static void build_memcmp(ILBlock* block, ILSize size);
		static void build_memcmp2(ILBlock* block, ILSize size);
		static void build_offset(ILBlock* block, ILSize offest);
		static void build_roffset(ILBlock* block, ILDataType src, ILDataType dst, ILSize offset);
		static void build_rtoffset(ILBlock* block);
		static void build_rtoffset2(ILBlock* block);
		static void build_rmemcmp(ILBlock* block, ILDataType type);
		static void build_rmemcmp2(ILBlock* block, ILDataType type);
		static void build_malloc(ILBlock* block);
		static void build_free(ILBlock* block);
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
		static void build_call(ILBlock* block, ILDataType type, uint16_t argc);
		static void build_callstart(ILBlock* block);
		static void build_jmp(ILBlock* block,ILBlock* address);
		static void build_jmpz(ILBlock* block, ILBlock* ifz, ILBlock* ifnz);
		static void build_insintric(ILBlock* block, ILInsintric fun);
	};
}

#endif
