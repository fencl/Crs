#include "IL.hpp"
#include "../Error.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <cstring>
#include <csignal>
#include <climits>

namespace Corrosive {

	thread_local err_handler eval_extern_err = err_handler(err_undef_tag());
	void ILEvaluator::ex_throw() { eval_extern_err = false; }

	int sigint_value = INT_MIN;
	int sigseg_value = INT_MIN;
	bool sandbox_started = false;

	void sandbox_siginthandler(int signum) noexcept {
		sigseg_value = INT_MIN;
		sigint_value = signum;
		longjmp_func(sandbox, 1);
	}

	void sandbox_sigseghandler(int signum) noexcept {
		sigseg_value = signum;
		sigint_value = INT_MIN;
		longjmp_func(sandbox, 1);
	}

	bool ILEvaluator::sandboxed() {
		return sandbox_started;
	}

	void ILEvaluator::sandbox_begin() {
		sandbox_started = true;
		build_sandbox();
		signal(SIGINT, sandbox_siginthandler);
		signal(SIGSEGV, sandbox_sigseghandler);
		signal(SIGILL, sandbox_siginthandler);
		signal(SIGFPE, sandbox_siginthandler);
	}

	void ILEvaluator::sandbox_end() {
		sandbox_started = false;
		signal(SIGINT, SIG_DFL);
		signal(SIGSEGV, SIG_DFL);
		signal(SIGILL, SIG_DFL);
		signal(SIGFPE, SIG_DFL);
	}

	errvoid throw_runtime_handler_exception(const ILEvaluator* eval) {
		if (sigint_value != INT_MIN) {
			return throw_interrupt_exception(eval, sigint_value);
		}
		else if (sigseg_value != INT_MIN) {
			return throw_segfault_exception(eval, sigseg_value);
		}

		return errvoid();
	}


	errvoid ILEvaluator::write_register_value_indirect(std::size_t size, void* value) {
		if (!wrap || wrap(sandbox) == 0) {
			switch (size) {
				case 0: break;
				case 1:
					std::memcpy(register_stack_pointer_1b++, value, size); break;
				case 2:
					std::memcpy(register_stack_pointer_2b++, value, size); break;
				case 3:
				case 4:
					std::memcpy(register_stack_pointer_4b++, value, size); break;
				case 5:
				case 6:
				case 7:
				case 8:
					std::memcpy(register_stack_pointer_8b++, value, size); break;
				default:
					std::memcpy(register_stack_pointer_8b++, value, size); register_stack_pointer_8b++; break;
			}
		}
		else {
			return throw_runtime_handler_exception(this);
		}

		return errvoid();
	}

	errvoid ILEvaluator::pop_register_value_indirect(std::size_t size, void* into) {
		if (!wrap || wrap(sandbox) == 0) {

			switch (size) {
				case 0: break;
				case 1:
					std::memcpy(into,--register_stack_pointer_1b, size); break;
				case 2:
					std::memcpy(into, --register_stack_pointer_2b, size); break;
				case 3:
				case 4:
					std::memcpy(into, --register_stack_pointer_4b, size); break;
				case 5:
				case 6:
				case 7:
				case 8:
					std::memcpy(into, --register_stack_pointer_8b, size); break;
				default:
					--register_stack_pointer_8b;
					std::memcpy(into, --register_stack_pointer_8b, size); break;
			}
		} else {
			return throw_runtime_handler_exception(this);
		}

		return errvoid();
	}


	errvoid ILBuilder::eval_const_i8(ILEvaluator* eval_ctx, std::int8_t   value) { if (!eval_ctx->write_register_value_indirect(sizeof(std::int8_t), (unsigned char*)&value)) return pass(); return errvoid(); }
	errvoid ILBuilder::eval_const_i16(ILEvaluator* eval_ctx, std::int16_t  value) { if (!eval_ctx->write_register_value_indirect(sizeof(std::int16_t), (unsigned char*)&value)) return pass(); return errvoid(); }
	errvoid ILBuilder::eval_const_i32(ILEvaluator* eval_ctx, std::int32_t  value) { 
		if (!eval_ctx->write_register_value_indirect(sizeof(std::int32_t), (unsigned char*)&value)) return pass(); 
		return errvoid(); 
	}
	errvoid ILBuilder::eval_const_i64(ILEvaluator* eval_ctx, std::int64_t  value) { if (!eval_ctx->write_register_value_indirect(sizeof(std::int64_t), (unsigned char*)&value)) return pass(); return errvoid(); }
	errvoid ILBuilder::eval_const_u8(ILEvaluator* eval_ctx, std::uint8_t  value) { if (!eval_ctx->write_register_value_indirect(sizeof(std::uint8_t), (unsigned char*)&value)) return pass(); return errvoid(); }
	errvoid ILBuilder::eval_const_u16(ILEvaluator* eval_ctx, std::uint16_t value) { if (!eval_ctx->write_register_value_indirect(sizeof(std::uint16_t), (unsigned char*)&value)) return pass(); return errvoid(); }
	errvoid ILBuilder::eval_const_u32(ILEvaluator* eval_ctx, std::uint32_t value) { if (!eval_ctx->write_register_value_indirect(sizeof(std::uint32_t), (unsigned char*)&value)) return pass(); return errvoid(); }
	errvoid ILBuilder::eval_const_u64(ILEvaluator* eval_ctx, std::uint64_t value) { if (!eval_ctx->write_register_value_indirect(sizeof(std::uint64_t), (unsigned char*)&value)) return pass(); return errvoid(); }
	errvoid ILBuilder::eval_const_f32(ILEvaluator* eval_ctx, float    value) { if (!eval_ctx->write_register_value_indirect(sizeof(float), (unsigned char*)&value)) return pass(); return errvoid(); }
	errvoid ILBuilder::eval_const_f64(ILEvaluator* eval_ctx, double   value) { if (!eval_ctx->write_register_value_indirect(sizeof(double), (unsigned char*)&value)) return pass(); return errvoid(); }
	errvoid ILBuilder::eval_const_word(ILEvaluator* eval_ctx, void* value) { if (!eval_ctx->write_register_value_indirect(sizeof(void*), (unsigned char*)&value)) return pass(); return errvoid(); }
	errvoid ILBuilder::eval_const_dword(ILEvaluator* eval_ctx, dword_t value) { if (!eval_ctx->write_register_value_indirect(sizeof(dword_t), (unsigned char*)&value)) return pass(); return errvoid(); }
	errvoid ILBuilder::eval_const_size(ILEvaluator* eval_ctx, std::size_t value) { if (!eval_ctx->write_register_value_indirect(sizeof(std::size_t), (unsigned char*)&value)) return pass(); return errvoid(); }


	template<typename T> inline errvoid _il_evaluator_value_pop_into(T& r, ILEvaluator* eval_ctx, ILDataType t) {
		switch (t)
		{
			case Corrosive::ILDataType::u8: {
				std::uint8_t v;
				if (!eval_ctx->pop_register_value<std::uint8_t>(v)) return pass();
				r = (T)v;
			} break;
			case Corrosive::ILDataType::i8: {
				std::int8_t v;
				if (!eval_ctx->pop_register_value<std::int8_t>(v)) return pass();
				r = (T)v;
			} break;

			case Corrosive::ILDataType::u16: {
				std::uint16_t v;
				if (!eval_ctx->pop_register_value<std::uint16_t>(v)) return pass();
				r = (T)v;
			} break;
			case Corrosive::ILDataType::i16: {
				std::int16_t v;
				if (!eval_ctx->pop_register_value<std::int16_t>(v)) return pass();
				r = (T)v;
			} break;

			case Corrosive::ILDataType::u32: {
				std::uint32_t v;
				if (!eval_ctx->pop_register_value<std::uint32_t>(v)) return pass();
				r = (T)v;
			} break;
			case Corrosive::ILDataType::i32: {
				std::int32_t v;
				if (!eval_ctx->pop_register_value<std::int32_t>(v)) return pass();
				r = (T)v;
			} break;

			case Corrosive::ILDataType::u64: {
				std::uint64_t v;
				if (!eval_ctx->pop_register_value<std::uint64_t>(v)) return pass();
				r = (T)v;
			} break;
			case Corrosive::ILDataType::i64: {
				std::int64_t v;
				if (!eval_ctx->pop_register_value<std::int64_t>(v)) return pass();
				r = (T)v;
			} break;
			
			case Corrosive::ILDataType::f32: {
				float v;
				if (!eval_ctx->pop_register_value<float>(v)) return pass();
				r = (T)v;
			} break;
			case Corrosive::ILDataType::f64: {
				double v;
				if (!eval_ctx->pop_register_value<double>(v)) return pass();
				r = (T)v;
			} break;
			case Corrosive::ILDataType::word: {
				std::size_t v;
				if (!eval_ctx->pop_register_value<std::size_t>(v)) return pass();
				r = (T)v;
			} break;

			default: break;
		}

		return errvoid();
	}

	errvoid _il_evaluator_cast(ILEvaluator* eval_ctx, ILDataType l, ILDataType res_t) {
		switch (res_t) {
			case ILDataType::i8: {
				std::int8_t lval;
				if (!_il_evaluator_value_pop_into<std::int8_t>(lval, eval_ctx, l)) return pass();;
				if (!ILBuilder::eval_const_i8(eval_ctx, lval)) return pass();
			}break;
			case ILDataType::u8: {
				std::uint8_t lval;
				if(!_il_evaluator_value_pop_into<std::uint8_t>(lval, eval_ctx, l)) return pass();
				if (!ILBuilder::eval_const_u8(eval_ctx, lval)) return pass();
			}break;
			case ILDataType::i16: {
				std::int16_t lval;
				if(!_il_evaluator_value_pop_into<std::int16_t>(lval, eval_ctx, l)) return pass();
				if (!ILBuilder::eval_const_i16(eval_ctx, lval)) return pass();
			}break;
			case ILDataType::u16: {
				std::uint16_t lval;
				if(!_il_evaluator_value_pop_into<std::uint16_t>(lval, eval_ctx, l)) return pass();
				if (!ILBuilder::eval_const_u16(eval_ctx, lval)) return pass();
			}break;
			case ILDataType::i32: {
				std::int32_t lval;
				if(!_il_evaluator_value_pop_into<std::int32_t>(lval, eval_ctx, l)) return pass();
				if (!ILBuilder::eval_const_i32(eval_ctx, lval)) return pass();
			}break;
			case ILDataType::u32: {
				std::uint32_t lval;
				if(!_il_evaluator_value_pop_into<std::uint32_t>(lval, eval_ctx, l)) return pass();
				if (!ILBuilder::eval_const_u32(eval_ctx, lval)) return pass();
			}break;
			case ILDataType::i64: {
				std::int64_t lval;
				if(!_il_evaluator_value_pop_into<std::int64_t>(lval, eval_ctx, l)) return pass();
				if (!ILBuilder::eval_const_i64(eval_ctx, lval)) return pass();
			}break;
			case ILDataType::u64: {
				std::uint64_t lval;
				if(!_il_evaluator_value_pop_into<std::uint64_t>(lval, eval_ctx, l)) return pass();
				if (!ILBuilder::eval_const_u64(eval_ctx, lval)) return pass();
			}break;
			case ILDataType::f32: {
				float lval;
				if(!_il_evaluator_value_pop_into<float>(lval, eval_ctx, l)) return pass();
				if (!ILBuilder::eval_const_f32(eval_ctx, lval)) return pass();
			}break;
			case ILDataType::f64: {
				double lval;
				if(!_il_evaluator_value_pop_into<double>(lval, eval_ctx, l)) return pass();
				if (!ILBuilder::eval_const_f64(eval_ctx, lval)) return pass();
			}break;
			case ILDataType::word: {
				std::size_t lval;
				if(!_il_evaluator_value_pop_into<std::size_t>(lval, eval_ctx, l)) return pass();
				if (!ILBuilder::eval_const_size(eval_ctx, lval)) return pass();
			}break;
		}
		return errvoid();
	}

	template< template<typename Ty> class op> errvoid _il_evaluator_const_op(ILEvaluator* eval_ctx, ILDataType l, ILDataType r) {
		ILDataType res_t = ILBuilder::arith_result(l, r);
		switch (res_t) {
			case ILDataType::i8: {
				std::int8_t rval;
				if(!_il_evaluator_value_pop_into<std::int8_t>(rval, eval_ctx, r)) return pass();
				std::int8_t lval;
				if(!_il_evaluator_value_pop_into<std::int8_t>(lval, eval_ctx, l)) return pass();
				op<std::int8_t> o;
				if (!ILBuilder::eval_const_i8(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::u8: {
				std::uint8_t rval;
				if(!_il_evaluator_value_pop_into<std::uint8_t>(rval, eval_ctx, r)) return pass();
				std::uint8_t lval;
				if(!_il_evaluator_value_pop_into<std::uint8_t>(lval, eval_ctx, l)) return pass();
				op<std::uint8_t> o;
				if (!ILBuilder::eval_const_u8(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::i16: {
				std::int16_t rval;
				if(!_il_evaluator_value_pop_into<std::int16_t>(rval, eval_ctx, r)) return pass();
				std::int16_t lval;
				if(!_il_evaluator_value_pop_into<std::int16_t>(lval, eval_ctx, l)) return pass();
				op<std::int16_t> o;
				if (!ILBuilder::eval_const_i16(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::u16: {
				std::uint16_t rval;
				if(!_il_evaluator_value_pop_into<std::uint16_t>(rval, eval_ctx, r)) return pass();
				std::uint16_t lval;
				if(!_il_evaluator_value_pop_into<std::uint16_t>(lval, eval_ctx, l)) return pass();
				op<std::uint16_t> o;
				if (!ILBuilder::eval_const_u16(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::i32: {
				std::int32_t rval;
				if(!_il_evaluator_value_pop_into<std::int32_t>(rval, eval_ctx, r)) return pass();
				std::int32_t lval;
				if(!_il_evaluator_value_pop_into<std::int32_t>(lval, eval_ctx, l)) return pass();
				op<std::int32_t> o;
				if (!ILBuilder::eval_const_i32(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::u32: {
				std::uint32_t rval;
				if(!_il_evaluator_value_pop_into<std::uint32_t>(rval, eval_ctx, r)) return pass();
				std::uint32_t lval;
				if(!_il_evaluator_value_pop_into<std::uint32_t>(lval, eval_ctx, l)) return pass();
				op<std::uint32_t> o;
				if (!ILBuilder::eval_const_u32(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::i64: {
				std::int64_t rval;
				if(!_il_evaluator_value_pop_into<std::int64_t>(rval, eval_ctx, r)) return pass();
				std::int64_t lval;
				if(!_il_evaluator_value_pop_into<std::int64_t>(lval, eval_ctx, l)) return pass();
				op<std::int64_t> o;
				if (!ILBuilder::eval_const_i64(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::u64: {
				std::uint64_t rval;
				if(!_il_evaluator_value_pop_into<std::uint64_t>(rval, eval_ctx, r)) return pass();
				std::uint64_t lval;
				if(!_il_evaluator_value_pop_into<std::uint64_t>(lval, eval_ctx, l)) return pass();
				op<std::uint64_t> o;
				if (!ILBuilder::eval_const_u64(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::f32: {
				float rval;
				if(!_il_evaluator_value_pop_into<float>(rval, eval_ctx, r)) return pass();
				float lval;
				if(!_il_evaluator_value_pop_into<float>(lval, eval_ctx, l)) return pass();
				op<float> o;
				if (!ILBuilder::eval_const_f32(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::f64: {
				double rval;
				if(!_il_evaluator_value_pop_into<double>(rval, eval_ctx, r)) return pass();
				double lval;
				if(!_il_evaluator_value_pop_into<double>(lval, eval_ctx, l)) return pass();
				op<double> o;
				if (!ILBuilder::eval_const_f64(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::word: {
				std::size_t rval;
				if(!_il_evaluator_value_pop_into<std::size_t>(rval, eval_ctx, r)) return pass();
				std::size_t lval;
				if(!_il_evaluator_value_pop_into<std::size_t>(lval, eval_ctx, l)) return pass();
				op<std::size_t> o;
				if (!ILBuilder::eval_const_size(eval_ctx, o(lval, rval))) return pass();
			}break;
			default: break;
		}

		return errvoid();
	}

	template< template<typename Ty> class op> errvoid _il_evaluator_const_op_binary(ILEvaluator* eval_ctx, ILDataType l, ILDataType r) {
		ILDataType res_t = ILBuilder::arith_result(l, r);
		switch (res_t) {
			case ILDataType::i8: {
				std::int8_t rval;
				if(!_il_evaluator_value_pop_into<std::int8_t>(rval, eval_ctx, r)) return pass();
				std::int8_t lval;
				if(!_il_evaluator_value_pop_into<std::int8_t>(lval, eval_ctx, l)) return pass();
				op<std::int8_t> o;
				if (!ILBuilder::eval_const_i8(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::u8: {
				std::uint8_t rval;
				if(!_il_evaluator_value_pop_into<std::uint8_t>(rval, eval_ctx, r)) return pass();
				std::uint8_t lval;
				if(!_il_evaluator_value_pop_into<std::uint8_t>(lval, eval_ctx, l)) return pass();
				op<std::uint8_t> o;
				if (!ILBuilder::eval_const_u8(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::i16: {
				std::int16_t rval;
				if(!_il_evaluator_value_pop_into<std::int16_t>(rval, eval_ctx, r)) return pass();
				std::int16_t lval;
				if(!_il_evaluator_value_pop_into<std::int16_t>(lval, eval_ctx, l)) return pass();
				op<std::int16_t> o;
				if (!ILBuilder::eval_const_i16(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::u16: {
				std::uint16_t rval;
				if(!_il_evaluator_value_pop_into<std::uint16_t>(rval, eval_ctx, r)) return pass();
				std::uint16_t lval;
				if(!_il_evaluator_value_pop_into<std::uint16_t>(lval, eval_ctx, l)) return pass();
				op<std::uint16_t> o;
				if (!ILBuilder::eval_const_u16(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::i32: {
				std::int32_t rval;
				if(!_il_evaluator_value_pop_into<std::int32_t>(rval, eval_ctx, r)) return pass();
				std::int32_t lval;
				if(!_il_evaluator_value_pop_into<std::int32_t>(lval, eval_ctx, l)) return pass();
				op<std::int32_t> o;
				if (!ILBuilder::eval_const_i32(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::u32: {
				std::uint32_t rval;
				if(!_il_evaluator_value_pop_into<std::uint32_t>(rval, eval_ctx, r)) return pass();
				std::uint32_t lval;
				if(!_il_evaluator_value_pop_into<std::uint32_t>(lval, eval_ctx, l)) return pass();
				op<std::uint32_t> o;
				if (!ILBuilder::eval_const_u32(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::i64: {
				std::int64_t rval;
				if(!_il_evaluator_value_pop_into<std::int64_t>(rval, eval_ctx, r)) return pass();
				std::int64_t lval;
				if(!_il_evaluator_value_pop_into<std::int64_t>(lval, eval_ctx, l)) return pass();
				op<std::int64_t> o;
				if (!ILBuilder::eval_const_i64(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::u64: {
				std::uint64_t rval;
				if(!_il_evaluator_value_pop_into<std::uint64_t>(rval, eval_ctx, r)) return pass();
				std::uint64_t lval;
				if(!_il_evaluator_value_pop_into<std::uint64_t>(lval, eval_ctx, l)) return pass();
				op<std::uint64_t> o;
				if (!ILBuilder::eval_const_u64(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::word: {
				std::size_t rval;
				if(!_il_evaluator_value_pop_into<std::size_t>(rval, eval_ctx, r)) return pass();
				std::size_t lval;
				if(!_il_evaluator_value_pop_into<std::size_t>(lval, eval_ctx, l)) return pass();
				op<std::size_t> o;
				if (!ILBuilder::eval_const_size(eval_ctx, o(lval, rval))) return pass();
			}break;
			
			default: break;
		}

		return errvoid();
	}

	template< template<typename Ty> class op> errvoid _il_evaluator_const_op_bool(ILEvaluator* eval_ctx, ILDataType l, ILDataType r) {
		ILDataType res_t = ILBuilder::arith_result(l, r);
		switch (res_t) {
			case ILDataType::i8: {
				std::int8_t rval;
				if(!_il_evaluator_value_pop_into<std::int8_t>(rval, eval_ctx, r)) return pass();
				std::int8_t lval;
				if(!_il_evaluator_value_pop_into<std::int8_t>(lval, eval_ctx, l)) return pass();
				op<std::int8_t> o;
				if (!ILBuilder::eval_const_i8(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::u8: {
				std::uint8_t rval;
				if(!_il_evaluator_value_pop_into<std::uint8_t>(rval, eval_ctx, r)) return pass();
				std::uint8_t lval;
				if(!_il_evaluator_value_pop_into<std::uint8_t>(lval, eval_ctx, l)) return pass();
				op<std::uint8_t> o;
				if (!ILBuilder::eval_const_i8(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::i16: {
				std::int16_t rval;
				if(!_il_evaluator_value_pop_into<std::int16_t>(rval, eval_ctx, r)) return pass();
				std::int16_t lval;
				if(!_il_evaluator_value_pop_into<std::int16_t>(lval, eval_ctx, l)) return pass();
				op<std::int16_t> o;
				if (!ILBuilder::eval_const_i8(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::u16: {
				std::uint16_t rval;
				if(!_il_evaluator_value_pop_into<std::uint16_t>(rval, eval_ctx, r)) return pass();
				std::uint16_t lval;
				if(!_il_evaluator_value_pop_into<std::uint16_t>(lval, eval_ctx, l)) return pass();
				op<std::uint16_t> o;
				if (!ILBuilder::eval_const_i8(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::i32: {
				std::int32_t rval;
				if(!_il_evaluator_value_pop_into<std::int32_t>(rval, eval_ctx, r)) return pass();
				std::int32_t lval;
				if(!_il_evaluator_value_pop_into<std::int32_t>(lval, eval_ctx, l)) return pass();
				op<std::int32_t> o;
				if (!ILBuilder::eval_const_i8(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::u32: {
				std::uint32_t rval;
				if(!_il_evaluator_value_pop_into<std::uint32_t>(rval, eval_ctx, r)) return pass();
				std::uint32_t lval;
				if(!_il_evaluator_value_pop_into<std::uint32_t>(lval, eval_ctx, l)) return pass();
				op<std::uint32_t> o;
				if (!ILBuilder::eval_const_i8(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::i64: {
				std::int64_t rval;
				if(!_il_evaluator_value_pop_into<std::int64_t>(rval, eval_ctx, r)) return pass();
				std::int64_t lval;
				if(!_il_evaluator_value_pop_into<std::int64_t>(lval, eval_ctx, l)) return pass();
				op<std::int64_t> o;
				if (!ILBuilder::eval_const_i8(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::u64: {
				std::uint64_t rval;
				if(!_il_evaluator_value_pop_into<std::uint64_t>(rval, eval_ctx, r)) return pass();
				std::uint64_t lval;
				if(!_il_evaluator_value_pop_into<std::uint64_t>(lval, eval_ctx, l)) return pass();
				op<std::uint64_t> o;
				if (!ILBuilder::eval_const_i8(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::f32: {
				float rval;
				if(!_il_evaluator_value_pop_into<float>(rval, eval_ctx, r)) return pass();
				float lval;
				if(!_il_evaluator_value_pop_into<float>(lval, eval_ctx, l)) return pass();
				op<float> o;
				if (!ILBuilder::eval_const_i8(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::f64: {
				double rval;
				if(!_il_evaluator_value_pop_into<double>(rval, eval_ctx, r)) return pass();
				double lval;
				if(!_il_evaluator_value_pop_into<double>(lval, eval_ctx, l)) return pass();
				op<double> o;
				if (!ILBuilder::eval_const_i8(eval_ctx, o(lval, rval))) return pass();
			}break;
			case ILDataType::word: {
				std::size_t rval;
				if(!_il_evaluator_value_pop_into<std::size_t>(rval, eval_ctx, r)) return pass();
				std::size_t lval;
				if(!_il_evaluator_value_pop_into<std::size_t>(lval, eval_ctx, l)) return pass();
				op<std::size_t> o;
				if (!ILBuilder::eval_const_i8(eval_ctx, o(lval, rval))) return pass();
			}break;
			default: break;
		}
		
		return errvoid();
	}



	errvoid ILBuilder::eval_forget(ILEvaluator* eval_ctx, ILDataType type) {
		if (type != ILDataType::none) {
			ilsize_t storage;
			if (!eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(type), &storage)) return pass();
		}
		return errvoid();
	}


	std::size_t ILEvaluator::compile_time_register_size(ILDataType t) {
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


	errvoid ILBuilder::eval_discard(ILEvaluator* eval_ctx, ILDataType type) {
		return errvoid();
	}

	errvoid ILBuilder::eval_yield(ILEvaluator* eval_ctx, ILDataType type) {
		if (!eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(type), &eval_ctx->yield_storage)) return pass();
		return errvoid();
	}

	errvoid ILBuilder::eval_accept(ILEvaluator* eval_ctx, ILDataType type) {
		if (!eval_ctx->write_register_value_indirect(eval_ctx->compile_time_register_size(type), &eval_ctx->yield_storage)) return pass();
		return errvoid();
	}

	errvoid ILBuilder::eval_load(ILEvaluator* eval_ctx, ILDataType type) {
		void* ptr;
		if (!eval_ctx->pop_register_value<void*>(ptr)) return pass();	
		if (!eval_ctx->write_register_value_indirect(eval_ctx->compile_time_register_size(type), ptr)) return pass();
		return errvoid();
	}

	errvoid ILBuilder::eval_offset(ILEvaluator* eval_ctx, ILSize offset) {
		if (offset.type == ILSizeType::table || offset.type == ILSizeType::array || offset.value > 0) {
			std::size_t mem;
			if (!eval_ctx->pop_register_value<std::size_t>(mem)) return pass();
			mem += offset.eval(eval_ctx->parent, compiler_arch);
			eval_ctx->write_register_value(mem);
		}
		return errvoid();
	}



	errvoid ILBuilder::eval_aoffset(ILEvaluator* eval_ctx, std::uint32_t offset) {
		if (offset > 0) {
			std::size_t mem;
			if(!eval_ctx->pop_register_value(mem)) return pass();
			mem += offset;
			eval_ctx->write_register_value(mem);
		}
		return errvoid();
	}


	errvoid ILBuilder::eval_woffset(ILEvaluator* eval_ctx, std::uint32_t offset) {
		if (offset > 0) {
			std::size_t mem;
			if(!eval_ctx->pop_register_value(mem)) return pass();
			mem += offset * eval_ctx->compile_time_register_size(ILDataType::word);
			eval_ctx->write_register_value(mem);
		}
		return errvoid();
	}

	errvoid ILBuilder::eval_constref(ILEvaluator* eval_ctx, std::uint32_t cid) {
		eval_ctx->write_register_value(eval_ctx->parent->constant_memory[cid].second.get());
		return errvoid();
	}
	
	errvoid ILBuilder::eval_const_slice(ILEvaluator* eval_ctx, std::uint32_t cid, ILSize s) {
		dword_t val((void*)eval_ctx->parent->constant_memory[cid].second.get() , (void*)(s.eval(eval_ctx->parent,compiler_arch)));
		eval_ctx->write_register_value(val);
		return errvoid();
	}

	errvoid ILBuilder::eval_staticref(ILEvaluator* eval_ctx, std::uint32_t cid) {
		eval_ctx->write_register_value(eval_ctx->parent->static_memory[cid].second.get());
		return errvoid();
	}


	errvoid ILBuilder::eval_combine_dword(ILEvaluator* eval_ctx) {
		void* p2;
		if (!eval_ctx->pop_register_value<void*>(p2)) return pass();
		void* p1;
		if (!eval_ctx->pop_register_value<void*>(p1)) return pass();
		dword_t dw(p1,p2);
		eval_ctx->write_register_value(dw);
		return errvoid();
	}
	
	errvoid ILBuilder::eval_split_dword(ILEvaluator* eval_ctx) {
		dword_t dw;
		if (!eval_ctx->pop_register_value<dword_t>(dw)) return pass();
		eval_ctx->write_register_value(dw.p1);
		eval_ctx->write_register_value(dw.p2);
		return errvoid();
	}

	errvoid ILBuilder::eval_high_word(ILEvaluator* eval_ctx) {
		dword_t dw;
		if (!eval_ctx->pop_register_value<dword_t>(dw)) return pass();
		eval_ctx->write_register_value(dw.p2);
		return errvoid();
	}

	errvoid ILBuilder::eval_debug(ILEvaluator* eval_ctx, std::uint16_t file, std::uint16_t line) {
		eval_ctx->debug_file = file;
		eval_ctx->debug_line = line;
		return errvoid();
	}

	errvoid ILBuilder::eval_rtoffset(ILEvaluator* eval_ctx) {
		std::size_t offset;
			if(!eval_ctx->pop_register_value(offset)) return pass();
		unsigned char* mem;
			if(!eval_ctx->pop_register_value(mem)) return pass();
		mem += offset;
		eval_ctx->write_register_value(mem);
		return errvoid();
	}

	errvoid ILBuilder::eval_rtoffset_rev(ILEvaluator* eval_ctx) {
		unsigned char* mem;
			if(!eval_ctx->pop_register_value(mem)) return pass();
		std::size_t offset;
			if(!eval_ctx->pop_register_value(offset)) return pass();
		mem += offset;
		eval_ctx->write_register_value(mem);
		return errvoid();
	}

	errvoid ILBuilder::eval_roffset(ILEvaluator* eval_ctx, ILDataType from, ILDataType to, std::size_t offset) {
		ilsize_t mem;
		if (!eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(from), &mem)) return pass();
		mem = mem << offset;
		if (!eval_ctx->write_register_value_indirect(eval_ctx->compile_time_register_size(to), &mem)) return pass();
		return errvoid();
	}

	errvoid ILBuilder::eval_aroffset(ILEvaluator* eval_ctx, ILDataType from, ILDataType to, std::uint32_t offset) {

		if (offset > UINT8_MAX) {
			throw string_exception("Compiler error: aroffset > 255 should not be possible");
		}

		ilsize_t mem;
		if (!eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(from), &mem)) return pass();
		mem = mem << offset;
		if (!eval_ctx->write_register_value_indirect(eval_ctx->compile_time_register_size(to), &mem)) return pass();
		return errvoid();
	}

	errvoid ILBuilder::eval_wroffset(ILEvaluator* eval_ctx, ILDataType from, ILDataType to, std::uint32_t offset) {

		if (offset > UINT8_MAX) {
			throw string_exception("Compiler error: wroffset > 255 should not be possible");
		}

		ilsize_t mem;
		if (!eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(from), &mem)) return pass();
		mem = mem << (offset * eval_ctx->compile_time_register_size(ILDataType::word));
		if (!eval_ctx->write_register_value_indirect(eval_ctx->compile_time_register_size(to), &mem)) return pass();
		return errvoid();
	}


	errvoid ILBuilder::eval_store(ILEvaluator* eval_ctx, ILDataType type) {
		void* mem;
		if(!eval_ctx->pop_register_value<void*>(mem)) return pass();
		if (!eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(type), mem)) return pass();
		return errvoid();
	}

	errvoid ILBuilder::eval_store_rev(ILEvaluator* eval_ctx, ILDataType type) {
		ilsize_t storage;
		std::size_t regs = eval_ctx->compile_time_register_size(type);
		if (!eval_ctx->pop_register_value_indirect(regs, &storage)) return pass();
		void* mem;
		if(!eval_ctx->pop_register_value<void*>(mem)) return pass();

		if (!wrap || wrap(sandbox) == 0) {	
			std::memcpy(mem, &storage, regs);
		}
		else {
			return throw_runtime_handler_exception(eval_ctx);
		}
		return errvoid();
	}

	errvoid ILBuilder::eval_duplicate(ILEvaluator* eval_ctx, ILDataType type) {
		std::size_t reg_s = eval_ctx->compile_time_register_size(type);
		void* lv = eval_ctx->read_last_register_value_indirect(type);
		if (!eval_ctx->write_register_value_indirect(reg_s, lv)) return pass();
		return errvoid();
	}

	errvoid ILBuilder::eval_clone(ILEvaluator* eval_ctx, ILDataType type, std::uint16_t times) {
		std::size_t reg_s = eval_ctx->compile_time_register_size(type);
		void* lv = eval_ctx->read_last_register_value_indirect(type);
		for (std::uint16_t i = 0; i < times; ++i) {
			if (!eval_ctx->write_register_value_indirect(reg_s, lv)) return pass();
		}
		return errvoid();
	}

	errvoid ILBuilder::eval_fnptr(ILEvaluator* eval_ctx, ILFunction* fun) {
		void* wrtptr;
		if (!wrap || wrap(sandbox) == 0) {
			if (auto native_fun = dynamic_cast<ILNativeFunction*>(fun)) {
				wrtptr = native_fun->ptr;
			}
			else {
				wrtptr = fun;
			}
		} else {
			return throw_runtime_handler_exception(eval_ctx);
		}

		eval_ctx->write_register_value(wrtptr);
		return errvoid();
	}


	errvoid ILBuilder::eval_fncall(ILEvaluator* eval_ctx, ILFunction* fun) {
		uint32_t declid;

		if (!wrap || wrap(sandbox) == 0) {
			declid = fun->decl_id;
			if (auto native_fun = dynamic_cast<ILNativeFunction*>(fun)) {				
				eval_ctx->callstack.push_back(native_fun->ptr);
			}
			else {
				eval_ctx->callstack.push_back(fun);
			}
		} else {
			return throw_runtime_handler_exception(eval_ctx);
		}

		return eval_call(eval_ctx, declid);
	}

	errvoid ILBuilder::eval_null(ILEvaluator* eval_ctx) {
		eval_ctx->write_register_value((std::size_t)0);
		return errvoid();
	}

	errvoid ILBuilder::eval_isnotzero(ILEvaluator* eval_ctx, ILDataType type) {
		ilsize_t z = 0;
		if (!eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(type), &z)) return pass();
		eval_const_i8(eval_ctx, (z == 0 ? 0 : 1));
		return errvoid();
	}


	errvoid ILBuilder::eval_callstart(ILEvaluator* eval_ctx) {
		ILFunction* func;
		if(!eval_ctx->pop_register_value<ILFunction*>(func)) return pass();

		eval_ctx->callstack.push_back(func);
		return errvoid();
	}

	errvoid ILBuilder::eval_insintric(ILEvaluator* eval_ctx, ILInsintric fun) {
		if (!eval_ctx->parent->insintric_function[(unsigned char)fun](eval_ctx)) return errvoid();
		return errvoid();
	}

	errvoid ILBuilder::eval_vtable(ILEvaluator* eval_ctx, std::uint32_t id) {
		eval_ctx->write_register_value(eval_ctx->parent->vtable_data[id].second.get());
		return errvoid();
	}


	errvoid ILBuilder::eval_memcpy(ILEvaluator* eval_ctx, std::size_t size) {

		void* dst;
			if(!eval_ctx->pop_register_value(dst)) return pass();
		void* src;
			if(!eval_ctx->pop_register_value(src)) return pass();

		if (!wrap || wrap(sandbox) == 0) {
			std::memcpy(dst, src, size);
		}
		else {
			return throw_runtime_handler_exception(eval_ctx);
		}
		return errvoid();
	}

	errvoid ILBuilder::eval_memcpy_rev(ILEvaluator* eval_ctx, std::size_t size) {

		void* src;
			if(!eval_ctx->pop_register_value(src)) return pass();
		void* dst;
			if(!eval_ctx->pop_register_value(dst)) return pass();

		if (!wrap || wrap(sandbox) == 0) {
			std::memcpy(dst, src, size);
		}
		else {
			return throw_runtime_handler_exception(eval_ctx);
		}
		return errvoid();
	}


	errvoid ILBuilder::eval_memcmp(ILEvaluator* eval_ctx, std::size_t size) {

		void* dst;
			if(!eval_ctx->pop_register_value(dst)) return pass();
		void* src;
			if(!eval_ctx->pop_register_value(src)) return pass();

		if (!wrap || wrap(sandbox) == 0) {
			eval_ctx->write_register_value((std::int8_t)memcmp(dst, src, size));
		}
		else {
			return throw_runtime_handler_exception(eval_ctx);
		}
		return errvoid();
	}

	errvoid ILBuilder::eval_memcmp_rev(ILEvaluator* eval_ctx, std::size_t size) {

		void* src;
			if(!eval_ctx->pop_register_value(src)) return pass();
		void* dst;
			if(!eval_ctx->pop_register_value(dst)) return pass();

		if (!wrap || wrap(sandbox) == 0) {
			eval_ctx->write_register_value((std::int8_t)memcmp(dst, src, size));
		}
		else {
			return throw_runtime_handler_exception(eval_ctx);
		}
		return errvoid();
	}

	errvoid ILBuilder::eval_tableoffset(ILEvaluator* eval_ctx, tableid_t tableid, tableelement_t itemid) {
		auto& table = eval_ctx->parent->structure_tables[tableid];
		table.calculate(eval_ctx->parent, compiler_arch);
		unsigned char* ptr;
			if(!eval_ctx->pop_register_value(ptr)) return pass();
		ptr += table.calculated_offsets[itemid];
		eval_ctx->write_register_value(ptr);
		return errvoid();
	}


	errvoid ILBuilder::eval_tableroffset(ILEvaluator* eval_ctx, ILDataType src, ILDataType dst, tableid_t tableid, tableelement_t itemid) {
		auto& table = eval_ctx->parent->structure_tables[tableid];
		table.calculate(eval_ctx->parent, compiler_arch);
		ilsize_t storage;
		if (!eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(src), &storage)) return pass();
		storage = storage >> table.calculated_offsets[itemid];
		if (!eval_ctx->write_register_value_indirect(eval_ctx->compile_time_register_size(dst), &storage)) return pass();
		return errvoid();
	}

	errvoid ILBuilder::eval_rmemcmp(ILEvaluator* eval_ctx, ILDataType type) {
		std::size_t reg_v = eval_ctx->compile_time_register_size(type);
		ilsize_t s1, s2;
		if (!eval_ctx->pop_register_value_indirect(reg_v, &s1)) return pass();
		if (!eval_ctx->pop_register_value_indirect(reg_v, &s2)) return pass();
		std::uint8_t res;
		if (!wrap || wrap(sandbox) == 0) {
			res = (std::int8_t)memcmp(&s1, &s2, reg_v);
		}
		else {
			return throw_runtime_handler_exception(eval_ctx);
		}

		eval_ctx->write_register_value(res);
		return errvoid();
	}

	errvoid ILBuilder::eval_rmemcmp_rev(ILEvaluator* eval_ctx, ILDataType type) {
		std::size_t reg_v = eval_ctx->compile_time_register_size(type);
		ilsize_t s1, s2;
		if (!eval_ctx->pop_register_value_indirect(reg_v, &s2)) return pass();
		if (!eval_ctx->pop_register_value_indirect(reg_v, &s1)) return pass();
		std::uint8_t res;
		if (!wrap || wrap(sandbox) == 0) {
			res = (std::int8_t)memcmp(&s1, &s2, reg_v);
		}
		else {
			return throw_runtime_handler_exception(eval_ctx);
		}
		eval_ctx->write_register_value(res);
		return errvoid();
	}


	errvoid ILBuilder::eval_swap(ILEvaluator* eval_ctx, ILDataType type) {
		std::size_t reg_v = eval_ctx->compile_time_register_size(type);
		ilsize_t s1, s2;
		if (!eval_ctx->pop_register_value_indirect(reg_v, &s1)) return pass();
		if (!eval_ctx->pop_register_value_indirect(reg_v, &s2)) return pass();
		if (!eval_ctx->write_register_value_indirect(reg_v, &s1)) return pass();
		if (!eval_ctx->write_register_value_indirect(reg_v, &s2)) return pass();
		return errvoid();
	}



	errvoid ILBuilder::eval_negative(ILEvaluator* eval_ctx, ILDataType type) {

		switch (type) {
			case ILDataType::u8:
			case ILDataType::i8: {
				std::int8_t v;
				if (!eval_ctx->pop_register_value<std::int8_t>(v)) return pass();
				eval_ctx->write_register_value(-v);
			} break;
			case ILDataType::u16:
			case ILDataType::i16:{
				std::int16_t v;
				if (!eval_ctx->pop_register_value<std::int16_t>(v)) return pass();
				eval_ctx->write_register_value(-v);
			} break;
			case ILDataType::u32:
			case ILDataType::i32:{
				std::int32_t v;
				if (!eval_ctx->pop_register_value<std::int32_t>(v)) return pass();
				eval_ctx->write_register_value(-v);
			} break;
			case ILDataType::u64:
			case ILDataType::i64:{
				std::int64_t v;
				if (!eval_ctx->pop_register_value<std::int64_t>(v)) return pass();
				eval_ctx->write_register_value(-v);
			} break;
			case ILDataType::f32:{
				float v;
				if (!eval_ctx->pop_register_value<float>(v)) return pass();
				eval_ctx->write_register_value(-v);
			} break;
			case ILDataType::f64:{
				double v;
				if (!eval_ctx->pop_register_value<double>(v)) return pass();
				eval_ctx->write_register_value(-v);
			} break;
				break;
			case ILDataType::word:{
				std::size_t v;
				if (!eval_ctx->pop_register_value<std::size_t>(v)) return pass();
				eval_ctx->write_register_value((SIZE_MAX ^ v) - 1);
			} break;
		}
		return errvoid();
	}


	errvoid ILBuilder::eval_negate(ILEvaluator* eval_ctx) {
		std::uint8_t val;
		if (!eval_ctx->pop_register_value<std::uint8_t>(val)) return pass();

		if (val) {
			eval_ctx->write_register_value<std::uint8_t>(0);
		}
		else {
			eval_ctx->write_register_value<std::uint8_t>(1);
		}
		return errvoid();
	}



	errvoid ILBuilder::eval_swap2(ILEvaluator* eval_ctx, ILDataType type1, ILDataType type2) {
		std::size_t reg_v_1 = eval_ctx->compile_time_register_size(type1);
		std::size_t reg_v_2 = eval_ctx->compile_time_register_size(type2);
		ilsize_t s1, s2;
		if (!eval_ctx->pop_register_value_indirect(reg_v_2, &s2)) return pass();
		if (!eval_ctx->pop_register_value_indirect(reg_v_1, &s1)) return pass();
		if (!eval_ctx->write_register_value_indirect(reg_v_2, &s2)) return pass();
		if (!eval_ctx->write_register_value_indirect(reg_v_1, &s1)) return pass();
		return errvoid();
	}

	errvoid ILBuilder::eval_call(ILEvaluator* eval_ctx, std::uint32_t decl) {

		if (!wrap || wrap(sandbox) == 0) {
			auto& declaration = eval_ctx->parent->function_decl[decl];

			void* ptr = eval_ctx->callstack.back();
			eval_ctx->callstack.pop_back();

			if (std::get<0>(declaration) == ILCallingConvention::bytecode) {
				ILBytecodeFunction* bytecode_fun = (ILBytecodeFunction*)ptr;

				//eval_ctx->callstack_debug.push_back(std::make_tuple(eval_ctx->debug_line, eval_ctx->debug_file, std::string_view(bytecode_fun->alias)));

				bytecode_fun->calculate_stack(compiler_arch);

				ILBlock* block = bytecode_fun->blocks[0];
				bool running = true;

				unsigned char* lstack_base = eval_ctx->stack_base;
				unsigned char* lstack_base_aligned = eval_ctx->stack_base_aligned;
				unsigned char* lstack_pointer = eval_ctx->stack_pointer;

				eval_ctx->stack_base = eval_ctx->stack_pointer;
				eval_ctx->stack_base_aligned = (unsigned char*)align_up((std::size_t)eval_ctx->stack_base, bytecode_fun->calculated_local_stack_alignment);
				eval_ctx->stack_pointer = eval_ctx->stack_base_aligned + bytecode_fun->calculated_local_stack_size;


				unsigned char* local_base = eval_ctx->stack_base_aligned;

				std::size_t instr = 0;

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
								auto decl = ILBlock::read_data<std::uint32_t>(it);
								if (!eval_call(eval_ctx, decl)) return false;
							} break;
							case ILInstruction::memcpy: {
								auto size = ILBlock::read_data<ILSize>(it);
								if (!eval_memcpy(eval_ctx, size.eval(eval_ctx->parent, compiler_arch))) return pass();
							} break;
							case ILInstruction::memcpy2: {
								auto size = ILBlock::read_data<ILSize>(it);
								if (!eval_memcpy_rev(eval_ctx, size.eval(eval_ctx->parent, compiler_arch))) return pass();
							} break;
							case ILInstruction::memcmp: {
								auto size = ILBlock::read_data<ILSize>(it);
								if (!eval_memcmp(eval_ctx, size.eval(eval_ctx->parent, compiler_arch))) return pass();
							} break;
							case ILInstruction::memcmp2: {
								auto size = ILBlock::read_data<ILSize>(it);
								if (!eval_memcmp_rev(eval_ctx, size.eval(eval_ctx->parent, compiler_arch))) return pass();
							} break;
							case ILInstruction::fnptr: {
								auto id = ILBlock::read_data<std::uint32_t>(it);
								if (!eval_fnptr(eval_ctx,eval_ctx->parent->functions[id].get())) return pass();
							} break;
							case ILInstruction::fncall: {
								auto id = ILBlock::read_data<std::uint32_t>(it);
								auto fn = eval_ctx->parent->functions[id].get();
								if (!eval_fncall(eval_ctx, fn)) return pass();
							} break;
							case ILInstruction::vtable: {
								auto id = ILBlock::read_data<std::uint32_t>(it);
								if (!eval_vtable(eval_ctx, id)) return pass();
							} break;
							case ILInstruction::duplicate: {
								auto type = ILBlock::read_data<ILDataType>(it);
								if (!eval_duplicate(eval_ctx, type)) return pass();
							} break;
							case ILInstruction::clone: {
								auto type = ILBlock::read_data<ILDataType>(it);
								auto times = ILBlock::read_data<std::uint16_t>(it);
								if (!eval_clone(eval_ctx, type, times)) return pass();
							} break;
							case ILInstruction::swap: {
								auto type = ILBlock::read_data<ILDataType>(it);
								if (!eval_swap(eval_ctx, type)) return pass();
							} break;
							case ILInstruction::swap2: {
								auto p = ILBlock::read_data<ILDataTypePair>(it);
								if (!eval_swap2(eval_ctx, p.first(), p.second())) return pass();
							} break;
							case ILInstruction::insintric: {
								auto id = ILBlock::read_data<std::uint8_t>(it);
								if (!eval_insintric(eval_ctx, (ILInsintric)id)) return pass();
							} break;
							case ILInstruction::rmemcmp: {
								auto type = ILBlock::read_data<ILDataType>(it);
								if (!eval_rmemcmp(eval_ctx, type)) return pass();
							} break;
							case ILInstruction::rmemcmp2: {
								auto type = ILBlock::read_data<ILDataType>(it);
								if (!eval_rmemcmp_rev(eval_ctx, type)) return pass();
							} break;
							case ILInstruction::sub: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								if (!eval_sub(eval_ctx, type.first(), type.second())) return pass();
							} break;
							case ILInstruction::div: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								if (!eval_div(eval_ctx, type.first(), type.second())) return pass();
							} break;
							case ILInstruction::rem: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								if (!eval_rem(eval_ctx, type.first(), type.second())) return pass();
							} break;
							case ILInstruction::mul: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								if (!eval_mul(eval_ctx, type.first(), type.second())) return pass();
							} break;
							case ILInstruction::add: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								if (!eval_add(eval_ctx, type.first(), type.second())) return pass();
							} break;
							case ILInstruction::bit_and: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								if (!eval_and(eval_ctx, type.first(), type.second())) return pass();
							} break;
							case ILInstruction::bit_or: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								if (!eval_or(eval_ctx, type.first(), type.second())) return pass();
							} break;
							case ILInstruction::bit_xor: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								if (!eval_xor(eval_ctx, type.first(), type.second())) return pass();
							} break;
							case ILInstruction::eq: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								if (!eval_eq(eval_ctx, type.first(), type.second())) return pass();
							} break;
							case ILInstruction::ne: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								if (!eval_ne(eval_ctx, type.first(), type.second())) return pass();
							} break;
							case ILInstruction::gt: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								if (!eval_gt(eval_ctx, type.first(), type.second())) return pass();
							} break;
							case ILInstruction::lt: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								if (!eval_lt(eval_ctx, type.first(), type.second())) return pass();
							} break;
							case ILInstruction::ge: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								if (!eval_ge(eval_ctx, type.first(), type.second())) return pass();
							} break;
							case ILInstruction::le: {
								auto type = ILBlock::read_data<ILDataTypePair>(it);
								if (!eval_le(eval_ctx, type.first(), type.second())) return pass();
							} break;

							case ILInstruction::cast: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								if (!eval_cast(eval_ctx, pair.first(), pair.second())) return pass();
							} break;
							case ILInstruction::bitcast: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								if (!eval_bitcast(eval_ctx, pair.first(), pair.second())) return pass();
							} break;
							case ILInstruction::store: {
								auto type = ILBlock::read_data<ILDataType>(it);
								if (!eval_store(eval_ctx, type)) return pass();
							} break;
							case ILInstruction::store2: {
								auto type = ILBlock::read_data<ILDataType>(it);
								if (!eval_store_rev(eval_ctx, type)) return pass();
							} break;
							case ILInstruction::yield: {
								auto type = ILBlock::read_data<ILDataType>(it);
								if (!eval_yield(eval_ctx, type)) return pass();
							} break;
							case ILInstruction::accept: {
								auto type = ILBlock::read_data<ILDataType>(it);
								if (!eval_accept(eval_ctx, type)) return pass();
							} break;
							case ILInstruction::discard: {
								auto type = ILBlock::read_data<ILDataType>(it);
								if (!eval_discard(eval_ctx, type)) return pass();
							} break;
							case ILInstruction::start: {
								if (!eval_callstart(eval_ctx)) return pass();
							} break;

							case ILInstruction::null: {
								if (!eval_null(eval_ctx)) return pass();
							} break;

							case ILInstruction::jmp: {
								auto address = ILBlock::read_data<std::uint32_t>(it);
								block = block->parent->blocks_memory[address].get();
								goto next_block;
							}break;
							case ILInstruction::offset32: {
								auto t = ILBlock::read_data<std::uint8_t>(it);
								auto v = ILBlock::read_data<std::uint32_t>(it);
								if (!eval_offset(eval_ctx, ILSize((ILSizeType)t,v))) return pass();
							} break;
							case ILInstruction::offset16: {
								auto t = ILBlock::read_data<std::uint8_t>(it);
								auto v = ILBlock::read_data<std::uint16_t>(it);
								if (!eval_offset(eval_ctx, ILSize((ILSizeType)t,v))) return pass();
							} break;
							case ILInstruction::offset8: {
								auto t = ILBlock::read_data<std::uint8_t>(it);
								auto v = ILBlock::read_data<std::uint8_t>(it);
								if (!eval_offset(eval_ctx, ILSize((ILSizeType)t,v))) return pass();
							} break;
							case ILInstruction::aoffset8: {
								auto size = ILBlock::read_data<std::uint8_t>(it);
								if (!eval_aoffset(eval_ctx, size)) return pass();
							} break;
							case ILInstruction::aoffset16: {
								auto size = ILBlock::read_data<std::uint16_t>(it);
								if (!eval_aoffset(eval_ctx, size)) return pass();
							} break;
							case ILInstruction::aoffset32: {
								auto size = ILBlock::read_data<std::uint32_t>(it);
								if (!eval_aoffset(eval_ctx, size)) return pass();
							} break;
							case ILInstruction::woffset8: {
								auto size = ILBlock::read_data<std::uint8_t>(it);
								if (!eval_woffset(eval_ctx, size)) return pass();
							} break;
							case ILInstruction::woffset16: {
								auto size = ILBlock::read_data<std::uint16_t>(it);
								if (!eval_woffset(eval_ctx, size)) return pass();
							} break;
							case ILInstruction::woffset32: {
								auto size = ILBlock::read_data<std::uint32_t>(it);
								if (!eval_woffset(eval_ctx, size)) return pass();
							} break;
							case ILInstruction::constref: {
								auto cid = ILBlock::read_data<std::uint32_t>(it);
								if (!eval_constref(eval_ctx, cid)) return pass();
							} break;
							case ILInstruction::staticref: {
								auto cid = ILBlock::read_data<std::uint32_t>(it);
								if (!eval_staticref(eval_ctx, cid)) return pass();
							} break;
							case ILInstruction::rtoffset: {
								if (!eval_rtoffset(eval_ctx)) return pass();
							} break;
							case ILInstruction::rtoffset2: {
								if (!eval_rtoffset_rev(eval_ctx)) return pass();
							} break;
							case ILInstruction::roffset32: {
								auto from_t = ILBlock::read_data<ILDataType>(it);
								auto to_t = ILBlock::read_data<ILDataType>(it); 
								auto t = ILBlock::read_data<std::uint8_t>(it);
								auto v = ILBlock::read_data<std::uint32_t>(it);
								if (!eval_roffset(eval_ctx, from_t, to_t, ILSize((ILSizeType)t, v).eval(eval_ctx->parent, compiler_arch))) return pass();
							} break;
								
							case ILInstruction::roffset16: {
								auto from_t = ILBlock::read_data<ILDataType>(it);
								auto to_t = ILBlock::read_data<ILDataType>(it); 
								auto t = ILBlock::read_data<std::uint8_t>(it);
								auto v = ILBlock::read_data<std::uint16_t>(it);
								if (!eval_roffset(eval_ctx, from_t, to_t, ILSize((ILSizeType)t, v).eval(eval_ctx->parent, compiler_arch))) return pass();
							} break;
								
							case ILInstruction::roffset8: {
								auto from_t = ILBlock::read_data<ILDataType>(it);
								auto to_t = ILBlock::read_data<ILDataType>(it); 
								auto t = ILBlock::read_data<std::uint8_t>(it);
								auto v = ILBlock::read_data<std::uint8_t>(it);
								if (!eval_roffset(eval_ctx, from_t, to_t, ILSize((ILSizeType)t, v).eval(eval_ctx->parent, compiler_arch))) return pass();
							} break;

							case ILInstruction::aroffset: {
								auto p = ILBlock::read_data<ILDataTypePair>(it);
								auto size = ILBlock::read_data<std::uint8_t>(it);
								if (!eval_aroffset(eval_ctx, p.first(), p.second(), size)) return pass();
							} break;

							case ILInstruction::wroffset: {
								auto p = ILBlock::read_data<ILDataTypePair>(it);
								auto size = ILBlock::read_data<std::uint8_t>(it);
								if (!eval_wroffset(eval_ctx, p.first(), p.second(), size)) return pass();
							} break;

							case ILInstruction::local8: {
								auto id = ILBlock::read_data<std::uint8_t>(it);
								auto& offset = bytecode_fun->calculated_local_offsets[id];
								if (!eval_const_word(eval_ctx, local_base + offset)) return pass();
							} break;
								
							case ILInstruction::local16: {
								auto id = ILBlock::read_data<std::uint16_t>(it);
								auto& offset = bytecode_fun->calculated_local_offsets[id];
								if (!eval_const_word(eval_ctx, local_base + offset)) return pass();
							} break;
								
							case ILInstruction::local32: {
								auto id = ILBlock::read_data<std::uint32_t>(it);
								auto& offset = bytecode_fun->calculated_local_offsets[id];
								if (!eval_const_word(eval_ctx, local_base + offset)) return pass();
							} break;

							case ILInstruction::table8offset8: {
								auto tid = ILBlock::read_data<std::uint8_t>(it);
								auto eid = ILBlock::read_data<std::uint8_t>(it);
								if (!eval_tableoffset(eval_ctx, tid, eid)) return pass();
							} break;
							case ILInstruction::table8offset16: {
								auto tid = ILBlock::read_data<std::uint8_t>(it);
								auto eid = ILBlock::read_data<std::uint16_t>(it);
								if (!eval_tableoffset(eval_ctx, tid, eid)) return pass();
							} break;
							case ILInstruction::table8offset32: {
								auto tid = ILBlock::read_data<std::uint8_t>(it);
								auto eid = ILBlock::read_data<std::uint32_t>(it);
								if (!eval_tableoffset(eval_ctx, tid, eid)) return pass();
							} break;
							case ILInstruction::table16offset8: {
								auto tid = ILBlock::read_data<std::uint16_t>(it);
								auto eid = ILBlock::read_data<std::uint8_t>(it);
								if (!eval_tableoffset(eval_ctx, tid, eid)) return pass();
							} break;
							case ILInstruction::table16offset16: {
								auto tid = ILBlock::read_data<std::uint16_t>(it);
								auto eid = ILBlock::read_data<std::uint16_t>(it);
								if (!eval_tableoffset(eval_ctx, tid, eid)) return pass();
							} break;
							case ILInstruction::table16offset32: {
								auto tid = ILBlock::read_data<std::uint16_t>(it);
								auto eid = ILBlock::read_data<std::uint32_t>(it);
								if (!eval_tableoffset(eval_ctx, tid, eid)) return pass();
							} break;
							case ILInstruction::table32offset8: {
								auto tid = ILBlock::read_data<std::uint32_t>(it);
								auto eid = ILBlock::read_data<std::uint8_t>(it);
								if (!eval_tableoffset(eval_ctx, tid, eid)) return pass();
							} break;
							case ILInstruction::table32offset16: {
								auto tid = ILBlock::read_data<std::uint32_t>(it);
								auto eid = ILBlock::read_data<std::uint16_t>(it);
								if (!eval_tableoffset(eval_ctx, tid, eid)) return pass();
							} break;
							case ILInstruction::table32offset32: {
								auto tid = ILBlock::read_data<std::uint32_t>(it);
								auto eid = ILBlock::read_data<std::uint32_t>(it);
								if (!eval_tableoffset(eval_ctx, tid, eid)) return pass();
							} break;

							case ILInstruction::table8roffset8: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								auto tid = ILBlock::read_data<std::uint8_t>(it);
								auto eid = ILBlock::read_data<std::uint8_t>(it);
								if (!eval_tableroffset(eval_ctx, pair.first(), pair.second(), tid, eid)) return pass();
							} break;
							case ILInstruction::table8roffset16: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								auto tid = ILBlock::read_data<std::uint8_t>(it);
								auto eid = ILBlock::read_data<std::uint16_t>(it);
								if (!eval_tableroffset(eval_ctx, pair.first(), pair.second(), tid, eid)) return pass();
							} break;
							case ILInstruction::table8roffset32: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								auto tid = ILBlock::read_data<std::uint8_t>(it);
								auto eid = ILBlock::read_data<std::uint32_t>(it);
								if (!eval_tableroffset(eval_ctx, pair.first(), pair.second(), tid, eid)) return pass();
							} break;
							case ILInstruction::table16roffset8: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								auto tid = ILBlock::read_data<std::uint16_t>(it);
								auto eid = ILBlock::read_data<std::uint8_t>(it);
								if (!eval_tableroffset(eval_ctx, pair.first(), pair.second(), tid, eid)) return pass();
							} break;
							case ILInstruction::table16roffset16: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								auto tid = ILBlock::read_data<std::uint16_t>(it);
								auto eid = ILBlock::read_data<std::uint16_t>(it);
								if (!eval_tableroffset(eval_ctx, pair.first(), pair.second(), tid, eid)) return pass();
							} break;
							case ILInstruction::table16roffset32: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								auto tid = ILBlock::read_data<std::uint16_t>(it);
								auto eid = ILBlock::read_data<std::uint32_t>(it);
								if (!eval_tableroffset(eval_ctx, pair.first(), pair.second(), tid, eid)) return pass();
							} break;
							case ILInstruction::table32roffset8: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								auto tid = ILBlock::read_data<std::uint32_t>(it);
								auto eid = ILBlock::read_data<std::uint8_t>(it);
								if (!eval_tableroffset(eval_ctx, pair.first(), pair.second(), tid, eid)) return pass();
							} break;
							case ILInstruction::table32roffset16: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								auto tid = ILBlock::read_data<std::uint32_t>(it);
								auto eid = ILBlock::read_data<std::uint16_t>(it);
								if (!eval_tableroffset(eval_ctx, pair.first(), pair.second(), tid, eid)) return pass();
							} break;
							case ILInstruction::table32roffset32: {
								auto pair = ILBlock::read_data<ILDataTypePair>(it);
								auto tid = ILBlock::read_data<std::uint32_t>(it);
								auto eid = ILBlock::read_data<std::uint32_t>(it);
								if (!eval_tableroffset(eval_ctx, pair.first(), pair.second(), tid, eid)) return pass();
							} break;

							case ILInstruction::debug: {
								auto file = ILBlock::read_data<std::uint16_t>(it);
								auto line = ILBlock::read_data<std::uint16_t>(it);
								if (!eval_debug(eval_ctx, file, line)) return pass();
							} break;

							case ILInstruction::load: {
								auto type = ILBlock::read_data<ILDataType>(it);
								if (!eval_load(eval_ctx, type)) return pass();
							} break;

							case ILInstruction::isnotzero: {
								auto type = ILBlock::read_data<ILDataType>(it);
								if (!eval_isnotzero(eval_ctx, type)) return pass();
							} break;

							case ILInstruction::negative: {
								auto type = ILBlock::read_data<ILDataType>(it);
								if (!eval_negative(eval_ctx, type)) return pass();
							} break;

							case ILInstruction::negate: {
								if (!eval_negate(eval_ctx)) return pass();
							} break;

							case ILInstruction::combinedw: {
								if (!eval_combine_dword(eval_ctx)) return pass();
							} break;
							case ILInstruction::highdw: {
								if (!eval_high_word(eval_ctx)) return pass();
							} break;
							case ILInstruction::splitdw: {
								if (!eval_split_dword(eval_ctx)) return pass();
							} break;

							case ILInstruction::forget: {
								auto type = ILBlock::read_data<ILDataType>(it);
								if (!eval_forget(eval_ctx, type)) return pass();
							} break;
							case ILInstruction::jmpz: {
								auto addressz = ILBlock::read_data<std::uint32_t>(it);
								auto addressnz = ILBlock::read_data<std::uint32_t>(it);

								std::uint8_t z;
								if (!eval_ctx->pop_register_value<std::uint8_t>(z)) return pass();

								if (z) {
									block = block->parent->blocks_memory[addressnz].get();
								}
								else {
									block = block->parent->blocks_memory[addressz].get();
								}

								goto next_block;
							} break;

							case ILInstruction::u8: if (!eval_const_u8(eval_ctx, ILBlock::read_data<std::uint8_t>(it))) return pass(); break;
							case ILInstruction::i8: if (!eval_const_i8(eval_ctx, ILBlock::read_data<std::int8_t>(it))) return pass(); break;
							case ILInstruction::u16: if (!eval_const_u16(eval_ctx, ILBlock::read_data<std::uint16_t>(it))) return pass(); break;
							case ILInstruction::i16: if (!eval_const_i16(eval_ctx, ILBlock::read_data<std::int16_t>(it))) return pass(); break;
							case ILInstruction::u32: if (!eval_const_u32(eval_ctx, ILBlock::read_data<std::uint32_t>(it))) return pass(); break;
							case ILInstruction::i32: if (!eval_const_i32(eval_ctx, ILBlock::read_data<std::int32_t>(it))) return pass(); break;
							case ILInstruction::u64: if (!eval_const_u64(eval_ctx, ILBlock::read_data<std::uint64_t>(it))) return pass(); break;
							case ILInstruction::i64: if (!eval_const_i64(eval_ctx, ILBlock::read_data<std::int64_t>(it))) return pass(); break;
							case ILInstruction::f32: if (!eval_const_f32(eval_ctx, ILBlock::read_data<float>(it))) return pass(); break;
							case ILInstruction::f64: if (!eval_const_f64(eval_ctx, ILBlock::read_data<double>(it))) return pass(); break;
							case ILInstruction::word: if (!eval_const_word(eval_ctx, ILBlock::read_data<void*>(it))) return pass(); break;
							case ILInstruction::dword: {
								dword_t v;
								v.p1 = ILBlock::read_data<void*>(it);
								v.p2 = ILBlock::read_data<void*>(it);
								if (!eval_const_dword(eval_ctx, v)) return pass(); break;
							}
							case ILInstruction::slice: {
								std::uint32_t cid = ILBlock::read_data<std::uint32_t>(it);
								auto st = ILBlock::read_data<ILSizeType>(it);
								auto sv = ILBlock::read_data<std::uint32_t>(it);
								if (!eval_const_slice(eval_ctx, cid, ILSize(st,sv))) return pass(); 
							}break;
							case ILInstruction::size8: {
								auto t = ILBlock::read_data<ILSizeType>(it);
								auto v = ILBlock::read_data<std::uint8_t>(it);
								if (!eval_const_size(eval_ctx, ILSize(t,v).eval(eval_ctx->parent, compiler_arch))) return pass();
							} break;
							case ILInstruction::size16: {
								auto t = ILBlock::read_data<ILSizeType>(it);
								auto v = ILBlock::read_data<std::uint16_t>(it);
								if (!eval_const_size(eval_ctx, ILSize(t,v).eval(eval_ctx->parent, compiler_arch))) return pass();
							} break;
							case ILInstruction::size32: {
								auto t = ILBlock::read_data<ILSizeType>(it);
								auto v = ILBlock::read_data<std::uint32_t>(it);
								if (!eval_const_size(eval_ctx, ILSize(t,v).eval(eval_ctx->parent, compiler_arch))) return pass();
							} break;

						}
					}

				next_block:
					continue;
				}

			returned:
				//eval_ctx->callstack_debug.pop_back();
				eval_ctx->stack_pointer = lstack_pointer;
				eval_ctx->stack_base_aligned = lstack_base_aligned;
				eval_ctx->stack_base = lstack_base;

			}
			else {
				eval_extern_err = true;
				//eval_ctx->callstack_debug.push_back(std::make_tuple(eval_ctx->debug_line, eval_ctx->debug_file, "External code"));
				if (!abi_dynamic_call(eval_ctx, std::get<0>(declaration), ptr, declaration)) return pass();
				//eval_ctx->callstack_debug.pop_back();
				if (!eval_extern_err) return pass();
			}

		}
		else {
			return throw_runtime_handler_exception(eval_ctx);
		}

		return errvoid();
	}


	errvoid ILBuilder::eval_add(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op<std::plus>(eval_ctx, t_l, t_r)) return pass();
		return errvoid();
	}

	errvoid ILBuilder::eval_sub(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op<std::minus>(eval_ctx, t_l, t_r)) return pass();
		return errvoid();
	}


	errvoid ILBuilder::eval_div(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		if (!wrap || wrap(sandbox) == 0) {
			if (!_il_evaluator_const_op<std::divides>(eval_ctx, t_l, t_r)) return pass();
		} else {
			return throw_runtime_exception(eval_ctx, "Division operation failed");
		}
		return errvoid();
	}


	errvoid ILBuilder::eval_rem(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		if (!wrap || wrap(sandbox) == 0) {
			if (!_il_evaluator_const_op_binary<std::modulus>(eval_ctx, t_l, t_r)) return pass();
		} else {
			return throw_runtime_exception(eval_ctx, "Division operation failed");
		}
		return errvoid();
	}


	errvoid ILBuilder::eval_and(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_binary<std::bit_and>(eval_ctx, t_l, t_r)) return pass();
		return errvoid();
	}


	errvoid ILBuilder::eval_or(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_binary<std::bit_or>(eval_ctx, t_l, t_r)) return pass();
		return errvoid();
	}


	errvoid ILBuilder::eval_xor(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_binary<std::bit_xor>(eval_ctx, t_l, t_r)) return pass();
		return errvoid();
	}


	errvoid ILBuilder::eval_mul(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op<std::multiplies>(eval_ctx, t_l, t_r)) return pass();
		return errvoid();
	}


	errvoid ILBuilder::eval_eq(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_bool<std::equal_to>(eval_ctx, t_l, t_r)) return pass();
		return errvoid();
	}


	errvoid ILBuilder::eval_ne(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_bool<std::not_equal_to>(eval_ctx, t_l, t_r)) return pass();
		return errvoid();
	}


	errvoid ILBuilder::eval_gt(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_bool<std::greater>(eval_ctx, t_l, t_r)) return pass();
		return errvoid();
	}

	errvoid ILBuilder::eval_ge(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_bool<std::greater_equal>(eval_ctx, t_l, t_r)) return pass();
		return errvoid();
	}


	errvoid ILBuilder::eval_lt(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_bool<std::less>(eval_ctx, t_l, t_r)) return pass();
		return errvoid();
	}


	errvoid ILBuilder::eval_le(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_bool<std::less_equal>(eval_ctx, t_l, t_r)) return pass();
		return errvoid();
	}


	errvoid ILBuilder::eval_cast(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_cast(eval_ctx, t_l, t_r)) return pass();
		return errvoid();
	}
	

	errvoid ILBuilder::eval_bitcast(ILEvaluator* eval_ctx, ILDataType t_l, ILDataType t_r) {
		ilsize_t storage = 0;
		if (!eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(t_l), &storage)) return pass();
		if (!eval_ctx->write_register_value_indirect(eval_ctx->compile_time_register_size(t_r), &storage)) return pass();
		return errvoid();
	}

	
}