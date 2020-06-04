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

	bool ILBuilder::build_const_size  (ILBlock* block, ILSize   value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::size);   block->write_value(sizeof(ILSize),   (unsigned char*)&value); return true; }



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
		if (type != ILDataType::none) {
			block->write_instruction(ILInstruction::forget);
			block->write_const_type(type);
		}
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

	bool ILBuilder::build_roffset(ILBlock* block, ILDataType from, ILDataType to, ILSmallSize offset) {
		block->write_instruction(ILInstruction::roffset);
		block->write_const_type(from);
		block->write_const_type(to);
		block->write_value(sizeof(ILSmallSize), (unsigned char*)&offset);
		return true;
	}


	bool ILBuilder::build_memcpy(ILBlock* block, ILSize size) {
		block->write_instruction(ILInstruction::memcpy);
		block->write_value(sizeof(ILSize), (unsigned char*)&size);
		return true;
	}

	bool ILBuilder::build_memcpy2(ILBlock* block, ILSize size) {
		block->write_instruction(ILInstruction::memcpy2);
		block->write_value(sizeof(ILSize), (unsigned char*)&size);
		return true;
	}

	bool ILBuilder::build_memcmp(ILBlock* block, ILSize size) {
		block->write_instruction(ILInstruction::memcmp);
		block->write_value(sizeof(ILSize), (unsigned char*)&size);
		return true;
	}

	bool ILBuilder::build_memcmp2(ILBlock* block, ILSize size) {
		block->write_instruction(ILInstruction::memcmp2);
		block->write_value(sizeof(ILSize), (unsigned char*)&size);
		return true;
	}

	bool ILBuilder::build_rmemcmp(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::rmemcmp);
		block->write_const_type(type);
		return true;
	}

	bool ILBuilder::build_rmemcmp2(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::rmemcmp2);
		block->write_const_type(type);
		return true;
	}

	bool ILBuilder::build_store(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::store);
		block->write_const_type(type);
		return true;
	}
	

	bool ILBuilder::build_store2(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::store2);
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


	bool ILBuilder::build_duplicate(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::duplicate);
		block->write_const_type(type);
		return true;
	}
	
	bool ILBuilder::build_swap(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::swap);
		block->write_const_type(type);
		return true;
	}

	bool ILBuilder::build_swap2(ILBlock* block, ILDataType type1, ILDataType type2) {
		if (type1 == type2) {
			return build_swap(block,type1);
		}

		block->write_instruction(ILInstruction::swap2);
		block->write_const_type(type1);
		block->write_const_type(type2);
		return true;
	}
	
	bool ILBuilder::build_offset(ILBlock* block,ILSize offset) {
		block->write_instruction(ILInstruction::offset);
		block->write_value(sizeof(ILSize), (unsigned char*)&offset);
		return true;
	}
	

	bool ILBuilder::build_callstart(ILBlock* block) {
		block->write_instruction(ILInstruction::start);
		return true;
	}

	bool ILBuilder::build_malloc(ILBlock* block) {
		block->write_instruction(ILInstruction::malloc);
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