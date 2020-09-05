#include "IL.hpp"

namespace Corrosive {

	using il_l_itr = std::vector<std::uint8_t>::iterator&;
	using il_b = ILBlock*&;

	struct il_no_pass {	};


	#define il_exec static errvoid exec(ILInstruction instr,il_l_itr it, il_b block, il_no_pass& pass) {
	#define il_print static errvoid print(ILInstruction instr,il_l_itr it, il_b block, il_no_pass& pass) {
	#define il_ok return err::ok; }

	struct il_memop_instr {
		il_exec {
			auto size = ILBlock::read_data<ILSize>(it);
			switch (instr)
			{
				case ILInstruction::memcpy: return ILBuilder::eval_memcpy(size);
				case ILInstruction::memcpy2: return ILBuilder::eval_memcpy_rev(size);
				case ILInstruction::memcmp: return ILBuilder::eval_memcmp(size);
				case ILInstruction::memcmp2: return ILBuilder::eval_memcmp_rev(size);
			}
		} il_ok;

		il_print {
			auto size = ILBlock::read_data<ILSize>(it);
			switch (instr)
			{
				case ILInstruction::memcpy: std::cout<<"memcpy "; size.print(ILEvaluator::active->parent); std::cout<<"\n"; break;
				case ILInstruction::memcpy2:   std::cout<<"memcpy rev "; size.print(ILEvaluator::active->parent); std::cout<<"\n"; break;
				case ILInstruction::memcmp:  std::cout<<"memcmp "; size.print(ILEvaluator::active->parent); std::cout<<"\n"; break;
				case ILInstruction::memcmp2:  std::cout<<"memcmp rev "; size.print(ILEvaluator::active->parent); std::cout<<"\n"; break;
			}
		} il_ok;
	};

	struct il_clone_instr {
		il_exec {
			auto type = ILBlock::read_data<ILDataType>(it);
			auto times = ILBlock::read_data<std::uint16_t>(it);
			if (!ILBuilder::eval_clone(type, times)) return err::fail;
		} il_ok;

		il_print {
			auto type = ILBlock::read_data<ILDataType>(it);
			auto times = ILBlock::read_data<std::uint16_t>(it);
			std::cout<<"clone "<<times<<"\n";
		} il_ok;
	};

	struct il_swap2_instr {
		il_exec {
			auto p = ILBlock::read_data<ILDataTypePair>(it);
			if (!ILBuilder::eval_swap2(p.first(), p.second())) return err::fail;
		} il_ok;

		il_print {
			auto p = ILBlock::read_data<ILDataTypePair>(it);
			std::cout<<"swap pair ";
			ILBlock::dump_data_type(p.first());
			std::cout<<" ";
			ILBlock::dump_data_type(p.second());
			std::cout<<"\n";
		} il_ok;
	};

	struct il_insintric_instr {
		il_exec {
			auto id = ILBlock::read_data<std::uint8_t>(it);
			if (!ILBuilder::eval_insintric((ILInsintric)id)) return err::fail;
		} il_ok;

		il_print {
			auto id = ILBlock::read_data<std::uint8_t>(it);
			std::cout<<"insintric "<<id<<"\n";
		} il_ok;
	};

	struct il_operator_instr {
		il_exec {
			auto type = ILBlock::read_data<ILDataTypePair>(it);
			switch (instr) {
				case ILInstruction::sub: return ILBuilder::eval_sub(type.first(), type.second());
				case ILInstruction::div: return ILBuilder::eval_div(type.first(), type.second());
				case ILInstruction::rem: return ILBuilder::eval_rem(type.first(), type.second());
				case ILInstruction::mul: return ILBuilder::eval_mul(type.first(), type.second());
				case ILInstruction::add: return ILBuilder::eval_add(type.first(), type.second());
				case ILInstruction::bit_and: return ILBuilder::eval_and(type.first(), type.second());
				case ILInstruction::bit_or: return ILBuilder::eval_or(type.first(), type.second());
				case ILInstruction::bit_xor: return ILBuilder::eval_xor(type.first(), type.second());
				case ILInstruction::eq: return ILBuilder::eval_eq(type.first(), type.second());
				case ILInstruction::ne: return ILBuilder::eval_ne(type.first(), type.second());
				case ILInstruction::gt: return ILBuilder::eval_gt(type.first(), type.second());
				case ILInstruction::lt: return ILBuilder::eval_lt(type.first(), type.second());
				case ILInstruction::ge: return ILBuilder::eval_ge(type.first(), type.second());
				case ILInstruction::le: return ILBuilder::eval_le(type.first(), type.second());
				case ILInstruction::cast: return ILBuilder::eval_cast(type.first(), type.second());
				case ILInstruction::bitcast: return ILBuilder::eval_bitcast(type.first(), type.second());
			}
		} il_ok;

		il_print {
			auto type = ILBlock::read_data<ILDataTypePair>(it);
			switch (instr) {
				case ILInstruction::sub: std::cout<<"sub "; break;
				case ILInstruction::div: std::cout<<"div "; break;
				case ILInstruction::rem: std::cout<<"rem "; break;
				case ILInstruction::mul: std::cout<<"mul "; break;
				case ILInstruction::add: std::cout<<"add "; break;
				case ILInstruction::bit_and: std::cout<<"and "; break;
				case ILInstruction::bit_or: std::cout<<"or "; break;
				case ILInstruction::bit_xor: std::cout<<"xor "; break;
				case ILInstruction::eq: std::cout<<"eq "; break;
				case ILInstruction::ne: std::cout<<"ne "; break;
				case ILInstruction::gt: std::cout<<"gt "; break;
				case ILInstruction::lt: std::cout<<"lt "; break;
				case ILInstruction::ge: std::cout<<"ge "; break;
				case ILInstruction::le: std::cout<<"le "; break;
				case ILInstruction::cast: std::cout<<"cast "; break;
				case ILInstruction::bitcast: std::cout<<"bitcast "; break;
			}

			ILBlock::dump_data_type(type.first());
			std::cout<<" ";
			ILBlock::dump_data_type(type.second());
			std::cout<<"\n";
		} il_ok;
	};

	struct il_regop_instr {
		il_exec {
			auto type = ILBlock::read_data<ILDataType>(it);
			switch (instr) {
				case ILInstruction::swap: return ILBuilder::eval_swap(type);
				case ILInstruction::duplicate: return ILBuilder::eval_duplicate(type);
				case ILInstruction::isnotzero: return ILBuilder::eval_isnotzero(type);
				case ILInstruction::negative: return ILBuilder::eval_negative(type);
				case ILInstruction::forget: return ILBuilder::eval_forget(type);
				case ILInstruction::load: return ILBuilder::eval_load(type);
				case ILInstruction::store: return ILBuilder::eval_store(type);
				case ILInstruction::store2: return ILBuilder::eval_store_rev(type);
				case ILInstruction::yield: return ILBuilder::eval_yield(type);
				case ILInstruction::accept: return ILBuilder::eval_accept(type);
				case ILInstruction::discard: return ILBuilder::eval_discard(type);
				case ILInstruction::rmemcmp: return ILBuilder::eval_rmemcmp(type);
				case ILInstruction::rmemcmp2: return ILBuilder::eval_rmemcmp_rev(type);
				case ILInstruction::ret: it = block->data_pool.end(); return err::ok;
			}
		} il_ok;

		il_print {
			auto type = ILBlock::read_data<ILDataType>(it);
			switch (instr) {
				case ILInstruction::swap: std::cout<<"swap "; break;
				case ILInstruction::duplicate: std::cout<<"duplicate "; break;
				case ILInstruction::isnotzero: std::cout<<"not zero "; break;
				case ILInstruction::negative: std::cout<<"negative "; break;
				case ILInstruction::forget: std::cout<<"forget "; break;
				case ILInstruction::load: std::cout<<"load "; break;
				case ILInstruction::store: std::cout<<"store "; break;
				case ILInstruction::store2: std::cout<<"store rev "; break;
				case ILInstruction::yield: std::cout<<"yield "; break;
				case ILInstruction::accept: std::cout<<"accept "; break;
				case ILInstruction::discard: std::cout<<"discard "; break;
				case ILInstruction::rmemcmp: std::cout<<"rmemcmp "; break;
				case ILInstruction::rmemcmp2: std::cout<<"rmemcmp rev "; break;
				case ILInstruction::ret: std::cout<<"ret "; break;
			}

			ILBlock::dump_data_type(type);
			std::cout<<"\n";
		} il_ok;
	};

	struct il_simple_instr {
		il_exec {
			switch (instr) {
				case ILInstruction::negate: return ILBuilder::eval_negate();
				case ILInstruction::combinedw: return ILBuilder::eval_combine_dword();
				case ILInstruction::highdw: return ILBuilder::eval_high_word();
				case ILInstruction::lowdw: return ILBuilder::eval_low_word();
				case ILInstruction::splitdw: return ILBuilder::eval_split_dword();
				case ILInstruction::start: return ILBuilder::eval_callstart();
				case ILInstruction::null: return ILBuilder::eval_null();
				case ILInstruction::rtoffset: return ILBuilder::eval_rtoffset();
				case ILInstruction::rtoffset2: return ILBuilder::eval_rtoffset_rev();
			}
		} il_ok;

		il_print {
			switch (instr) {
				case ILInstruction::negate: std::cout<<"negate\n"; break;
				case ILInstruction::combinedw: std::cout<<"combine dword\n"; break;
				case ILInstruction::highdw: std::cout<<"high dword\n"; break;
				case ILInstruction::lowdw: std::cout<<"low dword\n"; break;
				case ILInstruction::splitdw: std::cout<<"split dword\n"; break;
				case ILInstruction::start: std::cout<<"start\n"; break;
				case ILInstruction::null: std::cout<<"null\n"; break;
				case ILInstruction::rtoffset: std::cout<<"rtoffset\n"; break;
				case ILInstruction::rtoffset2: std::cout<<"rtoffset rev\n"; break;
			}
		} il_ok;
	};


	template<typename T>
	struct il_size_instr {
		il_exec {
			auto t = ILBlock::read_data<ILSizeType>(it);
			auto v = ILBlock::read_data<T>(it);
			ILSize s(t,(std::uint32_t)v);

			switch (instr) {
				case ILInstruction::offset8:
				case ILInstruction::offset16:
				case ILInstruction::offset32:
					return ILBuilder::eval_offset(s);
					
				case ILInstruction::extract8:
				case ILInstruction::extract16:
				case ILInstruction::extract32:
					return ILBuilder::eval_extract(s);
					
				case ILInstruction::cut8:
				case ILInstruction::cut16:
				case ILInstruction::cut32:
					return ILBuilder::eval_cut(s);

				case ILInstruction::size8:
				case ILInstruction::size16:
				case ILInstruction::size32:
					return ILBuilder::eval_const_size(s);
			}

		} il_ok;

		il_print {
			auto t = ILBlock::read_data<ILSizeType>(it);
			auto v = ILBlock::read_data<T>(it);
			ILSize s(t,(std::uint32_t)v);

			switch (instr) {
				case ILInstruction::offset8:
				case ILInstruction::offset16:
				case ILInstruction::offset32:
					std::cout<<"offset "; break;
					
				case ILInstruction::extract8:
				case ILInstruction::extract16:
				case ILInstruction::extract32:
					std::cout<<"extract "; break;
					
				case ILInstruction::cut8:
				case ILInstruction::cut16:
				case ILInstruction::cut32:
					std::cout<<"cut "; break;

				case ILInstruction::size8:
				case ILInstruction::size16:
				case ILInstruction::size32:
					std::cout<<"size "; break;
			}

			s.print(ILEvaluator::active->parent);
			std::cout<<"\n";
		} il_ok;
	};

	template<typename T>
	struct il_aoffset_instr {
		il_exec {
			auto s = ILBlock::read_data<T>(it);
			return ILBuilder::eval_aoffset((std::uint32_t)s);
		} il_ok;

		il_print {
			auto s = ILBlock::read_data<T>(it);
			std::cout<<"aoffset "<<(std::uint32_t)s<<"\n";
		} il_ok;
	};

	template<typename T>
	struct il_woffset_instr {
		il_exec{
			auto s = ILBlock::read_data<T>(it);
			return ILBuilder::eval_woffset((std::uint32_t)s);
		} il_ok;

		il_print {
			auto s = ILBlock::read_data<T>(it);
			std::cout<<"woffset "<<(std::uint32_t)s<<"\n";
		} il_ok;
	};


	struct il_const_u8_instr {
		il_exec {
			auto s = ILBlock::read_data<std::uint8_t>(it);
			return ILBuilder::eval_const_u8(s);
		} il_ok;

		il_print {
			auto s = ILBlock::read_data<std::uint8_t>(it);
			std::cout<<"u8 " << (std::uint64_t)s<<"\n";
		} il_ok;
	};

	struct il_const_i8_instr {
		il_exec {
			auto s = ILBlock::read_data<std::uint8_t>(it);
			return ILBuilder::eval_const_i8(s);
		} il_ok;

		il_print {
			auto s = ILBlock::read_data<std::int8_t>(it);
			std::cout<<"i8 " << (std::int64_t)s<<"\n";
		} il_ok;
	};

	template<typename T>
	struct il_const_u16_instr {
		il_exec {
			auto s = ILBlock::read_data<T>(it);
			return ILBuilder::eval_const_u16((std::uint16_t)s);
		} il_ok;

		il_print {
			auto s = ILBlock::read_data<T>(it);
			std::cout<<"u16 " << (std::uint64_t)s<<"\n";
		} il_ok;
	};

	template<typename T>
	struct il_const_i16_instr {
		il_exec {
			auto s = ILBlock::read_data<T>(it);
			return ILBuilder::eval_const_i16((std::int16_t)s);
		} il_ok;

		il_print {
			auto s = ILBlock::read_data<T>(it);
			std::cout<<"i16 " << (std::int64_t)s<<"\n";
		} il_ok;
	};

	template<typename T>
	struct il_const_u32_instr {
		il_exec {
			auto s = ILBlock::read_data<T>(it);
			return ILBuilder::eval_const_u32((std::uint32_t)s);
		} il_ok;

		il_print {
			auto s = ILBlock::read_data<T>(it);
			std::cout<<"u32 " << (std::uint64_t)s<<"\n";
		} il_ok;
	};

	template<typename T>
	struct il_const_i32_instr {
		il_exec {
			auto s = ILBlock::read_data<T>(it);
			return ILBuilder::eval_const_i32((std::int32_t)s);
		} il_ok;

		il_print {
			auto s = ILBlock::read_data<T>(it);
			std::cout<<"i32 " << (std::int64_t)s<<"\n";
		} il_ok;
	};

	template<typename T>
	struct il_const_u64_instr {
		il_exec {
			auto s = ILBlock::read_data<T>(it);
			return ILBuilder::eval_const_u64((std::uint64_t)s);
		} il_ok;

		il_print {
			auto s = ILBlock::read_data<T>(it);
			std::cout<<"u64 " << (std::uint64_t)s<<"\n";
		} il_ok;
	};

	template<typename T>
	struct il_const_i64_instr {
		il_exec {
			auto s = ILBlock::read_data<T>(it);
			return ILBuilder::eval_const_i64((std::int64_t)s);
		} il_ok;

		il_print {
			auto s = ILBlock::read_data<T>(it);
			std::cout<<"i64 " << (std::int64_t)s<<"\n";
		} il_ok;
	};

	struct il_const_f32_instr {
		il_exec {
			auto s = ILBlock::read_data<float>(it);
			return ILBuilder::eval_const_f32(s);
		} il_ok;
		il_print {
			auto s = ILBlock::read_data<float>(it);
			std::cout<<"f32 " << s <<"\n";
		} il_ok;
	};

	struct il_const_f64_instr {
		il_exec {
			auto s = ILBlock::read_data<float>(it);
			return ILBuilder::eval_const_f64(s);
		} il_ok;
		il_print {
			auto s = ILBlock::read_data<double>(it);
			std::cout<<"f64 " << s <<"\n";
		} il_ok;
	};

	struct il_const_word_instr {
		il_exec{
			auto s = ILBlock::read_data<void*>(it);
			return ILBuilder::eval_const_word(s);
		} il_ok;
		il_print {
			auto s = ILBlock::read_data<std::size_t>(it);
			std::cout<<"word " << s <<"\n";
		} il_ok;
	};

	struct il_const_dword_instr {
		il_exec {
			auto v1 = ILBlock::read_data<void*>(it);
			auto v2 = ILBlock::read_data<void*>(it);
			dword_t dw = {v1,v2};
			return ILBuilder::eval_const_dword(dw);
		} il_ok;

		il_print {
			auto v1 = ILBlock::read_data<std::size_t>(it);
			auto v2 = ILBlock::read_data<std::size_t>(it);
			std::cout<<"dword " << v1 << ", " << v2 <<"\n";
		} il_ok;
	};	

	template<typename T>
	struct il_identifier_instr {
		il_exec {
			auto v = ILBlock::read_data<T>(it);
			
			switch (instr) {
				case ILInstruction::local8:
				case ILInstruction::local16:
				case ILInstruction::local32: {
					auto& offset = block->parent->calculated_local_offsets[v];
					return ILBuilder::eval_const_word(ILEvaluator::active->stack_base_aligned + offset);
				}

				case ILInstruction::call: return ILBuilder::eval_call(v);
				case ILInstruction::constref: return ILBuilder::eval_constref(v);
				case ILInstruction::staticref: return ILBuilder::eval_staticref(v);
				case ILInstruction::fnptr: return ILBuilder::eval_fnptr(ILEvaluator::active->parent->functions[v].get());
				case ILInstruction::fncall: return ILBuilder::eval_fncall(ILEvaluator::active->parent->functions[v].get());
				case ILInstruction::vtable: return ILBuilder::eval_vtable(v);
				case ILInstruction::jmp: {
					block = block->parent->blocks_memory[v].get();
					it = block->data_pool.begin();
					return err::ok;
				}
			}
		} il_ok;

		il_print {
			auto v = ILBlock::read_data<T>(it);
			
			switch (instr) {
				case ILInstruction::local8:
				case ILInstruction::local16:
				case ILInstruction::local32: 
					std::cout<<"local "; break;

				case ILInstruction::call: std::cout<<"call "; break;
				case ILInstruction::constref: std::cout<<"constref "; break;
				case ILInstruction::staticref: std::cout<<"staticref "; break;
				case ILInstruction::fnptr: std::cout<<"fnptr "; break;
				case ILInstruction::fncall: std::cout<<"fncall "; break;
				case ILInstruction::vtable: std::cout<<"vtable "; break;
				case ILInstruction::jmp: std::cout<<"jmp "; break;
			}

			std::cout << (std::uint64_t)v << "\n";
		} il_ok;
	};
	

	struct il_jmpt_instr {
		il_exec {
			auto v1 = ILBlock::read_data<std::uint32_t>(it);
			auto v2 = ILBlock::read_data<std::uint32_t>(it);
			
			std::uint8_t z;
			if (!ILEvaluator::active->pop_register_value<std::uint8_t>(z)) return err::fail;

			if (z) {
				block = block->parent->blocks_memory[v2].get();
				it = block->data_pool.begin();
			}
			else {
				block = block->parent->blocks_memory[v1].get();
				it = block->data_pool.begin();
			}
		} il_ok;

		il_print {
			auto v1 = ILBlock::read_data<std::uint32_t>(it);
			auto v2 = ILBlock::read_data<std::uint32_t>(it);
			std::cout<<"jmpz " << v1 << ", "<<v2<<"\n";
		} il_ok;
	};


	template<typename T>
	struct il_roffset_instr {
		il_exec {
			auto types = ILBlock::read_data<ILDataTypePair>(it);
			auto t = ILBlock::read_data<ILSizeType>(it);
			auto v = ILBlock::read_data<T>(it);
			return ILBuilder::eval_roffset(types.first(), types.second(), ILSize(t,(std::uint32_t)v));
		} il_ok;

		il_print {
			auto types = ILBlock::read_data<ILDataTypePair>(it);
			auto t = ILBlock::read_data<ILSizeType>(it);
			auto v = ILBlock::read_data<T>(it);
			std::cout<<"roffset ";
			ILBlock::dump_data_type(types.first());
			std::cout<<" ";
			ILBlock::dump_data_type(types.second());
			std::cout<<" ";
			ILSize(t,(std::uint32_t)v).print(ILEvaluator::active->parent);
			std::cout<<"\n";
		} il_ok;
	};


	struct il_aroffset_instr {
		il_exec {
			auto types = ILBlock::read_data<ILDataTypePair>(it);
			auto v = ILBlock::read_data<std::uint8_t>(it);
			return ILBuilder::eval_aroffset(types.first(), types.second(), (std::uint32_t)v);
		} il_ok;
		
		il_print {
			auto types = ILBlock::read_data<ILDataTypePair>(it);
			auto v = ILBlock::read_data<std::uint8_t>(it);
			std::cout<<"aroffset ";
			ILBlock::dump_data_type(types.first());
			std::cout<<" ";
			ILBlock::dump_data_type(types.second());
			std::cout<<" "<<(std::uint64_t)v<<"\n";
		} il_ok;
	};

	struct il_wroffset_instr {
		il_exec {
			auto types = ILBlock::read_data<ILDataTypePair>(it);
			auto v = ILBlock::read_data<std::uint8_t>(it);
			return ILBuilder::eval_wroffset(types.first(), types.second(), (std::uint32_t)v);
		} il_ok;

		il_print {
			auto types = ILBlock::read_data<ILDataTypePair>(it);
			auto v = ILBlock::read_data<std::uint8_t>(it);
			std::cout<<"wroffset ";
			ILBlock::dump_data_type(types.first());
			std::cout<<" ";
			ILBlock::dump_data_type(types.second());
			std::cout<<" "<<(std::uint64_t)v<<"\n";
		} il_ok;
	};


	template<typename T1, typename T2>
	struct il_tableoffset_instr {
		il_exec {
			auto v1 = ILBlock::read_data<T1>(it);
			auto v2 = ILBlock::read_data<T2>(it);
			return ILBuilder::eval_tableoffset(v1, v2);
		} il_ok;

		il_print {
			auto v1 = ILBlock::read_data<T1>(it);
			auto v2 = ILBlock::read_data<T2>(it);
			std::cout<<"tableoffset "<<(std::uint64_t) v1<<" "<<(std::uint64_t)v2<<"\n";
		} il_ok;
	};


	struct il_debug_instr {
		il_exec {
			auto v1 = ILBlock::read_data<std::uint16_t>(it);
			auto v2 = ILBlock::read_data<std::uint16_t>(it);
			return ILBuilder::eval_debug(v1, v2);
		} il_ok;

		il_print {
			auto v1 = ILBlock::read_data<std::uint16_t>(it);
			auto v2 = ILBlock::read_data<std::uint16_t>(it);
		} il_ok;
	};

	struct il_slice_instr {
		il_exec {
			std::uint32_t cid = ILBlock::read_data<std::uint32_t>(it);
			auto st = ILBlock::read_data<ILSizeType>(it);
			auto sv = ILBlock::read_data<std::uint32_t>(it);
			return ILBuilder::eval_const_slice(cid, ILSize(st,sv));
		} il_ok;

		il_print {
			std::uint32_t cid = ILBlock::read_data<std::uint32_t>(it);
			auto st = ILBlock::read_data<ILSizeType>(it);
			auto sv = ILBlock::read_data<std::uint32_t>(it);
			std::cout<<"slice " << cid << " ";
			ILSize(st,sv).print(ILEvaluator::active->parent);
			std::cout<<"\n";
		} il_ok;
	};

	
	template<template<typename Ty1, typename Ty2> class op, typename P, typename E>
	errvoid loop_code(ILBytecodeFunction* fun, P& pass) {
		ILBlock* block = fun->blocks[0];
		auto it = block->data_pool.begin();

		while (true) {
			if (it == block->data_pool.end()) {
				if (E::block_end(it,block)) continue; else break;
			}


			auto inst = block->read_data<ILInstruction>(it);
			E::pre(inst, it, block);

			switch (inst)
			{

				case ILInstruction::memcpy:
				case ILInstruction::memcpy2:
				case ILInstruction::memcmp:
				case ILInstruction::memcmp2: if (!op<il_memop_instr, P>::call(inst, it, block, pass)) return err::fail; break;

				case ILInstruction::clone: if (!op<il_clone_instr, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::swap2: if (!op<il_swap2_instr, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::insintric: if (!op<il_insintric_instr, P>::call(inst, it, block, pass)) return err::fail; break;
				
				case ILInstruction::sub: 
				case ILInstruction::div: 
				case ILInstruction::rem:
				case ILInstruction::mul: 
				case ILInstruction::add: 
				case ILInstruction::bit_and:
				case ILInstruction::bit_or:
				case ILInstruction::bit_xor: 
				case ILInstruction::eq:
				case ILInstruction::ne:
				case ILInstruction::gt:
				case ILInstruction::lt:
				case ILInstruction::ge:
				case ILInstruction::le: 
				case ILInstruction::cast:
				case ILInstruction::bitcast: if (!op<il_operator_instr, P>::call(inst, it, block, pass)) return err::fail; break;

				case ILInstruction::duplicate:
				case ILInstruction::isnotzero:
				case ILInstruction::forget:
				case ILInstruction::negative:
				case ILInstruction::load:
				case ILInstruction::ret:
				case ILInstruction::rmemcmp:
				case ILInstruction::rmemcmp2:
				case ILInstruction::store: 
				case ILInstruction::store2: 
				case ILInstruction::yield: 
				case ILInstruction::accept: 
				case ILInstruction::discard:
				case ILInstruction::swap: if (!op<il_regop_instr, P>::call(inst, it, block, pass)) return err::fail; break;

				case ILInstruction::negate:
				case ILInstruction::combinedw: 
				case ILInstruction::highdw:
				case ILInstruction::lowdw:
				case ILInstruction::splitdw:
				case ILInstruction::rtoffset:
				case ILInstruction::rtoffset2: 
				case ILInstruction::start: 
				case ILInstruction::null: if (!op<il_simple_instr, P>::call(inst, it, block, pass)) return err::fail; break;

				case ILInstruction::size8:
				case ILInstruction::cut8:
				case ILInstruction::extract8:
				case ILInstruction::offset8: if (!op<il_size_instr<std::uint8_t>, P>::call(inst, it, block, pass)) return err::fail; break;

				case ILInstruction::size16:
				case ILInstruction::cut16:
				case ILInstruction::extract16:
				case ILInstruction::offset16: if (!op<il_size_instr<std::uint16_t>, P>::call(inst, it, block, pass)) return err::fail; break;

				case ILInstruction::size32:
				case ILInstruction::cut32:
				case ILInstruction::extract32:
				case ILInstruction::offset32: if (!op<il_size_instr<std::uint32_t>, P>::call(inst, it, block, pass)) return err::fail; break;

				case ILInstruction::aoffset8: if (!op<il_aoffset_instr<std::uint8_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::woffset8: if (!op<il_woffset_instr<std::uint8_t>, P>::call(inst, it, block, pass)) return err::fail; break;

				case ILInstruction::aoffset16: if (!op<il_aoffset_instr<std::uint16_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::woffset16: if (!op<il_woffset_instr<std::uint16_t>, P>::call(inst, it, block, pass)) return err::fail; break;

				case ILInstruction::aoffset32: if (!op<il_aoffset_instr<std::uint32_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::woffset32: if (!op<il_woffset_instr<std::uint32_t>, P>::call(inst, it, block, pass)) return err::fail; break;

				case ILInstruction::i8: if (!op<il_const_i8_instr, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::u8: if (!op<il_const_u8_instr, P>::call(inst, it, block, pass)) return err::fail; break;
				
				case ILInstruction::i16: if (!op<il_const_i16_instr<std::int16_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::i16_8: if (!op<il_const_i16_instr<std::int8_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::u16: if (!op<il_const_u16_instr<std::uint16_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::u16_8: if (!op<il_const_u16_instr<std::uint8_t>, P>::call(inst, it, block, pass)) return err::fail; break;

				case ILInstruction::i32: if (!op<il_const_i32_instr<std::int32_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::i32_16: if (!op<il_const_i32_instr<std::int16_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::i32_8: if (!op<il_const_i32_instr<std::int8_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::u32: if (!op<il_const_u32_instr<std::uint32_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::u32_16: if (!op<il_const_u32_instr<std::uint16_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::u32_8: if (!op<il_const_u32_instr<std::uint8_t>, P>::call(inst, it, block, pass)) return err::fail; break;

				case ILInstruction::i64: if (!op<il_const_i64_instr<std::int64_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::i64_32: if (!op<il_const_i64_instr<std::int32_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::i64_16: if (!op<il_const_i64_instr<std::int16_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::i64_8: if (!op<il_const_i64_instr<std::int8_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::u64: if (!op<il_const_u64_instr<std::uint64_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::u64_32: if (!op<il_const_u64_instr<std::uint32_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::u64_16: if (!op<il_const_u64_instr<std::uint16_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::u64_8: if (!op<il_const_u64_instr<std::uint8_t>, P>::call(inst, it, block, pass)) return err::fail; break;

				case ILInstruction::f32: if (!op<il_const_f32_instr, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::f64: if (!op<il_const_f64_instr, P>::call(inst, it, block, pass)) return err::fail; break;

				case ILInstruction::word: if (!op<il_const_word_instr, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::dword: if (!op<il_const_dword_instr, P>::call(inst, it, block, pass)) return err::fail; break;

				case ILInstruction::local8: if (!op<il_identifier_instr<std::uint8_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::local16: if (!op<il_identifier_instr<std::uint16_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::call:
				case ILInstruction::jmp:
				case ILInstruction::fnptr:
				case ILInstruction::fncall:
				case ILInstruction::vtable:
				case ILInstruction::constref:
				case ILInstruction::staticref: 
				case ILInstruction::local32: if (!op<il_identifier_instr<std::uint32_t>, P>::call(inst, it, block, pass)) return err::fail; break;

				case ILInstruction::jmpz: if (!op<il_jmpt_instr, P>::call(inst, it, block, pass)) return err::fail; break;

				case ILInstruction::roffset8: if (!op<il_roffset_instr<std::uint8_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::roffset16: if (!op<il_roffset_instr<std::uint16_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::roffset32: if (!op<il_roffset_instr<std::uint32_t>, P>::call(inst, it, block, pass)) return err::fail; break;
					
				case ILInstruction::aroffset: if (!op<il_aroffset_instr, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::wroffset: if (!op<il_wroffset_instr, P>::call(inst, it, block, pass)) return err::fail; break;


				case ILInstruction::table8offset8: if (!op<il_tableoffset_instr<std::uint8_t, std::uint8_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::table16offset8: if (!op<il_tableoffset_instr<std::uint16_t, std::uint8_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::table32offset8: if (!op<il_tableoffset_instr<std::uint32_t, std::uint8_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::table8offset16: if (!op<il_tableoffset_instr<std::uint8_t, std::uint16_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::table16offset16: if (!op<il_tableoffset_instr<std::uint16_t, std::uint16_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::table32offset16: if (!op<il_tableoffset_instr<std::uint32_t, std::uint16_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::table8offset32: if (!op<il_tableoffset_instr<std::uint8_t, std::uint32_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::table16offset32: if (!op<il_tableoffset_instr<std::uint16_t, std::uint32_t>, P>::call(inst, it, block, pass)) return err::fail; break;
				case ILInstruction::table32offset32: if (!op<il_tableoffset_instr<std::uint32_t, std::uint32_t>, P>::call(inst, it, block, pass)) return err::fail; break;

				case ILInstruction::debug: if (!op<il_debug_instr, P>::call(inst, it, block, pass)) return err::fail; break;
				
				case ILInstruction::slice: if (!op<il_slice_instr, P>::call(inst, it, block, pass)) return err::fail; break;
			
			default:
				break;
			}

		}
		
		return err::ok;
	}

	struct exec_end_op {
		static bool block_end(il_l_itr it, il_b block) {
			return false;
		}

		static void pre(ILInstruction inst, il_l_itr it, il_b block) {
			
		}
	};

	struct print_end_op {
		static bool block_end(il_l_itr it, il_b block) {
			auto fun = block->parent;
			if (block->id >= fun->blocks.size()-1)
				return false;

			block = fun->blocks[block->id+1];
			it = block->data_pool.begin();

			
			std::cout<<"\nblock "<< block->id <<":\n";

			return true;
		}

		static void pre(ILInstruction inst, il_l_itr it, il_b block) {
			if (inst!= ILInstruction::debug)
				std::cout << "\t";
		}
	};

	template<typename T, typename P>
	struct ILExec {
		static errvoid call(ILInstruction instr,std::vector<std::uint8_t>::iterator& it, ILBlock*& block, P& pass) {
			return T::exec(instr,it, block, pass);
		}
	};

	template<typename T, typename P>
	struct ILPrint {
		static errvoid call(ILInstruction instr,std::vector<std::uint8_t>::iterator& it, ILBlock*& block, P& pass) {
			return T::print(instr, it, block, pass);
		}
	};


	errvoid ILEvaluator::exec_function(ILBytecodeFunction* fun) {
		il_no_pass pass;
		return loop_code<ILExec,il_no_pass,exec_end_op>(fun, pass);
	}

	errvoid ILBytecodeFunction::print_function(ILBytecodeFunction* fun) {
		std::cout<<"function "<<fun->id<<" "<<fun->name<<"\n\n";
		std::cout<<"block 0:\n";
		il_no_pass pass;
		return loop_code<ILPrint,il_no_pass,print_end_op>(fun, pass);
	}

}