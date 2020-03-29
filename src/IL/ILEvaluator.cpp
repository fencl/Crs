#include "IL.h"
#include "../Error.h"
#include <algorithm>
#include <functional>
#include <iostream>

namespace Corrosive {


	void ILEvaluator::write_register_value_indirect(size_t size, void* value) {
		memcpy(register_stack_pointer, value, size);
		register_stack_pointer += size;
	}

	bool ILBuilder::eval_const_ibool (ILEvaluator* eval_ctx, int8_t   value) { eval_ctx->write_register_value_indirect(sizeof(int8_t),   (unsigned char*)&value); return true; }
	bool ILBuilder::eval_const_i8    (ILEvaluator* eval_ctx, int8_t   value) { eval_ctx->write_register_value_indirect(sizeof(int8_t),   (unsigned char*)&value); return true; }
	bool ILBuilder::eval_const_i16   (ILEvaluator* eval_ctx, int16_t  value) { eval_ctx->write_register_value_indirect(sizeof(int16_t),  (unsigned char*)&value); return true; }
	bool ILBuilder::eval_const_i32   (ILEvaluator* eval_ctx, int32_t  value) { eval_ctx->write_register_value_indirect(sizeof(int32_t),  (unsigned char*)&value); return true; }
	bool ILBuilder::eval_const_i64   (ILEvaluator* eval_ctx, int64_t  value) { eval_ctx->write_register_value_indirect(sizeof(int64_t),  (unsigned char*)&value); return true; }
	bool ILBuilder::eval_const_u8    (ILEvaluator* eval_ctx, uint8_t  value) { eval_ctx->write_register_value_indirect(sizeof(uint8_t),  (unsigned char*)&value); return true; }
	bool ILBuilder::eval_const_u16   (ILEvaluator* eval_ctx, uint16_t value) { eval_ctx->write_register_value_indirect(sizeof(uint16_t), (unsigned char*)&value); return true; }
	bool ILBuilder::eval_const_u32   (ILEvaluator* eval_ctx, uint32_t value) { eval_ctx->write_register_value_indirect(sizeof(uint32_t), (unsigned char*)&value); return true; }
	bool ILBuilder::eval_const_u64   (ILEvaluator* eval_ctx, uint64_t value) { eval_ctx->write_register_value_indirect(sizeof(uint64_t), (unsigned char*)&value); return true; }
	bool ILBuilder::eval_const_f32   (ILEvaluator* eval_ctx, float    value) { eval_ctx->write_register_value_indirect(sizeof(float),    (unsigned char*)&value); return true; }
	bool ILBuilder::eval_const_f64   (ILEvaluator* eval_ctx, double   value) { eval_ctx->write_register_value_indirect(sizeof(double),   (unsigned char*)&value); return true; }
	bool ILBuilder::eval_const_type  (ILEvaluator* eval_ctx, void*    value) { eval_ctx->write_register_value_indirect(sizeof(void*),    (unsigned char*)&value); return true; }
	bool ILBuilder::eval_const_ptr   (ILEvaluator* eval_ctx, ILPtr    value) { eval_ctx->write_register_value_ilptr(value); return true; }


	template<typename T> inline T _il_evaluator_value_pop_into(ILEvaluator* eval_ctx, ILDataType t) {
		switch (t)
		{
		case Corrosive::ILDataType::ibool:
			return (T)eval_ctx->pop_register_value<uint8_t>();
		case Corrosive::ILDataType::u8:
			return (T)eval_ctx->pop_register_value<uint8_t>();
		case Corrosive::ILDataType::i8:
			return (T)eval_ctx->pop_register_value<int8_t>();
		case Corrosive::ILDataType::u16:
			return (T)eval_ctx->pop_register_value<uint16_t>();
		case Corrosive::ILDataType::i16:
			return (T)eval_ctx->pop_register_value<int16_t>();
		case Corrosive::ILDataType::u32:
			return (T)eval_ctx->pop_register_value<uint32_t>();
		case Corrosive::ILDataType::i32:
			return (T)eval_ctx->pop_register_value<int32_t>();
		case Corrosive::ILDataType::u64:
			return (T)eval_ctx->pop_register_value<uint64_t>();
		case Corrosive::ILDataType::i64:
			return (T)eval_ctx->pop_register_value<int64_t>();
		case Corrosive::ILDataType::f32:
			return (T)eval_ctx->pop_register_value<float>();
		case Corrosive::ILDataType::f64:
			return (T)eval_ctx->pop_register_value<double>();
		default:
			return 0;
		}

		return 0;
	}

	template< template<typename Ty> class op> bool _il_evaluator_const_op(ILEvaluator* eval_ctx, ILDataType l, ILDataType r) {
		ILDataType res_t = ILBuilder::arith_result(l, r);
		switch (res_t) {
			case ILDataType::i8: {
					int8_t rval = _il_evaluator_value_pop_into<int8_t>(eval_ctx, r);
					int8_t lval = _il_evaluator_value_pop_into<int8_t>(eval_ctx, l);
					op<int8_t> o;
					if (!ILBuilder::eval_const_i8(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::u8: {
					uint8_t rval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, r);
					uint8_t lval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, l);
					op<uint8_t> o;
					if (!ILBuilder::eval_const_u8(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::ibool: {
					uint8_t rval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, r);
					uint8_t lval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, l);
					op<uint8_t> o;
					if (!ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::i16: {
					int16_t rval = _il_evaluator_value_pop_into<int16_t>(eval_ctx, r);
					int16_t lval = _il_evaluator_value_pop_into<int16_t>(eval_ctx, l);
					op<int16_t> o;
					if (!ILBuilder::eval_const_i16(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::u16: {
					uint16_t rval = _il_evaluator_value_pop_into<uint16_t>(eval_ctx, r);
					uint16_t lval = _il_evaluator_value_pop_into<uint16_t>(eval_ctx, l);
					op<uint16_t> o;
					if (!ILBuilder::eval_const_u16(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::i32: {
					int32_t rval = _il_evaluator_value_pop_into<int32_t>(eval_ctx, r);
					int32_t lval = _il_evaluator_value_pop_into<int32_t>(eval_ctx, l);
					op<int32_t> o;
					if (!ILBuilder::eval_const_i32(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::u32: {
					uint32_t rval = _il_evaluator_value_pop_into<uint32_t>(eval_ctx, r);
					uint32_t lval = _il_evaluator_value_pop_into<uint32_t>(eval_ctx, l);
					op<uint32_t> o;
					if (!ILBuilder::eval_const_u32(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::i64: {
					int64_t rval = _il_evaluator_value_pop_into<int64_t>(eval_ctx, r);
					int64_t lval = _il_evaluator_value_pop_into<int64_t>(eval_ctx, l);
					op<int64_t> o;
					if (!ILBuilder::eval_const_i64(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::u64: {
					uint64_t rval = _il_evaluator_value_pop_into<uint64_t>(eval_ctx, r);
					uint64_t lval = _il_evaluator_value_pop_into<uint64_t>(eval_ctx, l);
					op<uint64_t> o;
					if (!ILBuilder::eval_const_u64(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::f32: {
					float rval = _il_evaluator_value_pop_into<float>(eval_ctx, r);
					float lval = _il_evaluator_value_pop_into<float>(eval_ctx, l);
					op<float> o;
					if (!ILBuilder::eval_const_f32(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::f64: {
					double rval = _il_evaluator_value_pop_into<double>(eval_ctx, r);
					double lval = _il_evaluator_value_pop_into<double>(eval_ctx, l);
					op<double> o;
					if (!ILBuilder::eval_const_f64(eval_ctx, o(lval, rval))) return false;
				}break;
				
		}

		return true;
	}

	template< template<typename Ty> class op> bool _il_evaluator_const_op_binary(ILEvaluator* eval_ctx, ILDataType l, ILDataType r) {
		ILDataType res_t = ILBuilder::arith_result(l, r);
		switch (res_t) {
			case ILDataType::i8: {
					int8_t rval = _il_evaluator_value_pop_into<int8_t>(eval_ctx, r);
					int8_t lval = _il_evaluator_value_pop_into<int8_t>(eval_ctx, l);
					op<int8_t> o;
					if (!ILBuilder::eval_const_i8(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::u8: {
					uint8_t rval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, r);
					uint8_t lval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, l);
					op<uint8_t> o;
					if (!ILBuilder::eval_const_u8(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::ibool: {
					uint8_t rval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, r);
					uint8_t lval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, l);
					op<uint8_t> o;
					if (!ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::i16: {
					int16_t rval = _il_evaluator_value_pop_into<int16_t>(eval_ctx, r);
					int16_t lval = _il_evaluator_value_pop_into<int16_t>(eval_ctx, l);
					op<int16_t> o;
					if (!ILBuilder::eval_const_i16(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::u16: {
					uint16_t rval = _il_evaluator_value_pop_into<uint16_t>(eval_ctx, r);
					uint16_t lval = _il_evaluator_value_pop_into<uint16_t>(eval_ctx, l);
					op<uint16_t> o;
					if (!ILBuilder::eval_const_u16(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::i32: {
					int32_t rval = _il_evaluator_value_pop_into<int32_t>(eval_ctx, r);
					int32_t lval = _il_evaluator_value_pop_into<int32_t>(eval_ctx, l);
					op<int32_t> o;
					if (!ILBuilder::eval_const_i32(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::u32: {
					uint32_t rval = _il_evaluator_value_pop_into<uint32_t>(eval_ctx, r);
					uint32_t lval = _il_evaluator_value_pop_into<uint32_t>(eval_ctx, l);
					op<uint32_t> o;
					if (!ILBuilder::eval_const_u32(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::i64: {
					int64_t rval = _il_evaluator_value_pop_into<int64_t>(eval_ctx, r);
					int64_t lval = _il_evaluator_value_pop_into<int64_t>(eval_ctx, l);
					op<int64_t> o;
					if (!ILBuilder::eval_const_i64(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::u64: {
					uint64_t rval = _il_evaluator_value_pop_into<uint64_t>(eval_ctx, r);
					uint64_t lval = _il_evaluator_value_pop_into<uint64_t>(eval_ctx, l);
					op<uint64_t> o;
					if (!ILBuilder::eval_const_u64(eval_ctx, o(lval, rval))) return false;
				}break;

		}

		return true;
	}

	template< template<typename Ty> class op> bool _il_evaluator_const_op_bool(ILEvaluator* eval_ctx, ILDataType l, ILDataType r) {
		ILDataType res_t = ILBuilder::arith_result(l, r);
		switch (res_t) {
			case ILDataType::i8: {
					int8_t rval = _il_evaluator_value_pop_into<int8_t>(eval_ctx, r);
					int8_t lval = _il_evaluator_value_pop_into<int8_t>(eval_ctx, l);
					op<int8_t> o;
					if (!ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::u8: {
					uint8_t rval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, r);
					uint8_t lval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, l);
					op<uint8_t> o;
					if (!ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::ibool: {
					uint8_t rval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, r);
					uint8_t lval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, l);
					op<uint8_t> o;
					if (!ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::i16: {
					int16_t rval = _il_evaluator_value_pop_into<int16_t>(eval_ctx, r);
					int16_t lval = _il_evaluator_value_pop_into<int16_t>(eval_ctx, l);
					op<int16_t> o;
					if (!ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::u16: {
					uint16_t rval = _il_evaluator_value_pop_into<uint16_t>(eval_ctx, r);
					uint16_t lval = _il_evaluator_value_pop_into<uint16_t>(eval_ctx, l);
					op<uint16_t> o;
					if (!ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::i32: {
					int32_t rval = _il_evaluator_value_pop_into<int32_t>(eval_ctx, r);
					int32_t lval = _il_evaluator_value_pop_into<int32_t>(eval_ctx, l);
					op<int32_t> o;
					if (!ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::u32: {
					uint32_t rval = _il_evaluator_value_pop_into<uint32_t>(eval_ctx, r);
					uint32_t lval = _il_evaluator_value_pop_into<uint32_t>(eval_ctx, l);
					op<uint32_t> o;
					if (!ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::i64: {
					int64_t rval = _il_evaluator_value_pop_into<int64_t>(eval_ctx, r);
					int64_t lval = _il_evaluator_value_pop_into<int64_t>(eval_ctx, l);
					op<int64_t> o;
					if (!ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval))) return false;
				}break;
			case ILDataType::u64: {
					uint64_t rval = _il_evaluator_value_pop_into<uint64_t>(eval_ctx, r);
					uint64_t lval = _il_evaluator_value_pop_into<uint64_t>(eval_ctx, l);
					op<uint64_t> o;
					if (!ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval))) return false;
				}break;
		}

		return true;
	}



	void ILEvaluator::pop_register_value_indirect(size_t size, void* into) {
		register_stack_pointer -= size;
		memcpy(into, register_stack_pointer, size);
	}

	bool ILBuilder::eval_accept(ILEvaluator* eval_ctx) {
		eval_ctx->write_register_value_indirect(eval_ctx->compile_time_register_size(eval_ctx->yield_type), (unsigned char*)&eval_ctx->yield);
		return true;
	}

	bool ILBuilder::eval_discard(ILEvaluator* eval_ctx) {
		return true;
	}

	bool ILBuilder::eval_yield(ILEvaluator* eval_ctx, ILDataType yt) {
		eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(yt), (unsigned char*)&eval_ctx->yield);
		eval_ctx->yield_type = yt;
		return true;
	}


	size_t ILEvaluator :: compile_time_register_size(ILDataType t) {
		switch (t)
		{
			case Corrosive::ILDataType::ibool:
				return 1;
			case Corrosive::ILDataType::u8:
				return 1;
			case Corrosive::ILDataType::i8:
				return 1;
			case Corrosive::ILDataType::u16:
				return 2;
			case Corrosive::ILDataType::i16:
				return 2;
			case Corrosive::ILDataType::u32:
				return 4;
			case Corrosive::ILDataType::i32:
				return 4;
			case Corrosive::ILDataType::u64:
				return 8;
			case Corrosive::ILDataType::i64:
				return 8;
			case Corrosive::ILDataType::f32:
				return sizeof(float);
			case Corrosive::ILDataType::f64:
				return sizeof(double);
			case Corrosive::ILDataType::type:
				return sizeof(void*);
			case Corrosive::ILDataType::ptr: {
					switch (parent->architecture)
					{
						case ILArchitecture::i386:
							return 4;
						case ILArchitecture::x86_64:
							return 8;
					}
					return 0;
				}
			case Corrosive::ILDataType::none:
				return 0;
			case Corrosive::ILDataType::undefined:
				return 0;
			default:
				return 0;
		}
	}



	void ILEvaluator::setup_allocator() {
		ILEvaluator::ILEvalAllocHeader* heap_start = (ILEvaluator::ILEvalAllocHeader*)memory_heap;
		heap_start->next = 0;
		heap_start->prev = 0;
		heap_start->size = UINT16_MAX;
		heap_start->used = false;
		mem_allocated = sizeof(ILEvaluator::ILEvalAllocHeader);
		mem_fragmentation = 0;
	}


	ILEvaluator::ILEvalAllocHeader* ILEvaluator::read_header(uint16_t ptr) {
		return (ILEvaluator::ILEvalAllocHeader*)&memory_heap[ptr - sizeof(ILEvaluator::ILEvalAllocHeader)];
	}

	

	ILPtr ILEvaluator::malloc(size_t size) {

		unsigned char* from_mem = (unsigned char*)memory_heap;

		uint16_t from = sizeof(ILEvaluator::ILEvalAllocHeader);

		while (from != 0) {
			ILEvaluator::ILEvalAllocHeader* hdr = read_header(from);
			if (!hdr->used && hdr->size >= size) {
				hdr->used = true;
				uint16_t hdr_size = hdr->size;

				

				if (hdr_size- (uint16_t)size > sizeof(ILEvaluator::ILEvalAllocHeader)) {
					uint16_t nfrom = from + (uint16_t)size + sizeof(ILEvaluator::ILEvalAllocHeader);


					hdr->size = (uint16_t)size;

					ILEvaluator::ILEvalAllocHeader* nhdr = read_header(nfrom);
					nhdr->prev = from;
					nhdr->next = hdr->next;
					hdr->next = nfrom;
					nhdr->size = hdr_size - (uint16_t)size - sizeof(ILEvaluator::ILEvalAllocHeader);
					nhdr->used = false;
					mem_allocated += sizeof(ILEvaluator::ILEvalAllocHeader) + hdr->size;
					return map_back(&from_mem[from]);
				}

			}
			else {
				from = hdr->next;
			}
		}

		return 0;
	}

	uint16_t ILEvaluator::ilptr_to_heap(ILPtr p) {
		return (uint16_t)p;
	}


	void ILEvaluator::dump_memory_statistics(bool print_blocks) {
		std::cout << "ILEvaluator allocator statistics:\n\tallocated: "<<mem_allocated<<"B\n\tfragmentation: "<<mem_fragmentation<<"\n";
		if (print_blocks) {
			uint16_t from = sizeof(ILEvaluator::ILEvalAllocHeader);
			while (from != 0) {
				ILEvaluator::ILEvalAllocHeader* hdr = read_header(from);
				std::cout << "\t\t" << from << "-" << (from + hdr->size) << " (" << hdr->used << "): <- " << hdr->prev << ", -> " << hdr->next << "\n";
				from = hdr->next;
			}
		}
	}

	void ILEvaluator::free(ILPtr p) {
		uint16_t ptr = ilptr_to_heap(p);
		ILEvaluator::ILEvalAllocHeader* hdr = read_header(ptr);
		hdr->used = false;


		mem_allocated -= sizeof(ILEvaluator::ILEvalAllocHeader) + hdr->size;
		mem_fragmentation += 1;

		if (hdr->next != 0) {
			ILEvaluator::ILEvalAllocHeader* hdr_next = read_header(hdr->next);
			if (!hdr_next->used) {
				hdr->size = hdr->size + hdr_next->size + sizeof(ILEvaluator::ILEvalAllocHeader);
				hdr->next = hdr_next->next;
				mem_fragmentation -= 1;
			}
		}

		if (hdr->prev != 0) {
			ILEvaluator::ILEvalAllocHeader* hdr_prev = read_header(hdr->prev);
			if (!hdr_prev->used) {
				hdr_prev->size = hdr_prev->size + hdr->size + sizeof(ILEvaluator::ILEvalAllocHeader);
				hdr_prev->next = hdr->next;
				mem_fragmentation -= 1;
			}
		}

	}


	bool ILBuilder::eval_load(ILEvaluator* eval_ctx, ILDataType type) {
		ILPtr ptr = eval_ctx->pop_register_value_ilptr();
		void* mem = eval_ctx->map(ptr);
		eval_ctx->write_register_value_indirect(eval_ctx->compile_time_register_size(type), mem);
		return true;
	}

	bool ILBuilder::eval_member(ILEvaluator* eval_ctx, uint32_t offset) {
		if (offset > 0) {
			ILPtr mem = eval_ctx->pop_register_value_ilptr();
			mem += offset;
			eval_ctx->write_register_value_ilptr(mem);
		}
		return true;
	}


	void* ILEvaluator::map(ILPtr ptr) {
		if (ptr < heap_size) {
			return (memory_heap)+ ptr;
		}
		else if (ptr < heap_size + stack_size) {
			return memory_stack + (ptr - heap_size);
		}
		else {
			return nullptr;
		}
	}

	bool ILBuilder::eval_store(ILEvaluator* eval_ctx, ILDataType type) {
		void* mem = eval_ctx->map(eval_ctx->pop_register_value_ilptr());
		eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(type), mem);
		return true;
	}


	ILPtr ILEvaluator::stack_push() {
		std::vector<ILPtr>tmp;
		on_stack.push_back(std::move(tmp));
		return map_back(memory_stack_pointer);
	}
	void ILEvaluator::stack_pop(ILPtr stack_pointer) {
		on_stack.pop_back();
		memory_stack_pointer = (unsigned char*)map(stack_pointer);
	}


	ILPtr ILEvaluator::read_register_value_ilptr() {
		switch (parent->architecture)
		{
			case ILArchitecture::i386:
				return read_register_value<uint32_t>();
			case ILArchitecture::x86_64:
				return read_register_value<uint64_t>();
		}
		return 0;
	}

	ILPtr ILEvaluator::pop_register_value_ilptr() {
		switch (parent->architecture)
		{
			case ILArchitecture::i386:
				return pop_register_value<uint32_t>();
			case ILArchitecture::x86_64:
				return pop_register_value<uint64_t>();
		}
		return 0;
	}

	void ILEvaluator::write_register_value_ilptr(ILPtr ptr) {
		switch (parent->architecture)
		{
			case ILArchitecture::i386:
				write_register_value<uint32_t>((uint32_t)ptr);
				break;
			case ILArchitecture::x86_64:
				write_register_value<uint64_t>((uint64_t)ptr);
				break;
		}
	}


	ILPtr ILEvaluator::load_ilptr(void* from) {
		switch (parent->architecture)
		{
			case ILArchitecture::i386:
				return *((uint32_t*)from);
			case ILArchitecture::x86_64:
				return *((uint64_t*)from);
		}
		return 0;
	}

	void ILEvaluator::store_ilptr(ILPtr ptr, void* to) {
		switch (parent->architecture)
		{
			case ILArchitecture::i386:
				*((uint32_t*)to) = (uint32_t)ptr;
			case ILArchitecture::x86_64:
				*((uint64_t*)to) = (uint64_t)ptr;
		}
	}

	ILPtr ILEvaluator::map_back(void* from) {
		if ((unsigned char*)from - memory_heap < heap_size) {
			switch (parent->architecture)
			{
				case ILArchitecture::i386:
					return (uint32_t)((unsigned char*)from - memory_heap);
				case ILArchitecture::x86_64:
					return (uint64_t)((unsigned char*)from - memory_heap);
			}
		}
		else if ((unsigned char*)from - memory_stack < stack_size) {
			switch (parent->architecture)
			{
				case ILArchitecture::i386:
					return (uint32_t)((unsigned char*)from - memory_stack + heap_size);
				case ILArchitecture::x86_64:
					return (uint64_t)((unsigned char*)from - memory_stack + heap_size);
			}
		}
		
		return 0;
	}

	bool ILBuilder::eval_local(ILEvaluator* eval_ctx, unsigned int id) {
		eval_const_ptr(eval_ctx, eval_ctx->on_stack.back()[id]);
		return true;
	}


	void ILEvaluator::stack_write(size_t size, void* from) {
		on_stack.back().push_back(map_back(memory_stack_pointer));
		memcpy(memory_stack_pointer, from, size);
		memory_stack_pointer += size;
	}


	void ILEvaluator::stack_push_pointer(ILPtr ptr) {
		on_stack.back().push_back(ptr);
	}

	ILPtr ILEvaluator::stack_reserve(size_t size) {
		on_stack.back().push_back(map_back(memory_stack_pointer));
		unsigned char* res = memory_stack_pointer;
		memory_stack_pointer += size;
		return map_back(res);
	}

	bool ILBuilder::eval_add(ILEvaluator* eval_ctx,ILDataType t_l,ILDataType t_r) {
		if (!_il_evaluator_const_op<std::plus>(eval_ctx,t_l,t_r)) return false;

		return true;
	}

	bool ILBuilder::eval_sub(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {


		if (!_il_evaluator_const_op<std::minus>(eval_ctx, t_l, t_r)) return false;

		return true;
	}


	bool ILBuilder::eval_div(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {


		if (!_il_evaluator_const_op<std::divides>(eval_ctx, t_l, t_r)) return false;

		return true;
	}


	bool ILBuilder::eval_rem(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {

		if (!_il_evaluator_const_op_binary<std::modulus>(eval_ctx, t_l, t_r)) return false;

		return true;
	}


	bool ILBuilder::eval_and(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {


		if (!_il_evaluator_const_op_binary<std::bit_and>(eval_ctx, t_l, t_r)) return false;

		return true;
	}


	bool ILBuilder::eval_or(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {


		if (!_il_evaluator_const_op_binary<std::bit_or>(eval_ctx, t_l, t_r)) return false;

		return true;
	}


	bool ILBuilder::eval_xor(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {

		if (!_il_evaluator_const_op_binary<std::bit_xor>(eval_ctx, t_l, t_r)) return false;

		return true;
	}


	bool ILBuilder::eval_mul(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {

		if (!_il_evaluator_const_op<std::multiplies>(eval_ctx, t_l, t_r)) return false;

		return true;
	}


	bool ILBuilder::eval_eq(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {

		if (!_il_evaluator_const_op_bool<std::equal_to>(eval_ctx, t_l, t_r)) return false;

		return true;
	}


	bool ILBuilder::eval_ne(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {

		if (!_il_evaluator_const_op_bool<std::not_equal_to>(eval_ctx, t_l, t_r)) return false;

		return true;
	}


	bool ILBuilder::eval_gt(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_bool<std::greater>(eval_ctx, t_l, t_r)) return false;

		return true;
	}

	bool ILBuilder::eval_ge(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_bool<std::greater_equal>(eval_ctx, t_l, t_r)) return false;

		return true;
	}


	bool ILBuilder::eval_lt(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_bool<std::less>(eval_ctx, t_l, t_r)) return false;

		return true;
	}


	bool ILBuilder::eval_le(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_bool<std::less_equal>(eval_ctx, t_l, t_r)) return false;

		return true;
	}


	
}