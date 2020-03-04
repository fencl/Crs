#include "IR.h"
#include "../Error.h"
#include <algorithm>

namespace Corrosive {
	void IRBuilder::build_const_ibool (IRBlock* block, int8_t   value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::ibool);  block->write_value(sizeof(int8_t),   (unsigned char*)&value); block->stack.push_back(IRDataType::ibool); }
	void IRBuilder::build_const_i8    (IRBlock* block, int8_t   value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::i8);     block->write_value(sizeof(int8_t),   (unsigned char*)&value); block->stack.push_back(IRDataType::i8);    }
	void IRBuilder::build_const_i16   (IRBlock* block, int16_t  value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::i16);    block->write_value(sizeof(int16_t),  (unsigned char*)&value); block->stack.push_back(IRDataType::i16);   }
	void IRBuilder::build_const_i32   (IRBlock* block, int32_t  value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::i32);    block->write_value(sizeof(int32_t),  (unsigned char*)&value); block->stack.push_back(IRDataType::i32);   }
	void IRBuilder::build_const_i64   (IRBlock* block, int64_t  value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::i64);    block->write_value(sizeof(int64_t),  (unsigned char*)&value); block->stack.push_back(IRDataType::i64);   }
	void IRBuilder::build_const_u8    (IRBlock* block, uint8_t  value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::u8);     block->write_value(sizeof(uint8_t),  (unsigned char*)&value); block->stack.push_back(IRDataType::u8);    }
	void IRBuilder::build_const_u16   (IRBlock* block, uint16_t value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::u16);    block->write_value(sizeof(uint16_t), (unsigned char*)&value); block->stack.push_back(IRDataType::u16);   }
	void IRBuilder::build_const_u32   (IRBlock* block, uint32_t value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::u32);    block->write_value(sizeof(uint32_t), (unsigned char*)&value); block->stack.push_back(IRDataType::u32);   }
	void IRBuilder::build_const_u64   (IRBlock* block, uint64_t value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::u64);    block->write_value(sizeof(uint64_t), (unsigned char*)&value); block->stack.push_back(IRDataType::u64);   }
	void IRBuilder::build_const_f32   (IRBlock* block, float    value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::f32);    block->write_value(sizeof(float),    (unsigned char*)&value); block->stack.push_back(IRDataType::f32);   }
	void IRBuilder::build_const_f64   (IRBlock* block, double   value) { block->write_instruction(IRInstruction::value); block->write_const_type(IRDataType::f64);    block->write_value(sizeof(double),   (unsigned char*)&value); block->stack.push_back(IRDataType::f64);   }

	void IRBuilder::build_accept(IRBlock* block) {
		block->write_instruction(IRInstruction::accept);
		block->stack.push_back(block->accepts);
	}

	void IRBuilder::build_discard(IRBlock* block) {
		block->write_instruction(IRInstruction::discard);
	}

	void IRBuilder::build_yield(IRBlock* block) {
		block->write_instruction(IRInstruction::yield);
		if (block->stack.size() <1) {
			throw_ir_nothing_on_stack_error();
		}
		block->yields = block->stack.back();
		block->stack.pop_back();
	}


	void IRBuilder::build_ret(IRBlock* block) {
		// test for return type
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

		IRDataType t_v = block->stack.back();
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

		IRDataType t_v = block->stack.back();
		block->stack.pop_back();
		if (t_v != IRDataType::ptr)
			throw_ir_wrong_arguments_error();

		block->write_instruction(IRInstruction::load); 
		block->write_const_type(type);
	}

	IRDataType IRBuilder::arith_result(IRDataType l, IRDataType r) {
		return std::max(l, r);
	}

	void IRBuilder::build_add(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		IRDataType t_r = block->stack.back();
		block->stack.pop_back();
		IRDataType t_l = block->stack.back();
		block->stack.pop_back();
		block->write_instruction(IRInstruction::add);
		block->stack.push_back(arith_result(t_r, t_l));
	}

	void IRBuilder::build_sub(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		IRDataType t_r = block->stack.back();
		block->stack.pop_back();
		IRDataType t_l = block->stack.back();
		block->stack.pop_back();
		if (t_l > IRDataType::f64 || t_r > IRDataType::f64)
			throw_ir_wrong_arguments_error();

		block->write_instruction(IRInstruction::sub);
		block->stack.push_back(arith_result(t_r, t_l));
	}

	void IRBuilder::build_div(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		IRDataType t_r = block->stack.back();
		block->stack.pop_back();
		IRDataType t_l = block->stack.back();
		block->stack.pop_back();
		if (t_l > IRDataType::f64 || t_r > IRDataType::f64)
			throw_ir_wrong_arguments_error();
		block->write_instruction(IRInstruction::div);
		block->stack.push_back(arith_result(t_r, t_l));
	}

	void IRBuilder::build_rem(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		IRDataType t_r = block->stack.back();
		block->stack.pop_back();
		IRDataType t_l = block->stack.back();
		block->stack.pop_back();
		if (t_l > IRDataType::f64 || t_r > IRDataType::f64)
			throw_ir_wrong_arguments_error();
		block->write_instruction(IRInstruction::rem);
		block->stack.push_back(arith_result(t_r, t_l));
	}

	void IRBuilder::build_and(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		IRDataType t_r = block->stack.back();
		block->stack.pop_back();
		IRDataType t_l = block->stack.back();
		block->stack.pop_back();
		if (t_l > IRDataType::f64 || t_r > IRDataType::f64)
			throw_ir_wrong_arguments_error();
		block->write_instruction(IRInstruction::o_and);
		block->stack.push_back(arith_result(t_r, t_l));
	}

	void IRBuilder::build_or(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		IRDataType t_r = block->stack.back();
		block->stack.pop_back();
		IRDataType t_l = block->stack.back();
		block->stack.pop_back();
		if (t_l > IRDataType::f64 || t_r > IRDataType::f64)
			throw_ir_wrong_arguments_error();
		block->write_instruction(IRInstruction::o_or);
		block->stack.push_back(arith_result(t_r, t_l));
	}

	void IRBuilder::build_xor(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		IRDataType t_r = block->stack.back();
		block->stack.pop_back();
		IRDataType t_l = block->stack.back();
		block->stack.pop_back();
		if (t_l > IRDataType::f64 || t_r > IRDataType::f64)
			throw_ir_wrong_arguments_error();
		block->write_instruction(IRInstruction::o_xor);
		block->stack.push_back(arith_result(t_r, t_l));
	}

	void IRBuilder::build_mul(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		IRDataType t_r = block->stack.back();
		block->stack.pop_back();
		IRDataType t_l = block->stack.back();
		block->stack.pop_back();
		block->write_instruction(IRInstruction::mul);
		block->stack.push_back(arith_result(t_r, t_l));
	}

	void IRBuilder::build_eq(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		IRDataType t_r = block->stack.back();
		block->stack.pop_back();
		IRDataType t_l = block->stack.back();
		block->stack.pop_back();
		block->write_instruction(IRInstruction::eq);
		block->stack.push_back(IRDataType::ibool);
	}
	void IRBuilder::build_ne(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		IRDataType t_r = block->stack.back();
		block->stack.pop_back();
		IRDataType t_l = block->stack.back();
		block->stack.pop_back();
		block->write_instruction(IRInstruction::ne);
		block->stack.push_back(IRDataType::ibool);
	}
	void IRBuilder::build_gt(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		IRDataType t_r = block->stack.back();
		block->stack.pop_back();
		IRDataType t_l = block->stack.back();
		block->stack.pop_back();
		block->write_instruction(IRInstruction::gt);
		block->stack.push_back(IRDataType::ibool);
	}

	void IRBuilder::build_ge(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		IRDataType t_r = block->stack.back();
		block->stack.pop_back();
		IRDataType t_l = block->stack.back();
		block->stack.pop_back();
		block->write_instruction(IRInstruction::ge);
		block->stack.push_back(IRDataType::ibool);
	}


	void IRBuilder::build_lt(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		IRDataType t_r = block->stack.back();
		block->stack.pop_back();
		IRDataType t_l = block->stack.back();
		block->stack.pop_back();
		block->write_instruction(IRInstruction::lt);
		block->stack.push_back(IRDataType::ibool);
	}

	void IRBuilder::build_le(IRBlock* block) {
		if (block->stack.size() < 2) {
			throw_ir_nothing_on_stack_error();
		}
		IRDataType t_r = block->stack.back();
		block->stack.pop_back();
		IRDataType t_l = block->stack.back();
		block->stack.pop_back();
		block->write_instruction(IRInstruction::le);
		block->stack.push_back(IRDataType::ibool);
	}
}