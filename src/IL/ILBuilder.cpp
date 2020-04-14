#include "IL.h"
#include "../Error.h"
#include <algorithm>
#include <functional>

namespace Corrosive {
	bool ILBuilder::build_const_ibool (ILBlock* block, int8_t   value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::ibool);  block->write_value(sizeof(int8_t),   (unsigned char*)&value); block->push_const(true); return true; }
	bool ILBuilder::build_const_i8    (ILBlock* block, int8_t   value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::i8);     block->write_value(sizeof(int8_t),   (unsigned char*)&value); block->push_const(true); return true; }
	bool ILBuilder::build_const_i16   (ILBlock* block, int16_t  value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::i16);    block->write_value(sizeof(int16_t),  (unsigned char*)&value); block->push_const(true); return true; }
	bool ILBuilder::build_const_i32   (ILBlock* block, int32_t  value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::i32);    block->write_value(sizeof(int32_t),  (unsigned char*)&value); block->push_const(true); return true; }
	bool ILBuilder::build_const_i64   (ILBlock* block, int64_t  value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::i64);    block->write_value(sizeof(int64_t),  (unsigned char*)&value); block->push_const(true); return true; }
	bool ILBuilder::build_const_u8    (ILBlock* block, uint8_t  value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::u8);     block->write_value(sizeof(uint8_t),  (unsigned char*)&value); block->push_const(true); return true; }
	bool ILBuilder::build_const_u16   (ILBlock* block, uint16_t value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::u16);    block->write_value(sizeof(uint16_t), (unsigned char*)&value); block->push_const(true); return true; }
	bool ILBuilder::build_const_u32   (ILBlock* block, uint32_t value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::u32);    block->write_value(sizeof(uint32_t), (unsigned char*)&value); block->push_const(true); return true; }
	bool ILBuilder::build_const_u64   (ILBlock* block, uint64_t value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::u64);    block->write_value(sizeof(uint64_t), (unsigned char*)&value); block->push_const(true); return true; }
	bool ILBuilder::build_const_f32   (ILBlock* block, float    value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::f32);    block->write_value(sizeof(float),    (unsigned char*)&value); block->push_const(true); return true; }
	bool ILBuilder::build_const_f64   (ILBlock* block, double   value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::f64);    block->write_value(sizeof(double),   (unsigned char*)&value); block->push_const(true); return true; }
	bool ILBuilder::build_const_type   (ILBlock* block, void*   value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::type);   block->write_value(sizeof(void*),    (unsigned char*)&value); block->push_const(true); return true; }


	template<typename T> inline T _il_block_value_pop_into(ILBlock* block, ILDataType t) {
		switch (t)
		{
		case Corrosive::ILDataType::ibool:
			return (T)block->pop<uint8_t>();
		case Corrosive::ILDataType::u8:
			return (T)block->pop<uint8_t>();
		case Corrosive::ILDataType::i8:
			return (T)block->pop<int8_t>();
		case Corrosive::ILDataType::u16:
			return (T)block->pop<uint16_t>();
		case Corrosive::ILDataType::i16:
			return (T)block->pop<int16_t>();
		case Corrosive::ILDataType::u32:
			return (T)block->pop<uint32_t>();
		case Corrosive::ILDataType::i32:
			return (T)block->pop<int32_t>();
		case Corrosive::ILDataType::u64:
			return (T)block->pop<uint64_t>();
		case Corrosive::ILDataType::i64:
			return (T)block->pop<int64_t>();
		case Corrosive::ILDataType::f32:
			return (T)block->pop<float>();
		case Corrosive::ILDataType::f64:
			return (T)block->pop<double>();
		case Corrosive::ILDataType::ptr:
			return (T)block->pop<int64_t>();
		case Corrosive::ILDataType::none:
			return 0;
		case Corrosive::ILDataType::undefined:
			break;
		default:
			break;
		}

		return 0;
	}

	template< template<typename Ty> class op> bool _il_builder_const_op_int(ILBlock* block, ILDataType l, ILDataType r) {
		ILDataType res_t = ILBuilder::arith_result(l, r);
		switch (res_t) {
			case ILDataType::i8: {
					int8_t rval = _il_block_value_pop_into<int8_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					int8_t lval = _il_block_value_pop_into<int8_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<int8_t> o;
					if (!ILBuilder::build_const_i8(block, o(lval, rval))) return false;
				}break;
			case ILDataType::u8: {
					uint8_t rval = _il_block_value_pop_into<uint8_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					uint8_t lval = _il_block_value_pop_into<uint8_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<uint8_t> o;
					if (!ILBuilder::build_const_u8(block, o(lval, rval))) return false;
				}break;
			case ILDataType::ibool: {
					uint8_t rval = _il_block_value_pop_into<uint8_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					uint8_t lval = _il_block_value_pop_into<uint8_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<uint8_t> o;
					if (!ILBuilder::build_const_ibool(block, o(lval, rval))) return false;
				}break;
			case ILDataType::i16: {
					int16_t rval = _il_block_value_pop_into<int16_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					int16_t lval = _il_block_value_pop_into<int16_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<int16_t> o;
					if (!ILBuilder::build_const_i16(block, o(lval, rval))) return false;
				}break;
			case ILDataType::u16: {
					uint16_t rval = _il_block_value_pop_into<uint16_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					uint16_t lval = _il_block_value_pop_into<uint16_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<uint16_t> o;
					if (!ILBuilder::build_const_u16(block, o(lval, rval))) return false;
				}break;
			case ILDataType::i32: {
					int32_t rval = _il_block_value_pop_into<int32_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					int32_t lval = _il_block_value_pop_into<int32_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<int32_t> o;
					if (!ILBuilder::build_const_i32(block, o(lval, rval))) return false;
				}break;
			case ILDataType::u32: {
					uint32_t rval = _il_block_value_pop_into<uint32_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					uint32_t lval = _il_block_value_pop_into<uint32_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<uint32_t> o;
					if (!ILBuilder::build_const_u32(block, o(lval, rval))) return false;
				}break;
			case ILDataType::i64: {
					int64_t rval = _il_block_value_pop_into<int64_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					int64_t lval = _il_block_value_pop_into<int64_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<int64_t> o;
					if (!ILBuilder::build_const_i64(block, o(lval, rval))) return false;
				}break;
			case ILDataType::u64: {
					uint64_t rval = _il_block_value_pop_into<uint64_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					uint64_t lval = _il_block_value_pop_into<uint64_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<uint64_t> o;
					if (!ILBuilder::build_const_u64(block, o(lval, rval))) return false;
				}break;
		}

		return true;
	}

	template< template<typename Ty> class op> bool _il_builder_const_op(ILBlock* block, ILDataType l, ILDataType r) {
		ILDataType res_t = ILBuilder::arith_result(l, r);
		switch (res_t) {
			case ILDataType::i8: {
					int8_t rval = _il_block_value_pop_into<int8_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					int8_t lval = _il_block_value_pop_into<int8_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<int8_t> o;
					if (!ILBuilder::build_const_i8(block, o(lval, rval))) return false;
				}break;
			case ILDataType::u8: {
					uint8_t rval = _il_block_value_pop_into<uint8_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					uint8_t lval = _il_block_value_pop_into<uint8_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<uint8_t> o;
					if (!ILBuilder::build_const_u8(block, o(lval, rval))) return false;
				}break;
			case ILDataType::ibool: {
					uint8_t rval = _il_block_value_pop_into<uint8_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					uint8_t lval = _il_block_value_pop_into<uint8_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<uint8_t> o;
					if (!ILBuilder::build_const_ibool(block, o(lval, rval))) return false;
				}break;
			case ILDataType::i16: {
					int16_t rval = _il_block_value_pop_into<int16_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					int16_t lval = _il_block_value_pop_into<int16_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<int16_t> o;
					if (!ILBuilder::build_const_i16(block, o(lval, rval))) return false;
				}break;
			case ILDataType::u16: {
					uint16_t rval = _il_block_value_pop_into<uint16_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					uint16_t lval = _il_block_value_pop_into<uint16_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<uint16_t> o;
					if (!ILBuilder::build_const_u16(block, o(lval, rval))) return false;
				}break;
			case ILDataType::i32: {
					int32_t rval = _il_block_value_pop_into<int32_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					int32_t lval = _il_block_value_pop_into<int32_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<int32_t> o;
					if (!ILBuilder::build_const_i32(block, o(lval, rval))) return false;
				}break;
			case ILDataType::u32: {
					uint32_t rval = _il_block_value_pop_into<uint32_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					uint32_t lval = _il_block_value_pop_into<uint32_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<uint32_t> o;
					if (!ILBuilder::build_const_u32(block, o(lval, rval))) return false;
				}break;
			case ILDataType::i64: {
					int64_t rval = _il_block_value_pop_into<int64_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					int64_t lval = _il_block_value_pop_into<int64_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<int64_t> o;
					if (!ILBuilder::build_const_i64(block, o(lval, rval))) return false;
				}break;
			case ILDataType::u64: {
					uint64_t rval = _il_block_value_pop_into<uint64_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					uint64_t lval = _il_block_value_pop_into<uint64_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<uint64_t> o;
					if (!ILBuilder::build_const_u64(block, o(lval, rval))) return false;
				}break;
			case ILDataType::f32: {
					float rval = _il_block_value_pop_into<float>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					float lval = _il_block_value_pop_into<float>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<float> o;
					if (!ILBuilder::build_const_f32(block, o(lval, rval))) return false;
				}break;
			case ILDataType::f64: {
					double rval = _il_block_value_pop_into<double>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					double lval = _il_block_value_pop_into<double>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<double> o;
					if (!ILBuilder::build_const_f64(block, o(lval, rval))) return false;
				}break;
		}

		return true;
	}


	template< template<typename Ty> class op> bool _il_builder_const_op_bool(ILBlock* block, ILDataType l, ILDataType r) {
		ILDataType res_t = ILBuilder::arith_result(l, r);

		switch (res_t) {
			case ILDataType::i8: {
					int8_t rval = _il_block_value_pop_into<int8_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					int8_t lval = _il_block_value_pop_into<int8_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<int8_t> o;
					if (!ILBuilder::build_const_ibool(block, (uint8_t)o(lval, rval))) return false;
				}break;
			case ILDataType::u8: {
					uint8_t rval = _il_block_value_pop_into<uint8_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					uint8_t lval = _il_block_value_pop_into<uint8_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<uint8_t> o;
					if (!ILBuilder::build_const_ibool(block, (uint8_t)o(lval, rval))) return false;
				}break;
			case ILDataType::ibool: {
					uint8_t rval = _il_block_value_pop_into<uint8_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					uint8_t lval = _il_block_value_pop_into<uint8_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<uint8_t> o;
					if (!ILBuilder::build_const_ibool(block, (uint8_t)o(lval, rval))) return false;
				}break;
			case ILDataType::i16: {
					int16_t rval = _il_block_value_pop_into<int16_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					int16_t lval = _il_block_value_pop_into<int16_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<int16_t> o;
					if (!ILBuilder::build_const_ibool(block, (uint8_t)o(lval, rval))) return false;
				}break;
			case ILDataType::u16: {
					uint16_t rval = _il_block_value_pop_into<uint16_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					uint16_t lval = _il_block_value_pop_into<uint16_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<uint16_t> o;
					if (!ILBuilder::build_const_ibool(block, (uint8_t)o(lval, rval))) return false;
				}break;
			case ILDataType::i32: {
					int32_t rval = _il_block_value_pop_into<int32_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					int32_t lval = _il_block_value_pop_into<int32_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<int32_t> o;
					if (!ILBuilder::build_const_ibool(block, (uint8_t)o(lval, rval))) return false;
				}break;
			case ILDataType::u32: {
					uint32_t rval = _il_block_value_pop_into<uint32_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					uint32_t lval = _il_block_value_pop_into<uint32_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<uint32_t> o;
					if (!ILBuilder::build_const_ibool(block, (uint8_t)o(lval, rval))) return false;
				}break;
			case ILDataType::i64: {
					int64_t rval = _il_block_value_pop_into<int64_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					int64_t lval = _il_block_value_pop_into<int64_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<int64_t> o;
					if (!ILBuilder::build_const_ibool(block, (uint8_t)o(lval, rval))) return false;
				}break;
			case ILDataType::u64: {
					uint64_t rval = _il_block_value_pop_into<uint64_t>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					uint64_t lval = _il_block_value_pop_into<uint64_t>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<uint64_t> o;
					if (!ILBuilder::build_const_ibool(block, (uint8_t)o(lval, rval))) return false;
				}break;

			case ILDataType::f32: {
					float rval = _il_block_value_pop_into<float>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					float lval = _il_block_value_pop_into<float>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<float> o;
					if (!ILBuilder::build_const_ibool(block, (uint8_t)o(lval, rval))) return false;
				}break;
			case ILDataType::f64: {
					double rval = _il_block_value_pop_into<double>(block, r);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					double lval = _il_block_value_pop_into<double>(block, l);
					block->pop<ILDataType>();
					block->pop<ILInstruction>();
					op<double> o;
					if (!ILBuilder::build_const_ibool(block, (uint8_t)o(lval, rval))) return false;
				}break;
		}

		return true;
	}

	bool ILBuilder::build_accept(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::accept); block->push_const(false);
		block->write_const_type(type);
		block->accepts = type;
		return true;
	}

	bool ILBuilder::build_discard(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::discard); block->push_const(false);
		block->write_const_type(type);
		block->accepts = type;
		return true;
	}

	bool ILBuilder::build_yield(ILBlock* block,ILDataType type) {
		block->write_instruction(ILInstruction::yield); block->pop_const();
		block->write_const_type(type);
		block->yields = type;
		return true;
	}

	bool ILBuilder::build_forget(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::forget); block->pop_const();
		block->write_const_type(type);
		return true;
	}

	bool ILBuilder::build_ret(ILBlock* block, ILDataType type) {
		block->parent->return_blocks.insert(block);
		block->write_instruction(ILInstruction::ret);
		block->write_const_type(type);
		block->yields = type;
		return true;
	}

	bool ILBuilder::build_fnptr(ILBlock* block, ILFunction* fun) {
		block->parent->return_blocks.insert(block);
		block->write_instruction(ILInstruction::fnptr);
		block->write_value(sizeof(uint32_t), (unsigned char*)&(fun->id));
		block->push_const(false);
		return true;
	}

	bool ILBuilder::build_call(ILBlock* block, ILDataType type,uint16_t argc) {
		block->parent->return_blocks.insert(block);
		block->write_instruction(ILInstruction::call);
		block->write_const_type(type);
		block->write_value(sizeof(uint16_t), (unsigned char*)&argc);
		for (uint16_t i=0;i<argc;i++)
			block->pop_const();
		block->push_const(false);
		return true;
	}

	bool ILBuilder::build_jmp(ILBlock* block, ILBlock* address) {
		address->predecessors.insert(block);
		block->write_instruction(ILInstruction::jmp);
		block->write_value(sizeof(uint32_t), (unsigned char*)&address->id);

		return true;
	}

	bool ILBuilder::build_jmpz(ILBlock* block, ILBlock* ifz, ILBlock* ifnz) {
		ifz->predecessors.insert(block);
		ifnz->predecessors.insert(block);

		block->write_instruction(ILInstruction::jmpz);
		block->write_value(sizeof(uint32_t), (unsigned char*)&ifz->id);
		block->write_value(sizeof(uint32_t), (unsigned char*)&ifnz->id);
		block->pop_const();
		
		return true;
	}

	bool ILBuilder::build_load(ILBlock* block, ILDataType type) {

		block->write_instruction(ILInstruction::load); 
		block->write_const_type(type);
		block->push_const(false);
		return true;
	}

	bool ILBuilder::build_member2(ILBlock* block, uint16_t compile_offset, uint16_t offset) {
		if (offset > 0) {
			block->write_instruction(ILInstruction::member2);
			block->write_value(sizeof(uint16_t), (unsigned char*)&offset);
			block->write_value(sizeof(uint16_t), (unsigned char*)&compile_offset);
			block->push_const(false);
		}
		return true;
	}

	bool ILBuilder::build_member(ILBlock* block, uint16_t offset) {
		if (offset > 0) {
			block->write_instruction(ILInstruction::member);
			block->write_value(sizeof(uint16_t), (unsigned char*)&offset);
			block->push_const(false);
		}
		return true;
	}

	bool ILBuilder::build_store(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::store);
		block->write_const_type(type);
		block->pop_const();
		return true;
	}


	bool ILBuilder::build_local(ILBlock* block, uint16_t id) {
		block->write_instruction(ILInstruction::local);
		block->write_value(sizeof(uint16_t), (unsigned char*)&id);
		block->push_const(false);
		return true;
	}


	bool ILBuilder::build_priv(ILBlock* block, uint8_t fun) {
		block->write_instruction(ILInstruction::priv);
		block->write_value(sizeof(uint8_t), (unsigned char*)&fun);
		return true;
	}

	bool ILBuilder::build_callstart(ILBlock* block) {
		block->write_instruction(ILInstruction::start);
		return true;
	}

	ILDataType ILBuilder::arith_result(ILDataType l, ILDataType r) {
		return std::max(l, r);
	}

	bool ILBuilder::build_add(ILBlock* block, ILDataType tl, ILDataType tr) {
		
		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		if (block->test_const()) {
			block->pop_const();
			block->pop_const();
			if (!_il_builder_const_op<std::plus>(block, tl, tr)) return false;
		}
		else {
			block->write_instruction(ILInstruction::add);
			block->write_const_type(tl);
			block->write_const_type(tr);
			block->push_const(false);
		}

		return true;
	}

	bool ILBuilder::build_sub(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		if (block->test_const()) {
			block->pop_const();
			block->pop_const();
			if (!_il_builder_const_op<std::minus>(block, tl, tr)) return false;
		}
		else {
			block->write_instruction(ILInstruction::sub);
			block->write_const_type(tl);
			block->write_const_type(tr);
			block->push_const(false);
		}

		return true;
	}

	bool ILBuilder::build_div(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		if (block->test_const()) {
			block->pop_const();
			block->pop_const();
			if (!_il_builder_const_op<std::divides>(block, tl, tr)) return false;
		}
		else {
			block->write_instruction(ILInstruction::div);
			block->write_const_type(tl);
			block->write_const_type(tr);
			block->push_const(false);
		}

		return true;
	}

	bool ILBuilder::build_rem(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		if (block->test_const()) {
			block->pop_const();
			block->pop_const();
			if (!_il_builder_const_op_int<std::modulus>(block, tl, tr)) return false;
		}
		else {
			block->write_instruction(ILInstruction::rem);
			block->write_const_type(tl);
			block->write_const_type(tr);
			block->push_const(false);
		}

		return true;
	}

	bool ILBuilder::build_and(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		if (block->test_const()) {
			block->pop_const();
			block->pop_const();
			if (!_il_builder_const_op_int<std::bit_and>(block, tl, tr)) return false;
		}
		else {
			block->write_instruction(ILInstruction::bit_and);
			block->write_const_type(tl);
			block->write_const_type(tr);
			block->push_const(false);
		}

		return true;
	}

	bool ILBuilder::build_or(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		if (block->test_const()) {
			block->pop_const();
			block->pop_const();
			if (!_il_builder_const_op_int<std::bit_or>(block, tl, tr)) return false;
		}
		else {
			block->write_instruction(ILInstruction::bit_or);
			block->write_const_type(tl);
			block->write_const_type(tr);
			block->push_const(false);
		}

		return true;
	}

	bool ILBuilder::build_xor(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		if (block->test_const()) {
			block->pop_const();
			block->pop_const();
			if (!_il_builder_const_op_int<std::bit_xor>(block, tl, tr)) return false;
		}
		else {
			block->write_instruction(ILInstruction::bit_xor);
			block->write_const_type(tl);
			block->write_const_type(tr);
			block->push_const(false);
		}

		return true;
	}

	bool ILBuilder::build_mul(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		if (block->test_const()) {
			block->pop_const();
			block->pop_const();
			if (!_il_builder_const_op<std::multiplies>(block, tl, tr)) return false;
		}
		else {
			block->write_instruction(ILInstruction::mul);
			block->write_const_type(tl);
			block->write_const_type(tr);
			block->push_const(false);
		}

		return true;
	}

	bool ILBuilder::build_eq(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		if (block->test_const()) {
			block->pop_const();
			block->pop_const();
			if (!_il_builder_const_op_bool<std::equal_to>(block, tl, tr)) return false;
		}
		else {
			block->write_instruction(ILInstruction::eq);
			block->write_const_type(tl);
			block->write_const_type(tr);
			block->push_const(false);
		}

		return true;
	}


	bool ILBuilder::build_ne(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		if (block->test_const()) {
			block->pop_const();
			block->pop_const();
			if (!_il_builder_const_op_bool<std::not_equal_to>(block, tl, tr)) return false;
		}
		else {
			block->write_instruction(ILInstruction::ne);
			block->write_const_type(tl);
			block->write_const_type(tr);
			block->push_const(false);
		}

		return true;
	}

	bool ILBuilder::build_gt(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		if (block->test_const()) {
			block->pop_const();
			block->pop_const();
			if (!_il_builder_const_op_bool<std::greater>(block, tl, tr)) return false;
		}
		else {
			block->write_instruction(ILInstruction::gt);
			block->write_const_type(tl);
			block->write_const_type(tr);
			block->push_const(false);
		}

		return true;
	}


	bool ILBuilder::build_ge(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		if (block->test_const()) {
			block->pop_const();
			block->pop_const();
			if (!_il_builder_const_op_bool<std::greater_equal>(block, tl, tr)) return false;
		}
		else {
			block->write_instruction(ILInstruction::ge);
			block->write_const_type(tl);
			block->write_const_type(tr);
			block->push_const(false);
		}

		return true;
	}



	bool ILBuilder::build_lt(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		if (block->test_const()) {
			block->pop_const();
			block->pop_const();
			if (!_il_builder_const_op_bool<std::less>(block, tl, tr)) return false;
		}
		else {
			block->write_instruction(ILInstruction::lt);
			block->write_const_type(tl);
			block->write_const_type(tr);
			block->push_const(false);
		}

		return true;
	}


	bool ILBuilder::build_le(ILBlock* block, ILDataType tl, ILDataType tr) {
		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		if (block->test_const()) {
			block->pop_const();
			block->pop_const();
			if (!_il_builder_const_op_bool<std::less_equal>(block, tl, tr)) return false;
		}
		else {
			block->write_instruction(ILInstruction::le);
			block->write_const_type(tl);
			block->write_const_type(tr);
			block->push_const(false);
		}

		return true;
	}

	
}