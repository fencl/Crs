#include "IL.h"
#include "../Error.h"
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
			throw_interrupt_exception(eval,sigint_value);
		}
		else if (sigseg_value != INT_MIN){
			throw_segfault_exception(eval,sigseg_value);
		}
	}


	void ILEvaluator::write_register_value_indirect(size_t size, void* value) {
		if (register_stack_pointer - register_stack + size >= stack_size) { throw_runtime_exception(this, "Stack overflow"); }

		if (setjmp(sandbox) == 0) {
			memcpy(register_stack_pointer, value, size);
			//std::cout << "+" << size << "\n";
			register_stack_pointer += size;
		}
		else {
			throw_runtime_handler_exception(this);
		}
	}

	void ILBuilder::eval_const_ibool(ILEvaluator* eval_ctx, uint8_t   value) { eval_ctx->write_register_value_indirect(sizeof(uint8_t), (unsigned char*)&value); }
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
			case Corrosive::ILDataType::size:
				return (T)eval_ctx->pop_register_value<size_t>();
			case Corrosive::ILDataType::ptr:
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
			case ILDataType::ibool: {
				uint8_t lval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, l);
				ILBuilder::eval_const_ibool(eval_ctx, lval);
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
			case ILDataType::size: {
				size_t lval = _il_evaluator_value_pop_into<size_t>(eval_ctx, l);
				ILBuilder::eval_const_size(eval_ctx, lval);
			}break;
			case ILDataType::ptr: {
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
			case ILDataType::ibool: {
				uint8_t rval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, r);
				uint8_t lval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, l);
				op<uint8_t> o;
				ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval));
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
			case ILDataType::size: {
				size_t rval = _il_evaluator_value_pop_into<size_t>(eval_ctx, r);
				size_t lval = _il_evaluator_value_pop_into<size_t>(eval_ctx, l);
				op<size_t> o;
				ILBuilder::eval_const_size(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::ptr: {
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
			case ILDataType::ibool: {
				uint8_t rval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, r);
				uint8_t lval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, l);
				op<uint8_t> o;
				ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval));
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
			case ILDataType::size: {
				size_t rval = _il_evaluator_value_pop_into<size_t>(eval_ctx, r);
				size_t lval = _il_evaluator_value_pop_into<size_t>(eval_ctx, l);
				op<size_t> o;
				ILBuilder::eval_const_size(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::ptr: {
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
				ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::u8: {
				uint8_t rval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, r);
				uint8_t lval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, l);
				op<uint8_t> o;
				ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::ibool: {
				uint8_t rval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, r);
				uint8_t lval = _il_evaluator_value_pop_into<uint8_t>(eval_ctx, l);
				op<uint8_t> o;
				ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::i16: {
				int16_t rval = _il_evaluator_value_pop_into<int16_t>(eval_ctx, r);
				int16_t lval = _il_evaluator_value_pop_into<int16_t>(eval_ctx, l);
				op<int16_t> o;
				ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::u16: {
				uint16_t rval = _il_evaluator_value_pop_into<uint16_t>(eval_ctx, r);
				uint16_t lval = _il_evaluator_value_pop_into<uint16_t>(eval_ctx, l);
				op<uint16_t> o;
				ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::i32: {
				int32_t rval = _il_evaluator_value_pop_into<int32_t>(eval_ctx, r);
				int32_t lval = _il_evaluator_value_pop_into<int32_t>(eval_ctx, l);
				op<int32_t> o;
				ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::u32: {
				uint32_t rval = _il_evaluator_value_pop_into<uint32_t>(eval_ctx, r);
				uint32_t lval = _il_evaluator_value_pop_into<uint32_t>(eval_ctx, l);
				op<uint32_t> o;
				ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::i64: {
				int64_t rval = _il_evaluator_value_pop_into<int64_t>(eval_ctx, r);
				int64_t lval = _il_evaluator_value_pop_into<int64_t>(eval_ctx, l);
				op<int64_t> o;
				ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::u64: {
				uint64_t rval = _il_evaluator_value_pop_into<uint64_t>(eval_ctx, r);
				uint64_t lval = _il_evaluator_value_pop_into<uint64_t>(eval_ctx, l);
				op<uint64_t> o;
				ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::f32: {
				float rval = _il_evaluator_value_pop_into<float>(eval_ctx, r);
				float lval = _il_evaluator_value_pop_into<float>(eval_ctx, l);
				op<float> o;
				ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::f64: {
				double rval = _il_evaluator_value_pop_into<double>(eval_ctx, r);
				double lval = _il_evaluator_value_pop_into<double>(eval_ctx, l);
				op<double> o;
				ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::size: {
				size_t rval = _il_evaluator_value_pop_into<size_t>(eval_ctx, r);
				size_t lval = _il_evaluator_value_pop_into<size_t>(eval_ctx, l);
				op<size_t> o;
				ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval));
			}break;
			case ILDataType::ptr: {
				size_t rval = _il_evaluator_value_pop_into<size_t>(eval_ctx, r);
				size_t lval = _il_evaluator_value_pop_into<size_t>(eval_ctx, l);
				op<size_t> o;
				ILBuilder::eval_const_ibool(eval_ctx, o(lval, rval));
			}break;
		}

	}




	void ILEvaluator::pop_register_value_indirect(size_t size, void* into) {		
		if (setjmp(sandbox) == 0) {

			//std::cout << "-" << size << "\n";
			register_stack_pointer -= size;
			if (into!=nullptr)
				memcpy(into, register_stack_pointer, size);
		}
		else {
			
			throw_runtime_handler_exception(this);
		}
	}

	void ILBuilder::eval_forget(ILEvaluator* eval_ctx, ILDataType type) {
		eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(type), nullptr);
	}


	size_t ILEvaluator::compile_time_register_size(ILDataType t) {
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
				return 4;
			case Corrosive::ILDataType::f64:
				return 8;
			case Corrosive::ILDataType::ptr:
				return sizeof(void*);
			case Corrosive::ILDataType::size:
				return sizeof(size_t);
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
			mem += offset.eval(eval_ctx->parent,compiler_arch);
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

	void ILBuilder::eval_aoffset2(ILEvaluator* eval_ctx, uint32_t offset) {
		if (offset > 0) {
			auto mem2 = eval_ctx->pop_register_value<size_t>();
			auto mem1 = eval_ctx->pop_register_value<size_t>();
			mem1 += offset;
			mem2 += offset;
			eval_ctx->write_register_value(mem1);
			eval_ctx->write_register_value(mem2);
		}
	}
	
	void ILBuilder::eval_woffset(ILEvaluator* eval_ctx, uint32_t offset) {
		if (offset > 0) {
			auto mem = eval_ctx->pop_register_value<size_t>();
			mem += offset * eval_ctx->compile_time_register_size(ILDataType::ptr);
			eval_ctx->write_register_value(mem);
		}
	}
	

	void ILBuilder::eval_woffset2(ILEvaluator* eval_ctx, uint32_t offset) {
		if (offset > 0) {
			auto mem2 = eval_ctx->pop_register_value<size_t>();
			auto mem1 = eval_ctx->pop_register_value<size_t>();
			mem2 += offset * eval_ctx->compile_time_register_size(ILDataType::ptr);
			mem1 += offset * eval_ctx->compile_time_register_size(ILDataType::ptr);
			eval_ctx->write_register_value(mem1);
			eval_ctx->write_register_value(mem2);
		}
	}


	void ILBuilder::eval_constref(ILEvaluator* eval_ctx, uint32_t cid) {
		eval_ctx->write_register_value(eval_ctx->parent->constant_memory[cid].get());
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
	
	void ILBuilder::eval_rtoffset2(ILEvaluator* eval_ctx) {
		auto mem = eval_ctx->pop_register_value<unsigned char*>();
		auto offset = eval_ctx->pop_register_value<size_t>();
		mem += offset;
		eval_ctx->write_register_value(mem);
	}

	void ILBuilder::eval_roffset(ILEvaluator* eval_ctx, ILDataType from,ILDataType to, size_t offset) {
		ilsize_t mem;
		eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(from), &mem);
		mem = mem << offset;
		return eval_ctx->write_register_value_indirect(eval_ctx->compile_time_register_size(to), &mem);
	}
	
	void ILBuilder::eval_aroffset(ILEvaluator* eval_ctx, ILDataType from,ILDataType to, uint8_t offset) {
		ilsize_t mem;
		eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(from), &mem);
		mem = mem << offset;
		return eval_ctx->write_register_value_indirect(eval_ctx->compile_time_register_size(to), &mem);
	}
	
	void ILBuilder::eval_wroffset(ILEvaluator* eval_ctx, ILDataType from,ILDataType to, uint8_t offset) {
		ilsize_t mem;
		eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(from), &mem);
		mem = mem << (offset * eval_ctx->compile_time_register_size(ILDataType::ptr));
		return eval_ctx->write_register_value_indirect(eval_ctx->compile_time_register_size(to), &mem);
	}


	void ILBuilder::eval_store(ILEvaluator* eval_ctx, ILDataType type) {
		auto mem = eval_ctx->pop_register_value<void*>();
		eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(type), mem);
	}
	
	void ILBuilder::eval_store2(ILEvaluator* eval_ctx, ILDataType type) {
		
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

	void ILBuilder::eval_local(ILEvaluator* eval_ctx, uint16_t id) {
		eval_const_ptr(eval_ctx, eval_ctx->stack_ptr(id));
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
	void ILBuilder::eval_duplicate2(ILEvaluator* eval_ctx, ILDataType type) {
		size_t reg_s = eval_ctx->compile_time_register_size(type);

		ilsize_t storage1, storage2;
		eval_ctx->pop_register_value_indirect(reg_s, &storage1);
		eval_ctx->pop_register_value_indirect(reg_s, &storage2);

		eval_ctx->write_register_value_indirect(reg_s, &storage1);
		eval_ctx->write_register_value_indirect(reg_s, &storage2);
		eval_ctx->write_register_value_indirect(reg_s, &storage1);
		eval_ctx->write_register_value_indirect(reg_s, &storage2);
	}

	void ILBuilder::eval_clone2(ILEvaluator* eval_ctx, ILDataType type, uint16_t times) {
		size_t reg_s = eval_ctx->compile_time_register_size(type);

		ilsize_t storage1, storage2;
		eval_ctx->pop_register_value_indirect(reg_s, &storage1);
		eval_ctx->pop_register_value_indirect(reg_s, &storage2);

		for (uint16_t i = 0; i < times; ++i) {
			eval_ctx->write_register_value_indirect(reg_s, &storage1);
			eval_ctx->write_register_value_indirect(reg_s, &storage2);
		}
	}

	void ILBuilder::eval_fnptr(ILEvaluator* eval_ctx, ILFunction* fun) {
		eval_ctx->write_register_value(fun);
	}
	
	void ILBuilder::eval_null(ILEvaluator* eval_ctx) {
		eval_ctx->write_register_value((size_t)0);
	}
	
	void ILBuilder::eval_isnotzero(ILEvaluator* eval_ctx, ILDataType type) {
		ilsize_t z = 0;
		eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(type), &z);
		eval_const_ibool(eval_ctx, (z == 0?0:1));
	}


	void ILBuilder::eval_callstart(ILEvaluator* eval_ctx) {
		auto func = eval_ctx->pop_register_value<ILFunction*>();
		eval_ctx->callstack.push_back(func);
	}

	void ILBuilder::eval_insintric(ILEvaluator* eval_ctx, ILInsintric fun) {
		eval_ctx->parent->insintric_function[(unsigned char)fun](eval_ctx);
	}

	void ILBuilder::eval_vtable(ILEvaluator* eval_ctx, uint32_t id) {
		return eval_ctx->write_register_value(eval_ctx->parent->vtable_data[id].get());
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

	void ILBuilder::eval_malloc(ILEvaluator* eval_ctx) {
		
		if (setjmp(sandbox) == 0) {
			auto size = eval_ctx->pop_register_value<size_t>();
			void* mlc = malloc(size);
			if (mlc == nullptr) {
				throw_runtime_exception(eval_ctx, "Failed to allocate memory");
			}
			eval_const_ptr(eval_ctx, mlc);
			
		}
		else {
			
			throw_runtime_handler_exception(eval_ctx);
		}
	}

	void ILBuilder::eval_free(ILEvaluator* eval_ctx) {
		
		if (setjmp(sandbox) == 0) {
			auto ptr = eval_ctx->pop_register_value<void*>();
			free(ptr);
			
		}
		else {
			
			throw_runtime_handler_exception(eval_ctx);
		}
	}
	
	void ILBuilder::eval_memcpy2(ILEvaluator* eval_ctx, size_t size) {
		
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

	void ILBuilder::eval_memcmp2(ILEvaluator* eval_ctx, size_t size) {
		
		if (setjmp(sandbox) == 0) {
			auto src = eval_ctx->pop_register_value<void*>();
			auto dst = eval_ctx->pop_register_value<void*>();
			eval_ctx->write_register_value((int8_t)memcmp(dst, src, size));
			
		}
		else {
			
			throw_runtime_handler_exception(eval_ctx);
		}
	}

	void ILBuilder::eval_tableoffset(ILEvaluator* eval_ctx, uint32_t tableid, uint16_t itemid) {
		auto& table = eval_ctx->parent->structure_tables[tableid];
		table.calculate(eval_ctx->parent, compiler_arch);
		auto ptr = eval_ctx->pop_register_value<unsigned char*>();
		ptr += table.calculated_offsets[itemid];
		eval_ctx->write_register_value(ptr);
	}

	void ILBuilder::eval_tableoffset2(ILEvaluator* eval_ctx, uint32_t tableid, uint16_t itemid) {
		auto& table = eval_ctx->parent->structure_tables[tableid];
		table.calculate(eval_ctx->parent, compiler_arch);
		auto ptr2 = eval_ctx->pop_register_value<unsigned char*>();
		auto ptr1 = eval_ctx->pop_register_value<unsigned char*>();
		ptr1 += table.calculated_offsets[itemid];
		ptr2 += table.calculated_offsets[itemid];
		eval_ctx->write_register_value(ptr1);
		eval_ctx->write_register_value(ptr2);
	}
	
	void ILBuilder::eval_tableroffset(ILEvaluator* eval_ctx, ILDataType src, ILDataType dst ,uint32_t tableid, uint16_t itemid) {
		auto& table = eval_ctx->parent->structure_tables[tableid];
		table.calculate(eval_ctx->parent, compiler_arch);
		ilsize_t storage;
		eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(src),&storage);
		storage >>= table.calculated_offsets[itemid];
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

	void ILBuilder::eval_rmemcmp2(ILEvaluator* eval_ctx, ILDataType type) {
		
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
			case ILDataType::ibool:
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
			case ILDataType::ptr:
			case ILDataType::size:
				eval_ctx->write_register_value((SIZE_MAX^eval_ctx->pop_register_value<size_t>())-1);
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

#define read_data_type(T) ((T*)block->read_data(sizeof(T),mempool,memoff))
#define read_data_size(S) (block->read_data((S),mempool,memoff))
	void ILBuilder::eval_call(ILEvaluator* eval_ctx, ILDataType rett, uint16_t argc) {
		
		if (setjmp(sandbox) == 0) {
			ILFunction* fun = eval_ctx->callstack.back();
			eval_ctx->callstack.pop_back();
			eval_ctx->callstack_debug.push_back(std::make_tuple(eval_ctx->debug_line, eval_ctx->debug_file,std::string_view(fun->alias)));

			fun->calculate_stack(compiler_arch);

			ILBlock* block = fun->blocks[0];
			bool running = true;

			eval_ctx->stack_push(fun->calculated_local_stack_alignment);
			eval_ctx->local_stack_size.back() = fun->calculated_local_stack_size;
			unsigned char* lstack_base = eval_ctx->local_stack_base.back();

			size_t instr = 0;

			while (true) {
				std::list<std::unique_ptr<ILBlockData>>::iterator mempool = block->data_pool.begin();
				size_t memoff = 0;
				while (mempool != block->data_pool.end()) {

					auto inst = read_data_type(ILInstruction);
					instr++;

					switch (*inst) {
						case ILInstruction::ret: {
							auto type = *read_data_type(ILDataType);
							running = false;
							goto returned;
						} break;
						case ILInstruction::call: {
							auto type = *read_data_type(ILDataType);
							auto argc = *read_data_type(uint16_t);
							eval_call(eval_ctx, type, argc);
						} break;
						case ILInstruction::memcpy: {
							auto size = read_data_type(ILSize);
							eval_memcpy(eval_ctx, size->eval(eval_ctx->parent, compiler_arch));
						} break;
						case ILInstruction::memcpy2: {
							auto size = read_data_type(ILSize);
							eval_memcpy2(eval_ctx, size->eval(eval_ctx->parent, compiler_arch));
						} break;
						case ILInstruction::memcmp: {
							auto size = read_data_type(ILSize);
							eval_memcmp(eval_ctx, size->eval(eval_ctx->parent, compiler_arch));
						} break;
						case ILInstruction::memcmp2: {
							auto size = read_data_type(ILSize);
							eval_memcmp2(eval_ctx, size->eval(eval_ctx->parent, compiler_arch));
						} break;
						case ILInstruction::fnptr: {
							auto id = *read_data_type(uint32_t);
							eval_ctx->write_register_value(eval_ctx->parent->functions[id].get());
						} break;
						case ILInstruction::vtable: {
							auto id = *read_data_type(uint32_t);
							eval_vtable(eval_ctx, id);
						} break;
						case ILInstruction::duplicate: {
							auto type = *read_data_type(ILDataType);
							eval_duplicate(eval_ctx, type);
						} break;
						case ILInstruction::clone: {
							auto type = *read_data_type(ILDataType);
							auto times = *read_data_type(uint16_t);
							eval_clone(eval_ctx, type, times);
						} break;
						case ILInstruction::duplicate2: {
							auto type = *read_data_type(ILDataType);
							eval_duplicate2(eval_ctx, type);
						} break;
						case ILInstruction::clone2: {
							auto type = *read_data_type(ILDataType);
							auto times = *read_data_type(uint16_t);
							eval_clone2(eval_ctx, type, times);
						} break;
						case ILInstruction::swap: {
							auto type = *read_data_type(ILDataType);
							eval_swap(eval_ctx, type);
						} break;
						case ILInstruction::swap2: {
							auto type1 = *read_data_type(ILDataType);
							auto type2 = *read_data_type(ILDataType);
							eval_swap2(eval_ctx, type1, type2);
						} break;
						case ILInstruction::insintric: {
							auto id = *read_data_type(uint8_t);
							eval_insintric(eval_ctx, (ILInsintric)id);
						} break;
						case ILInstruction::rmemcmp: {
							auto type = *read_data_type(ILDataType);
							eval_rmemcmp(eval_ctx, type);
						} break;
						case ILInstruction::rmemcmp2: {
							auto type = *read_data_type(ILDataType);
							eval_rmemcmp2(eval_ctx, type);
						} break;
						case ILInstruction::sub: {
							auto left = *read_data_type(ILDataType);
							auto right = *read_data_type(ILDataType);
							eval_sub(eval_ctx, left, right);
						} break;
						case ILInstruction::div: {
							auto left = read_data_type(ILDataType);
							auto right = read_data_type(ILDataType);
							eval_div(eval_ctx, *left, *right);
						} break;
						case ILInstruction::rem: {
							auto left = read_data_type(ILDataType);
							auto right = read_data_type(ILDataType);
							eval_rem(eval_ctx, *left, *right);
						} break;
						case ILInstruction::mul: {
							auto left = read_data_type(ILDataType);
							auto right = read_data_type(ILDataType);
							eval_mul(eval_ctx, *left, *right);
						} break;
						case ILInstruction::add: {
							auto left = read_data_type(ILDataType);
							auto right = read_data_type(ILDataType);
							eval_add(eval_ctx, *left, *right);
						} break;
						case ILInstruction::bit_and: {
							auto left = read_data_type(ILDataType);
							auto right = read_data_type(ILDataType);
							eval_and(eval_ctx, *left, *right);
						} break;
						case ILInstruction::bit_or: {
							auto left = read_data_type(ILDataType);
							auto right = read_data_type(ILDataType);
							eval_or(eval_ctx, *left, *right);
						} break;
						case ILInstruction::bit_xor: {
							auto left = read_data_type(ILDataType);
							auto right = read_data_type(ILDataType);
							eval_xor(eval_ctx, *left, *right);
						} break;
						case ILInstruction::eq: {
							auto left = read_data_type(ILDataType);
							auto right = read_data_type(ILDataType);
							eval_eq(eval_ctx, *left, *right);
						} break;
						case ILInstruction::ne: {
							auto left = read_data_type(ILDataType);
							auto right = read_data_type(ILDataType);
							eval_ne(eval_ctx, *left, *right);
						} break;
						case ILInstruction::gt: {
							auto left = read_data_type(ILDataType);
							auto right = read_data_type(ILDataType);
							eval_gt(eval_ctx, *left, *right);
						} break;
						case ILInstruction::lt: {
							auto left = read_data_type(ILDataType);
							auto right = read_data_type(ILDataType);
							eval_lt(eval_ctx, *left, *right);
						} break;
						case ILInstruction::ge: {
							auto left = read_data_type(ILDataType);
							auto right = read_data_type(ILDataType);
							eval_ge(eval_ctx, *left, *right);
						} break;
						case ILInstruction::le: {
							auto left = read_data_type(ILDataType);
							auto right = read_data_type(ILDataType);
							eval_le(eval_ctx, *left, *right);
						} break;
						case ILInstruction::cast: {
							auto left = read_data_type(ILDataType);
							auto right = read_data_type(ILDataType);
							eval_cast(eval_ctx, *left, *right);
						} break;
						case ILInstruction::store: {
							auto type = read_data_type(ILDataType);
							eval_store(eval_ctx, *type);
						} break;
						case ILInstruction::store2: {
							auto type = read_data_type(ILDataType);
							eval_store2(eval_ctx, *type);
						} break;
						case ILInstruction::yield: {
							auto type = read_data_type(ILDataType);
							eval_yield(eval_ctx, *type);
						} break;
						case ILInstruction::accept: {
							auto type = read_data_type(ILDataType);
							eval_accept(eval_ctx, *type);
						} break;
						case ILInstruction::discard: {
							auto type = read_data_type(ILDataType);
							eval_discard(eval_ctx, *type);
						} break;
						case ILInstruction::start: {
							eval_callstart(eval_ctx);
						} break;

						case ILInstruction::null: {
							eval_null(eval_ctx);
						} break;

						case ILInstruction::jmp: {
							auto address = read_data_type(uint32_t);
							block = block->parent->blocks_memory[*address].get();
							goto next_block;
						}break;
						case ILInstruction::offset: {
							auto size = *read_data_type(ILSize);
							eval_offset(eval_ctx, size);
						} break;
						case ILInstruction::aoffset: {
							auto size = *read_data_type(uint32_t);
							eval_aoffset(eval_ctx, size);
						} break;
						case ILInstruction::woffset: {
							auto size = *read_data_type(uint32_t);
							eval_woffset(eval_ctx, size);
						} break;
						case ILInstruction::aoffset2: {
							auto size = *read_data_type(uint32_t);
							eval_aoffset2(eval_ctx, size);
						} break;
						case ILInstruction::woffset2: {
							auto size = *read_data_type(uint32_t);
							eval_woffset2(eval_ctx, size);
						} break;
						case ILInstruction::constref: {
							auto cid = *read_data_type(uint32_t);
							eval_constref(eval_ctx, cid);
						} break;
						case ILInstruction::rtoffset: {
							eval_rtoffset(eval_ctx);
						} break;
						case ILInstruction::rtoffset2: {
							eval_rtoffset2(eval_ctx);
						} break;
						case ILInstruction::roffset: {
							auto from_t = *read_data_type(ILDataType);
							auto to_t = *read_data_type(ILDataType);
							auto size = read_data_type(ILSize);
							eval_roffset(eval_ctx, from_t, to_t, size->eval(eval_ctx->parent,compiler_arch));
						} break;

						case ILInstruction::aroffset: {
							auto from_t = *read_data_type(ILDataType);
							auto to_t = *read_data_type(ILDataType);
							auto size = *read_data_type(uint8_t);
							eval_aroffset(eval_ctx, from_t, to_t, size);
						} break;

						case ILInstruction::wroffset: {
							auto from_t = *read_data_type(ILDataType);
							auto to_t = *read_data_type(ILDataType);
							auto size = *read_data_type(uint8_t);
							eval_wroffset(eval_ctx, from_t, to_t, size);
						} break;

						case ILInstruction::local: {
							auto id = read_data_type(uint16_t);
							auto& offset = fun->calculated_local_offsets[*id];
							eval_const_ptr(eval_ctx, lstack_base + offset);
						} break;

						case ILInstruction::tableoffset: {
							auto tableid = *read_data_type(uint32_t);
							auto id = *read_data_type(uint16_t);
							eval_tableoffset(eval_ctx, tableid,id);
						} break;
						case ILInstruction::tableoffset2: {
							auto tableid = *read_data_type(uint32_t);
							auto id = *read_data_type(uint16_t);
							eval_tableoffset2(eval_ctx, tableid,id);
						} break;

						case ILInstruction::tableroffset: {
							auto from_t = *read_data_type(ILDataType);
							auto to_t = *read_data_type(ILDataType);
							auto tableid = *read_data_type(uint32_t);
							auto id = *read_data_type(uint16_t);
							eval_tableroffset(eval_ctx,from_t,to_t, tableid,id);
						} break;
							
						case ILInstruction::debug: {
							auto file = *read_data_type(uint16_t);
							auto line = *read_data_type(uint16_t);
							eval_debug(eval_ctx, file,line);
						} break;

						case ILInstruction::load: {
							auto type = read_data_type(ILDataType);
							eval_load(eval_ctx, *type);
						} break;

						case ILInstruction::malloc: {
							eval_malloc(eval_ctx);
						} break;

						case ILInstruction::free: {
							eval_free(eval_ctx);
						} break;

						case ILInstruction::isnotzero: {
							auto type = read_data_type(ILDataType);
							eval_isnotzero(eval_ctx, *type);
						} break;

						case ILInstruction::negative: {
							auto type = read_data_type(ILDataType);
							eval_negative(eval_ctx, *type);
						} break;

						case ILInstruction::negate: {
							eval_negate(eval_ctx);
						} break;

						case ILInstruction::forget: {
							auto type = read_data_type(ILDataType);
							eval_forget(eval_ctx, *type);
						} break;
						case ILInstruction::jmpz: {
							auto addressz = read_data_type(uint32_t);
							auto addressnz = read_data_type(uint32_t);

							auto z = eval_ctx->pop_register_value<uint8_t>();

							if (z) {
								block = block->parent->blocks_memory[*addressnz].get();
							}
							else {
								block = block->parent->blocks_memory[*addressz].get();
							}

							goto next_block;
						} break;
						case ILInstruction::value: {

							auto type = read_data_type(ILDataType);

							switch (*type) {
								case ILDataType::ibool:  eval_const_ibool(eval_ctx, *read_data_type(int8_t)); break;
								case ILDataType::u8:     eval_const_u8(eval_ctx, *read_data_type(uint8_t)); break;
								case ILDataType::u16:    eval_const_u16(eval_ctx, *read_data_type(uint16_t)); break;
								case ILDataType::u32:    eval_const_u32(eval_ctx, *read_data_type(uint32_t)); break;
								case ILDataType::u64:    eval_const_u64(eval_ctx, *read_data_type(uint64_t)); break;
								case ILDataType::i8:     eval_const_i8(eval_ctx, *read_data_type(int8_t)); break;
								case ILDataType::i16:    eval_const_i16(eval_ctx, *read_data_type(int16_t)); break;
								case ILDataType::i32:    eval_const_i32(eval_ctx, *read_data_type(int32_t)); break;
								case ILDataType::i64:    eval_const_i64(eval_ctx, *read_data_type(int64_t)); break;
								case ILDataType::f32:    eval_const_f32(eval_ctx, *read_data_type(float)); break;
								case ILDataType::f64:    eval_const_f64(eval_ctx, *read_data_type(double)); break;
								case ILDataType::ptr:    eval_const_ptr(eval_ctx, *read_data_type(void*)); break;
								case ILDataType::size:   eval_const_size(eval_ctx, read_data_type(ILSize)->eval(eval_ctx->parent, compiler_arch)); break;
							}
						} break;
					}
				}

			next_block:
				continue;
			}


			
			throw_il_wrong_data_flow_error();
		returned:

			eval_ctx->callstack_debug.pop_back();
			eval_ctx->stack_pop();
		}
		else {
			
			throw_runtime_handler_exception(eval_ctx);
		}
	}

#undef read_data_type
#undef read_data_size

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


}