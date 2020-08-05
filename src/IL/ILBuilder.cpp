#include "IL.h"
#include "../Error.h"
#include <algorithm>
#include <functional>

namespace Corrosive {
	void ILBuilder::build_const_i8(ILBlock* block, int8_t   value) { block->write_instruction(ILInstruction::i8);     block->write_value(sizeof(int8_t), (unsigned char*)&value); }
	void ILBuilder::build_const_i16(ILBlock* block, int16_t  value) { block->write_instruction(ILInstruction::i16);    block->write_value(sizeof(int16_t), (unsigned char*)&value); }
	void ILBuilder::build_const_i32(ILBlock* block, int32_t  value) { block->write_instruction(ILInstruction::i32);    block->write_value(sizeof(int32_t), (unsigned char*)&value); }
	void ILBuilder::build_const_i64(ILBlock* block, int64_t  value) { block->write_instruction(ILInstruction::i64);    block->write_value(sizeof(int64_t), (unsigned char*)&value); }
	void ILBuilder::build_const_u8(ILBlock* block, uint8_t  value) { block->write_instruction(ILInstruction::u8);   block->write_value(sizeof(uint8_t), (unsigned char*)&value); }
	void ILBuilder::build_const_u16(ILBlock* block, uint16_t value) { block->write_instruction(ILInstruction::u16);    block->write_value(sizeof(uint16_t), (unsigned char*)&value); }
	void ILBuilder::build_const_u32(ILBlock* block, uint32_t value) { block->write_instruction(ILInstruction::u32);    block->write_value(sizeof(uint32_t), (unsigned char*)&value); }
	void ILBuilder::build_const_u64(ILBlock* block, uint64_t value) { block->write_instruction(ILInstruction::u64);    block->write_value(sizeof(uint64_t), (unsigned char*)&value); }
	void ILBuilder::build_const_f32(ILBlock* block, float    value) { block->write_instruction(ILInstruction::f32);    block->write_value(sizeof(float), (unsigned char*)&value); }
	void ILBuilder::build_const_f64(ILBlock* block, double   value) { block->write_instruction(ILInstruction::f64);    block->write_value(sizeof(double), (unsigned char*)&value); }
	void ILBuilder::build_const_type(ILBlock* block, void* value) { block->write_instruction(ILInstruction::word); 	  block->write_value(sizeof(void*), (unsigned char*)&value); }
	
	void ILBuilder::build_const_size(ILBlock* block, ILSize value) {
		if (value.value <= UINT8_MAX) {
			uint8_t offset8 = value.value;
			block->write_instruction(ILInstruction::size8);
			block->write_value(sizeof(uint8_t), (unsigned char*)&value.type);
			block->write_value(sizeof(uint8_t), (unsigned char*)&offset8);
		}
		else if (value.value <= UINT16_MAX) {
			uint16_t offset16 = value.value;
			block->write_instruction(ILInstruction::size16);
			block->write_value(sizeof(uint8_t), (unsigned char*)&value.type);
			block->write_value(sizeof(uint16_t), (unsigned char*)&offset16);
		}
		else if (value.value <= UINT32_MAX) {
			block->write_instruction(ILInstruction::size32);
			block->write_value(sizeof(uint8_t), (unsigned char*)&value.type);
			block->write_value(sizeof(uint32_t), (unsigned char*)&value.value);
		}
		else {
			throw std::exception("Compiler error: please fix aoffset type");
		}
	}
	

	void ILBuilder::build_const_slice(ILBlock* block, uint32_t constid, uint64_t size) {
		block->write_instruction(ILInstruction::slice);
		block->write_value(sizeof(uint32_t), (unsigned char*)&constid);
		block->write_value(sizeof(uint64_t), (unsigned char*)&size);
	}



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

	void ILBuilder::build_yield(ILBlock* block, ILDataType type) {
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
	
	void ILBuilder::build_fncall(ILBlock* block, ILFunction* fun) {
		block->write_instruction(ILInstruction::fncall);
		block->write_value(sizeof(uint32_t), (unsigned char*)&(fun->id));
	}

	void ILBuilder::build_constref(ILBlock* block, uint32_t constid) {
		block->write_instruction(ILInstruction::constref);
		block->write_value(sizeof(uint32_t), (unsigned char*)&constid);
	}
	
	void ILBuilder::build_staticref(ILBlock* block, uint32_t constid) {
		block->write_instruction(ILInstruction::staticref);
		block->write_value(sizeof(uint32_t), (unsigned char*)&constid);
	}

	void ILBuilder::build_call(ILBlock* block, uint32_t decl) {
		block->write_instruction(ILInstruction::call);
		block->write_value(sizeof(uint32_t), (unsigned char*)&decl);

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

	void ILBuilder::build_roffset(ILBlock* block, ILDataType from, ILDataType to, ILSize offset) {
		if (offset.value > 0) {
			if (offset.value <= UINT8_MAX) {
				uint8_t offset8 = offset.value;
				block->write_instruction(ILInstruction::roffset8);
				auto off = ILDataTypePair(from, to);
				block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
				block->write_value(sizeof(uint8_t), (unsigned char*)&offset.type);
				block->write_value(sizeof(uint8_t), (unsigned char*)&offset8);
			}
			else if (offset.value <= UINT16_MAX) {
				uint16_t offset16 = offset.value;
				block->write_instruction(ILInstruction::roffset16);
				auto off = ILDataTypePair(from, to);
				block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
				block->write_value(sizeof(uint8_t), (unsigned char*)&offset.type);
				block->write_value(sizeof(uint16_t), (unsigned char*)&offset16);
			}
			else if (offset.value <= UINT32_MAX) {
				block->write_instruction(ILInstruction::roffset32);
				auto off = ILDataTypePair(from, to);
				block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
				block->write_value(sizeof(uint8_t), (unsigned char*)&offset.type);
				block->write_value(sizeof(uint32_t), (unsigned char*)&offset.value);
			}
			else {
				throw std::exception("Compiler error: please fix aoffset type");
			}
		}
		else {
			build_bitcast(block, from, to);
		}

	}


	void ILBuilder::build_null(ILBlock* block) {
		block->write_instruction(ILInstruction::null);

	}

	void ILBuilder::build_memcpy(ILBlock* block, ILSize size) {
		block->write_instruction(ILInstruction::memcpy);
		block->write_value(sizeof(ILSize), (unsigned char*)&size);

	}

	void ILBuilder::build_memcpy_rev(ILBlock* block, ILSize size) {
		block->write_instruction(ILInstruction::memcpy2);
		block->write_value(sizeof(ILSize), (unsigned char*)&size);

	}

	void ILBuilder::build_memcmp(ILBlock* block, ILSize size) {
		block->write_instruction(ILInstruction::memcmp);
		block->write_value(sizeof(ILSize), (unsigned char*)&size);

	}

	void ILBuilder::build_memcmp_rev(ILBlock* block, ILSize size) {
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


	void ILBuilder::build_store_rev(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::store2);
		block->write_const_type(type);
	}


	void ILBuilder::build_local(ILBlock* block, stackid_t id) {
		if (id <= UINT8_MAX) {
			uint8_t id8 = id;
			block->write_instruction(ILInstruction::local8);
			block->write_value(sizeof(uint8_t), (unsigned char*)&id8);
		}
		else if (id <= UINT16_MAX) {
			uint16_t id16 = id;
			block->write_instruction(ILInstruction::local16);
			block->write_value(sizeof(uint16_t), (unsigned char*)&id16);
		}
		else if (id<=UINT32_MAX){
			block->write_instruction(ILInstruction::local32);
			block->write_value(sizeof(uint32_t), (unsigned char*)&id);
		}
		else {
			throw std::exception("compiler error: please fix local type");
		}
	}

	void ILBuilder::build_vtable(ILBlock* block, uint32_t id) {
		block->write_instruction(ILInstruction::vtable);
		block->write_value(sizeof(uint32_t), (unsigned char*)&id);
	}

	void ILBuilder::build_tableoffset(ILBlock* block, tableid_t tableid, tableelement_t itemid) {
		if (itemid > 0) {
			if (tableid <= UINT8_MAX) {
				uint8_t tableid8 = tableid;

				if (itemid <= UINT8_MAX) {
					uint8_t itemid8 = itemid;
					block->write_instruction(ILInstruction::table8offset8);
					block->write_value(sizeof(uint8_t), (unsigned char*)&tableid8);
					block->write_value(sizeof(uint8_t), (unsigned char*)&itemid8);
				}
				else if (itemid <= UINT16_MAX) {
					uint16_t itemid16 = itemid;
					block->write_instruction(ILInstruction::table8offset16);
					block->write_value(sizeof(uint8_t), (unsigned char*)&tableid8);
					block->write_value(sizeof(uint16_t), (unsigned char*)&itemid16);
				}
				else if (itemid <= UINT32_MAX) {
					block->write_instruction(ILInstruction::table8offset32);
					block->write_value(sizeof(uint8_t), (unsigned char*)&tableid8);
					block->write_value(sizeof(uint32_t), (unsigned char*)&itemid);
				}
				else {
					throw std::exception("Compiler error: please fix table item id types");
				}
			}
			else if (tableid <= UINT16_MAX) {
				uint16_t tableid16 = tableid;

				if (itemid <= UINT8_MAX) {
					uint8_t itemid8 = itemid;
					block->write_instruction(ILInstruction::table16offset8);
					block->write_value(sizeof(uint16_t), (unsigned char*)&tableid16);
					block->write_value(sizeof(uint8_t), (unsigned char*)&itemid8);
				}
				else if (itemid <= UINT16_MAX) {
					uint16_t itemid16 = itemid;
					block->write_instruction(ILInstruction::table16offset16);
					block->write_value(sizeof(uint16_t), (unsigned char*)&tableid16);
					block->write_value(sizeof(uint16_t), (unsigned char*)&itemid16);
				}
				else if (itemid <= UINT32_MAX) {
					block->write_instruction(ILInstruction::table16offset32);
					block->write_value(sizeof(uint16_t), (unsigned char*)&tableid16);
					block->write_value(sizeof(uint32_t), (unsigned char*)&itemid);
				}
				else {
					throw std::exception("Compiler error: please fix table item id types");
				}
			}
			else if (tableid <= UINT32_MAX) {

				if (itemid <= UINT8_MAX) {
					uint8_t itemid8 = itemid;
					block->write_instruction(ILInstruction::table32offset8);
					block->write_value(sizeof(uint32_t), (unsigned char*)&tableid);
					block->write_value(sizeof(uint8_t), (unsigned char*)&itemid8);
				}
				else if (itemid <= UINT16_MAX) {
					uint16_t itemid16 = itemid;
					block->write_instruction(ILInstruction::table32offset16);
					block->write_value(sizeof(uint32_t), (unsigned char*)&tableid);
					block->write_value(sizeof(uint16_t), (unsigned char*)&itemid16);
				}
				else if (itemid <= UINT32_MAX) {
					block->write_instruction(ILInstruction::table32offset32);
					block->write_value(sizeof(uint32_t), (unsigned char*)&tableid);
					block->write_value(sizeof(uint32_t), (unsigned char*)&itemid);
				}
				else {
					throw std::exception("Compiler error: please fix table item id types");
				}
			}
			else {
				throw std::exception("Compiler error: please fix tableoffset types");
			}
		}
	}

	void ILBuilder::build_tableroffset(ILBlock* block, ILDataType src, ILDataType dst, tableid_t tableid, tableelement_t itemid) {

		if (itemid > 0) {
			if (tableid <= UINT8_MAX) {
				uint8_t tableid8 = tableid;

				if (itemid <= UINT8_MAX) {
					uint8_t itemid8 = itemid;
					block->write_instruction(ILInstruction::table8roffset8);
					auto off = ILDataTypePair(src, dst);
					block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
					block->write_value(sizeof(uint8_t), (unsigned char*)&tableid8);
					block->write_value(sizeof(uint8_t), (unsigned char*)&itemid8);
				}
				else if (itemid <= UINT16_MAX) {
					uint16_t itemid16 = itemid;
					block->write_instruction(ILInstruction::table8roffset16);
					auto off = ILDataTypePair(src, dst);
					block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
					block->write_value(sizeof(uint8_t), (unsigned char*)&tableid8);
					block->write_value(sizeof(uint16_t), (unsigned char*)&itemid16);
				}
				else if (itemid <= UINT32_MAX) {
					block->write_instruction(ILInstruction::table8roffset32);
					auto off = ILDataTypePair(src, dst);
					block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
					block->write_value(sizeof(uint8_t), (unsigned char*)&tableid8);
					block->write_value(sizeof(uint32_t), (unsigned char*)&itemid);
				}
				else {
					throw std::exception("Compiler error: please fix table item id types");
				}
			}
			else if (tableid <= UINT16_MAX) {
				uint16_t tableid16 = tableid;

				if (itemid <= UINT8_MAX) {
					uint8_t itemid8 = itemid;
					block->write_instruction(ILInstruction::table16roffset8);
					auto off = ILDataTypePair(src, dst);
					block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
					block->write_value(sizeof(uint16_t), (unsigned char*)&tableid16);
					block->write_value(sizeof(uint8_t), (unsigned char*)&itemid8);
				}
				else if (itemid <= UINT16_MAX) {
					uint16_t itemid16 = itemid;
					block->write_instruction(ILInstruction::table16roffset16);
					auto off = ILDataTypePair(src, dst);
					block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
					block->write_value(sizeof(uint16_t), (unsigned char*)&tableid16);
					block->write_value(sizeof(uint16_t), (unsigned char*)&itemid16);
				}
				else if (itemid <= UINT32_MAX) {
					block->write_instruction(ILInstruction::table16roffset32);
					auto off = ILDataTypePair(src, dst);
					block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
					block->write_value(sizeof(uint16_t), (unsigned char*)&tableid16);
					block->write_value(sizeof(uint32_t), (unsigned char*)&itemid);
				}
				else {
					throw std::exception("Compiler error: please fix table item id types");
				}
			}
			else if (tableid <= UINT32_MAX) {

				if (itemid <= UINT8_MAX) {
					uint8_t itemid8 = itemid;
					block->write_instruction(ILInstruction::table32roffset8);
					auto off = ILDataTypePair(src, dst);
					block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
					block->write_value(sizeof(uint32_t), (unsigned char*)&tableid);
					block->write_value(sizeof(uint8_t), (unsigned char*)&itemid8);
				}
				else if (itemid <= UINT16_MAX) {
					uint16_t itemid16 = itemid;
					block->write_instruction(ILInstruction::table32roffset16);
					auto off = ILDataTypePair(src, dst);
					block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
					block->write_value(sizeof(uint32_t), (unsigned char*)&tableid);
					block->write_value(sizeof(uint16_t), (unsigned char*)&itemid16);
				}
				else if (itemid <= UINT32_MAX) {
					block->write_instruction(ILInstruction::table32roffset32);
					auto off = ILDataTypePair(src, dst);
					block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
					block->write_value(sizeof(uint32_t), (unsigned char*)&tableid);
					block->write_value(sizeof(uint32_t), (unsigned char*)&itemid);
				}
				else {
					throw std::exception("Compiler error: please fix table item id types");
				}
			}
			else {
				throw std::exception("Compiler error: please fix tableoffset types");
			}
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

	void ILBuilder::build_clone(ILBlock* block, ILDataType type, uint16_t times) {
		if (times == 2) {
			build_duplicate(block, type);
		}
		else if (times == 0) {
			build_forget(block, type);
		}
		else if (times == 1) {

		}
		else {
			block->write_instruction(ILInstruction::clone);
			block->write_const_type(type);
			block->write_value(sizeof(uint16_t), (unsigned char*)&times);
		}
	}


	void ILBuilder::build_swap(ILBlock* block, ILDataType type) {
		block->write_instruction(ILInstruction::swap);
		block->write_const_type(type);
	}

	void ILBuilder::build_swap2(ILBlock* block, ILDataType type1, ILDataType type2) {
		if (type1 == type2) {
			return build_swap(block, type1);
		}

		block->write_instruction(ILInstruction::swap2);
		auto off = ILDataTypePair(type1, type2);
		block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
	}

	void ILBuilder::build_offset(ILBlock* block, ILSize offset) {
		if (offset.value > 0) {
			if (offset.value <= UINT8_MAX) {
				uint8_t offset8 = offset.value;
				block->write_instruction(ILInstruction::offset8);
				block->write_value(sizeof(uint8_t), (unsigned char*)&offset.type);
				block->write_value(sizeof(uint8_t), (unsigned char*)&offset8);
			}
			else if (offset.value <= UINT16_MAX) {
				uint16_t offset16 = offset.value;
				block->write_instruction(ILInstruction::offset16);
				block->write_value(sizeof(uint8_t), (unsigned char*)&offset.type);
				block->write_value(sizeof(uint16_t), (unsigned char*)&offset16);
			}
			else if (offset.value <= UINT32_MAX) {
				block->write_instruction(ILInstruction::offset32);
				block->write_value(sizeof(uint8_t), (unsigned char*)&offset.type);
				block->write_value(sizeof(uint32_t), (unsigned char*)&offset.value);
			}
			else {
				throw std::exception("Compiler error: please fix aoffset type");
			}
		}
	}

	void ILBuilder::build_aoffset(ILBlock* block, uint32_t offset) {
		if (offset == 0) { return; }

		if (offset<=UINT8_MAX) {
			uint8_t offset8 = offset;
			block->write_instruction(ILInstruction::aoffset8);
			block->write_value(sizeof(uint8_t), (unsigned char*)&offset8);
		}
		else if (offset <= UINT16_MAX) {
			uint16_t offset16 = offset;
			block->write_instruction(ILInstruction::aoffset16);
			block->write_value(sizeof(uint16_t), (unsigned char*)&offset16);
		}
		else if (offset <= UINT32_MAX) {
			block->write_instruction(ILInstruction::aoffset32);
			block->write_value(sizeof(uint32_t), (unsigned char*)&offset);
		}
		else {
			throw std::exception("Compiler error: please fix aoffset type");
		}
	}

	void ILBuilder::build_woffset(ILBlock* block, uint32_t offset) {
		if (offset == 0) { return; }

		if (offset <= UINT8_MAX) {
			uint8_t offset8 = offset;
			block->write_instruction(ILInstruction::woffset8);
			block->write_value(sizeof(uint8_t), (unsigned char*)&offset8);
		}
		else if (offset <= UINT16_MAX) {
			uint16_t offset16 = offset;
			block->write_instruction(ILInstruction::woffset16);
			block->write_value(sizeof(uint16_t), (unsigned char*)&offset16);
		}
		else if (offset <= UINT32_MAX) {
			block->write_instruction(ILInstruction::woffset32);
			block->write_value(sizeof(uint32_t), (unsigned char*)&offset);
		}
		else {
			throw std::exception("Compiler error: please fix aoffset type");
		}
	}

	void ILBuilder::build_aroffset(ILBlock* block, ILDataType from, ILDataType to, uint32_t offset) {
		if (offset>0 || from != to) {
			if (offset > UINT8_MAX) {
				throw std::exception("Compiler error: aroffset > 255 should not be possible");
			}
			block->write_instruction(ILInstruction::aroffset);
			auto off = ILDataTypePair(from, to);
			block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
			block->write_value(sizeof(uint8_t), (unsigned char*)&offset);
		}
	}

	void ILBuilder::build_wroffset(ILBlock* block, ILDataType from, ILDataType to, uint32_t offset) {
		if (offset > 0) {
			if (offset > UINT8_MAX) {
				throw std::exception("Compiler error: wroffset > 255 should not be possible");
			}

			block->write_instruction(ILInstruction::wroffset); 
			auto off = ILDataTypePair(from, to);
			block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
			block->write_value(sizeof(uint8_t), (unsigned char*)&offset);
		}
		else {
			build_bitcast(block, from, to);
		}
	}

	void ILBuilder::build_rtoffset(ILBlock* block) {
		block->write_instruction(ILInstruction::rtoffset);

	}

	void ILBuilder::build_rtoffset_rev(ILBlock* block) {
		block->write_instruction(ILInstruction::rtoffset2);

	}

	void ILBuilder::build_callstart(ILBlock* block) {
		block->write_instruction(ILInstruction::start);

	}

	ILDataType ILBuilder::arith_result(ILDataType l, ILDataType r) {
		return std::max(l, r);
	}

	void ILBuilder::build_add(ILBlock* block, ILDataType tl, ILDataType tr) {

		if (tl >= ILDataType::none || tr >= ILDataType::none) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::add);
		auto off = ILDataTypePair(tl, tr);
		block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
	}

	void ILBuilder::build_sub(ILBlock* block, ILDataType tl, ILDataType tr) {

		if (tl >= ILDataType::none || tr >= ILDataType::none) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::sub);
		auto off = ILDataTypePair(tl, tr);
		block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
	}

	void ILBuilder::build_div(ILBlock* block, ILDataType tl, ILDataType tr) {

		if (tl >= ILDataType::none || tr >= ILDataType::none) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::div);
		auto off = ILDataTypePair(tl, tr);
		block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
	}

	void ILBuilder::build_rem(ILBlock* block, ILDataType tl, ILDataType tr) {

		if (tl >= ILDataType::none || tr >= ILDataType::none) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::rem);
		auto off = ILDataTypePair(tl, tr);
		block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
	}

	void ILBuilder::build_and(ILBlock* block, ILDataType tl, ILDataType tr) {

		if (tl >= ILDataType::none || tr >= ILDataType::none) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::bit_and);
		auto off = ILDataTypePair(tl, tr);
		block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
	}

	void ILBuilder::build_or(ILBlock* block, ILDataType tl, ILDataType tr) {

		if ((tl >= ILDataType::none || tr >= ILDataType::none)) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::bit_or);
		auto off = ILDataTypePair(tl, tr);
		block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
	}

	void ILBuilder::build_xor(ILBlock* block, ILDataType tl, ILDataType tr) {

		if (tl >= ILDataType::none || tr >= ILDataType::none) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::bit_xor);
		auto off = ILDataTypePair(tl, tr);
		block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
	}

	void ILBuilder::build_mul(ILBlock* block, ILDataType tl, ILDataType tr) {

		if (tl >= ILDataType::none || tr >= ILDataType::none) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::mul);
		auto off = ILDataTypePair(tl, tr);
		block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
	}

	void ILBuilder::build_eq(ILBlock* block, ILDataType tl, ILDataType tr) {

		if (tl >= ILDataType::none || tr >= ILDataType::none) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::eq);
		auto off = ILDataTypePair(tl, tr);
		block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
	}


	void ILBuilder::build_ne(ILBlock* block, ILDataType tl, ILDataType tr) {

		if (tl >= ILDataType::none || tr >= ILDataType::none) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::ne);
		auto off = ILDataTypePair(tl, tr);
		block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
	}

	void ILBuilder::build_gt(ILBlock* block, ILDataType tl, ILDataType tr) {

		if (tl >= ILDataType::none || tr >= ILDataType::none) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::gt);
		auto off = ILDataTypePair(tl, tr);
		block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
	}


	void ILBuilder::build_ge(ILBlock* block, ILDataType tl, ILDataType tr) {

		if (tl >= ILDataType::none || tr >= ILDataType::none) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::ge);
		auto off = ILDataTypePair(tl, tr);
		block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
	}



	void ILBuilder::build_lt(ILBlock* block, ILDataType tl, ILDataType tr) {

		if (tl >= ILDataType::none || tr >= ILDataType::none) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::lt);
		auto off = ILDataTypePair(tl, tr);
		block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
	}


	void ILBuilder::build_le(ILBlock* block, ILDataType tl, ILDataType tr) {
		if (tl >= ILDataType::none || tr >= ILDataType::none) {
			throw_il_wrong_arguments_error();
		}

		block->write_instruction(ILInstruction::le);
		auto off = ILDataTypePair(tl, tr);
		block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
	}



	void ILBuilder::build_cast(ILBlock* block, ILDataType tl, ILDataType tr) {
		if (tl >= ILDataType::none || tr >= ILDataType::none) {
			throw_il_wrong_arguments_error();
		}
		if (tl != tr) {
			block->write_instruction(ILInstruction::cast);
			auto off = ILDataTypePair(tl, tr);
			block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
		}
	}

	void ILBuilder::build_bitcast(ILBlock* block, ILDataType tl, ILDataType tr) {
		if (tl != tr) {
			block->write_instruction(ILInstruction::bitcast);
			auto off = ILDataTypePair(tl, tr);
			block->write_value(sizeof(ILDataTypePair), (unsigned char*)&off);
		}
	}

	void ILBuilder::build_combine_dword(ILBlock* block) {
		block->write_instruction(ILInstruction::combinedw);
	}
	void ILBuilder::build_split_dword(ILBlock* block) {
		block->write_instruction(ILInstruction::splitdw);
	}
	void ILBuilder::build_high_word(ILBlock* block) {
		block->write_instruction(ILInstruction::highdw);
	}


}