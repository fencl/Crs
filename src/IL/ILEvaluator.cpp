#include "IL.hpp"
#include "../Error.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <cstring>
#include <csignal>
#include <climits>

namespace Corrosive {

#ifdef DEBUG
	thread_local errvoid eval_extern_err = err_handler(err_undef_tag());
#else
	thread_local errvoid eval_extern_err = false;
#endif

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

		return err::ok;
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

		return err::ok;
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

		return err::ok;
	}


	errvoid ILBuilder::eval_const_i8(std::int8_t   value) { if (!ILEvaluator::active->write_register_value_indirect(sizeof(std::int8_t), (unsigned char*)&value)) return err::fail; return err::ok; }
	errvoid ILBuilder::eval_const_i16(std::int16_t  value) { if (!ILEvaluator::active->write_register_value_indirect(sizeof(std::int16_t), (unsigned char*)&value)) return err::fail; return err::ok; }
	errvoid ILBuilder::eval_const_i32(std::int32_t  value) { 
		if (!ILEvaluator::active->write_register_value_indirect(sizeof(std::int32_t), (unsigned char*)&value)) return err::fail; 
		return err::ok; 
	}
	errvoid ILBuilder::eval_const_i64(std::int64_t  value) { if (!ILEvaluator::active->write_register_value_indirect(sizeof(std::int64_t), (unsigned char*)&value)) return err::fail; return err::ok; }
	errvoid ILBuilder::eval_const_u8(std::uint8_t  value) { if (!ILEvaluator::active->write_register_value_indirect(sizeof(std::uint8_t), (unsigned char*)&value)) return err::fail; return err::ok; }
	errvoid ILBuilder::eval_const_u16(std::uint16_t value) { if (!ILEvaluator::active->write_register_value_indirect(sizeof(std::uint16_t), (unsigned char*)&value)) return err::fail; return err::ok; }
	errvoid ILBuilder::eval_const_u32(std::uint32_t value) { if (!ILEvaluator::active->write_register_value_indirect(sizeof(std::uint32_t), (unsigned char*)&value)) return err::fail; return err::ok; }
	errvoid ILBuilder::eval_const_u64(std::uint64_t value) { if (!ILEvaluator::active->write_register_value_indirect(sizeof(std::uint64_t), (unsigned char*)&value)) return err::fail; return err::ok; }
	errvoid ILBuilder::eval_const_f32(float    value) { if (!ILEvaluator::active->write_register_value_indirect(sizeof(float), (unsigned char*)&value)) return err::fail; return err::ok; }
	errvoid ILBuilder::eval_const_f64(double   value) { if (!ILEvaluator::active->write_register_value_indirect(sizeof(double), (unsigned char*)&value)) return err::fail; return err::ok; }
	errvoid ILBuilder::eval_const_word(void* value) { if (!ILEvaluator::active->write_register_value_indirect(sizeof(void*), (unsigned char*)&value)) return err::fail; return err::ok; }
	errvoid ILBuilder::eval_const_dword(dword_t value) { if (!ILEvaluator::active->write_register_value_indirect(sizeof(dword_t), (unsigned char*)&value)) return err::fail; return err::ok; }
	errvoid ILBuilder::eval_const_size(std::size_t value) { if (!ILEvaluator::active->write_register_value_indirect(sizeof(std::size_t), (unsigned char*)&value)) return err::fail; return err::ok; }
	errvoid ILBuilder::eval_const_size(ILSize value) { return eval_const_size(value.eval(ILEvaluator::active->parent)); }


	template<typename T> inline errvoid _il_evaluator_value_pop_into(T& r, ILDataType t) {
		switch (t)
		{
			case Corrosive::ILDataType::u8: {
				std::uint8_t v;
				if (!ILEvaluator::active->pop_register_value<std::uint8_t>(v)) return err::fail;
				r = (T)v;
			} break;
			case Corrosive::ILDataType::i8: {
				std::int8_t v;
				if (!ILEvaluator::active->pop_register_value<std::int8_t>(v)) return err::fail;
				r = (T)v;
			} break;

			case Corrosive::ILDataType::u16: {
				std::uint16_t v;
				if (!ILEvaluator::active->pop_register_value<std::uint16_t>(v)) return err::fail;
				r = (T)v;
			} break;
			case Corrosive::ILDataType::i16: {
				std::int16_t v;
				if (!ILEvaluator::active->pop_register_value<std::int16_t>(v)) return err::fail;
				r = (T)v;
			} break;

			case Corrosive::ILDataType::u32: {
				std::uint32_t v;
				if (!ILEvaluator::active->pop_register_value<std::uint32_t>(v)) return err::fail;
				r = (T)v;
			} break;
			case Corrosive::ILDataType::i32: {
				std::int32_t v;
				if (!ILEvaluator::active->pop_register_value<std::int32_t>(v)) return err::fail;
				r = (T)v;
			} break;

			case Corrosive::ILDataType::u64: {
				std::uint64_t v;
				if (!ILEvaluator::active->pop_register_value<std::uint64_t>(v)) return err::fail;
				r = (T)v;
			} break;
			case Corrosive::ILDataType::i64: {
				std::int64_t v;
				if (!ILEvaluator::active->pop_register_value<std::int64_t>(v)) return err::fail;
				r = (T)v;
			} break;
			
			case Corrosive::ILDataType::f32: {
				float v;
				if (!ILEvaluator::active->pop_register_value<float>(v)) return err::fail;
				r = (T)v;
			} break;
			case Corrosive::ILDataType::f64: {
				double v;
				if (!ILEvaluator::active->pop_register_value<double>(v)) return err::fail;
				r = (T)v;
			} break;
			case Corrosive::ILDataType::word: {
				std::size_t v;
				if (!ILEvaluator::active->pop_register_value<std::size_t>(v)) return err::fail;
				r = (T)v;
			} break;

			default: break;
		}

		return err::ok;
	}

	errvoid _il_evaluator_cast(ILDataType l, ILDataType res_t) {
		switch (res_t) {
			case ILDataType::i8: {
				std::int8_t lval;
				if (!_il_evaluator_value_pop_into<std::int8_t>(lval, l)) return err::fail;;
				if (!ILBuilder::eval_const_i8(lval)) return err::fail;
			}break;
			case ILDataType::u8: {
				std::uint8_t lval;
				if(!_il_evaluator_value_pop_into<std::uint8_t>(lval, l)) return err::fail;
				if (!ILBuilder::eval_const_u8(lval)) return err::fail;
			}break;
			case ILDataType::i16: {
				std::int16_t lval;
				if(!_il_evaluator_value_pop_into<std::int16_t>(lval, l)) return err::fail;
				if (!ILBuilder::eval_const_i16(lval)) return err::fail;
			}break;
			case ILDataType::u16: {
				std::uint16_t lval;
				if(!_il_evaluator_value_pop_into<std::uint16_t>(lval, l)) return err::fail;
				if (!ILBuilder::eval_const_u16(lval)) return err::fail;
			}break;
			case ILDataType::i32: {
				std::int32_t lval;
				if(!_il_evaluator_value_pop_into<std::int32_t>(lval, l)) return err::fail;
				if (!ILBuilder::eval_const_i32(lval)) return err::fail;
			}break;
			case ILDataType::u32: {
				std::uint32_t lval;
				if(!_il_evaluator_value_pop_into<std::uint32_t>(lval, l)) return err::fail;
				if (!ILBuilder::eval_const_u32(lval)) return err::fail;
			}break;
			case ILDataType::i64: {
				std::int64_t lval;
				if(!_il_evaluator_value_pop_into<std::int64_t>(lval, l)) return err::fail;
				if (!ILBuilder::eval_const_i64(lval)) return err::fail;
			}break;
			case ILDataType::u64: {
				std::uint64_t lval;
				if(!_il_evaluator_value_pop_into<std::uint64_t>(lval, l)) return err::fail;
				if (!ILBuilder::eval_const_u64(lval)) return err::fail;
			}break;
			case ILDataType::f32: {
				float lval;
				if(!_il_evaluator_value_pop_into<float>(lval, l)) return err::fail;
				if (!ILBuilder::eval_const_f32(lval)) return err::fail;
			}break;
			case ILDataType::f64: {
				double lval;
				if(!_il_evaluator_value_pop_into<double>(lval, l)) return err::fail;
				if (!ILBuilder::eval_const_f64(lval)) return err::fail;
			}break;
			case ILDataType::word: {
				std::size_t lval;
				if(!_il_evaluator_value_pop_into<std::size_t>(lval, l)) return err::fail;
				if (!ILBuilder::eval_const_size(lval)) return err::fail;
			}break;
		}
		return err::ok;
	}

	template< template<typename Ty> class op> errvoid _il_evaluator_const_op(ILDataType l, ILDataType r) {
		ILDataType res_t = ILBuilder::arith_result(l, r);
		switch (res_t) {
			case ILDataType::i8: {
				std::int8_t rval;
				if(!_il_evaluator_value_pop_into<std::int8_t>(rval, r)) return err::fail;
				std::int8_t lval;
				if(!_il_evaluator_value_pop_into<std::int8_t>(lval, l)) return err::fail;
				op<std::int8_t> o;
				if (!ILBuilder::eval_const_i8(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::u8: {
				std::uint8_t rval;
				if(!_il_evaluator_value_pop_into<std::uint8_t>(rval, r)) return err::fail;
				std::uint8_t lval;
				if(!_il_evaluator_value_pop_into<std::uint8_t>(lval, l)) return err::fail;
				op<std::uint8_t> o;
				if (!ILBuilder::eval_const_u8(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::i16: {
				std::int16_t rval;
				if(!_il_evaluator_value_pop_into<std::int16_t>(rval, r)) return err::fail;
				std::int16_t lval;
				if(!_il_evaluator_value_pop_into<std::int16_t>(lval, l)) return err::fail;
				op<std::int16_t> o;
				if (!ILBuilder::eval_const_i16(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::u16: {
				std::uint16_t rval;
				if(!_il_evaluator_value_pop_into<std::uint16_t>(rval, r)) return err::fail;
				std::uint16_t lval;
				if(!_il_evaluator_value_pop_into<std::uint16_t>(lval, l)) return err::fail;
				op<std::uint16_t> o;
				if (!ILBuilder::eval_const_u16(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::i32: {
				std::int32_t rval;
				if(!_il_evaluator_value_pop_into<std::int32_t>(rval, r)) return err::fail;
				std::int32_t lval;
				if(!_il_evaluator_value_pop_into<std::int32_t>(lval, l)) return err::fail;
				op<std::int32_t> o;
				if (!ILBuilder::eval_const_i32(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::u32: {
				std::uint32_t rval;
				if(!_il_evaluator_value_pop_into<std::uint32_t>(rval, r)) return err::fail;
				std::uint32_t lval;
				if(!_il_evaluator_value_pop_into<std::uint32_t>(lval, l)) return err::fail;
				op<std::uint32_t> o;
				if (!ILBuilder::eval_const_u32(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::i64: {
				std::int64_t rval;
				if(!_il_evaluator_value_pop_into<std::int64_t>(rval, r)) return err::fail;
				std::int64_t lval;
				if(!_il_evaluator_value_pop_into<std::int64_t>(lval, l)) return err::fail;
				op<std::int64_t> o;
				if (!ILBuilder::eval_const_i64(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::u64: {
				std::uint64_t rval;
				if(!_il_evaluator_value_pop_into<std::uint64_t>(rval, r)) return err::fail;
				std::uint64_t lval;
				if(!_il_evaluator_value_pop_into<std::uint64_t>(lval, l)) return err::fail;
				op<std::uint64_t> o;
				if (!ILBuilder::eval_const_u64(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::f32: {
				float rval;
				if(!_il_evaluator_value_pop_into<float>(rval, r)) return err::fail;
				float lval;
				if(!_il_evaluator_value_pop_into<float>(lval, l)) return err::fail;
				op<float> o;
				if (!ILBuilder::eval_const_f32(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::f64: {
				double rval;
				if(!_il_evaluator_value_pop_into<double>(rval, r)) return err::fail;
				double lval;
				if(!_il_evaluator_value_pop_into<double>(lval, l)) return err::fail;
				op<double> o;
				if (!ILBuilder::eval_const_f64(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::word: {
				std::size_t rval;
				if(!_il_evaluator_value_pop_into<std::size_t>(rval, r)) return err::fail;
				std::size_t lval;
				if(!_il_evaluator_value_pop_into<std::size_t>(lval, l)) return err::fail;
				op<std::size_t> o;
				if (!ILBuilder::eval_const_size(o(lval, rval))) return err::fail;
			}break;
			default: break;
		}

		return err::ok;
	}

	template< template<typename Ty> class op> errvoid _il_evaluator_const_op_binary(ILDataType l, ILDataType r) {
		ILDataType res_t = ILBuilder::arith_result(l, r);
		switch (res_t) {
			case ILDataType::i8: {
				std::int8_t rval;
				if(!_il_evaluator_value_pop_into<std::int8_t>(rval, r)) return err::fail;
				std::int8_t lval;
				if(!_il_evaluator_value_pop_into<std::int8_t>(lval, l)) return err::fail;
				op<std::int8_t> o;
				if (!ILBuilder::eval_const_i8(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::u8: {
				std::uint8_t rval;
				if(!_il_evaluator_value_pop_into<std::uint8_t>(rval, r)) return err::fail;
				std::uint8_t lval;
				if(!_il_evaluator_value_pop_into<std::uint8_t>(lval, l)) return err::fail;
				op<std::uint8_t> o;
				if (!ILBuilder::eval_const_u8(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::i16: {
				std::int16_t rval;
				if(!_il_evaluator_value_pop_into<std::int16_t>(rval, r)) return err::fail;
				std::int16_t lval;
				if(!_il_evaluator_value_pop_into<std::int16_t>(lval, l)) return err::fail;
				op<std::int16_t> o;
				if (!ILBuilder::eval_const_i16(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::u16: {
				std::uint16_t rval;
				if(!_il_evaluator_value_pop_into<std::uint16_t>(rval, r)) return err::fail;
				std::uint16_t lval;
				if(!_il_evaluator_value_pop_into<std::uint16_t>(lval, l)) return err::fail;
				op<std::uint16_t> o;
				if (!ILBuilder::eval_const_u16(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::i32: {
				std::int32_t rval;
				if(!_il_evaluator_value_pop_into<std::int32_t>(rval, r)) return err::fail;
				std::int32_t lval;
				if(!_il_evaluator_value_pop_into<std::int32_t>(lval, l)) return err::fail;
				op<std::int32_t> o;
				if (!ILBuilder::eval_const_i32(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::u32: {
				std::uint32_t rval;
				if(!_il_evaluator_value_pop_into<std::uint32_t>(rval, r)) return err::fail;
				std::uint32_t lval;
				if(!_il_evaluator_value_pop_into<std::uint32_t>(lval, l)) return err::fail;
				op<std::uint32_t> o;
				if (!ILBuilder::eval_const_u32(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::i64: {
				std::int64_t rval;
				if(!_il_evaluator_value_pop_into<std::int64_t>(rval, r)) return err::fail;
				std::int64_t lval;
				if(!_il_evaluator_value_pop_into<std::int64_t>(lval, l)) return err::fail;
				op<std::int64_t> o;
				if (!ILBuilder::eval_const_i64(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::u64: {
				std::uint64_t rval;
				if(!_il_evaluator_value_pop_into<std::uint64_t>(rval, r)) return err::fail;
				std::uint64_t lval;
				if(!_il_evaluator_value_pop_into<std::uint64_t>(lval, l)) return err::fail;
				op<std::uint64_t> o;
				if (!ILBuilder::eval_const_u64(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::word: {
				std::size_t rval;
				if(!_il_evaluator_value_pop_into<std::size_t>(rval, r)) return err::fail;
				std::size_t lval;
				if(!_il_evaluator_value_pop_into<std::size_t>(lval, l)) return err::fail;
				op<std::size_t> o;
				if (!ILBuilder::eval_const_size(o(lval, rval))) return err::fail;
			}break;
			
			default: break;
		}

		return err::ok;
	}

	template< template<typename Ty> class op> errvoid _il_evaluator_const_op_bool(ILDataType l, ILDataType r) {
		ILDataType res_t = ILBuilder::arith_result(l, r);
		switch (res_t) {
			case ILDataType::i8: {
				std::int8_t rval;
				if(!_il_evaluator_value_pop_into<std::int8_t>(rval, r)) return err::fail;
				std::int8_t lval;
				if(!_il_evaluator_value_pop_into<std::int8_t>(lval, l)) return err::fail;
				op<std::int8_t> o;
				if (!ILBuilder::eval_const_i8(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::u8: {
				std::uint8_t rval;
				if(!_il_evaluator_value_pop_into<std::uint8_t>(rval, r)) return err::fail;
				std::uint8_t lval;
				if(!_il_evaluator_value_pop_into<std::uint8_t>(lval, l)) return err::fail;
				op<std::uint8_t> o;
				if (!ILBuilder::eval_const_i8(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::i16: {
				std::int16_t rval;
				if(!_il_evaluator_value_pop_into<std::int16_t>(rval, r)) return err::fail;
				std::int16_t lval;
				if(!_il_evaluator_value_pop_into<std::int16_t>(lval, l)) return err::fail;
				op<std::int16_t> o;
				if (!ILBuilder::eval_const_i8(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::u16: {
				std::uint16_t rval;
				if(!_il_evaluator_value_pop_into<std::uint16_t>(rval, r)) return err::fail;
				std::uint16_t lval;
				if(!_il_evaluator_value_pop_into<std::uint16_t>(lval, l)) return err::fail;
				op<std::uint16_t> o;
				if (!ILBuilder::eval_const_i8(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::i32: {
				std::int32_t rval;
				if(!_il_evaluator_value_pop_into<std::int32_t>(rval, r)) return err::fail;
				std::int32_t lval;
				if(!_il_evaluator_value_pop_into<std::int32_t>(lval, l)) return err::fail;
				op<std::int32_t> o;
				if (!ILBuilder::eval_const_i8(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::u32: {
				std::uint32_t rval;
				if(!_il_evaluator_value_pop_into<std::uint32_t>(rval, r)) return err::fail;
				std::uint32_t lval;
				if(!_il_evaluator_value_pop_into<std::uint32_t>(lval, l)) return err::fail;
				op<std::uint32_t> o;
				if (!ILBuilder::eval_const_i8(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::i64: {
				std::int64_t rval;
				if(!_il_evaluator_value_pop_into<std::int64_t>(rval, r)) return err::fail;
				std::int64_t lval;
				if(!_il_evaluator_value_pop_into<std::int64_t>(lval, l)) return err::fail;
				op<std::int64_t> o;
				if (!ILBuilder::eval_const_i8(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::u64: {
				std::uint64_t rval;
				if(!_il_evaluator_value_pop_into<std::uint64_t>(rval, r)) return err::fail;
				std::uint64_t lval;
				if(!_il_evaluator_value_pop_into<std::uint64_t>(lval, l)) return err::fail;
				op<std::uint64_t> o;
				if (!ILBuilder::eval_const_i8(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::f32: {
				float rval;
				if(!_il_evaluator_value_pop_into<float>(rval, r)) return err::fail;
				float lval;
				if(!_il_evaluator_value_pop_into<float>(lval, l)) return err::fail;
				op<float> o;
				if (!ILBuilder::eval_const_i8(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::f64: {
				double rval;
				if(!_il_evaluator_value_pop_into<double>(rval, r)) return err::fail;
				double lval;
				if(!_il_evaluator_value_pop_into<double>(lval, l)) return err::fail;
				op<double> o;
				if (!ILBuilder::eval_const_i8(o(lval, rval))) return err::fail;
			}break;
			case ILDataType::word: {
				std::size_t rval;
				if(!_il_evaluator_value_pop_into<std::size_t>(rval, r)) return err::fail;
				std::size_t lval;
				if(!_il_evaluator_value_pop_into<std::size_t>(lval, l)) return err::fail;
				op<std::size_t> o;
				if (!ILBuilder::eval_const_i8(o(lval, rval))) return err::fail;
			}break;
			default: break;
		}
		
		return err::ok;
	}



	errvoid ILBuilder::eval_forget(ILDataType type) {
		if (type != ILDataType::none) {
			ilsize_t storage;
			if (!ILEvaluator::active->pop_register_value_indirect(ILEvaluator::active->compile_time_register_size(type), &storage)) return err::fail;
		}
		return err::ok;
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


	errvoid ILBuilder::eval_discard(ILDataType type) {
		return err::ok;
	}

	errvoid ILBuilder::eval_yield(ILDataType type) {
		if (!ILEvaluator::active->pop_register_value_indirect(ILEvaluator::active->compile_time_register_size(type), &ILEvaluator::active->yield_storage)) return err::fail;
		return err::ok;
	}

	errvoid ILBuilder::eval_accept(ILDataType type) {
		if (!ILEvaluator::active->write_register_value_indirect(ILEvaluator::active->compile_time_register_size(type), &ILEvaluator::active->yield_storage)) return err::fail;
		return err::ok;
	}

	errvoid ILBuilder::eval_load(ILDataType type) {
		void* ptr;
		if (!ILEvaluator::active->pop_register_value<void*>(ptr)) return err::fail;	
		if (!ILEvaluator::active->write_register_value_indirect(ILEvaluator::active->compile_time_register_size(type), ptr)) return err::fail;
		return err::ok;
	}

	errvoid ILBuilder::eval_offset(ILSize offset) {
		if (offset.type == ILSizeType::table || offset.type == ILSizeType::array || offset.value > 0) {
			std::size_t mem;
			if (!ILEvaluator::active->pop_register_value<std::size_t>(mem)) return err::fail;
			mem += offset.eval(ILEvaluator::active->parent);
			ILEvaluator::active->write_register_value(mem);
		}
		return err::ok;
	}



	errvoid ILBuilder::eval_aoffset(std::uint32_t offset) {
		if (offset > 0) {
			std::size_t mem;
			if(!ILEvaluator::active->pop_register_value(mem)) return err::fail;
			mem += offset;
			ILEvaluator::active->write_register_value(mem);
		}
		return err::ok;
	}


	errvoid ILBuilder::eval_woffset(std::uint32_t offset) {
		if (offset > 0) {
			std::size_t mem;
			if(!ILEvaluator::active->pop_register_value(mem)) return err::fail;
			mem += offset * ILEvaluator::active->compile_time_register_size(ILDataType::word);
			ILEvaluator::active->write_register_value(mem);
		}
		return err::ok;
	}

	errvoid ILBuilder::eval_constref(std::uint32_t cid) {
		ILEvaluator::active->write_register_value(ILEvaluator::active->parent->constant_memory[cid].second.get());
		return err::ok;
	}
	
	errvoid ILBuilder::eval_const_slice(std::uint32_t cid, ILSize s) {
		dword_t val((void*)ILEvaluator::active->parent->constant_memory[cid].second.get() , (void*)(s.eval(ILEvaluator::active->parent)));
		ILEvaluator::active->write_register_value(val);
		return err::ok;
	}

	errvoid ILBuilder::eval_staticref(std::uint32_t cid) {
		ILEvaluator::active->write_register_value(std::get<1>(ILEvaluator::active->parent->static_memory[cid]).get());
		return err::ok;
	}


	errvoid ILBuilder::eval_combine_dword() {
		void* p2;
		if (!ILEvaluator::active->pop_register_value<void*>(p2)) return err::fail;
		void* p1;
		if (!ILEvaluator::active->pop_register_value<void*>(p1)) return err::fail;
		dword_t dw(p1,p2);
		ILEvaluator::active->write_register_value(dw);
		return err::ok;
	}
	
	errvoid ILBuilder::eval_split_dword() {
		dword_t dw;
		if (!ILEvaluator::active->pop_register_value<dword_t>(dw)) return err::fail;
		ILEvaluator::active->write_register_value(dw.p1);
		ILEvaluator::active->write_register_value(dw.p2);
		return err::ok;
	}

	errvoid ILBuilder::eval_high_word() {
		dword_t dw;
		if (!ILEvaluator::active->pop_register_value<dword_t>(dw)) return err::fail;
		ILEvaluator::active->write_register_value(dw.p2);
		return err::ok;
	}
	
	errvoid ILBuilder::eval_low_word() {
		dword_t dw;
		if (!ILEvaluator::active->pop_register_value<dword_t>(dw)) return err::fail;
		ILEvaluator::active->write_register_value(dw.p1);
		return err::ok;
	}

	errvoid ILBuilder::eval_debug(std::uint16_t file, std::uint16_t line) {
		ILEvaluator::active->debug_file = file;
		ILEvaluator::active->debug_line = line;
		return err::ok;
	}
	
	errvoid ILBuilder::eval_extract(ILSize s) {
		std::size_t off;
		if (!ILEvaluator::active->pop_register_value<std::size_t>(off)) return err::fail;
		std::uint8_t* ptr;
		if (!ILEvaluator::active->pop_register_value<std::uint8_t*>(ptr)) return err::fail;
		ptr+= off * s.eval(ILEvaluator::active->parent);
		ILEvaluator::active->write_register_value(ptr);		
		return err::ok;
	}

	errvoid ILBuilder::eval_cut(ILSize s) {
		std::size_t off1, off2;
		if (!ILEvaluator::active->pop_register_value<std::size_t>(off2)) return err::fail;
		if (!ILEvaluator::active->pop_register_value<std::size_t>(off1)) return err::fail;
		std::uint8_t* ptr;
		if (!ILEvaluator::active->pop_register_value<std::uint8_t*>(ptr)) return err::fail;
		std::size_t elem_size = s.eval(ILEvaluator::active->parent);
		ptr += off1 * elem_size;

		dword_t val;
		val.p1 = ptr;
		val.p2 = (void*)(elem_size*(off2-off1+1));

		ILEvaluator::active->write_register_value(val);
		return err::ok;
	}

	errvoid ILBuilder::eval_rtoffset() {
		std::size_t offset;
		if(!ILEvaluator::active->pop_register_value(offset)) return err::fail;
		unsigned char* mem;
		if(!ILEvaluator::active->pop_register_value(mem)) return err::fail;
		mem += offset;
		ILEvaluator::active->write_register_value(mem);
		return err::ok;
	}

	errvoid ILBuilder::eval_rtoffset_rev() {
		unsigned char* mem;
			if(!ILEvaluator::active->pop_register_value(mem)) return err::fail;
		std::size_t offset;
			if(!ILEvaluator::active->pop_register_value(offset)) return err::fail;
		mem += offset;
		ILEvaluator::active->write_register_value(mem);
		return err::ok;
	}

	errvoid ILBuilder::eval_roffset(ILDataType from, ILDataType to, ILSize offset) {
		ilsize_t mem;
		if (!ILEvaluator::active->pop_register_value_indirect(ILEvaluator::active->compile_time_register_size(from), &mem)) return err::fail;
		mem = mem << offset.eval(ILEvaluator::active->parent);
		if (!ILEvaluator::active->write_register_value_indirect(ILEvaluator::active->compile_time_register_size(to), &mem)) return err::fail;
		return err::ok;
	}

	errvoid ILBuilder::eval_aroffset(ILDataType from, ILDataType to, std::uint32_t offset) {

		if (offset > UINT8_MAX) {
			throw string_exception("Compiler error: aroffset > 255 should not be possible");
		}

		ilsize_t mem;
		if (!ILEvaluator::active->pop_register_value_indirect(ILEvaluator::active->compile_time_register_size(from), &mem)) return err::fail;
		mem = mem << offset;
		if (!ILEvaluator::active->write_register_value_indirect(ILEvaluator::active->compile_time_register_size(to), &mem)) return err::fail;
		return err::ok;
	}

	errvoid ILBuilder::eval_wroffset(ILDataType from, ILDataType to, std::uint32_t offset) {

		if (offset > UINT8_MAX) {
			throw string_exception("Compiler error: wroffset > 255 should not be possible");
		}

		ilsize_t mem;
		if (!ILEvaluator::active->pop_register_value_indirect(ILEvaluator::active->compile_time_register_size(from), &mem)) return err::fail;
		mem = mem << (offset * ILEvaluator::active->compile_time_register_size(ILDataType::word));
		if (!ILEvaluator::active->write_register_value_indirect(ILEvaluator::active->compile_time_register_size(to), &mem)) return err::fail;
		return err::ok;
	}


	errvoid ILBuilder::eval_store(ILDataType type) {
		void* mem;
		if(!ILEvaluator::active->pop_register_value<void*>(mem)) return err::fail;
		if (!ILEvaluator::active->pop_register_value_indirect(ILEvaluator::active->compile_time_register_size(type), mem)) return err::fail;
		return err::ok;
	}

	errvoid ILBuilder::eval_store_rev(ILDataType type) {
		ilsize_t storage;
		std::size_t regs = ILEvaluator::active->compile_time_register_size(type);
		if (!ILEvaluator::active->pop_register_value_indirect(regs, &storage)) return err::fail;
		void* mem;
		if(!ILEvaluator::active->pop_register_value<void*>(mem)) return err::fail;

		if (!wrap || wrap(sandbox) == 0) {	
			std::memcpy(mem, &storage, regs);
		}
		else {
			return throw_runtime_handler_exception(ILEvaluator::active);
		}
		return err::ok;
	}

	errvoid ILBuilder::eval_duplicate(ILDataType type) {
		std::size_t reg_s = ILEvaluator::active->compile_time_register_size(type);
		void* lv = ILEvaluator::active->read_last_register_value_indirect(type);
		if (!ILEvaluator::active->write_register_value_indirect(reg_s, lv)) return err::fail;
		return err::ok;
	}

	errvoid ILBuilder::eval_clone(ILDataType type, std::uint16_t times) {
		std::size_t reg_s = ILEvaluator::active->compile_time_register_size(type);
		void* lv = ILEvaluator::active->read_last_register_value_indirect(type);
		for (std::uint16_t i = 0; i < times; ++i) {
			if (!ILEvaluator::active->write_register_value_indirect(reg_s, lv)) return err::fail;
		}
		return err::ok;
	}

	errvoid ILBuilder::eval_fnptr(ILFunction* fun) {
		void* wrtptr;
		if (!wrap || wrap(sandbox) == 0) {
			if (auto native_fun = dynamic_cast<ILNativeFunction*>(fun)) {
				wrtptr = native_fun->ptr;
			}
			else {
				wrtptr = fun;
			}
		} else {
			return throw_runtime_handler_exception(ILEvaluator::active);
		}

		ILEvaluator::active->write_register_value(wrtptr);
		return err::ok;
	}


	errvoid ILBuilder::eval_fncall(ILFunction* fun) {
		uint32_t declid;

		if (!wrap || wrap(sandbox) == 0) {
			declid = fun->decl_id;
			if (auto native_fun = dynamic_cast<ILNativeFunction*>(fun)) {				
				ILEvaluator::active->callstack.push_back(native_fun->ptr);
			}
			else {
				ILEvaluator::active->callstack.push_back(fun);
			}
		} else {
			return throw_runtime_handler_exception(ILEvaluator::active);
		}

		return eval_call(declid);
	}

	errvoid ILBuilder::eval_null() {
		ILEvaluator::active->write_register_value((std::size_t)0);
		return err::ok;
	}

	errvoid ILBuilder::eval_isnotzero(ILDataType type) {
		ilsize_t z = 0;
		if (!ILEvaluator::active->pop_register_value_indirect(ILEvaluator::active->compile_time_register_size(type), &z)) return err::fail;
		eval_const_i8((z == 0 ? 0 : 1));
		return err::ok;
	}


	errvoid ILBuilder::eval_callstart() {
		ILFunction* func;
		if(!ILEvaluator::active->pop_register_value<ILFunction*>(func)) return err::fail;

		ILEvaluator::active->callstack.push_back(func);
		return err::ok;
	}

	errvoid ILBuilder::eval_insintric(ILInsintric fun) {
		if (!ILEvaluator::active->parent->insintric_function[(unsigned char)fun](ILEvaluator::active)) return err::ok;
		return err::ok;
	}

	errvoid ILBuilder::eval_vtable(std::uint32_t id) {
		ILEvaluator::active->write_register_value(ILEvaluator::active->parent->vtable_data[id].second.get());
		return err::ok;
	}


	errvoid ILBuilder::eval_memcpy(ILSize size) {

		void* dst;
			if(!ILEvaluator::active->pop_register_value(dst)) return err::fail;
		void* src;
			if(!ILEvaluator::active->pop_register_value(src)) return err::fail;

		if (!wrap || wrap(sandbox) == 0) {
			std::memcpy(dst, src, size.eval(ILEvaluator::active->parent));
		}
		else {
			return throw_runtime_handler_exception(ILEvaluator::active);
		}
		return err::ok;
	}

	errvoid ILBuilder::eval_memcpy_rev(ILSize size) {

		void* src;
			if(!ILEvaluator::active->pop_register_value(src)) return err::fail;
		void* dst;
			if(!ILEvaluator::active->pop_register_value(dst)) return err::fail;

		if (!wrap || wrap(sandbox) == 0) {
			std::memcpy(dst, src, size.eval(ILEvaluator::active->parent));
		}
		else {
			return throw_runtime_handler_exception(ILEvaluator::active);
		}
		return err::ok;
	}


	errvoid ILBuilder::eval_memcmp(ILSize size) {

		void* dst;
			if(!ILEvaluator::active->pop_register_value(dst)) return err::fail;
		void* src;
			if(!ILEvaluator::active->pop_register_value(src)) return err::fail;

		if (!wrap || wrap(sandbox) == 0) {
			ILEvaluator::active->write_register_value((std::int8_t)memcmp(dst, src, size.eval(ILEvaluator::active->parent)));
		}
		else {
			return throw_runtime_handler_exception(ILEvaluator::active);
		}
		return err::ok;
	}

	errvoid ILBuilder::eval_memcmp_rev(ILSize size) {

		void* src;
			if(!ILEvaluator::active->pop_register_value(src)) return err::fail;
		void* dst;
			if(!ILEvaluator::active->pop_register_value(dst)) return err::fail;

		if (!wrap || wrap(sandbox) == 0) {
			ILEvaluator::active->write_register_value((std::int8_t)memcmp(dst, src, size.eval(ILEvaluator::active->parent)));
		}
		else {
			return throw_runtime_handler_exception(ILEvaluator::active);
		}
		return err::ok;
	}

	errvoid ILBuilder::eval_tableoffset(tableid_t tableid, tableelement_t itemid) {
		auto& table = ILEvaluator::active->parent->structure_tables[tableid];
		table.calculate(ILEvaluator::active->parent);
		unsigned char* ptr;
			if(!ILEvaluator::active->pop_register_value(ptr)) return err::fail;
		ptr += table.calculated_offsets[itemid];
		ILEvaluator::active->write_register_value(ptr);
		return err::ok;
	}


	errvoid ILBuilder::eval_tableroffset(ILDataType src, ILDataType dst, tableid_t tableid, tableelement_t itemid) {
		auto& table = ILEvaluator::active->parent->structure_tables[tableid];
		table.calculate(ILEvaluator::active->parent);
		ilsize_t storage;
		if (!ILEvaluator::active->pop_register_value_indirect(ILEvaluator::active->compile_time_register_size(src), &storage)) return err::fail;
		storage = storage >> table.calculated_offsets[itemid];
		if (!ILEvaluator::active->write_register_value_indirect(ILEvaluator::active->compile_time_register_size(dst), &storage)) return err::fail;
		return err::ok;
	}

	errvoid ILBuilder::eval_rmemcmp(ILDataType type) {
		std::size_t reg_v = ILEvaluator::active->compile_time_register_size(type);
		ilsize_t s1, s2;
		if (!ILEvaluator::active->pop_register_value_indirect(reg_v, &s1)) return err::fail;
		if (!ILEvaluator::active->pop_register_value_indirect(reg_v, &s2)) return err::fail;
		std::uint8_t res;
		if (!wrap || wrap(sandbox) == 0) {
			res = (std::int8_t)memcmp(&s1, &s2, reg_v);
		}
		else {
			return throw_runtime_handler_exception(ILEvaluator::active);
		}

		ILEvaluator::active->write_register_value(res);
		return err::ok;
	}

	errvoid ILBuilder::eval_rmemcmp_rev(ILDataType type) {
		std::size_t reg_v = ILEvaluator::active->compile_time_register_size(type);
		ilsize_t s1, s2;
		if (!ILEvaluator::active->pop_register_value_indirect(reg_v, &s2)) return err::fail;
		if (!ILEvaluator::active->pop_register_value_indirect(reg_v, &s1)) return err::fail;
		std::uint8_t res;
		if (!wrap || wrap(sandbox) == 0) {
			res = (std::int8_t)memcmp(&s1, &s2, reg_v);
		}
		else {
			return throw_runtime_handler_exception(ILEvaluator::active);
		}
		ILEvaluator::active->write_register_value(res);
		return err::ok;
	}


	errvoid ILBuilder::eval_swap(ILDataType type) {
		std::size_t reg_v = ILEvaluator::active->compile_time_register_size(type);
		ilsize_t s1, s2;
		if (!ILEvaluator::active->pop_register_value_indirect(reg_v, &s1)) return err::fail;
		if (!ILEvaluator::active->pop_register_value_indirect(reg_v, &s2)) return err::fail;
		if (!ILEvaluator::active->write_register_value_indirect(reg_v, &s1)) return err::fail;
		if (!ILEvaluator::active->write_register_value_indirect(reg_v, &s2)) return err::fail;
		return err::ok;
	}



	errvoid ILBuilder::eval_negative(ILDataType type) {

		switch (type) {
			case ILDataType::u8:
			case ILDataType::i8: {
				std::int8_t v;
				if (!ILEvaluator::active->pop_register_value<std::int8_t>(v)) return err::fail;
				ILEvaluator::active->write_register_value(-v);
			} break;
			case ILDataType::u16:
			case ILDataType::i16:{
				std::int16_t v;
				if (!ILEvaluator::active->pop_register_value<std::int16_t>(v)) return err::fail;
				ILEvaluator::active->write_register_value(-v);
			} break;
			case ILDataType::u32:
			case ILDataType::i32:{
				std::int32_t v;
				if (!ILEvaluator::active->pop_register_value<std::int32_t>(v)) return err::fail;
				ILEvaluator::active->write_register_value(-v);
			} break;
			case ILDataType::u64:
			case ILDataType::i64:{
				std::int64_t v;
				if (!ILEvaluator::active->pop_register_value<std::int64_t>(v)) return err::fail;
				ILEvaluator::active->write_register_value(-v);
			} break;
			case ILDataType::f32:{
				float v;
				if (!ILEvaluator::active->pop_register_value<float>(v)) return err::fail;
				ILEvaluator::active->write_register_value(-v);
			} break;
			case ILDataType::f64:{
				double v;
				if (!ILEvaluator::active->pop_register_value<double>(v)) return err::fail;
				ILEvaluator::active->write_register_value(-v);
			} break;
				break;
			case ILDataType::word:{
				std::size_t v;
				if (!ILEvaluator::active->pop_register_value<std::size_t>(v)) return err::fail;
				ILEvaluator::active->write_register_value((SIZE_MAX ^ v) - 1);
			} break;
		}
		return err::ok;
	}


	errvoid ILBuilder::eval_negate() {
		std::uint8_t val;
		if (!ILEvaluator::active->pop_register_value<std::uint8_t>(val)) return err::fail;

		if (val) {
			ILEvaluator::active->write_register_value<std::uint8_t>(0);
		}
		else {
			ILEvaluator::active->write_register_value<std::uint8_t>(1);
		}
		return err::ok;
	}



	errvoid ILBuilder::eval_swap2(ILDataType type1, ILDataType type2) {
		std::size_t reg_v_1 = ILEvaluator::active->compile_time_register_size(type1);
		std::size_t reg_v_2 = ILEvaluator::active->compile_time_register_size(type2);
		ilsize_t s1, s2;
		if (!ILEvaluator::active->pop_register_value_indirect(reg_v_2, &s2)) return err::fail;
		if (!ILEvaluator::active->pop_register_value_indirect(reg_v_1, &s1)) return err::fail;
		if (!ILEvaluator::active->write_register_value_indirect(reg_v_2, &s2)) return err::fail;
		if (!ILEvaluator::active->write_register_value_indirect(reg_v_1, &s1)) return err::fail;
		return err::ok;
	}

	errvoid ILBuilder::eval_call(std::uint32_t decl) {
		ILEvaluator::active->debug_callstack.push_back(std::make_tuple(ILEvaluator::active->debug_line, ILEvaluator::active->debug_file, "<native function>"));

		if (!wrap || wrap(sandbox) == 0) {
			auto& declaration = ILEvaluator::active->parent->function_decl[decl];

			void* ptr = ILEvaluator::active->callstack.back();
			ILEvaluator::active->callstack.pop_back();

			if (std::get<0>(declaration) == ILCallingConvention::bytecode) {
				ILBytecodeFunction* bytecode_fun = (ILBytecodeFunction*)ptr;
				std::get<2>(ILEvaluator::active->debug_callstack.back()) = bytecode_fun->name;
				//ILEvaluator::active->callstack_debug.push_back(std::make_tuple(ILEvaluator::active->debug_line, ILEvaluator::active->debug_file, std::string_view(bytecode_fun->alias)));

				bytecode_fun->calculate_stack();

				bool running = true;

				unsigned char* lstack_base = ILEvaluator::active->stack_base;
				unsigned char* lstack_base_aligned = ILEvaluator::active->stack_base_aligned;
				unsigned char* lstack_pointer = ILEvaluator::active->stack_pointer;

				ILEvaluator::active->stack_base = ILEvaluator::active->stack_pointer;
				ILEvaluator::active->stack_base_aligned = (unsigned char*)align_up((std::size_t)ILEvaluator::active->stack_base, bytecode_fun->calculated_local_stack_alignment);
				ILEvaluator::active->stack_pointer = ILEvaluator::active->stack_base_aligned + bytecode_fun->calculated_local_stack_size;

				ILEvaluator::exec_function(bytecode_fun);				

				//ILEvaluator::active->callstack_debug.pop_back();
				ILEvaluator::active->stack_pointer = lstack_pointer;
				ILEvaluator::active->stack_base_aligned = lstack_base_aligned;
				ILEvaluator::active->stack_base = lstack_base;

			}
			else {
				eval_extern_err = true;
				//ILEvaluator::active->callstack_debug.push_back(std::make_tuple(ILEvaluator::active->debug_line, ILEvaluator::active->debug_file, "External code"));
				if (!abi_dynamic_call(ILEvaluator::active, std::get<0>(declaration), ptr, declaration)) return err::fail;
				//ILEvaluator::active->callstack_debug.pop_back();
				if (!eval_extern_err) return err::fail;
			}

			if (ILEvaluator::active->debug_callstack.empty()) {
				return throw_runtime_exception(ILEvaluator::active, "Compiler error, please fix debug callstack");
			}

			ILEvaluator::active->debug_line = std::get<0>(ILEvaluator::active->debug_callstack.back());
			ILEvaluator::active->debug_file = std::get<1>(ILEvaluator::active->debug_callstack.back());
			ILEvaluator::active->debug_callstack.pop_back();

		}
		else {
			return throw_runtime_handler_exception(ILEvaluator::active);
		}

		
		return err::ok;
	}


	errvoid ILBuilder::eval_add(ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op<std::plus>(t_l, t_r)) return err::fail;
		return err::ok;
	}

	errvoid ILBuilder::eval_sub(ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op<std::minus>(t_l, t_r)) return err::fail;
		return err::ok;
	}


	errvoid ILBuilder::eval_div(ILDataType t_l, ILDataType t_r) {
		if (!wrap || wrap(sandbox) == 0) {
			if (!_il_evaluator_const_op<std::divides>(t_l, t_r)) return err::fail;
		} else {
			return throw_runtime_exception(ILEvaluator::active, "Division operation failed");
		}
		return err::ok;
	}


	errvoid ILBuilder::eval_rem(ILDataType t_l, ILDataType t_r) {
		if (!wrap || wrap(sandbox) == 0) {
			if (!_il_evaluator_const_op_binary<std::modulus>(t_l, t_r)) return err::fail;
		} else {
			return throw_runtime_exception(ILEvaluator::active, "Division operation failed");
		}
		return err::ok;
	}


	errvoid ILBuilder::eval_and(ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_binary<std::bit_and>(t_l, t_r)) return err::fail;
		return err::ok;
	}


	errvoid ILBuilder::eval_or(ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_binary<std::bit_or>(t_l, t_r)) return err::fail;
		return err::ok;
	}


	errvoid ILBuilder::eval_xor(ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_binary<std::bit_xor>(t_l, t_r)) return err::fail;
		return err::ok;
	}


	errvoid ILBuilder::eval_mul(ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op<std::multiplies>(t_l, t_r)) return err::fail;
		return err::ok;
	}


	errvoid ILBuilder::eval_eq(ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_bool<std::equal_to>(t_l, t_r)) return err::fail;
		return err::ok;
	}


	errvoid ILBuilder::eval_ne(ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_bool<std::not_equal_to>(t_l, t_r)) return err::fail;
		return err::ok;
	}


	errvoid ILBuilder::eval_gt(ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_bool<std::greater>(t_l, t_r)) return err::fail;
		return err::ok;
	}

	errvoid ILBuilder::eval_ge(ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_bool<std::greater_equal>(t_l, t_r)) return err::fail;
		return err::ok;
	}


	errvoid ILBuilder::eval_lt(ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_bool<std::less>(t_l, t_r)) return err::fail;
		return err::ok;
	}


	errvoid ILBuilder::eval_le(ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_const_op_bool<std::less_equal>(t_l, t_r)) return err::fail;
		return err::ok;
	}


	errvoid ILBuilder::eval_cast(ILDataType t_l, ILDataType t_r) {
		if (!_il_evaluator_cast(t_l, t_r)) return err::fail;
		return err::ok;
	}
	

	errvoid ILBuilder::eval_bitcast(ILDataType t_l, ILDataType t_r) {
		ilsize_t storage = 0;
		if (!ILEvaluator::active->pop_register_value_indirect(ILEvaluator::active->compile_time_register_size(t_l), &storage)) return err::fail;
		if (!ILEvaluator::active->write_register_value_indirect(ILEvaluator::active->compile_time_register_size(t_r), &storage)) return err::fail;
		return err::ok;
	}

	
}