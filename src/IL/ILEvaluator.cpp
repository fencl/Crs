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
	bool ILBuilder::eval_const_ptr   (ILEvaluator* eval_ctx, void*    value) { eval_ctx->write_register_value_indirect(sizeof(void*),    (unsigned char*)&value); return true; }


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
		if (into!=nullptr)
			memcpy(into, register_stack_pointer, size);
	}

	bool ILBuilder::eval_accept(ILEvaluator* eval_ctx, ILDataType type) {
		eval_ctx->write_register_value_indirect(eval_ctx->compile_time_register_size(type), (unsigned char*)&eval_ctx->yield);
		return true;
	}

	bool ILBuilder::eval_discard(ILEvaluator* eval_ctx, ILDataType type) {
		return true;
	}

	bool ILBuilder::eval_yield(ILEvaluator* eval_ctx, ILDataType type) {
		eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(type), (unsigned char*)&eval_ctx->yield);
		return true;
	}

	bool ILBuilder::eval_forget(ILEvaluator* eval_ctx, ILDataType type) {
		eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(type), nullptr);
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
			case Corrosive::ILDataType::ptr: 
				return sizeof(void*);
			case Corrosive::ILDataType::none:
				return 0;
			case Corrosive::ILDataType::undefined:
				return 0;
			default:
				return 0;
		}
	}

	bool ILBuilder::eval_load(ILEvaluator* eval_ctx, ILDataType type) {
		unsigned char* ptr = eval_ctx->pop_register_value<unsigned char*>();
		eval_ctx->write_register_value_indirect(eval_ctx->compile_time_register_size(type), ptr);
		return true;
	}

	bool ILBuilder::eval_member(ILEvaluator* eval_ctx, uint16_t offset) {
		if (offset > 0) {
			unsigned char* mem = eval_ctx->pop_register_value<unsigned char*>();
			mem += offset;
			eval_ctx->write_register_value(mem);
		}
		return true;
	}


	bool ILBuilder::eval_store(ILEvaluator* eval_ctx, ILDataType type) {
		void* mem = eval_ctx->pop_register_value<unsigned char*>();
		eval_ctx->pop_register_value_indirect(eval_ctx->compile_time_register_size(type), mem);
		return true;
	}


	std::pair<unsigned char*, unsigned char*> ILEvaluator::stack_push() {
		std::pair<unsigned char*, unsigned char*> res = std::make_pair(memory_stack_base_pointer, memory_stack_pointer);
		memory_stack_base_pointer = memory_stack_pointer;
		return res;
	}

	void ILEvaluator::stack_pop(std::pair<unsigned char*, unsigned char*> stack_state) {
		memory_stack_base_pointer = stack_state.first;
		memory_stack_pointer = stack_state.second;
	}

	bool ILBuilder::eval_local(ILEvaluator* eval_ctx, uint32_t offset) {
		eval_const_ptr(eval_ctx, eval_ctx->memory_stack_base_pointer + offset);
		return true;
	}

	bool ILBuilder::eval_fnptr(ILEvaluator* eval_ctx, ILFunction* fun) {
		eval_ctx->write_register_value(fun);
		return true;
	}


	bool ILBuilder::eval_callstart(ILEvaluator* eval_ctx) {
		ILFunction* func = eval_ctx->pop_register_value<ILFunction*>();
		eval_ctx->callstack[eval_ctx->callstack_depth] = func;
		eval_ctx->callstack_depth++;
		return true;
	}

	bool ILBuilder::eval_priv(ILEvaluator* eval_ctx, uint8_t fun,uint32_t data) {
		return eval_ctx->private_fun[fun](eval_ctx,data);
	}

#define read_data_type(T) ((T*)block->read_data(sizeof(T),mempool,memoff))
	bool ILBuilder::eval_call(ILEvaluator* eval_ctx, ILDataType rett, uint16_t argc) {
		
		ILFunction* fun = eval_ctx->callstack[eval_ctx->callstack_depth - 1];

		ILBlock* block = fun->blocks[0];
		bool running = true;

		unsigned char* lstack_base = eval_ctx->memory_stack_base_pointer;
		unsigned char* lstack = eval_ctx->memory_stack_pointer;

		eval_ctx->memory_stack_base_pointer = eval_ctx->memory_stack_pointer;
		eval_ctx->memory_stack_pointer += fun->compile_time_stack;

		while (true) {
			std::list<std::unique_ptr<ILBlockData>>::iterator mempool = block->data_pool.begin();
			size_t memoff = 0;
			while (mempool != block->data_pool.end()) {

				auto inst = read_data_type(ILInstruction);

				switch (*inst) {
					case ILInstruction::ret: {
						auto type = read_data_type(ILDataType);
						running = false;
						goto returned;
					} break;
					case ILInstruction::call: {
						auto type = read_data_type(ILDataType);
						auto argc = read_data_type(uint16_t);
						if (!eval_call(eval_ctx, *type, *argc)) return false;
					} break;
					case ILInstruction::fnptr: {
						auto id = read_data_type(uint32_t);
						eval_ctx->write_register_value(eval_ctx->parent->functions[*id].get());
					} break;
					case ILInstruction::priv: {
						auto id = read_data_type(uint8_t);
						auto data = read_data_type(uint32_t);
						if (!eval_priv(eval_ctx, *id,*data)) return false;
					} break;
					case ILInstruction::sub: {
						auto left = read_data_type(ILDataType);
						auto right = read_data_type(ILDataType);
						if (!eval_sub(eval_ctx, *left, *right)) return false;
					} break;
					case ILInstruction::div: {
						auto left = read_data_type(ILDataType);
						auto right = read_data_type(ILDataType);
						if (!eval_div(eval_ctx, *left, *right)) return false;
					} break;
					case ILInstruction::rem: {
						auto left = read_data_type(ILDataType);
						auto right = read_data_type(ILDataType);
						if (!eval_rem(eval_ctx, *left, *right)) return false;
					} break;
					case ILInstruction::mul: {
						auto left = read_data_type(ILDataType);
						auto right = read_data_type(ILDataType);
						if (!eval_mul(eval_ctx, *left, *right)) return false;
					} break;
					case ILInstruction::add: {
						auto left = read_data_type(ILDataType);
						auto right = read_data_type(ILDataType);
						if (!eval_add(eval_ctx, *left, *right)) return false;
					} break;
					case ILInstruction::bit_and: {
						auto left = read_data_type(ILDataType);
						auto right = read_data_type(ILDataType);
						if (!eval_and(eval_ctx, *left, *right)) return false;
					} break;
					case ILInstruction::bit_or: {
						auto left = read_data_type(ILDataType);
						auto right = read_data_type(ILDataType);
						if (!eval_or(eval_ctx, *left, *right)) return false;
					} break;
					case ILInstruction::bit_xor: {
						auto left = read_data_type(ILDataType);
						auto right = read_data_type(ILDataType);
						if (!eval_xor(eval_ctx, *left, *right)) return false;
					} break;
					case ILInstruction::eq: {
						auto left = read_data_type(ILDataType);
						auto right = read_data_type(ILDataType);
						if (!eval_eq(eval_ctx, *left, *right)) return false;
					} break;
					case ILInstruction::ne: {
						auto left = read_data_type(ILDataType);
						auto right = read_data_type(ILDataType);
						if (!eval_ne(eval_ctx, *left, *right)) return false;
					} break;
					case ILInstruction::gt: {
						auto left = read_data_type(ILDataType);
						auto right = read_data_type(ILDataType);
						if (!eval_gt(eval_ctx, *left, *right)) return false;
					} break;
					case ILInstruction::lt: {
						auto left = read_data_type(ILDataType);
						auto right = read_data_type(ILDataType);
						if (!eval_lt(eval_ctx, *left, *right)) return false;
					} break;
					case ILInstruction::ge: {
						auto left = read_data_type(ILDataType);
						auto right = read_data_type(ILDataType);
						if (!eval_ge(eval_ctx, *left, *right)) return false;
					} break;
					case ILInstruction::le: {
						auto left = read_data_type(ILDataType);
						auto right = read_data_type(ILDataType);
						if (!eval_le(eval_ctx, *left, *right)) return false;
					} break;
					case ILInstruction::store: {
						auto type = read_data_type(ILDataType);
						if (!eval_store(eval_ctx, *type)) return false;
					} break;
					case ILInstruction::start: {
						if (!eval_callstart(eval_ctx)) return false;
					} break;
					case ILInstruction::accept: {
						auto type = read_data_type(ILDataType);
						if (!eval_accept(eval_ctx, *type)) return false;
					} break;
					case ILInstruction::discard: {
						auto type = read_data_type(ILDataType);
						if (!eval_discard(eval_ctx, *type)) return false;
					} break;
					case ILInstruction::jmp: {
						auto address = read_data_type(uint32_t);
						block = block->parent->blocks[*address];
						goto next_block;
					}break;
					case ILInstruction::member2: {
						auto offset = read_data_type(uint16_t);
						auto compile_offset = read_data_type(uint16_t);
						if (!eval_member(eval_ctx, *compile_offset)) return false;
					} break;
					case ILInstruction::local: {
						auto id = read_data_type(uint16_t);
						auto& offsetdata = fun->local_offsets[*id];
						if (!eval_local(eval_ctx, offsetdata.first)) return false;
					} break;
					case ILInstruction::member: {
						auto offset = read_data_type(uint16_t);
						if (!eval_member(eval_ctx, *offset)) return false;
					} break;
					case ILInstruction::load: {
						auto type = read_data_type(ILDataType);
						if (!eval_load(eval_ctx, *type)) return false;
					} break;
					case ILInstruction::forget: {
						auto type = read_data_type(ILDataType);
						if (!eval_forget(eval_ctx, *type)) return false;
					} break;
					case ILInstruction::jmpz: {

						auto addressz = read_data_type(uint32_t);
						auto addressnz = read_data_type(uint32_t);
						int8_t z = eval_ctx->pop_register_value<int8_t>();
						if (z) {
							block = block->parent->blocks[*addressnz];
						}
						else {
							block = block->parent->blocks[*addressz];
						}

						goto next_block;
					} break;
					case ILInstruction::yield: {
						auto type = read_data_type(ILDataType);
						if (!eval_yield(eval_ctx, *type)) return false;
					} break;
					case ILInstruction::value: {
						
						auto type = read_data_type(ILDataType);

						switch (*type) {
							case ILDataType::ibool:  eval_const_ibool(eval_ctx,*read_data_type(int8_t)); break;
							case ILDataType::u8:  eval_const_u8(eval_ctx,*read_data_type(uint8_t)); break;
							case ILDataType::u16: eval_const_u16(eval_ctx,*read_data_type(uint16_t)); break;
							case ILDataType::u32: eval_const_u32(eval_ctx,*read_data_type(uint32_t)); break;
							case ILDataType::u64: eval_const_u64(eval_ctx,*read_data_type(uint64_t)); break;
							case ILDataType::i8:  eval_const_i8(eval_ctx,*read_data_type(int8_t)); break;
							case ILDataType::i16: eval_const_i16(eval_ctx,*read_data_type(int16_t)); break;
							case ILDataType::i32: eval_const_i32(eval_ctx,*read_data_type(int32_t)); break;
							case ILDataType::i64: eval_const_i64(eval_ctx,*read_data_type(int64_t)); break;
							case ILDataType::f32: eval_const_f32(eval_ctx, *read_data_type(float)); break;
							case ILDataType::f64: eval_const_f64(eval_ctx, *read_data_type(double)); break;
							case ILDataType::type: eval_const_type(eval_ctx, *read_data_type(void*)); break;
							case ILDataType::ptr: eval_const_ptr(eval_ctx, *read_data_type(void*)); break;
						}
					} break;
				}
			}

		next_block:
			continue;
		}

		

		throw_il_wrong_data_flow_error();
		return false;
		
		
	returned:

		eval_ctx->memory_stack_base_pointer = lstack_base;
		eval_ctx->memory_stack_pointer = lstack;
		eval_ctx->callstack_depth--;
		return true;
	}

#undef read_data_type

	unsigned char* ILEvaluator::stack_reserve(size_t size) {
		unsigned char* res = memory_stack_pointer;
		memory_stack_pointer += size;
		return res;
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