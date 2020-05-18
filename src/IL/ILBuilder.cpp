#include "IL.h"
#include "../Error.h"
#include <algorithm>
#include <functional>

namespace Corrosive {
	bool ILBuilder::build_const_ibool (ILBlock* block, int8_t   value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::ibool);  block->write_value(sizeof(int8_t),   (unsigned char*)&value); return true; }
	bool ILBuilder::build_const_i8    (ILBlock* block, int8_t   value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::i8);     block->write_value(sizeof(int8_t),   (unsigned char*)&value); return true; }
	bool ILBuilder::build_const_i16   (ILBlock* block, int16_t  value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::i16);    block->write_value(sizeof(int16_t),  (unsigned char*)&value); return true; }
	bool ILBuilder::build_const_i32   (ILBlock* block, int32_t  value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::i32);    block->write_value(sizeof(int32_t),  (unsigned char*)&value); return true; }
	bool ILBuilder::build_const_i64   (ILBlock* block, int64_t  value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::i64);    block->write_value(sizeof(int64_t),  (unsigned char*)&value); return true; }
	bool ILBuilder::build_const_u8    (ILBlock* block, uint8_t  value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::u8);     block->write_value(sizeof(uint8_t),  (unsigned char*)&value); return true; }
	bool ILBuilder::build_const_u16   (ILBlock* block, uint16_t value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::u16);    block->write_value(sizeof(uint16_t), (unsigned char*)&value); return true; }
	bool ILBuilder::build_const_u32   (ILBlock* block, uint32_t value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::u32);    block->write_value(sizeof(uint32_t), (unsigned char*)&value); return true; }
	bool ILBuilder::build_const_u64   (ILBlock* block, uint64_t value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::u64);    block->write_value(sizeof(uint64_t), (unsigned char*)&value); return true; }
	bool ILBuilder::build_const_f32   (ILBlock* block, float    value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::f32);    block->write_value(sizeof(float),    (unsigned char*)&value); return true; }
	bool ILBuilder::build_const_f64   (ILBlock* block, double   value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::f64);    block->write_value(sizeof(double),   (unsigned char*)&value); return true; }
	bool ILBuilder::build_const_type  (ILBlock* block, void*    value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::ptr);	  block->write_value(sizeof(void*),    (unsigned char*)&value); return true; }

	bool ILBuilder::build_const_size  (ILBlock* block, ilsize_t compile, ilsize_t runtime) { block->write_instruction(ILInstruction::size); block->write_value(block->parent->parent->get_compile_pointer_size(),    (unsigned char*)&compile); block->write_value(block->parent->parent->get_pointer_size(), (unsigned char*)&runtime); return true; }



	bool ILBuilder::build_accept(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::accept);
		block->write_const_type(type);
		block->accepts = type;
		return true;
	}

	bool ILBuilder::build_discard(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::discard);
		block->write_const_type(type);
		block->accepts = type;
		return true;
	}

	bool ILBuilder::build_yield(ILBlock* block,ILDataType type) {
		block->write_instruction(ILInstruction::yield);
		block->write_const_type(type);
		block->yields = type;
		return true;
	}

	bool ILBuilder::build_forget(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::forget);
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
		return true;
	}

	bool ILBuilder::build_call(ILBlock* block, ILDataType type,uint16_t argc) {
		block->parent->return_blocks.insert(block);
		block->write_instruction(ILInstruction::call);
		block->write_const_type(type);
		block->write_value(sizeof(uint16_t), (unsigned char*)&argc);
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
		
		return true;
	}

	bool ILBuilder::build_load(ILBlock* block, ILDataType type) {

		block->write_instruction(ILInstruction::load); 
		block->write_const_type(type);
		return true;
	}

	bool ILBuilder::build_member(ILBlock* block, uint16_t compile_offset, uint16_t offset) {
		if (compile_offset == offset) {
			if (offset > 0) {
				block->write_instruction(ILInstruction::member);
				block->write_value(sizeof(uint16_t), (unsigned char*)&offset);
			}
		}else if (offset > 0 || compile_offset>0) {
			block->write_instruction(ILInstruction::member2);
			block->write_value(sizeof(uint16_t), (unsigned char*)&offset);
			block->write_value(sizeof(uint16_t), (unsigned char*)&compile_offset);
		}
		return true;
	}

	bool ILBuilder::build_rmember(ILBlock* block, ILDataType from, ILDataType to, uint8_t compile_offset, uint8_t offset) {
		if (offset == compile_offset) {
			if (offset > 0) {
				block->write_instruction(ILInstruction::rmember);
				block->write_const_type(from);
				block->write_const_type(to);
				block->write_value(sizeof(uint8_t), (unsigned char*)&offset);
			}
		}
		else {
			if (offset > 0 || compile_offset>0) {
				block->write_instruction(ILInstruction::rmember2);
				block->write_const_type(from);
				block->write_const_type(to);
				block->write_value(sizeof(uint8_t), (unsigned char*)&offset);
				block->write_value(sizeof(uint8_t), (unsigned char*)&compile_offset);
			}
		}
		return true;
	}



	bool ILBuilder::build_store(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::store);
		block->write_const_type(type);
		return true;
	}


	bool ILBuilder::build_local(ILBlock* block, uint16_t id) {
		block->write_instruction(ILInstruction::local);
		block->write_value(sizeof(uint16_t), (unsigned char*)&id);
		return true;
	}

	bool ILBuilder::build_vtable(ILBlock* block, uint32_t id) {
		block->write_instruction(ILInstruction::vtable);
		block->write_value(sizeof(uint32_t), (unsigned char*)&id);
		return true;
	}


	bool ILBuilder::build_insintric(ILBlock* block, ILInsintric fun) {
		block->write_instruction(ILInstruction::insintric);
		block->write_value(sizeof(uint8_t), (unsigned char*)&fun);
		return true;
	}


	bool ILBuilder::build_copy(ILBlock* block, ILDataType type, uint16_t multiplier) {
		block->write_instruction(ILInstruction::copy);
		block->write_const_type(type);
		block->write_value(sizeof(uint16_t), (unsigned char*)&multiplier);
		return true;
	}
	
	bool ILBuilder::build_offset(ILBlock* block,uint16_t multiplier) {
		block->write_instruction(ILInstruction::offset);
		block->write_value(sizeof(uint16_t), (unsigned char*)&multiplier);
		return true;
	}
	
	bool ILBuilder::build_offset2(ILBlock* block,uint16_t compile_multiplier,uint16_t multiplier) {
		block->write_instruction(ILInstruction::offset2);
		block->write_value(sizeof(uint16_t), (unsigned char*)&multiplier);
		block->write_value(sizeof(uint16_t), (unsigned char*)&compile_multiplier);
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

		block->write_instruction(ILInstruction::add);
		block->write_const_type(tl);
		block->write_const_type(tr);
		

		return true;
	}

	bool ILBuilder::build_sub(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		block->write_instruction(ILInstruction::sub);
		block->write_const_type(tl);
		block->write_const_type(tr);

		return true;
	}

	bool ILBuilder::build_div(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}
		
		block->write_instruction(ILInstruction::div);
		block->write_const_type(tl);
		block->write_const_type(tr);
		

		return true;
	}

	bool ILBuilder::build_rem(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}
		
		block->write_instruction(ILInstruction::rem);
		block->write_const_type(tl);
		block->write_const_type(tr);		

		return true;
	}

	bool ILBuilder::build_and(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		block->write_instruction(ILInstruction::bit_and);
		block->write_const_type(tl);
		block->write_const_type(tr);

		return true;
	}

	bool ILBuilder::build_or(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		block->write_instruction(ILInstruction::bit_or);
		block->write_const_type(tl);
		block->write_const_type(tr);		

		return true;
	}

	bool ILBuilder::build_xor(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		block->write_instruction(ILInstruction::bit_xor);
		block->write_const_type(tl);
		block->write_const_type(tr);

		return true;
	}

	bool ILBuilder::build_mul(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		block->write_instruction(ILInstruction::mul);
		block->write_const_type(tl);
		block->write_const_type(tr);

		return true;
	}

	bool ILBuilder::build_eq(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		block->write_instruction(ILInstruction::eq);
		block->write_const_type(tl);
		block->write_const_type(tr);

		return true;
	}


	bool ILBuilder::build_ne(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		block->write_instruction(ILInstruction::ne);
		block->write_const_type(tl);
		block->write_const_type(tr);

		return true;
	}

	bool ILBuilder::build_gt(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		block->write_instruction(ILInstruction::gt);
		block->write_const_type(tl);
		block->write_const_type(tr);

		return true;
	}


	bool ILBuilder::build_ge(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		block->write_instruction(ILInstruction::ge);
		block->write_const_type(tl);
		block->write_const_type(tr);

		return true;
	}



	bool ILBuilder::build_lt(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		block->write_instruction(ILInstruction::lt);
		block->write_const_type(tl);
		block->write_const_type(tr);

		return true;
	}


	bool ILBuilder::build_le(ILBlock* block, ILDataType tl, ILDataType tr) {
		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		block->write_instruction(ILInstruction::le);
		block->write_const_type(tl);
		block->write_const_type(tr);

		return true;
	}



	bool ILBuilder::build_cast(ILBlock* block, ILDataType tl, ILDataType tr) {
		if ((tl > ILDataType::f64 || tr > ILDataType::f64)) {
			throw_il_wrong_arguments_error();
			return false;
		}

		block->write_instruction(ILInstruction::cast);
		block->write_const_type(tl);
		block->write_const_type(tr);
		return true;
	}

	
}