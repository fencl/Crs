#include "IL.h"
#include "../Error.h"
#include <algorithm>
#include <functional>

namespace Corrosive {
	void ILBuilder::build_const_ibool(ILBlock* block, uint8_t  value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::ibool);  block->write_value(sizeof(uint8_t),  (unsigned char*)&value); }
	void ILBuilder::build_const_i8    (ILBlock* block, int8_t   value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::i8);     block->write_value(sizeof(int8_t),   (unsigned char*)&value); }
	void ILBuilder::build_const_i16   (ILBlock* block, int16_t  value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::i16);    block->write_value(sizeof(int16_t),  (unsigned char*)&value); }
	void ILBuilder::build_const_i32   (ILBlock* block, int32_t  value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::i32);    block->write_value(sizeof(int32_t),  (unsigned char*)&value); }
	void ILBuilder::build_const_i64   (ILBlock* block, int64_t  value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::i64);    block->write_value(sizeof(int64_t),  (unsigned char*)&value); }
	void ILBuilder::build_const_u8    (ILBlock* block, uint8_t  value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::u8);     block->write_value(sizeof(uint8_t),  (unsigned char*)&value); }
	void ILBuilder::build_const_u16   (ILBlock* block, uint16_t value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::u16);    block->write_value(sizeof(uint16_t), (unsigned char*)&value); }
	void ILBuilder::build_const_u32   (ILBlock* block, uint32_t value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::u32);    block->write_value(sizeof(uint32_t), (unsigned char*)&value); }
	void ILBuilder::build_const_u64   (ILBlock* block, uint64_t value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::u64);    block->write_value(sizeof(uint64_t), (unsigned char*)&value); }
	void ILBuilder::build_const_f32   (ILBlock* block, float    value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::f32);    block->write_value(sizeof(float),    (unsigned char*)&value); }
	void ILBuilder::build_const_f64   (ILBlock* block, double   value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::f64);    block->write_value(sizeof(double),   (unsigned char*)&value); }
	void ILBuilder::build_const_type  (ILBlock* block, void*    value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::ptr);	  block->write_value(sizeof(void*),    (unsigned char*)&value); }
	void ILBuilder::build_const_size  (ILBlock* block, ILSize   value) { block->write_instruction(ILInstruction::value); block->write_const_type(ILDataType::size);   block->write_value(sizeof(ILSize),   (unsigned char*)&value); }



	void ILBuilder::build_accept(ILBlock* block, ILDataType type) {

		if (type != ILDataType::none) {
			block->write_instruction(ILInstruction::accept);
			block->write_const_type(type);
		}
		block->accepts = type;
		
	}

	void ILBuilder::build_discard(ILBlock* block, ILDataType type) {
		if (type != ILDataType::none) {
			block->write_instruction(ILInstruction::discard);
			block->write_const_type(type);
		}
		block->accepts = type;
		
	}

	void ILBuilder::build_yield(ILBlock* block,ILDataType type) {
		if (type != ILDataType::none) {
			block->write_instruction(ILInstruction::yield);
			block->write_const_type(type);
		}
		block->yields = type;
		
	}

	void ILBuilder::build_forget(ILBlock* block, ILDataType type) {
		if (type != ILDataType::none) {
			block->write_instruction(ILInstruction::forget);
			block->write_const_type(type);
		}
		
	}

	void ILBuilder::build_ret(ILBlock* block, ILDataType type) {
		block->parent->return_blocks.insert(block);
		block->write_instruction(ILInstruction::ret);
		block->write_const_type(type);
		block->yields = type;
		
	}

	void ILBuilder::build_negative(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::negative);
		block->write_const_type(type);
		
	}
	void ILBuilder::build_negate(ILBlock* block) {
		block->write_instruction(ILInstruction::negate);		
	}

	void ILBuilder::build_fnptr(ILBlock* block, ILFunction* fun) {
		block->write_instruction(ILInstruction::fnptr);
		block->write_value(sizeof(uint32_t), (unsigned char*)&(fun->id));
	}

	void ILBuilder::build_constref(ILBlock* block, uint32_t constid) {
		block->write_instruction(ILInstruction::constref);
		block->write_value(sizeof(uint32_t), (unsigned char*)&constid);
	}

	void ILBuilder::build_call(ILBlock* block, ILDataType type,uint16_t argc) {
		block->write_instruction(ILInstruction::call);
		block->write_const_type(type);
		block->write_value(sizeof(uint16_t), (unsigned char*)&argc);
		
	}

	void ILBuilder::build_jmp(ILBlock* block, ILBlock* address) {
		address->predecessors.insert(block);
		block->write_instruction(ILInstruction::jmp);
		block->write_value(sizeof(uint32_t), (unsigned char*)&address->id);
		
	}
	
	void ILBuilder::build_debug(ILBlock* block, uint16_t file, uint16_t line) {
		block->write_instruction(ILInstruction::debug);
		block->write_value(sizeof(uint16_t), (unsigned char*)&file);
		block->write_value(sizeof(uint16_t), (unsigned char*)&line);
	}

	void ILBuilder::build_jmpz(ILBlock* block, ILBlock* ifz, ILBlock* ifnz) {
		ifz->predecessors.insert(block);
		ifnz->predecessors.insert(block);

		block->write_instruction(ILInstruction::jmpz);
		block->write_value(sizeof(uint32_t), (unsigned char*)&ifz->id);
		block->write_value(sizeof(uint32_t), (unsigned char*)&ifnz->id);
		
		
	}

	void ILBuilder::build_load(ILBlock* block, ILDataType type) {

		block->write_instruction(ILInstruction::load); 
		block->write_const_type(type);
		
	}

	void ILBuilder::build_isnotzero(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::isnotzero); 
		block->write_const_type(type);
		
	}

	void ILBuilder::build_roffset(ILBlock* block, ILDataType from, ILDataType to, ILSmallSize offset) {
		block->write_instruction(ILInstruction::roffset);
		block->write_const_type(from);
		block->write_const_type(to);
		block->write_value(sizeof(ILSmallSize), (unsigned char*)&offset);
		
	}


	void ILBuilder::build_null(ILBlock* block) {
		block->write_instruction(ILInstruction::null);
		
	}

	void ILBuilder::build_memcpy(ILBlock* block, ILSize size) {
		block->write_instruction(ILInstruction::memcpy);
		block->write_value(sizeof(ILSize), (unsigned char*)&size);
		
	}

	void ILBuilder::build_memcpy2(ILBlock* block, ILSize size) {
		block->write_instruction(ILInstruction::memcpy2);
		block->write_value(sizeof(ILSize), (unsigned char*)&size);
		
	}

	void ILBuilder::build_memcmp(ILBlock* block, ILSize size) {
		block->write_instruction(ILInstruction::memcmp);
		block->write_value(sizeof(ILSize), (unsigned char*)&size);
		
	}

	void ILBuilder::build_memcmp2(ILBlock* block, ILSize size) {
		block->write_instruction(ILInstruction::memcmp2);
		block->write_value(sizeof(ILSize), (unsigned char*)&size);
		
	}

	void ILBuilder::build_rmemcmp(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::rmemcmp);
		block->write_const_type(type);
		
	}

	void ILBuilder::build_rmemcmp2(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::rmemcmp2);
		block->write_const_type(type);
		
	}

	void ILBuilder::build_store(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::store);
		block->write_const_type(type);
	}
	

	void ILBuilder::build_store2(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::store2);
		block->write_const_type(type);
	}


	void ILBuilder::build_local(ILBlock* block, uint16_t id) {
		block->write_instruction(ILInstruction::local);
		block->write_value(sizeof(uint16_t), (unsigned char*)&id);
		
	}

	void ILBuilder::build_vtable(ILBlock* block, uint32_t id) {
		block->write_instruction(ILInstruction::vtable);
		block->write_value(sizeof(uint32_t), (unsigned char*)&id);
	}

	void ILBuilder::build_tableoffset(ILBlock* block, uint32_t tableid, uint16_t itemid) {
		if (itemid != 0) {
			block->write_instruction(ILInstruction::tableoffset);
			block->write_value(sizeof(uint32_t), (unsigned char*)&tableid);
			block->write_value(sizeof(uint16_t), (unsigned char*)&itemid);
		}
	}


	void ILBuilder::build_insintric(ILBlock* block, ILInsintric fun) {
		block->write_instruction(ILInstruction::insintric);
		block->write_value(sizeof(uint8_t), (unsigned char*)&fun);
	}


	void ILBuilder::build_duplicate(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::duplicate);
		block->write_const_type(type);
	}
	
	void ILBuilder::build_swap(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::swap);
		block->write_const_type(type);
	}

	void ILBuilder::build_swap2(ILBlock* block, ILDataType type1, ILDataType type2) {
		if (type1 == type2) {
			return build_swap(block,type1);
		}

		block->write_instruction(ILInstruction::swap2);
		block->write_const_type(type1);
		block->write_const_type(type2);
		
	}
	
	void ILBuilder::build_offset(ILBlock* block, ILSize offset) {
		if (offset.type == ILSizeType::table || offset.value > 0) {
			block->write_instruction(ILInstruction::offset);
			block->write_value(sizeof(ILSize), (unsigned char*)&offset);
		}
	}
	
	void ILBuilder::build_rtoffset(ILBlock* block) {
		block->write_instruction(ILInstruction::rtoffset);
		
	}

	void ILBuilder::build_rtoffset2(ILBlock* block) {
		block->write_instruction(ILInstruction::rtoffset2);
		
	}

	void ILBuilder::build_callstart(ILBlock* block) {
		block->write_instruction(ILInstruction::start);
		
	}

	void ILBuilder::build_malloc(ILBlock* block) {
		block->write_instruction(ILInstruction::malloc);
		
	}

	void ILBuilder::build_free(ILBlock* block) {
		block->write_instruction(ILInstruction::free);
		
	}

	ILDataType ILBuilder::arith_result(ILDataType l, ILDataType r) {
		return std::max(l, r);
	}

	void ILBuilder::build_add(ILBlock* block, ILDataType tl, ILDataType tr) {
		
		if ((tl > ILDataType::ptr || tr > ILDataType::ptr)) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::add);
		block->write_const_type(tl);
		block->write_const_type(tr);
		

		
	}

	void ILBuilder::build_sub(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::ptr || tr > ILDataType::ptr)) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::sub);
		block->write_const_type(tl);
		block->write_const_type(tr);

		
	}

	void ILBuilder::build_div(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::ptr || tr > ILDataType::ptr)) {
			throw_il_wrong_arguments_error();
		}
		
		block->write_instruction(ILInstruction::div);
		block->write_const_type(tl);
		block->write_const_type(tr);
		

		
	}

	void ILBuilder::build_rem(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::ptr || tr > ILDataType::ptr)) {
			throw_il_wrong_arguments_error();
		}
		
		block->write_instruction(ILInstruction::rem);
		block->write_const_type(tl);
		block->write_const_type(tr);		

		
	}

	void ILBuilder::build_and(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::ptr || tr > ILDataType::ptr)) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::bit_and);
		block->write_const_type(tl);
		block->write_const_type(tr);

		
	}

	void ILBuilder::build_or(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::ptr || tr > ILDataType::ptr)) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::bit_or);
		block->write_const_type(tl);
		block->write_const_type(tr);		

		
	}

	void ILBuilder::build_xor(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::ptr || tr > ILDataType::ptr)) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::bit_xor);
		block->write_const_type(tl);
		block->write_const_type(tr);

		
	}

	void ILBuilder::build_mul(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::ptr || tr > ILDataType::ptr)) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::mul);
		block->write_const_type(tl);
		block->write_const_type(tr);

		
	}

	void ILBuilder::build_eq(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::ptr || tr > ILDataType::ptr)) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::eq);
		block->write_const_type(tl);
		block->write_const_type(tr);

		
	}


	void ILBuilder::build_ne(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::ptr || tr > ILDataType::ptr)) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::ne);
		block->write_const_type(tl);
		block->write_const_type(tr);

		
	}

	void ILBuilder::build_gt(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::ptr || tr > ILDataType::ptr)) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::gt);
		block->write_const_type(tl);
		block->write_const_type(tr);

		
	}


	void ILBuilder::build_ge(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::ptr || tr > ILDataType::ptr)) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::ge);
		block->write_const_type(tl);
		block->write_const_type(tr);

		
	}



	void ILBuilder::build_lt(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl > ILDataType::ptr || tr > ILDataType::ptr)) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::lt);
		block->write_const_type(tl);
		block->write_const_type(tr);

		
	}


	void ILBuilder::build_le(ILBlock* block, ILDataType tl, ILDataType tr) {
		if ((tl > ILDataType::ptr || tr > ILDataType::ptr)) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::le);
		block->write_const_type(tl);
		block->write_const_type(tr);

		
	}



	void ILBuilder::build_cast(ILBlock* block, ILDataType tl, ILDataType tr) {
		if ((tl > ILDataType::ptr || tr > ILDataType::ptr)) {
			throw_il_wrong_arguments_error();
		}
		if (tl != tr) {
			block->write_instruction(ILInstruction::cast);
			block->write_const_type(tl);
			block->write_const_type(tr);
		}
	}

	
}