#include "IR.h"
#include "../Error.h"
#include <algorithm>
#include <functional>

namespace Corrosive {
	void IRBuilder::build_const_ibool (IRBlock* block, int8_t   value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::ibool);  block->write_value(sizeof(int8_t),   (unsigned char*)&value); block->stack.push_back(std::make_pair(true,IRDataType::ibool)); }
	void IRBuilder::build_const_i8    (IRBlock* block, int8_t   value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::i8);     block->write_value(sizeof(int8_t),   (unsigned char*)&value); block->stack.push_back(std::make_pair(true,IRDataType::i8));    }
	void IRBuilder::build_const_i16   (IRBlock* block, int16_t  value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::i16);    block->write_value(sizeof(int16_t),  (unsigned char*)&value); block->stack.push_back(std::make_pair(true,IRDataType::i16));   }
	void IRBuilder::build_const_i32   (IRBlock* block, int32_t  value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::i32);    block->write_value(sizeof(int32_t),  (unsigned char*)&value); block->stack.push_back(std::make_pair(true,IRDataType::i32));   }
	void IRBuilder::build_const_i64   (IRBlock* block, int64_t  value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::i64);    block->write_value(sizeof(int64_t),  (unsigned char*)&value); block->stack.push_back(std::make_pair(true,IRDataType::i64));   }
	void IRBuilder::build_const_u8    (IRBlock* block, uint8_t  value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::u8);     block->write_value(sizeof(uint8_t),  (unsigned char*)&value); block->stack.push_back(std::make_pair(true,IRDataType::u8));    }
	void IRBuilder::build_const_u16   (IRBlock* block, uint16_t value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::u16);    block->write_value(sizeof(uint16_t), (unsigned char*)&value); block->stack.push_back(std::make_pair(true,IRDataType::u16));   }
	void IRBuilder::build_const_u32   (IRBlock* block, uint32_t value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::u32);    block->write_value(sizeof(uint32_t), (unsigned char*)&value); block->stack.push_back(std::make_pair(true,IRDataType::u32));   }
	void IRBuilder::build_const_u64   (IRBlock* block, uint64_t value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::u64);    block->write_value(sizeof(uint64_t), (unsigned char*)&value); block->stack.push_back(std::make_pair(true,IRDataType::u64));   }
	void IRBuilder::build_const_f32   (IRBlock* block, float    value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::f32);    block->write_value(sizeof(float),    (unsigned char*)&value); block->stack.push_back(std::make_pair(true,IRDataType::f32));   }
	void IRBuilder::build_const_f64   (IRBlock* block, double   value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::f64);    block->write_value(sizeof(double),   (unsigned char*)&value); block->stack.push_back(std::make_pair(true,IRDataType::f64));   }


	template<typename T> inline T block_pop_into(IRBlock* block, IRDataType t) {
		switch (t)
		{
		case Corrosive::IRDataType::ibool:
			return (T)block->pop<uint8_t>();
		case Corrosive::IRDataType::u8:
			return (T)block->pop<uint8_t>();
		case Corrosive::IRDataType::i8:
			return (T)block->pop<int8_t>();
		case Corrosive::IRDataType::u16:
			return (T)block->pop<uint16_t>();
		case Corrosive::IRDataType::i16:
			return (T)block->pop<int16_t>();
		case Corrosive::IRDataType::u32:
			return (T)block->pop<uint32_t>();
		case Corrosive::IRDataType::i32:
			return (T)block->pop<int32_t>();
		case Corrosive::IRDataType::u64:
			return (T)block->pop<uint64_t>();
		case Corrosive::IRDataType::i64:
			return (T)block->pop<int64_t>();
		case Corrosive::IRDataType::f32:
			return (T)block->pop<float>();
		case Corrosive::IRDataType::f64:
			return (T)block->pop<double>();
		case Corrosive::IRDataType::ptr:
			return (T)block->pop<int64_t>();
		case Corrosive::IRDataType::none:
			return 0;
		case Corrosive::IRDataType::undefined:
			break;
		default:
			break;
		}

		return 0;
	}

	template< template<typename Ty> class op> void _ir_uilder_const_op(IRBlock* block, IRDataType l, IRDataType r) {
		IRDataType res_t = IRBuilder::arith_result(l, r);

		if (res_t == IRDataType::i8) {
			int8_t rval = block_pop_into<int8_t>(block, r);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			int8_t lval = block_pop_into<int8_t>(block, l);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			op<int8_t> o;
			IRBuilder::build_const_i8(block, o(lval,rval));
		}
		else if (res_t == IRDataType::u8) {
			uint8_t rval = block_pop_into<uint8_t>(block, r);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			uint8_t lval = block_pop_into<uint8_t>(block, l);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			op<uint8_t> o;
			IRBuilder::build_const_u8(block, o(lval, rval));
		}
		else if (res_t == IRDataType::ibool) {
			uint8_t rval = block_pop_into<uint8_t>(block, r);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			uint8_t lval = block_pop_into<uint8_t>(block, l);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			op<uint8_t> o;
			IRBuilder::build_const_ibool(block, o(lval, rval));
		}
		else if (res_t == IRDataType::i16) {
			int16_t rval = block_pop_into<int16_t>(block, r);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			int16_t lval = block_pop_into<int16_t>(block, l);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			op<int16_t> o;
			IRBuilder::build_const_i16(block, o(lval, rval));
		}
		else if (res_t == IRDataType::u16) {
			uint16_t rval = block_pop_into<uint16_t>(block, r);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			uint16_t lval = block_pop_into<uint16_t>(block, l);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			op<uint16_t> o;
			IRBuilder::build_const_u16(block, o(lval, rval));
		}
		else if (res_t == IRDataType::i32) {
			int32_t rval = block_pop_into<int32_t>(block, r);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			int32_t lval = block_pop_into<int32_t>(block, l);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			op<int32_t> o;
			IRBuilder::build_const_i32(block, o(lval, rval));
		}
		else if (res_t == IRDataType::u32) {
			uint32_t rval = block_pop_into<uint32_t>(block, r);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			uint32_t lval = block_pop_into<uint32_t>(block, l);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			op<uint32_t> o;
			IRBuilder::build_const_u32(block, o(lval, rval));
		}
		else if (res_t == IRDataType::i64) {
			int64_t rval = block_pop_into<int64_t>(block, r);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			int64_t lval = block_pop_into<int64_t>(block, l);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			op<int64_t> o;
			IRBuilder::build_const_i64(block,o(lval, rval));
		}
		else if (res_t == IRDataType::u64) {
			uint64_t rval = block_pop_into<uint64_t>(block, r);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			uint64_t lval = block_pop_into<uint64_t>(block, l);
			block->pop<IRInstruction>();
			op<uint64_t> o;
			IRBuilder::build_const_u64(block, o(lval, rval));
		}
	}


	template< template<typename Ty> class op> void _ir_uilder_const_op_bool(IRBlock* block, IRDataType l, IRDataType r) {
		IRDataType res_t = IRBuilder::arith_result(l, r);

		if (res_t == IRDataType::i8) {
			int8_t rval = block_pop_into<int8_t>(block, r);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			int8_t lval = block_pop_into<int8_t>(block, l);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			op<int8_t> o;
			IRBuilder::build_const_ibool(block, (uint8_t)o(lval, rval));
		}
		else if (res_t == IRDataType::u8) {
			uint8_t rval = block_pop_into<uint8_t>(block, r);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			uint8_t lval = block_pop_into<uint8_t>(block, l);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			op<uint8_t> o;
			IRBuilder::build_const_ibool(block, (uint8_t)o(lval, rval));
		}
		else if (res_t == IRDataType::ibool) {
			uint8_t rval = block_pop_into<uint8_t>(block, r);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			uint8_t lval = block_pop_into<uint8_t>(block, l);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			op<uint8_t> o;
			IRBuilder::build_const_ibool(block, (uint8_t)o(lval, rval));
		}
		else if (res_t == IRDataType::i16) {
			int16_t rval = block_pop_into<int16_t>(block, r);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			int16_t lval = block_pop_into<int16_t>(block, l);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			op<int16_t> o;
			IRBuilder::build_const_ibool(block, (uint8_t)o(lval, rval));
		}
		else if (res_t == IRDataType::u16) {
			uint16_t rval = block_pop_into<uint16_t>(block, r);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			uint16_t lval = block_pop_into<uint16_t>(block, l);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			op<uint16_t> o;
			IRBuilder::build_const_ibool(block, (uint8_t)o(lval, rval));
		}
		else if (res_t == IRDataType::i32) {
			int32_t rval = block_pop_into<int32_t>(block, r);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			int32_t lval = block_pop_into<int32_t>(block, l);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			op<int32_t> o;
			IRBuilder::build_const_ibool(block, (uint8_t)o(lval, rval));
		}
		else if (res_t == IRDataType::u32) {
			uint32_t rval = block_pop_into<uint32_t>(block, r);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			uint32_t lval = block_pop_into<uint32_t>(block, l);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			op<uint32_t> o;
			IRBuilder::build_const_ibool(block, (uint8_t)o(lval, rval));
		}
		else if (res_t == IRDataType::i64) {
			int64_t rval = block_pop_into<int64_t>(block, r);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			int64_t lval = block_pop_into<int64_t>(block, l);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			op<int64_t> o;
			IRBuilder::build_const_ibool(block, (uint8_t)o(lval, rval));
		}
		else if (res_t == IRDataType::u64) {
			uint64_t rval = block_pop_into<uint64_t>(block, r);
			block->pop<IRDataType>();
			block->pop<IRInstruction>();
			uint64_t lval = block_pop_into<uint64_t>(block, l);
			block->pop<IRInstruction>();
			op<uint64_t> o;
			IRBuilder::build_const_ibool(block, (uint8_t)o(lval, rval));
		}
	}

	void IRBuilder::build_accept(IRBlock* block) {
		block->write_instruction(IRInstruction::accept);
		block->stack.push_back(std::make_pair(false,block->accepts));
	}

	void IRBuilder::build_discard(IRBlock* block) {
		block->write_instruction(IRInstruction::discard);
	}

	void IRBuilder::build_yield(IRBlock* block) {
		block->write_instruction(IRInstruction::yield);
		if (block->stack.size() <1) {
			throw_ir_nothing_on_stack_error();
		}
		block->yields = block->stack.back().second;
		block->stack.pop_back();
	}


	void IRBuilder::build_ret(IRBlock* block) {
		block->parent->return_blocks.insert(block);
		block->write_instruction(IRInstruction::ret);
		if (block->stack.size() > 0) throw_ir_remaining_stack_error();
	}

	void IRBuilder::build_jmp(IRBlock* block, IRBlock* address) {
		//block->ancestors.insert(address);
		address->predecessors.insert(block);
		block->write_instruction(IRInstruction::jmp);
		block->write_value(sizeof(unsigned int), (unsigned char*)&address->id);
		if (block->stack.size() > 0) throw_ir_remaining_stack_error();
	}

	void IRBuilder::build_jmpz(IRBlock* block, IRBlock* ifz, IRBlock* ifnz) {
		//block->ancestors.insert(address);
		ifz->predecessors.insert(block);
		ifnz->predecessors.insert(block);

		if (block->stack.size() < 1) {
			throw_ir_nothing_on_stack_error();
		}

		IRDataType t_v = block->stack.back().second;
		block->stack.pop_back();
		if (t_v != IRDataType::ibool)
			throw_ir_wrong_arguments_error();

		block->write_instruction(IRInstruction::jmpz);
		block->write_value(sizeof(unsigned int), (unsigned char*)&ifz->id);
		block->write_value(sizeof(unsigned int), (unsigned char*)&ifnz->id);
		if (block->stack.size() > 0) throw_ir_remaining_stack_error();
	}

	void IRBuilder::build_load(IRBlock* block, IRDataType type) {		
		if (block->stack.size() < 1) {
			throw_ir_nothing_on_stack_error();
		}

		IRDataType t_v = block->stack.back().second;
		block->stack.pop_back();
		//if (t_v != IRDataType::ptr)
		//	throw_ir_wrong_arguments_error();

		block->write_instruction(IRInstruction::load); 
		block->write_const_type(type);

		block->stack.push_back(std::make_pair(false, type));
	}

	void IRBuilder::build_store(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}

		IRDataType t_v = block->stack.back().second;
		block->stack.pop_back();

		IRDataType t_p = block->stack.back().second;
		block->stack.pop_back();
		if (t_p != IRDataType::ptr)
			throw_ir_wrong_arguments_error();

		block->write_instruction(IRInstruction::store);
	}


	void IRBuilder::build_local(IRBlock* block, unsigned int id) {
		block->stack.push_back(std::make_pair(false, IRDataType::ptr));
		block->write_instruction(IRInstruction::local);
		block->write_value(sizeof(unsigned int), (unsigned char*)&id);
	}


	void IRBuilder::build_member(IRBlock* block, IRStruct* type, unsigned int id) {
		if (block->stack.size() < 1) {
			throw_ir_nothing_on_stack_error();
		}
		IRDataType t_p = block->stack.back().second;
		block->stack.pop_back();
		if (t_p != IRDataType::ptr)
			throw_ir_wrong_arguments_error();

		block->stack.push_back(std::make_pair(false, IRDataType::ptr));
		block->write_instruction(IRInstruction::member);
		block->write_value(sizeof(IRType*), (unsigned char*)&type);
		block->write_value(sizeof(unsigned int), (unsigned char*)&id);
	}

	IRDataType IRBuilder::arith_result(IRDataType l, IRDataType r) {
		return std::max(l, r);
	}

	void IRBuilder::build_add(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > IRDataType::f64 || t_r.second > IRDataType::f64))
			throw_ir_wrong_arguments_error();

		if (t_l.first && t_r.first) {
			_ir_uilder_const_op<std::plus>(block, t_l.second, t_r.second);
		}
		else {
			block->write_instruction(IRInstruction::add);
			block->stack.push_back(std::make_pair(false,arith_result(t_l.second, t_r.second)));
		}
	}

	void IRBuilder::build_sub(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > IRDataType::f64 || t_r.second > IRDataType::f64))
			throw_ir_wrong_arguments_error();
		if (t_l.first && t_r.first) {
			_ir_uilder_const_op<std::minus>(block, t_l.second, t_r.second);
		}
		else {
			block->write_instruction(IRInstruction::sub);
			block->stack.push_back(std::make_pair(false, arith_result(t_l.second, t_r.second)));
		}
	}

	void IRBuilder::build_div(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > IRDataType::f64 || t_r.second > IRDataType::f64))
			throw_ir_wrong_arguments_error();
		if (t_l.first && t_r.first) {
			_ir_uilder_const_op<std::divides>(block, t_l.second, t_r.second);
		}
		else {

			block->write_instruction(IRInstruction::div);
			block->stack.push_back(std::make_pair(false, arith_result(t_l.second, t_r.second)));
		}
	}

	void IRBuilder::build_rem(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > IRDataType::f64 || t_r.second > IRDataType::f64))
			throw_ir_wrong_arguments_error();
		if (t_l.first && t_r.first) {
			_ir_uilder_const_op<std::modulus>(block, t_l.second, t_r.second);
		}
		else {

			block->write_instruction(IRInstruction::rem);
			block->stack.push_back(std::make_pair(false, arith_result(t_l.second, t_r.second)));
		}
	}

	void IRBuilder::build_and(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > IRDataType::f64 || t_r.second > IRDataType::f64))
			throw_ir_wrong_arguments_error();
		if (t_l.first && t_r.first) {
			_ir_uilder_const_op<std::bit_and>(block, t_l.second, t_r.second);
		}
		else {

			block->write_instruction(IRInstruction::o_and);
			block->stack.push_back(std::make_pair(false, arith_result(t_l.second, t_r.second)));
		}
	}

	void IRBuilder::build_or(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > IRDataType::f64 || t_r.second > IRDataType::f64))
			throw_ir_wrong_arguments_error();
		if (t_l.first && t_r.first) {
			_ir_uilder_const_op<std::bit_or>(block, t_l.second, t_r.second);
		}
		else {

			block->write_instruction(IRInstruction::o_or);
			block->stack.push_back(std::make_pair(false, arith_result(t_l.second, t_r.second)));
		}
	}

	void IRBuilder::build_xor(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > IRDataType::f64 || t_r.second > IRDataType::f64))
			throw_ir_wrong_arguments_error();
		if (t_l.first && t_r.first) {
			_ir_uilder_const_op<std::bit_xor>(block, t_l.second, t_r.second);
		}
		else {

			block->write_instruction(IRInstruction::o_xor);
			block->stack.push_back(std::make_pair(false, arith_result(t_l.second, t_r.second)));
		}
	}

	void IRBuilder::build_mul(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > IRDataType::f64 || t_r.second > IRDataType::f64))
			throw_ir_wrong_arguments_error();
		if (t_l.first && t_r.first) {
			_ir_uilder_const_op<std::multiplies>(block, t_l.second, t_r.second);
		}
		else {
			block->write_instruction(IRInstruction::mul);
			block->stack.push_back(std::make_pair(false, arith_result(t_l.second, t_r.second)));
		}
	}

	void IRBuilder::build_eq(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > IRDataType::f64 || t_r.second > IRDataType::f64) && t_l != t_r)
			throw_ir_wrong_arguments_error();

		if (t_l.first && t_r.first) {
			_ir_uilder_const_op_bool<std::equal_to>(block, t_l.second, t_r.second);
		}
		else {
			block->write_instruction(IRInstruction::eq);
			block->stack.push_back(std::make_pair(false, IRDataType::ibool));
		}
	}
	void IRBuilder::build_ne(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > IRDataType::f64 || t_r.second > IRDataType::f64) && t_l != t_r)
			throw_ir_wrong_arguments_error();

		if (t_l.first && t_r.first) {
			_ir_uilder_const_op_bool<std::not_equal_to>(block, t_l.second, t_r.second);
		}
		else {
			block->write_instruction(IRInstruction::ne);
			block->stack.push_back(std::make_pair(false, IRDataType::ibool));
		}
	}
	void IRBuilder::build_gt(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > IRDataType::f64 || t_r.second > IRDataType::f64) && t_l != t_r)
			throw_ir_wrong_arguments_error();

		if (t_l.first && t_r.first) {
			_ir_uilder_const_op_bool<std::greater>(block, t_l.second, t_r.second);
		}
		else {
			block->write_instruction(IRInstruction::gt);
			block->stack.push_back(std::make_pair(false, IRDataType::ibool));
		}
	}

	void IRBuilder::build_ge(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > IRDataType::f64 || t_r.second > IRDataType::f64) && t_l != t_r)
			throw_ir_wrong_arguments_error();

		if (t_l.first && t_r.first) {
			_ir_uilder_const_op_bool<std::greater_equal>(block, t_l.second, t_r.second);
		}
		else {
			block->write_instruction(IRInstruction::ge);
			block->stack.push_back(std::make_pair(false, IRDataType::ibool));
		}
	}


	void IRBuilder::build_lt(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > IRDataType::f64 || t_r.second > IRDataType::f64) && t_l != t_r)
			throw_ir_wrong_arguments_error();

		if (t_l.first && t_r.first) {
			_ir_uilder_const_op_bool<std::less>(block, t_l.second, t_r.second);
		}
		else {
			block->write_instruction(IRInstruction::lt);
			block->stack.push_back(std::make_pair(false, IRDataType::ibool));
		}
	}

	void IRBuilder::build_le(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		auto t_r = block->stack.back();
		block->stack.pop_back();
		auto t_l = block->stack.back();
		block->stack.pop_back();
		if ((t_l.second > IRDataType::f64 || t_r.second > IRDataType::f64) && t_l != t_r)
			throw_ir_wrong_arguments_error();

		if (t_l.first && t_r.first) {
			_ir_uilder_const_op_bool<std::less_equal>(block, t_l.second, t_r.second);
		}
		else {
			block->write_instruction(IRInstruction::le);
			block->stack.push_back(std::make_pair(false, IRDataType::ibool));
		}
	}

	
}