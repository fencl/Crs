#include "IL.h"
#include "../Error.h"
#include <algorithm>
#include <functional>

namespace Corrosive {
	bool ILBuilder::build_const_ibool (ILBlock* block, int8_t   value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::ibool);  block->write_value(sizeof(int8_t),   (unsigned char*)&value); block->stack.push_back(std::make_pair(true,ILDataType::ibool)); return true; }
	bool ILBuilder::build_const_i8    (ILBlock* block, int8_t   value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::i8);     block->write_value(sizeof(int8_t),   (unsigned char*)&value); block->stack.push_back(std::make_pair(true,ILDataType::i8));    return true; }
	bool ILBuilder::build_const_i16   (ILBlock* block, int16_t  value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::i16);    block->write_value(sizeof(int16_t),  (unsigned char*)&value); block->stack.push_back(std::make_pair(true,ILDataType::i16));   return true; }
	bool ILBuilder::build_const_i32   (ILBlock* block, int32_t  value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::i32);    block->write_value(sizeof(int32_t),  (unsigned char*)&value); block->stack.push_back(std::make_pair(true,ILDataType::i32));   return true; }
	bool ILBuilder::build_const_i64   (ILBlock* block, int64_t  value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::i64);    block->write_value(sizeof(int64_t),  (unsigned char*)&value); block->stack.push_back(std::make_pair(true,ILDataType::i64));   return true; }
	bool ILBuilder::build_const_u8    (ILBlock* block, uint8_t  value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::u8);     block->write_value(sizeof(uint8_t),  (unsigned char*)&value); block->stack.push_back(std::make_pair(true,ILDataType::u8));    return true; }
	bool ILBuilder::build_const_u16   (ILBlock* block, uint16_t value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::u16);    block->write_value(sizeof(uint16_t), (unsigned char*)&value); block->stack.push_back(std::make_pair(true,ILDataType::u16));   return true; }
	bool ILBuilder::build_const_u32   (ILBlock* block, uint32_t value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::u32);    block->write_value(sizeof(uint32_t), (unsigned char*)&value); block->stack.push_back(std::make_pair(true,ILDataType::u32));   return true; }
	bool ILBuilder::build_const_u64   (ILBlock* block, uint64_t value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::u64);    block->write_value(sizeof(uint64_t), (unsigned char*)&value); block->stack.push_back(std::make_pair(true,ILDataType::u64));   return true; }
	bool ILBuilder::build_const_f32   (ILBlock* block, float    value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::f32);    block->write_value(sizeof(float),    (unsigned char*)&value); block->stack.push_back(std::make_pair(true,ILDataType::f32));   return true; }
	bool ILBuilder::build_const_f64   (ILBlock* block, double   value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::f64);    block->write_value(sizeof(double),   (unsigned char*)&value); block->stack.push_back(std::make_pair(true,ILDataType::f64));   return true; }


	bool ILBuilder::build_const_ctype(ILBlock* block, ILCtype value) {
		block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::ctype);   block->write_value(sizeof(ILCtype), (unsigned char*)&value);  block->stack.push_back(std::make_pair(true, ILDataType::ctype)); return true;
	}

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
		}

		return true;
	}

	bool ILBuilder::build_accept(ILBlock* block) {
		block->write_instruction(ILInstruction::accept);
		block->stack.push_back(std::make_pair(false,block->accepts));
		return true;
	}

	bool ILBuilder::build_discard(ILBlock* block) {
		block->write_instruction(ILInstruction::discard);
		return true;
	}

	bool ILBuilder::build_yield(ILBlock* block) {
		block->write_instruction(ILInstruction::yield);
		if (block->stack.size() <1) {
			throw_il_nothing_on_stack_error();
			return false;
		}
		block->yields = block->stack.back().second;
		block->stack.pop_back();
		return true;
	}


	bool ILBuilder::build_yield_type(ILBlock* block,std::string_view name) {
		block->write_instruction(ILInstruction::yield_type);
		if (block->stack.size() < 1) {
			throw_il_nothing_on_stack_error();
			return false;
		}
		block->stack.pop_back();
		block->write_value(sizeof(std::string_view), (unsigned char*) & name);
		return true;
	}

	bool ILBuilder::build_ret(ILBlock* block) {
		block->parent->return_blocks.insert(block);
		block->write_instruction(ILInstruction::ret);
		if (block->stack.size() > 0) {
			throw_il_remaining_stack_error();
			return false;
		}

		return true;
	}

	bool ILBuilder::build_jmp(ILBlock* block, ILBlock* address) {
		address->predecessors.insert(block);
		block->write_instruction(ILInstruction::jmp);
		block->write_value(sizeof(unsigned int), (unsigned char*)&address->id);
		if (block->stack.size() > 0) {
			throw_il_remaining_stack_error();
			return false;
		}

		return true;
	}

	bool ILBuilder::build_jmpz(ILBlock* block, ILBlock* ifz, ILBlock* ifnz) {
		ifz->predecessors.insert(block);
		ifnz->predecessors.insert(block);

		if (block->stack.size() < 1) {
			throw_il_nothing_on_stack_error();
			return false;
		}

		ILDataType t_v = block->stack.back().second;
		block->stack.pop_back();
		if (t_v != ILDataType::ibool) {
			throw_il_wrong_arguments_error();
			return false;
		}

		block->write_instruction(ILInstruction::jmpz);
		block->write_value(sizeof(unsigned int), (unsigned char*)&ifz->id);
		block->write_value(sizeof(unsigned int), (unsigned char*)&ifnz->id);
		if (block->stack.size() > 0) {
			throw_il_remaining_stack_error();
			return false;
		}

		return true;
	}

	bool ILBuilder::build_load(ILBlock* block, ILDataType type) {		
		if (block->stack.size() < 1) {
			throw_il_nothing_on_stack_error();
			return false;
		}

		ILDataType t_v = block->stack.back().second;
		block->stack.pop_back();
		//if (t_v != IRDataType::ptr)
		//	throw_ir_wrong_arguments_error();

		block->write_instruction(ILInstruction::load); 
		block->write_const_type(type);

		block->stack.push_back(std::make_pair(false, type));

		return true;
	}

	bool ILBuilder::build_store(ILBlock* block) {
		if (block->stack.size() < 2) {
			throw_il_nothing_on_stack_error();
			return false;
		}

		ILDataType t_p = block->stack.back().second;
		block->stack.pop_back();

		ILDataType t_v = block->stack.back().second;
		block->stack.pop_back();

		if (t_p != ILDataType::ptr) {
			throw_il_wrong_arguments_error();
			return false;
		}

		block->write_instruction(ILInstruction::store);

		return true;
	}


	bool ILBuilder::build_local(ILBlock* block, unsigned int id) {
		block->stack.push_back(std::make_pair(false, ILDataType::ptr));
		block->write_instruction(ILInstruction::local);
		block->write_value(sizeof(unsigned int), (unsigned char*)&id);
		return true;
	}


	bool ILBuilder::build_member(ILBlock* block, ILStruct* type, unsigned int id) {
		if (block->stack.size() < 1) {
			throw_il_nothing_on_stack_error();
			return false;
		}
		ILDataType t_p = block->stack.back().second;
		block->stack.pop_back();
		if (t_p != ILDataType::ptr) {
			throw_il_wrong_arguments_error();
			return false;
		}

		block->stack.push_back(std::make_pair(false, ILDataType::ptr));
		block->write_instruction(ILInstruction::member);
		block->write_value(sizeof(ILType*), (unsigned char*)&type);
		block->write_value(sizeof(unsigned int), (unsigned char*)&id);

		return true;
	}

	ILDataType ILBuilder::arith_result(ILDataType l, ILDataType r) {
		return std::max(l, r);
	}

	bool ILBuilder::build_add(ILBlock* block) {
		if (block->stack.size() < 2) {
			throw_il_nothing_on_stack_error();
			return false;
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > ILDataType::f64 || t_r.second > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}


		if (t_l.first && t_r.first) {
			if (!_il_builder_const_op<std::plus>(block, t_l.second, t_r.second)) return false;
		}
		else {
			block->write_instruction(ILInstruction::add);
			block->stack.push_back(std::make_pair(false,arith_result(t_l.second, t_r.second)));
		}

		return true;
	}

	bool ILBuilder::build_sub(ILBlock* block) {
		if (block->stack.size() < 2) {
			throw_il_nothing_on_stack_error();
			return false;
		}

		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > ILDataType::f64 || t_r.second > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		if (t_l.first && t_r.first) {
			if (!_il_builder_const_op<std::minus>(block, t_l.second, t_r.second)) return false;
		}
		else {
			block->write_instruction(ILInstruction::sub);
			block->stack.push_back(std::make_pair(false, arith_result(t_l.second, t_r.second)));
		}

		return true;
	}

	bool ILBuilder::build_div(ILBlock* block) {
		if (block->stack.size() < 2) {
			throw_il_nothing_on_stack_error();
			return false;
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > ILDataType::f64 || t_r.second > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}
		if (t_l.first && t_r.first) {
			if (!_il_builder_const_op<std::divides>(block, t_l.second, t_r.second)) return false;
		}
		else {
			block->write_instruction(ILInstruction::div);
			block->stack.push_back(std::make_pair(false, arith_result(t_l.second, t_r.second)));
		}
		return true;
	}

	bool ILBuilder::build_rem(ILBlock* block) {
		if (block->stack.size() < 2) {
			throw_il_nothing_on_stack_error();
			return false;
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > ILDataType::f64 || t_r.second > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}
		if (t_l.first && t_r.first) {
			if (!_il_builder_const_op<std::modulus>(block, t_l.second, t_r.second)) return false;
		}
		else {

			block->write_instruction(ILInstruction::rem);
			block->stack.push_back(std::make_pair(false, arith_result(t_l.second, t_r.second)));
		}

		return true;
	}

	bool ILBuilder::build_and(ILBlock* block) {
		if (block->stack.size() < 2) {
			throw_il_nothing_on_stack_error();
			return false;
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > ILDataType::f64 || t_r.second > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}
		if (t_l.first && t_r.first) {
			if (!_il_builder_const_op<std::bit_and>(block, t_l.second, t_r.second)) return false;
		}
		else {

			block->write_instruction(ILInstruction::o_and);
			block->stack.push_back(std::make_pair(false, arith_result(t_l.second, t_r.second)));
		}
		return true;
	}

	bool ILBuilder::build_or(ILBlock* block) {
		if (block->stack.size() < 2) {
			throw_il_nothing_on_stack_error();
			return false;
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > ILDataType::f64 || t_r.second > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}
		if (t_l.first && t_r.first) {
			if (!_il_builder_const_op<std::bit_or>(block, t_l.second, t_r.second)) return false;
		}
		else {

			block->write_instruction(ILInstruction::o_or);
			block->stack.push_back(std::make_pair(false, arith_result(t_l.second, t_r.second)));
		}
		return true;
	}

	bool ILBuilder::build_xor(ILBlock* block) {
		if (block->stack.size() < 2) {
			throw_il_nothing_on_stack_error();
			return false;
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > ILDataType::f64 || t_r.second > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}
		if (t_l.first && t_r.first) {
			if (!_il_builder_const_op<std::bit_xor>(block, t_l.second, t_r.second)) return false;
		}
		else {

			block->write_instruction(ILInstruction::o_xor);
			block->stack.push_back(std::make_pair(false, arith_result(t_l.second, t_r.second)));
		}
		return true;
	}

	bool ILBuilder::build_mul(ILBlock* block) {
		if (block->stack.size() < 2) {
			throw_il_nothing_on_stack_error();
			return false;
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > ILDataType::f64 || t_r.second > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}
		if (t_l.first && t_r.first) {
			if (!_il_builder_const_op<std::multiplies>(block, t_l.second, t_r.second)) return false;
		}
		else {
			block->write_instruction(ILInstruction::mul);
			block->stack.push_back(std::make_pair(false, arith_result(t_l.second, t_r.second)));
		}
		return true;
	}

	bool ILBuilder::build_eq(ILBlock* block) {
		if (block->stack.size() < 2) {
			throw_il_nothing_on_stack_error();
			return false;
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > ILDataType::f64 || t_r.second > ILDataType::f64) && t_l != t_r) {
			throw_il_wrong_arguments_error();
			return false;
		}

		if (t_l.first && t_r.first) {
			if (!_il_builder_const_op_bool<std::equal_to>(block, t_l.second, t_r.second)) return false;
		}
		else {
			block->write_instruction(ILInstruction::eq);
			block->stack.push_back(std::make_pair(false, ILDataType::ibool));
		}
		return true;
	}
	bool ILBuilder::build_ne(ILBlock* block) {
		if (block->stack.size() < 2) {
			throw_il_nothing_on_stack_error();
			return false;
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > ILDataType::f64 || t_r.second > ILDataType::f64) && t_l != t_r) {
			throw_il_wrong_arguments_error();
			return false;
		}

		if (t_l.first && t_r.first) {
			if (!_il_builder_const_op_bool<std::not_equal_to>(block, t_l.second, t_r.second)) return false;
		}
		else {
			block->write_instruction(ILInstruction::ne);
			block->stack.push_back(std::make_pair(false, ILDataType::ibool));
		}
		return true;
	}
	bool ILBuilder::build_gt(ILBlock* block) {
		if (block->stack.size() < 2) {
			throw_il_nothing_on_stack_error();
			return false;
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > ILDataType::f64 || t_r.second > ILDataType::f64) && t_l != t_r) {
			throw_il_wrong_arguments_error();
			return false;
		}

		if (t_l.first && t_r.first) {
			if (!_il_builder_const_op_bool<std::greater>(block, t_l.second, t_r.second)) return false;
		}
		else {
			block->write_instruction(ILInstruction::gt);
			block->stack.push_back(std::make_pair(false, ILDataType::ibool));
		}
		return true;
	}

	bool ILBuilder::build_ge(ILBlock* block) {
		if (block->stack.size() < 2) {
			throw_il_nothing_on_stack_error();
			return false;
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > ILDataType::f64 || t_r.second > ILDataType::f64) && t_l != t_r) {
			throw_il_wrong_arguments_error();
			return false;
		}

		if (t_l.first && t_r.first) {
			if (!_il_builder_const_op_bool<std::greater_equal>(block, t_l.second, t_r.second)) return false;
		}
		else {
			block->write_instruction(ILInstruction::ge);
			block->stack.push_back(std::make_pair(false, ILDataType::ibool));
		}
		return true;
	}


	bool ILBuilder::build_lt(ILBlock* block) {
		if (block->stack.size() < 2) {
			throw_il_nothing_on_stack_error();
			return false;
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > ILDataType::f64 || t_r.second > ILDataType::f64) && t_l != t_r) {
			throw_il_wrong_arguments_error();
			return false;
		}

		if (t_l.first && t_r.first) {
			if (!_il_builder_const_op_bool<std::less>(block, t_l.second, t_r.second)) return false;
		}
		else {
			block->write_instruction(ILInstruction::lt);
			block->stack.push_back(std::make_pair(false, ILDataType::ibool));
		}
		return true;
	}

	bool ILBuilder::build_le(ILBlock* block) {
		if (block->stack.size() < 2) {
			throw_il_nothing_on_stack_error();
			return false;
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > ILDataType::f64 || t_r.second > ILDataType::f64) && t_l != t_r) {
			throw_il_wrong_arguments_error();
			return false;
		}

		if (t_l.first && t_r.first) {
			if (!_il_builder_const_op_bool<std::less_equal>(block, t_l.second, t_r.second)) return false;
		}
		else {
			block->write_instruction(ILInstruction::le);
			block->stack.push_back(std::make_pair(false, ILDataType::ibool));
		}
		return true;
	}

	
}