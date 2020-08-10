#include "IL.hpp"
#include "../Error.hpp"
#include <algorithm>
#include <functional>
#include <iostream>

#include <csignal>
#include <csetjmp>

namespace Corrosive {


	jmp_buf sandbox;
	int sigint_value = INT_MIN;
	int sigseg_value = INT_MIN;

	void sandbox_siginthandler(int signum) {
		sigseg_value = INT_MIN;
		sigint_value = signum;
		longjmp(sandbox, 1);
	}

	void sandbox_sigseghandler(int signum) {
		sigseg_value = signum;
		sigint_value = INT_MIN;
		longjmp(sandbox, 1);
	}

	void ILEvaluator::sandbox_begin() {
		signal(SIGINT, sandbox_siginthandler);
		signal(SIGSEGV, sandbox_sigseghandler);
		signal(SIGILL, sandbox_siginthandler);
		signal(SIGFPE, sandbox_siginthandler);
	}

	void ILEvaluator::sandbox_end() {
		signal(SIGINT, SIG_DFL);
		signal(SIGSEGV, SIG_DFL);
		signal(SIGILL, SIG_DFL);
		signal(SIGFPE, SIG_DFL);
	}

	void throw_runtime_handler_exception(const ILEvaluator* eval) {
		if (sigint_value != INT_MIN) {
			throw_interrupt_exception(eval, sigint_value);
		}
		else if (sigseg_value != INT_MIN) {
			throw_segfault_exception(eval, sigseg_value);
		}
	}


	void ILEvaluator::write_register_value_indirect(size_t size, void* value) {
		if (setjmp(sandbox) == 0) {
			switch (size) {
				case 0: break;
				case 1:
					memcpy(register_stack_pointer_1b++, value, size); break;
				case 2:
					memcpy(register_stack_pointer_2b++, value, size); break;
				case 3:
				case 4:
					memcpy(register_stack_pointer_4b++, value, size); break;
				case 5:
				case 6:
				case 7:
				case 8:
					memcpy(register_stack_pointer_8b++, value, size); break;
				default:
					memcpy(register_stack_pointer_8b++, value, size); register_stack_pointer_8b++; break;
			}
		}
		else {
			throw_runtime_handler_exception(this);
		}
	}

	void ILEvaluator::pop_register_value_indirect(size_t size, void* into) {
		if (setjmp(sandbox) == 0) {
			ilsize_t storage;
			if (into == nullptr) into = &storage;

			switch (size) {
				case 0: break;
				case 1:
					memcpy(into,--register_stack_pointer_1b, size); break;
				case 2:
					memcpy(into, --register_stack_pointer_2b, size); break;
				case 3:
				case 4:
					memcpy(into, --register_stack_pointer_4b, size); break;
				case 5:
				case 6:
				case 7:
				case 8:
					memcpy(into, --register_stack_pointer_8b, size); break;
				default:
					--register_stack_pointer_8b;
					memcpy(into, --register_stack_pointer_8b, size); break;
			}
		}
		else {

			throw_runtime_handler_exception(this);
		}
	}


	void ILBuilder::eval_const_i8(ILEvaluator* eval_ctx, int8_t   value) { eval_ctx->write_register_value_indirect(sizeof(int8_t), (unsigned char*)&value); }
	void ILBuilder::eval_const_i16(ILEvaluator* eval_ctx, int16_t  value) { eval_ctx->write_register_value_indirect(sizeof(int16_t), (unsigned char*)&value); }
	void ILBuilder::eval_const_i32(ILEvaluator* eval_ctx, int32_t  value) { eval_ctx->write_register_value_indirect(sizeof(int32_t), (unsigned char*)&value); }
	void ILBuilder::eval_const_i64(ILEvaluator* eval_ctx, int64_t  value) { eval_ctx->write_register_value_indirect(sizeof(int64_t), (unsigned char*)&value); }
	void ILBuilder::eval_const_u8(ILEvaluator* eval_ctx, uint8_t  value) { eval_ctx->write_register_value_indirect(sizeof(uint8_t), (unsigned char*)&value); }
	void ILBuilder::eval_const_u16(ILEvaluator* eval_ctx, uint16_t value) { eval_ctx->write_register_value_indirect(sizeof(uint16_t), (unsigned char*)&value); }
	void ILBuilder::eval_const_u32(ILEvaluator* eval_ctx, uint32_t value) { eval_ctx->write_register_value_indirect(sizeof(uint32_t), (unsigned char*)&value); }
	void ILBuilder::eval_const_u64(ILEvaluator* eval_ctx, uint64_t value) { eval_ctx->write_register_value_indirect(sizeof(uint64_t), (unsigned char*)&value); }
	void ILBuilder::eval_const_f32(ILEvaluator* eval_ctx, float    value) { eval_ctx->write_register_value_indirect(sizeof(float), (unsigned char*)&value); }
	void ILBuilder::eval_const_f64(ILEvaluator* eval_ctx, double   value) { eval_ctx->write_register_value_indirect(sizeof(double), (unsigned char*)&value); }
	void ILBuilder::eval_const_type(ILEvaluator* eval_ctx, void* value) { eval_ctx->write_register_value_indirect(sizeof(void*), (unsigned char*)&value); }
	void ILBuilder::eval_const_ptr(ILEvaluator* eval_ctx, void* value) { eval_ctx->write_register_value_indirect(sizeof(void*), (unsigned char*)&value); }
	void ILBuilder::eval_const_size(ILEvaluator* eval_ctx, size_t value) { eval_ctx->write_register_value_indirect(sizeof(size_t), (unsigned char*)&value); }


	template<typename T> inline T _il_evaluator_value_pop_into(ILEvaluator* eval_ctx, ILDataType t) {
		switch (t)
		{
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
			case Corrosive::ILDataType::word:
				return (T)(size_t)eval_ctx->pop_register_value<void*>();
			default:
				return 0;
		}

		return 0;
	}

	void _il_evaluator_cast(ILEvaluator* eval_ctx, ILDataType l, ILDataType res_t) {
		switch (res_t) {
			case ILDataType::i8: {
				int8_t lval = _il_evaluator_value_pop_into<int8_t>(eval_ctx, l);
				ILBuilder::eval_const_i8(eval_ctx, lval);
			}break;
			case ILDataType::u8: {
				uint8_t lval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, l);
				ILBuilder::eval_const_u8(eval_ctx, lval);
			}break;
			case ILDataType::i16: {
				int16_t lval = _il_evaluator_value_pop_into<int16_t>(eval_ctx, l);
				ILBuilder::eval_const_i16(eval_ctx, lval);
			}break;
			case ILDataType::u16: {
				uint16_t lval = _il_evaluator_value_pop_into<uint16_t>(eval_ctx, l);
				ILBuilder::eval_const_u16(eval_ctx, lval);
			}break;
			case ILDataType::i32: {
				int32_t lval = _il_evaluator_value_pop_into<int32_t>(eval_ctx, l);
				ILBuilder::eval_const_i32(eval_ctx, lval);
			}break;
			case ILDataType::u32: {
				uint32_t lval = _il_evaluator_value_pop_into<uint32_t>(eval_ctx, l);
				ILBuilder::eval_const_u32(eval_ctx, lval);
			}break;
			case ILDataType::i64: {
				int64_t lval = _il_evaluator_value_pop_into<int64_t>(eval_ctx, l);
				ILBuilder::eval_const_i64(eval_ctx, lval);
			}break;
			case ILDataType::u64: {
				uint64_t lval = _il_evaluator_value_pop_into<uint64_t>(eval_ctx, l);
				ILBuilder::eval_const_u64(eval_ctx, lval);
			}break;
			case ILDataType::f32: {
				float lval = _il_evaluator_value_pop_into<float>(eval_ctx, l);
				ILBuilder::eval_const_f32(eval_ctx, lval);
			}break;
			case ILDataType::f64: {
				double lval = _il_evaluator_value_pop_into<double>(eval_ctx, l);
				ILBuilder::eval_const_f64(eval_ctx, lval);
			}break;
			case ILDataType::word: {
				size_t lval = _il_evaluator_value_pop_into<size_t>(eval_ctx, l);
				ILBuilder::eval_const_size(eval_ctx, lval);
			}break;
		}
	}

	template< template<typename Ty> class op> void _il_evaluator_const_op(ILEvaluator* eval_ctx, ILDataType l, ILDataType r) {
		ILDataType res_t = ILBuilder::arith_result(l, r);
		switch (res_t) {
			case ILDataType::i8: {
				int8_t rval = _il_evaluator_value_pop_into<int8_t>(eval_ctx, r);
				int8_t lval = _il_evaluator_value_pop_into<int8_t>(eval_ctx, l);
				op<int8_t> o;
				ILBuilder::eval_const_i8(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::u8: {
				uint8_t rval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, r);
				uint8_t lval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, l);
				op<uint8_t> o;
				ILBuilder::eval_const_u8(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::i16: {
				int16_t rval = _il_evaluator_value_pop_into<int16_t>(eval_ctx, r);
				int16_t lval = _il_evaluator_value_pop_into<int16_t>(eval_ctx, l);
				op<int16_t> o;
				ILBuilder::eval_const_i16(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::u16: {
				uint16_t rval = _il_evaluator_value_pop_into<uint16_t>(eval_ctx, r);
				uint16_t lval = _il_evaluator_value_pop_into<uint16_t>(eval_ctx, l);
				op<uint16_t> o;
				ILBuilder::eval_const_u16(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::i32: {
				int32_t rval = _il_evaluator_value_pop_into<int32_t>(eval_ctx, r);
				int32_t lval = _il_evaluator_value_pop_into<int32_t>(eval_ctx, l);
				op<int32_t> o;
				ILBuilder::eval_const_i32(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::u32: {
				uint32_t rval = _il_evaluator_value_pop_into<uint32_t>(eval_ctx, r);
				uint32_t lval = _il_evaluator_value_pop_into<uint32_t>(eval_ctx, l);
				op<uint32_t> o;
				ILBuilder::eval_const_u32(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::i64: {
				int64_t rval = _il_evaluator_value_pop_into<int64_t>(eval_ctx, r);
				int64_t lval = _il_evaluator_value_pop_into<int64_t>(eval_ctx, l);
				op<int64_t> o;
				ILBuilder::eval_const_i64(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::u64: {
				uint64_t rval = _il_evaluator_value_pop_into<uint64_t>(eval_ctx, r);
				uint64_t lval = _il_evaluator_value_pop_into<uint64_t>(eval_ctx, l);
				op<uint64_t> o;
				ILBuilder::eval_const_u64(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::f32: {
				float rval = _il_evaluator_value_pop_into<float>(eval_ctx, r);
				float lval = _il_evaluator_value_pop_into<float>(eval_ctx, l);
				op<float> o;
				ILBuilder::eval_const_f32(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::f64: {
				double rval = _il_evaluator_value_pop_into<double>(eval_ctx, r);
				double lval = _il_evaluator_value_pop_into<double>(eval_ctx, l);
				op<double> o;
				ILBuilder::eval_const_f64(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::word: {
				size_t rval = _il_evaluator_value_pop_into<size_t>(eval_ctx, r);
				size_t lval = _il_evaluator_value_pop_into<size_t>(eval_ctx, l);
				op<size_t> o;
				ILBuilder::eval_const_size(eval_ctx, o(lval, rval));
			}break;

		}
	}

	template< template<typename Ty> class op> void _il_evaluator_const_op_binary(ILEvaluator* eval_ctx, ILDataType l, ILDataType r) {
		ILDataType res_t = ILBuilder::arith_result(l, r);
		switch (res_t) {
			case ILDataType::i8: {
				int8_t rval = _il_evaluator_value_pop_into<int8_t>(eval_ctx, r);
				int8_t lval = _il_evaluator_value_pop_into<int8_t>(eval_ctx, l);
				op<int8_t> o;
				ILBuilder::eval_const_i8(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::u8: {
				uint8_t rval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, r);
				uint8_t lval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, l);
				op<uint8_t> o;
				ILBuilder::eval_const_u8(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::i16: {
				int16_t rval = _il_evaluator_value_pop_into<int16_t>(eval_ctx, r);
				int16_t lval = _il_evaluator_value_pop_into<int16_t>(eval_ctx, l);
				op<int16_t> o;
				ILBuilder::eval_const_i16(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::u16: {
				uint16_t rval = _il_evaluator_value_pop_into<uint16_t>(eval_ctx, r);
				uint16_t lval = _il_evaluator_value_pop_into<uint16_t>(eval_ctx, l);
				op<uint16_t> o;
				ILBuilder::eval_const_u16(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::i32: {
				int32_t rval = _il_evaluator_value_pop_into<int32_t>(eval_ctx, r);
				int32_t lval = _il_evaluator_value_pop_into<int32_t>(eval_ctx, l);
				op<int32_t> o;
				ILBuilder::eval_const_i32(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::u32: {
				uint32_t rval = _il_evaluator_value_pop_into<uint32_t>(eval_ctx, r);
				uint32_t lval = _il_evaluator_value_pop_into<uint32_t>(eval_ctx, l);
				op<uint32_t> o;
				ILBuilder::eval_const_u32(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::i64: {
				int64_t rval = _il_evaluator_value_pop_into<int64_t>(eval_ctx, r);
				int64_t lval = _il_evaluator_value_pop_into<int64_t>(eval_ctx, l);
				op<int64_t> o;
				ILBuilder::eval_const_i64(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::u64: {
				uint64_t rval = _il_evaluator_value_pop_into<uint64_t>(eval_ctx, r);
				uint64_t lval = _il_evaluator_value_pop_into<uint64_t>(eval_ctx, l);
				op<uint64_t> o;
				ILBuilder::eval_const_u64(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::word: {
				size_t rval = _il_evaluator_value_pop_into<size_t>(eval_ctx, r);
				size_t lval = _il_evaluator_value_pop_into<size_t>(eval_ctx, l);
				op<size_t> o;
				ILBuilder::eval_const_size(eval_ctx, o(lval, rval));
			}break;

		}
	}

	template< template<typename Ty> class op> void _il_evaluator_const_op_bool(ILEvaluator* eval_ctx, ILDataType l, ILDataType r) {
		ILDataType res_t = ILBuilder::arith_result(l, r);
		switch (res_t) {
			case ILDataType::i8: {
				int8_t rval = _il_evaluator_value_pop_into<int8_t>(eval_ctx, r);
				int8_t lval = _il_evaluator_value_pop_into<int8_t>(eval_ctx, l);
				op<int8_t> o;
				ILBuilder::eval_const_i8(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::u8: {
				uint8_t rval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, r);
				uint8_t lval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, l);
				op<uint8_t> o;
				ILBuilder::eval_const_i8(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::i16: {
				int16_t rval = _il_evaluator_value_pop_into<int16_t>(eval_ctx, r);
				int16_t lval = _il_evaluator_value_pop_into<int16_t>(eval_ctx, l);
				op<int16_t> o;
				ILBuilder::eval_const_i8(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::u16: {
				uint16_t rval = _il_evaluator_value_pop_into<uint16_t>(eval_ctx, r);
				uint16_t lval = _il_evaluator_value_pop_into<uint16_t>(eval_ctx, l);
				op<uint16_t> o;
				ILBuilder::eval_const_i8(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::i32: {
				int32_t rval = _il_evaluator_value_pop_into<int32_t>(eval_ctx, r);
				int32_t lval = _il_evaluator_value_pop_into<int32_t>(eval_ctx, l);
				op<int32_t> o;
				ILBuilder::eval_const_i8(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::u32: {
				uint32_t rval = _il_evaluator_value_pop_into<uint32_t>(eval_ctx, r);
				uint32_t lval = _il_evaluator_value_pop_into<uint32_t>(eval_ctx, l);
				op<uint32_t> o;
				ILBuilder::eval_const_i8(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::i64: {
				int64_t rval = _il_evaluator_value_pop_into<int64_t>(eval_ctx, r);
				int64_t lval = _il_evaluator_value_pop_into<int64_t>(eval_ctx, l);
				op<int64_t> o;
				ILBuilder::eval_const_i8(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::u64: {
				uint64_t rval = _il_evaluator_value_pop_into<uint64_t>(eval_ctx, r);
				uint64_t lval = _il_evaluator_value_pop_into<uint64_t>(eval_ctx, l);
				op<uint64_t> o;
				ILBuilder::eval_const_i8(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::f32: {
				float rval = _il_evaluator_value_pop_into<float>(eval_ctx, r);
				float lval = _il_evaluator_value_pop_into<float>(eval_ctx, l);
				op<float> o;
				ILBuilder::eval_const_i8(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::f64: {
				double rval = _il_evaluator_value_pop_into<double>(eval_ctx, r);
				double lval = _il_evaluator_value_pop_into<double>(eval_ctx, l);
				op<double> o;
				ILBuilder::eval_const_i8(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::word: {
				size_t rval = _il_evaluator_value_pop_into<size_t>(eval_ctx, r);
				size_t lval = _il_evaluator_value_pop_into<size_t>(eval_ctx, l);
				op<size_t> o;
				ILBuilder::eval_const_i8(eval_ctx, o(lval, rval));
			}break;
		}

	}



	void ILBuilder::eval_forget(ILEvaluator* eval_ctx, ILDataType type) {
		if (type != ILDataType::none) {
			eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(type), nullptr);
		}
	}


	size_t ILEvaluator::compile_time_register_size(ILDataType t) {
		switch (t)
		{
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
				return 4;
			case Corrosive::ILDataType::f64:
				return 8;
			case Corrosive::ILDataType::word:
				return sizeof(void*);
			case Corrosive::ILDataType::dword:
				return 2*sizeof(void*);
			case Corrosive::ILDataType::none:
				return 0;
			case Corrosive::ILDataType::undefined:
				return 0;
			default:
				return 0;
		}
	}


	void ILBuilder::eval_discard(ILEvaluator* eval_ctx, ILDataType type) {}

	void ILBuilder::eval_yield(ILEvaluator* eval_ctx, ILDataType type) {
		eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(type), &eval_ctx->yield_storage);
	}

	void ILBuilder::eval_accept(ILEvaluator* eval_ctx, ILDataType type) {
		eval_ctx->write_register_value_indirect(eval_ctx->compile_time_register_size(type), &eval_ctx->yield_storage);
	}

	void ILBuilder::eval_load(ILEvaluator* eval_ctx, ILDataType type) {

		if (setjmp(sandbox) == 0) {
			auto ptr = eval_ctx->pop_register_value<void*>();
			eval_ctx->write_register_value_indirect(eval_ctx->compile_time_register_size(type), ptr);
		}
		else {
			throw_runtime_handler_exception(eval_ctx);
		}
	}

	void ILBuilder::eval_offset(ILEvaluator* eval_ctx, ILSize offset) {
		if (offset.type == ILSizeType::table || offset.type == ILSizeType::array || offset.value > 0) {
			auto mem = eval_ctx->pop_register_value<size_t>();
			mem += offset.eval(eval_ctx->parent, compiler_arch);
			eval_ctx->write_register_value(mem);
		}
	}



	void ILBuilder::eval_aoffset(ILEvaluator* eval_ctx, uint32_t offset) {
		if (offset > 0) {
			auto mem = eval_ctx->pop_register_value<size_t>();
			mem += offset;
			eval_ctx->write_register_value(mem);
		}
	}


	void ILBuilder::eval_woffset(ILEvaluator* eval_ctx, uint32_t offset) {
		if (offset > 0) {
			auto mem = eval_ctx->pop_register_value<size_t>();
			mem += offset * eval_ctx->compile_time_register_size(ILDataType::word);
			eval_ctx->write_register_value(mem);
		}
	}

	void ILBuilder::eval_constref(ILEvaluator* eval_ctx, uint32_t cid) {
		eval_ctx->write_register_value(eval_ctx->parent->constant_memory[cid].second.get());
	}
	
	void ILBuilder::eval_const_slice(ILEvaluator* eval_ctx, uint32_t cid, uint64_t v) {
		dword_t val = { eval_ctx->parent->constant_memory[cid].second.get() , (void*)v };
		eval_ctx->write_register_value(val);
	}

	void ILBuilder::eval_staticref(ILEvaluator* eval_ctx, uint32_t cid) {
		eval_ctx->write_register_value(eval_ctx->parent->static_memory[cid].second.get());
	}


	void ILBuilder::eval_combine_dword(ILEvaluator* eval_ctx) {
		void* p2 = eval_ctx->pop_register_value<void*>();
		void* p1 = eval_ctx->pop_register_value<void*>();
		dword_t dw = {p1,p2};
		eval_ctx->write_register_value(dw);
	}
	
	void ILBuilder::eval_split_dword(ILEvaluator* eval_ctx) {
		dword_t dw = eval_ctx->pop_register_value<dword_t>();
		eval_ctx->write_register_value(dw.p1);
		eval_ctx->write_register_value(dw.p2);
	}

	void ILBuilder::eval_high_word(ILEvaluator* eval_ctx) {
		dword_t dw = eval_ctx->pop_register_value<dword_t>();
		eval_ctx->write_register_value(dw.p2);
	}

	void ILBuilder::eval_debug(ILEvaluator* eval_ctx, uint16_t file, uint16_t line) {
		eval_ctx->debug_file = file;
		eval_ctx->debug_line = line;
	}

	void ILBuilder::eval_rtoffset(ILEvaluator* eval_ctx) {
		auto offset = eval_ctx->pop_register_value<size_t>();
		auto mem = eval_ctx->pop_register_value<unsigned char*>();
		mem += offset;
		eval_ctx->write_register_value(mem);
	}

	void ILBuilder::eval_rtoffset_rev(ILEvaluator* eval_ctx) {
		auto mem = eval_ctx->pop_register_value<unsigned char*>();
		auto offset = eval_ctx->pop_register_value<size_t>();
		mem += offset;
		eval_ctx->write_register_value(mem);
	}

	void ILBuilder::eval_roffset(ILEvaluator* eval_ctx, ILDataType from, ILDataType to, size_t offset) {
		ilsize_t mem;
		eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(from), &mem);
		mem = mem << offset;
		return eval_ctx->write_register_value_indirect(eval_ctx->compile_time_register_size(to), &mem);
	}

	void ILBuilder::eval_aroffset(ILEvaluator* eval_ctx, ILDataType from, ILDataType to, uint32_t offset) {

		if (offset > UINT8_MAX) {
			throw std::exception("Compiler error: aroffset > 255 should not be possible");
		}

		ilsize_t mem;
		eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(from), &mem);
		mem = mem << offset;
		return eval_ctx->write_register_value_indirect(eval_ctx->compile_time_register_size(to), &mem);
	}

	void ILBuilder::eval_wroffset(ILEvaluator* eval_ctx, ILDataType from, ILDataType to, uint32_t offset) {

		if (offset > UINT8_MAX) {
			throw std::exception("Compiler error: wroffset > 255 should not be possible");
		}

		ilsize_t mem;
		eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(from), &mem);
		mem = mem << (offset * eval_ctx->compile_time_register_size(ILDataType::word));
		return eval_ctx->write_register_value_indirect(eval_ctx->compile_time_register_size(to), &mem);
	}


	void ILBuilder::eval_store(ILEvaluator* eval_ctx, ILDataType type) {
		auto mem = eval_ctx->pop_register_value<void*>();
		eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(type), mem);
	}

	void ILBuilder::eval_store_rev(ILEvaluator* eval_ctx, ILDataType type) {
		if (setjmp(sandbox) == 0) {
			ilsize_t storage;
			size_t regs = eval_ctx->compile_time_register_size(type);
			eval_ctx->pop_register_value_indirect(regs, &storage);
			auto mem = eval_ctx->pop_register_value<unsigned char*>();
			memcpy(mem, &storage, regs);
		}
		else {
			throw_runtime_handler_exception(eval_ctx);
		}
	}

	void ILBuilder::eval_duplicate(ILEvaluator* eval_ctx, ILDataType type) {
		size_t reg_s = eval_ctx->compile_time_register_size(type);
		void* lv = eval_ctx->read_last_register_value_indirect(type);
		eval_ctx->write_register_value_indirect(reg_s, lv);
	}

	void ILBuilder::eval_clone(ILEvaluator* eval_ctx, ILDataType type, uint16_t times) {
		size_t reg_s = eval_ctx->compile_time_register_size(type);
		void* lv = eval_ctx->read_last_register_value_indirect(type);
		for (uint16_t i = 0; i < times; ++i) {
			eval_ctx->write_register_value_indirect(reg_s, lv);
		}
	}

	void ILBuilder::eval_fnptr(ILEvaluator* eval_ctx, ILFunction* fun) {

		if (auto native_fun = dynamic_cast<ILNativeFunction*>(fun)) {
			eval_ctx->write_register_value(native_fun->ptr);
		}
		else {
			eval_ctx->write_register_value(fun);
		}
	}


	void ILBuilder::eval_fncall(ILEvaluator* eval_ctx, ILFunction* fun) {
		if (auto native_fun = dynamic_cast<ILNativeFunction*>(fun)) {
			eval_ctx->callstack.push_back(native_fun->ptr);
			eval_call(eval_ctx, fun->decl_id);
		}
		else {
			eval_ctx->callstack.push_back(fun);
			eval_call(eval_ctx, fun->decl_id);
		}
	}

	void ILBuilder::eval_null(ILEvaluator* eval_ctx) {
		eval_ctx->write_register_value((size_t)0);
	}

	void ILBuilder::eval_isnotzero(ILEvaluator* eval_ctx, ILDataType type) {
		ilsize_t z = 0;
		eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(type), &z);
		eval_const_i8(eval_ctx, (z == 0 ? 0 : 1));
	}


	void ILBuilder::eval_callstart(ILEvaluator* eval_ctx) {
		auto func = eval_ctx->pop_register_value<ILFunction*>();
		eval_ctx->callstack.push_back(func);
	}

	void ILBuilder::eval_insintric(ILEvaluator* eval_ctx, ILInsintric fun) {
		eval_ctx->parent->insintric_function[(unsigned char)fun](eval_ctx);
	}

	void ILBuilder::eval_vtable(ILEvaluator* eval_ctx, uint32_t id) {
		return eval_ctx->write_register_value(eval_ctx->parent->vtable_data[id].second.get());
	}


	void ILBuilder::eval_memcpy(ILEvaluator* eval_ctx, size_t size) {

		if (setjmp(sandbox) == 0) {
			auto dst = eval_ctx->pop_register_value<void*>();
			auto src = eval_ctx->pop_register_value<void*>();
			memcpy(dst, src, size);

		}
		else {

			throw_runtime_handler_exception(eval_ctx);
		}
	}

	void ILBuilder::eval_memcpy_rev(ILEvaluator* eval_ctx, size_t size) {

		if (setjmp(sandbox) == 0) {
			auto src = eval_ctx->pop_register_value<void*>();
			auto dst = eval_ctx->pop_register_value<void*>();
			memcpy(dst, src, size);

		}
		else {

			throw_runtime_handler_exception(eval_ctx);
		}
	}


	void ILBuilder::eval_memcmp(ILEvaluator* eval_ctx, size_t size) {

		if (setjmp(sandbox) == 0) {
			auto dst = eval_ctx->pop_register_value<void*>();
			auto src = eval_ctx->pop_register_value<void*>();
			eval_ctx->write_register_value((int8_t)memcmp(dst, src, size));

		}
		else {

			throw_runtime_handler_exception(eval_ctx);
		}
	}

	void ILBuilder::eval_memcmp_rev(ILEvaluator* eval_ctx, size_t size) {

		if (setjmp(sandbox) == 0) {
			auto src = eval_ctx->pop_register_value<void*>();
			auto dst = eval_ctx->pop_register_value<void*>();
			eval_ctx->write_register_value((int8_t)memcmp(dst, src, size));

		}
		else {

			throw_runtime_handler_exception(eval_ctx);
		}
	}

	void ILBuilder::eval_tableoffset(ILEvaluator* eval_ctx, tableid_t tableid, tableelement_t itemid) {
		auto& table = eval_ctx->parent->structure_tables[tableid];
		table.calculate(eval_ctx->parent, compiler_arch);
		auto ptr = eval_ctx->pop_register_value<unsigned char*>();
		ptr += table.calculated_offsets[itemid];
		eval_ctx->write_register_value(ptr);
	}


	void ILBuilder::eval_tableroffset(ILEvaluator* eval_ctx, ILDataType src, ILDataType dst, tableid_t tableid, tableelement_t itemid) {
		auto& table = eval_ctx->parent->structure_tables[tableid];
		table.calculate(eval_ctx->parent, compiler_arch);
		ilsize_t storage;
		eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(src), &storage);
		storage = storage >> table.calculated_offsets[itemid];
		eval_ctx->write_register_value_indirect(eval_ctx->compile_time_register_size(dst), &storage);
	}

	void ILBuilder::eval_rmemcmp(ILEvaluator* eval_ctx, ILDataType type) {

		if (setjmp(sandbox) == 0) {
			size_t reg_v = eval_ctx->compile_time_register_size(type);
			ilsize_t s1, s2;
			eval_ctx->pop_register_value_indirect(reg_v, &s1);
			eval_ctx->pop_register_value_indirect(reg_v, &s2);
			eval_ctx->write_register_value((int8_t)memcmp(&s1, &s2, reg_v));

		}
		else {

			throw_runtime_handler_exception(eval_ctx);
		}
	}

	void ILBuilder::eval_rmemcmp_rev(ILEvaluator* eval_ctx, ILDataType type) {

		if (setjmp(sandbox) == 0) {
			size_t reg_v = eval_ctx->compile_time_register_size(type);
			ilsize_t s1, s2;
			eval_ctx->pop_register_value_indirect(reg_v, &s2);
			eval_ctx->pop_register_value_indirect(reg_v, &s1);
			eval_ctx->write_register_value((int8_t)memcmp(&s1, &s2, reg_v));

		}
		else {

			throw_runtime_handler_exception(eval_ctx);
		}
	}


	void ILBuilder::eval_swap(ILEvaluator* eval_ctx, ILDataType type) {
		size_t reg_v = eval_ctx->compile_time_register_size(type);
		ilsize_t s1, s2;
		eval_ctx->pop_register_value_indirect(reg_v, &s1);
		eval_ctx->pop_register_value_indirect(reg_v, &s2);
		eval_ctx->write_register_value_indirect(reg_v, &s1);
		eval_ctx->write_register_value_indirect(reg_v, &s2);
	}



	void ILBuilder::eval_negative(ILEvaluator* eval_ctx, ILDataType type) {

		switch (type) {
			case ILDataType::u8:
				eval_ctx->write_register_value(-eval_ctx->pop_register_value<uint8_t>());
				break;
			case ILDataType::i8:
				eval_ctx->write_register_value(-eval_ctx->pop_register_value<int8_t>());
				break;
			case ILDataType::u16:
			case ILDataType::i16:
				eval_ctx->write_register_value(-eval_ctx->pop_register_value<int16_t>());
				break;
			case ILDataType::u32:
			case ILDataType::i32:
				eval_ctx->write_register_value(-eval_ctx->pop_register_value<int32_t>());
				break;
			case ILDataType::u64:
			case ILDataType::i64:
				eval_ctx->write_register_value(-eval_ctx->pop_register_value<int64_t>());
				break;
			case ILDataType::f32:
				eval_ctx->write_register_value(-eval_ctx->pop_register_value<float>());
				break;
			case ILDataType::f64:
				eval_ctx->write_register_value(-eval_ctx->pop_register_value<double>());
				break;
			case ILDataType::word:
				eval_ctx->write_register_value((SIZE_MAX ^ eval_ctx->pop_register_value<size_t>()) - 1);
				break;
		}
	}


	void ILBuilder::eval_negate(ILEvaluator* eval_ctx) {
		if (eval_ctx->pop_register_value<uint8_t>()) {
			eval_ctx->write_register_value<uint8_t>(0);
		}
		else {
			eval_ctx->write_register_value<uint8_t>(1);
		}
	}



	void ILBuilder::eval_swap2(ILEvaluator* eval_ctx, ILDataType type1, ILDataType type2) {
		size_t reg_v_1 = eval_ctx->compile_time_register_size(type1);
		size_t reg_v_2 = eval_ctx->compile_time_register_size(type2);
		ilsize_t s1, s2;
		eval_ctx->pop_register_value_indirect(reg_v_2, &s2);
		eval_ctx->pop_register_value_indirect(reg_v_1, &s1);
		eval_ctx->write_register_value_indirect(reg_v_2, &s2);
		eval_ctx->write_register_value_indirect(reg_v_1, &s1);
	}

	void ILBuilder::eval_call(ILEvaluator* eval_ctx, uint32_t decl) {

		if (setjmp(sandbox) == 0) {
			auto& declaration = eval_ctx->parent->function_decl[decl];

			void* ptr = eval_ctx->callstack.back();
			eval_ctx->callstack.pop_back();

			if (std::get<0>(declaration) == ILCallingConvention::bytecode) {
				ILBytecodeFunction* bytecode_fun = (ILBytecodeFunction*)ptr;

				eval_ctx->callstack_debug.push_back(std::make_tuple(eval_ctx->debug_line, eval_ctx->debug_file, std::string_view(bytecode_fun->alias)));

				bytecode_fun->calculate_stack(compiler_arch);

				ILBlock* block = bytecode_fun->blocks[0];
				bool running = true;

				unsigned char* lstack_base = eval_ctx->stack_base;
				unsigned char* lstack_base_aligned = eval_ctx->stack_base_aligned;
				unsigned char* lstack_pointer = eval_ctx->stack_pointer;

				eval_ctx->stack_base = eval_ctx->stack_pointer;
				eval_ctx->stack_base_aligned = (unsigned char*)_align_up((size_t)eval_ctx->stack_base, bytecode_fun->calculated_local_stack_alignment);
				eval_ctx->stack_pointer = eval_ctx->stack_base_aligned + bytecode_fun->calculated_local_stack_size;


				unsigned char* local_base = eval_ctx->stack_base_aligned;

				size_t instr = 0;

				while (true) {
					auto it = block->data_pool.begin();

					while (it != block->data_pool.end()) {

						auto inst = block->read_data<ILInstruction>(it);
						instr++;

						switch (inst) {
							case ILInstruction::ret: {
								auto type = ILBlock::read_data<ILDataType>(it);
								running = false;
								goto returned;
							} break;
							case ILInstruction::call: {
								auto decl = ILBlock::read_data<uint32_t>(it);
								eval_call(eval_ctx, decl);
							} break;
							case ILInstruction::memcpy: {
								auto size = ILBlock::read_data<ILSize>(it);
								eval_memcpy(eval_ctx, size.eval(eval_ctx->parent, compiler_arch));
							} break;
							case ILInstruction::memcpy2: {
								auto size = ILBlock::read_data<ILSize>(it);
								eval_memcpy_rev(eval_ctx, size.eval(eval_ctx->parent, compiler_arch));
							} break;
							case ILInstruction::memcmp: {
								auto size = ILBlock::read_data<ILSize>(it);
								eval_memcmp(eval_ctx, size.eval(eval_ctx->parent, compiler_arch));
							} break;
							case ILInstruction::memcmp2: {
								auto size = ILBlock::read_data<ILSize>(it);
								eval_memcmp_rev(eval_ctx, size.eval(eval_ctx->parent, compiler_arch));
							} break;
							case ILInstruction::fnptr: {
								auto id = ILBlock::read_data<uint32_t>(it);
								eval_fnptr(eval_ctx,eval_ctx->parent->functions[id].get());
							} break;
							case ILInstruction::fncall: {
								auto id = ILBlock::read_data<uint32_t>(it);
								auto fn = eval_ctx->parent->functions[id].get();
								eval_fncall(eval_ctx, fn);
							} break;
							case ILInstruction::vtable: {
								auto id = ILBlock::read_data<uint32_t>(it);
								eval_vtable(eval_ctx, id);
							} break;
							case ILInstruction::duplicate: {
								auto type = ILBlock::read_data<ILDataType>(it);
								eval_duplicate(eval_ctx, type);
							} break;
							case ILInstruction::clone: {
								auto type = ILBlock::read_data<ILDataType>(it);
								auto times = ILBlock::read_data<uint16_t>(it);
								eval_clone(eval_ctx, type, times);
							} break;
							case ILInstruction::swap: {
								auto type = ILBlock::read_data<ILDataType>(it);
								eval_swap(eval_ctx, type);
							} break;
							case ILInstruction::swap2: {
								auto p = ILBlock::read_data<ILDataTypePair>(it);
								eval_swap2(eval_ctx, p.first(), p.second());
							} break;
							case ILInstruction::insintric: {
								auto id = ILBlock::read_data<uint8_t>(it);
								eval_insintric(eval_ctx, (ILInsintric)id);
							} break;
							case ILInstruction::rmemcmp: {
								auto type = ILBlock::read_data<ILDataType>(it);
								eval_rmemcmp(eval_ctx, type);
							} break;
							case ILInstruction::rmemcmp2: {
								auto type = ILBlock::read_data<ILDataType>(it);
								eval_rmemcmp_rev(eval_ctx, type);
							} break;
							case ILInstruction::sub: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								eval_sub(eval_ctx, type.first(), type.second());
							} break;
							case ILInstruction::div: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								eval_div(eval_ctx, type.first(), type.second());
							} break;
							case ILInstruction::rem: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								eval_rem(eval_ctx, type.first(), type.second());
							} break;
							case ILInstruction::mul: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								eval_mul(eval_ctx, type.first(), type.second());
							} break;
							case ILInstruction::add: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								eval_add(eval_ctx, type.first(), type.second());
							} break;
							case ILInstruction::bit_and: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								eval_and(eval_ctx, type.first(), type.second());
							} break;
							case ILInstruction::bit_or: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								eval_or(eval_ctx, type.first(), type.second());
							} break;
							case ILInstruction::bit_xor: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								eval_xor(eval_ctx, type.first(), type.second());
							} break;
							case ILInstruction::eq: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								eval_eq(eval_ctx, type.first(), type.second());
							} break;
							case ILInstruction::ne: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								eval_ne(eval_ctx, type.first(), type.second());
							} break;
							case ILInstruction::gt: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								eval_gt(eval_ctx, type.first(), type.second());
							} break;
							case ILInstruction::lt: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								eval_lt(eval_ctx, type.first(), type.second());
							} break;
							case ILInstruction::ge: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								eval_ge(eval_ctx, type.first(), type.second());
							} break;
							case ILInstruction::le: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								eval_le(eval_ctx, type.first(), type.second());
							} break;

							case ILInstruction::cast: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								eval_cast(eval_ctx, pair.first(), pair.second());
							} break;
							case ILInstruction::bitcast: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								eval_bitcast(eval_ctx, pair.first(), pair.second());
							} break;
							case ILInstruction::store: {
								auto type = ILBlock::read_data<ILDataType>(it);
								eval_store(eval_ctx, type);
							} break;
							case ILInstruction::store2: {
								auto type = ILBlock::read_data<ILDataType>(it);
								eval_store_rev(eval_ctx, type);
							} break;
							case ILInstruction::yield: {
								auto type = ILBlock::read_data<ILDataType>(it);
								eval_yield(eval_ctx, type);
							} break;
							case ILInstruction::accept: {
								auto type = ILBlock::read_data<ILDataType>(it);
								eval_accept(eval_ctx, type);
							} break;
							case ILInstruction::discard: {
								auto type = ILBlock::read_data<ILDataType>(it);
								eval_discard(eval_ctx, type);
							} break;
							case ILInstruction::start: {
								eval_callstart(eval_ctx);
							} break;

							case ILInstruction::null: {
								eval_null(eval_ctx);
							} break;

							case ILInstruction::jmp: {
								auto address = ILBlock::read_data<uint32_t>(it);
								block = block->parent->blocks_memory[address].get();
								goto next_block;
							}break;
							case ILInstruction::offset32: {
								auto t = ILBlock::read_data<uint8_t>(it);
								auto v = ILBlock::read_data<uint32_t>(it);
								eval_offset(eval_ctx, ILSize((ILSizeType)t,v));
							} break;
							case ILInstruction::offset16: {
								auto t = ILBlock::read_data<uint8_t>(it);
								auto v = ILBlock::read_data<uint16_t>(it);
								eval_offset(eval_ctx, ILSize((ILSizeType)t,v));
							} break;
							case ILInstruction::offset8: {
								auto t = ILBlock::read_data<uint8_t>(it);
								auto v = ILBlock::read_data<uint8_t>(it);
								eval_offset(eval_ctx, ILSize((ILSizeType)t,v));
							} break;
							case ILInstruction::aoffset8: {
								auto size = ILBlock::read_data<uint8_t>(it);
								eval_aoffset(eval_ctx, size);
							} break;
							case ILInstruction::aoffset16: {
								auto size = ILBlock::read_data<uint16_t>(it);
								eval_aoffset(eval_ctx, size);
							} break;
							case ILInstruction::aoffset32: {
								auto size = ILBlock::read_data<uint32_t>(it);
								eval_aoffset(eval_ctx, size);
							} break;
							case ILInstruction::woffset8: {
								auto size = ILBlock::read_data<uint8_t>(it);
								eval_woffset(eval_ctx, size);
							} break;
							case ILInstruction::woffset16: {
								auto size = ILBlock::read_data<uint16_t>(it);
								eval_woffset(eval_ctx, size);
							} break;
							case ILInstruction::woffset32: {
								auto size = ILBlock::read_data<uint32_t>(it);
								eval_woffset(eval_ctx, size);
							} break;
							case ILInstruction::constref: {
								auto cid = ILBlock::read_data<uint32_t>(it);
								eval_constref(eval_ctx, cid);
							} break;
							case ILInstruction::staticref: {
								auto cid = ILBlock::read_data<uint32_t>(it);
								eval_staticref(eval_ctx, cid);
							} break;
							case ILInstruction::rtoffset: {
								eval_rtoffset(eval_ctx);
							} break;
							case ILInstruction::rtoffset2: {
								eval_rtoffset_rev(eval_ctx);
							} break;
							case ILInstruction::roffset32: {
								auto from_t = ILBlock::read_data<ILDataType>(it);
								auto to_t = ILBlock::read_data<ILDataType>(it); 
								auto t = ILBlock::read_data<uint8_t>(it);
								auto v = ILBlock::read_data<uint32_t>(it);
								eval_roffset(eval_ctx, from_t, to_t, ILSize((ILSizeType)t, v).eval(eval_ctx->parent, compiler_arch));
							} break;
								
							case ILInstruction::roffset16: {
								auto from_t = ILBlock::read_data<ILDataType>(it);
								auto to_t = ILBlock::read_data<ILDataType>(it); 
								auto t = ILBlock::read_data<uint8_t>(it);
								auto v = ILBlock::read_data<uint16_t>(it);
								eval_roffset(eval_ctx, from_t, to_t, ILSize((ILSizeType)t, v).eval(eval_ctx->parent, compiler_arch));
							} break;
								
							case ILInstruction::roffset8: {
								auto from_t = ILBlock::read_data<ILDataType>(it);
								auto to_t = ILBlock::read_data<ILDataType>(it); 
								auto t = ILBlock::read_data<uint8_t>(it);
								auto v = ILBlock::read_data<uint8_t>(it);
								eval_roffset(eval_ctx, from_t, to_t, ILSize((ILSizeType)t, v).eval(eval_ctx->parent, compiler_arch));
							} break;

							case ILInstruction::aroffset: {
								auto p = ILBlock::read_data<ILDataTypePair>(it);
								auto size = ILBlock::read_data<uint8_t>(it);
								eval_aroffset(eval_ctx, p.first(), p.second(), size);
							} break;

							case ILInstruction::wroffset: {
								auto p = ILBlock::read_data<ILDataTypePair>(it);
								auto size = ILBlock::read_data<uint8_t>(it);
								eval_wroffset(eval_ctx, p.first(), p.second(), size);
							} break;

							case ILInstruction::local8: {
								auto id = ILBlock::read_data<uint8_t>(it);
								auto& offset = bytecode_fun->calculated_local_offsets[id];
								eval_const_ptr(eval_ctx, local_base + offset);
							} break;
								
							case ILInstruction::local16: {
								auto id = ILBlock::read_data<uint16_t>(it);
								auto& offset = bytecode_fun->calculated_local_offsets[id];
								eval_const_ptr(eval_ctx, local_base + offset);
							} break;
								
							case ILInstruction::local32: {
								auto id = ILBlock::read_data<uint32_t>(it);
								auto& offset = bytecode_fun->calculated_local_offsets[id];
								eval_const_ptr(eval_ctx, local_base + offset);
							} break;

							case ILInstruction::table8offset8: {
								auto tid = ILBlock::read_data<uint8_t>(it);
								auto eid = ILBlock::read_data<uint8_t>(it);
								eval_tableoffset(eval_ctx, tid, eid);
							} break;
							case ILInstruction::table8offset16: {
								auto tid = ILBlock::read_data<uint8_t>(it);
								auto eid = ILBlock::read_data<uint16_t>(it);
								eval_tableoffset(eval_ctx, tid, eid);
							} break;
							case ILInstruction::table8offset32: {
								auto tid = ILBlock::read_data<uint8_t>(it);
								auto eid = ILBlock::read_data<uint32_t>(it);
								eval_tableoffset(eval_ctx, tid, eid);
							} break;
							case ILInstruction::table16offset8: {
								auto tid = ILBlock::read_data<uint16_t>(it);
								auto eid = ILBlock::read_data<uint8_t>(it);
								eval_tableoffset(eval_ctx, tid, eid);
							} break;
							case ILInstruction::table16offset16: {
								auto tid = ILBlock::read_data<uint16_t>(it);
								auto eid = ILBlock::read_data<uint16_t>(it);
								eval_tableoffset(eval_ctx, tid, eid);
							} break;
							case ILInstruction::table16offset32: {
								auto tid = ILBlock::read_data<uint16_t>(it);
								auto eid = ILBlock::read_data<uint32_t>(it);
								eval_tableoffset(eval_ctx, tid, eid);
							} break;
							case ILInstruction::table32offset8: {
								auto tid = ILBlock::read_data<uint32_t>(it);
								auto eid = ILBlock::read_data<uint8_t>(it);
								eval_tableoffset(eval_ctx, tid, eid);
							} break;
							case ILInstruction::table32offset16: {
								auto tid = ILBlock::read_data<uint32_t>(it);
								auto eid = ILBlock::read_data<uint16_t>(it);
								eval_tableoffset(eval_ctx, tid, eid);
							} break;
							case ILInstruction::table32offset32: {
								auto tid = ILBlock::read_data<uint32_t>(it);
								auto eid = ILBlock::read_data<uint32_t>(it);
								eval_tableoffset(eval_ctx, tid, eid);
							} break;

							case ILInstruction::table8roffset8: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								auto tid = ILBlock::read_data<uint8_t>(it);
								auto eid = ILBlock::read_data<uint8_t>(it);
								eval_tableroffset(eval_ctx, pair.first(), pair.second(), tid, eid);
							} break;
							case ILInstruction::table8roffset16: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								auto tid = ILBlock::read_data<uint8_t>(it);
								auto eid = ILBlock::read_data<uint16_t>(it);
								eval_tableroffset(eval_ctx, pair.first(), pair.second(), tid, eid);
							} break;
							case ILInstruction::table8roffset32: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								auto tid = ILBlock::read_data<uint8_t>(it);
								auto eid = ILBlock::read_data<uint32_t>(it);
								eval_tableroffset(eval_ctx, pair.first(), pair.second(), tid, eid);
							} break;
							case ILInstruction::table16roffset8: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								auto tid = ILBlock::read_data<uint16_t>(it);
								auto eid = ILBlock::read_data<uint8_t>(it);
								eval_tableroffset(eval_ctx, pair.first(), pair.second(), tid, eid);
							} break;
							case ILInstruction::table16roffset16: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								auto tid = ILBlock::read_data<uint16_t>(it);
								auto eid = ILBlock::read_data<uint16_t>(it);
								eval_tableroffset(eval_ctx, pair.first(), pair.second(), tid, eid);
							} break;
							case ILInstruction::table16roffset32: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								auto tid = ILBlock::read_data<uint16_t>(it);
								auto eid = ILBlock::read_data<uint32_t>(it);
								eval_tableroffset(eval_ctx, pair.first(), pair.second(), tid, eid);
							} break;
							case ILInstruction::table32roffset8: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								auto tid = ILBlock::read_data<uint32_t>(it);
								auto eid = ILBlock::read_data<uint8_t>(it);
								eval_tableroffset(eval_ctx, pair.first(), pair.second(), tid, eid);
							} break;
							case ILInstruction::table32roffset16: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								auto tid = ILBlock::read_data<uint32_t>(it);
								auto eid = ILBlock::read_data<uint16_t>(it);
								eval_tableroffset(eval_ctx, pair.first(), pair.second(), tid, eid);
							} break;
							case ILInstruction::table32roffset32: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								auto tid = ILBlock::read_data<uint32_t>(it);
								auto eid = ILBlock::read_data<uint32_t>(it);
								eval_tableroffset(eval_ctx, pair.first(), pair.second(), tid, eid);
							} break;

							case ILInstruction::debug: {
								auto file = ILBlock::read_data<uint16_t>(it);
								auto line = ILBlock::read_data<uint16_t>(it);
								eval_debug(eval_ctx, file, line);
							} break;

							case ILInstruction::load: {
								auto type = ILBlock::read_data<ILDataType>(it);
								eval_load(eval_ctx, type);
							} break;

							case ILInstruction::isnotzero: {
								auto type = ILBlock::read_data<ILDataType>(it);
								eval_isnotzero(eval_ctx, type);
							} break;

							case ILInstruction::negative: {
								auto type = ILBlock::read_data<ILDataType>(it);
								eval_negative(eval_ctx, type);
							} break;

							case ILInstruction::negate: {
								eval_negate(eval_ctx);
							} break;

							case ILInstruction::combinedw: {
								eval_combine_dword(eval_ctx);
							} break;
							case ILInstruction::highdw: {
								eval_high_word(eval_ctx);
							} break;
							case ILInstruction::splitdw: {
								eval_split_dword(eval_ctx);
							} break;

							case ILInstruction::forget: {
								auto type = ILBlock::read_data<ILDataType>(it);
								eval_forget(eval_ctx, type);
							} break;
							case ILInstruction::jmpz: {
								auto addressz = ILBlock::read_data<uint32_t>(it);
								auto addressnz = ILBlock::read_data<uint32_t>(it);

								auto z = eval_ctx->pop_register_value<uint8_t>();

								if (z) {
									block = block->parent->blocks_memory[addressnz].get();
								}
								else {
									block = block->parent->blocks_memory[addressz].get();
								}

								goto next_block;
							} break;

							case ILInstruction::u8: eval_const_u8(eval_ctx, ILBlock::read_data<uint8_t>(it)); break;
							case ILInstruction::i8: eval_const_i8(eval_ctx, ILBlock::read_data<int8_t>(it)); break;
							case ILInstruction::u16: eval_const_u16(eval_ctx, ILBlock::read_data<uint16_t>(it)); break;
							case ILInstruction::i16: eval_const_i16(eval_ctx, ILBlock::read_data<int16_t>(it)); break;
							case ILInstruction::u32: eval_const_u32(eval_ctx, ILBlock::read_data<uint32_t>(it)); break;
							case ILInstruction::i32: eval_const_i32(eval_ctx, ILBlock::read_data<int32_t>(it)); break;
							case ILInstruction::u64: eval_const_u64(eval_ctx, ILBlock::read_data<uint64_t>(it)); break;
							case ILInstruction::i64: eval_const_i64(eval_ctx, ILBlock::read_data<int64_t>(it)); break;
							case ILInstruction::f32: eval_const_f32(eval_ctx, ILBlock::read_data<float>(it)); break;
							case ILInstruction::f64: eval_const_f64(eval_ctx, ILBlock::read_data<double>(it)); break;
							case ILInstruction::word: eval_const_ptr(eval_ctx, ILBlock::read_data<void*>(it)); break;
							case ILInstruction::slice: {
								uint32_t cid = ILBlock::read_data<uint32_t>(it);
								uint64_t s = ILBlock::read_data<uint64_t>(it);
								eval_const_slice(eval_ctx, cid, s); 
							}break;
							case ILInstruction::size8: {
								auto t = ILBlock::read_data<ILSizeType>(it);
								auto v = ILBlock::read_data<uint8_t>(it);
								eval_const_size(eval_ctx, ILSize(t,v).eval(eval_ctx->parent, compiler_arch));
							} break;
							case ILInstruction::size16: {
								auto t = ILBlock::read_data<ILSizeType>(it);
								auto v = ILBlock::read_data<uint16_t>(it);
								eval_const_size(eval_ctx, ILSize(t,v).eval(eval_ctx->parent, compiler_arch));
							} break;
							case ILInstruction::size32: {
								auto t = ILBlock::read_data<ILSizeType>(it);
								auto v = ILBlock::read_data<uint32_t>(it);
								eval_const_size(eval_ctx, ILSize(t,v).eval(eval_ctx->parent, compiler_arch));
							} break;

						}
					}

				next_block:
					continue;
				}


				throw_il_wrong_data_flow_error();

			returned:
				eval_ctx->callstack_debug.pop_back();
				eval_ctx->stack_pointer = lstack_pointer;
				eval_ctx->stack_base_aligned = lstack_base_aligned;
				eval_ctx->stack_base = lstack_base;
				
			}
			else {
				eval_ctx->callstack_debug.push_back(std::make_tuple(eval_ctx->debug_line, eval_ctx->debug_file, "External code"));
				abi_dynamic_call(eval_ctx, std::get<0>(declaration), ptr, declaration);
				eval_ctx->callstack_debug.pop_back();
			}

		}
		else {
			throw_runtime_handler_exception(eval_ctx);
		}

	}


	void ILBuilder::eval_add(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		_il_evaluator_const_op<std::plus>(eval_ctx, t_l, t_r);
	}

	void ILBuilder::eval_sub(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		_il_evaluator_const_op<std::minus>(eval_ctx, t_l, t_r);
	}


	void ILBuilder::eval_div(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		_il_evaluator_const_op<std::divides>(eval_ctx, t_l, t_r);
	}


	void ILBuilder::eval_rem(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		_il_evaluator_const_op_binary<std::modulus>(eval_ctx, t_l, t_r);
	}


	void ILBuilder::eval_and(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		_il_evaluator_const_op_binary<std::bit_and>(eval_ctx, t_l, t_r);
	}


	void ILBuilder::eval_or(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		_il_evaluator_const_op_binary<std::bit_or>(eval_ctx, t_l, t_r);
	}


	void ILBuilder::eval_xor(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		_il_evaluator_const_op_binary<std::bit_xor>(eval_ctx, t_l, t_r);
	}


	void ILBuilder::eval_mul(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		_il_evaluator_const_op<std::multiplies>(eval_ctx, t_l, t_r);
	}


	void ILBuilder::eval_eq(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		_il_evaluator_const_op_bool<std::equal_to>(eval_ctx, t_l, t_r);
	}


	void ILBuilder::eval_ne(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		_il_evaluator_const_op_bool<std::not_equal_to>(eval_ctx, t_l, t_r);
	}


	void ILBuilder::eval_gt(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		_il_evaluator_const_op_bool<std::greater>(eval_ctx, t_l, t_r);
	}

	void ILBuilder::eval_ge(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		_il_evaluator_const_op_bool<std::greater_equal>(eval_ctx, t_l, t_r);
	}


	void ILBuilder::eval_lt(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		_il_evaluator_const_op_bool<std::less>(eval_ctx, t_l, t_r);
	}


	void ILBuilder::eval_le(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		_il_evaluator_const_op_bool<std::less_equal>(eval_ctx, t_l, t_r);
	}


	void ILBuilder::eval_cast(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		_il_evaluator_cast(eval_ctx, t_l, t_r);
	}
	

	void ILBuilder::eval_bitcast(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		ilsize_t storage = 0;
		eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(t_l), &storage);
		eval_ctx->write_register_value_indirect(eval_ctx->compile_time_register_size(t_r), &storage);
	}

	
}