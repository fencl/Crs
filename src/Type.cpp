#include "Type.h"
#include "Error.h"
#include <iostream>
#include "Declaration.h"
#include "PredefinedTypes.h"
#include "Utilities.h"
#include <csetjmp>
#include "Compiler.h"

namespace Corrosive {

	extern jmp_buf sandbox;

	int8_t Type::compare_for_generic_storage(unsigned char* me, unsigned char* to) {
		if (setjmp(sandbox) == 0) {
			auto v = memcmp(me, to, size().eval(Compiler::current()->global_module(), compiler_arch));
			if (v < 0) return -1;
			else if (v > 0) return 1;
			else return 0;
		}
		else {
			throw_runtime_handler_exception(Compiler::current()->evaluator());
		}

		return 0;
	}

	void Type::copy_to_generic_storage(unsigned char* me, unsigned char* to) {
		if (setjmp(sandbox) == 0) {
			memcpy(to, me, size().eval(Compiler::current()->global_module(), compiler_arch));
		}
		else {
			throw_runtime_handler_exception(Compiler::current()->evaluator());
		}
	}

	int8_t TypeSlice::compare_for_generic_storage(unsigned char* me, unsigned char* to) {
		dword_t* me_dw = (dword_t*)me;
		dword_t* to_dw = (dword_t*)to;

		if (setjmp(sandbox) == 0) {

			if (me_dw->p2 < to_dw->p2) return -1;
			else if (me_dw->p2 > to_dw->p2) return 1;
			auto v = memcmp(me_dw->p1, to_dw->p1, (size_t)me_dw->p2);
			if (v < 0) return -1;
			else if (v > 0) return 1;
			else return 0;
		}
		else {
			throw_runtime_handler_exception(Compiler::current()->evaluator());
		}

		return 0;
	}

	void TypeSlice::copy_to_generic_storage(unsigned char* me, unsigned char* to) {
		dword_t* me_dw = (dword_t*)me;
		dword_t* to_dw = (dword_t*)to;

		if (setjmp(sandbox) == 0) {
			to_dw->p1 = Compiler::current()->constant_manager()->register_generic_storage((uint8_t*)me_dw->p1, (size_t)me_dw->p2, owner);
			to_dw->p2 = me_dw->p2;
		}
		else {
			throw_runtime_handler_exception(Compiler::current()->evaluator());
		}
	}


	void Type::constantize(Cursor& err, unsigned char* target, unsigned char* source) {
		throw_specific_error(err, "Cannot create constant value of this type");
	}



	void TypeStructureInstance::constantize(Cursor& err, unsigned char* target, unsigned char* source) {
		if (owner->structure_type == StructureInstanceType::primitive_structure && rvalue() != ILDataType::word && rvalue() != ILDataType::none && rvalue() != ILDataType::dword) {
			if (target != nullptr) {
				switch (rvalue())
				{
					case ILDataType::u8: *(uint8_t*)target = *(uint8_t*)source; break;
					case ILDataType::u16: *(uint16_t*)target = *(uint16_t*)source; break;
					case ILDataType::u32: *(uint32_t*)target = *(uint32_t*)source; break;
					case ILDataType::u64: *(uint64_t*)target = *(uint64_t*)source; break;
					case ILDataType::i8: *(int8_t*)target = *(int8_t*)source; break;
					case ILDataType::i16: *(int16_t*)target = *(int16_t*)source; break;
					case ILDataType::i32: *(int32_t*)target = *(int32_t*)source; break;
					case ILDataType::i64: *(int64_t*)target = *(int64_t*)source; break;
					case ILDataType::f32: *(float*)target = *(float*)source; break;
					case ILDataType::f64: *(double*)target = *(double*)source; break;
					default: break;
				}
			}
			else {
				switch (rvalue())
				{
					case ILDataType::u8: ILBuilder::build_const_u8(Compiler::current()->scope(), *(uint8_t*)source); break;
					case ILDataType::u16: ILBuilder::build_const_u16(Compiler::current()->scope(), *(uint16_t*)source); break;
					case ILDataType::u32: ILBuilder::build_const_u32(Compiler::current()->scope(), *(uint32_t*)source); break;
					case ILDataType::u64: ILBuilder::build_const_u64(Compiler::current()->scope(), *(uint64_t*)source); break;
					case ILDataType::i8: ILBuilder::build_const_i8(Compiler::current()->scope(), *(int8_t*)source); break;
					case ILDataType::i16: ILBuilder::build_const_i16(Compiler::current()->scope(), *(int16_t*)source); break;
					case ILDataType::i32: ILBuilder::build_const_i32(Compiler::current()->scope(), *(int32_t*)source); break;
					case ILDataType::i64: ILBuilder::build_const_i64(Compiler::current()->scope(), *(int64_t*)source); break;
					case ILDataType::f32: ILBuilder::build_const_f32(Compiler::current()->scope(), *(float*)source); break;
					case ILDataType::f64: ILBuilder::build_const_f64(Compiler::current()->scope(), *(double*)source); break;
					default: break;
				}
			}
		}
		else {
			throw_specific_error(err, "Cannot create constant value of this type");
		}
	}

	void TypeSlice::constantize(Cursor& err, unsigned char* target, unsigned char* source) {
		dword_t me = *(dword_t*)source;
		std::string data((size_t)me.p2,'\0');
		uint8_t* ptr_src = (uint8_t*)me.p1;
		uint8_t* ptr_dst = (uint8_t*)data.data();
		size_t elem_size = owner->size().eval(Compiler::current()->global_module(), compiler_arch);
		for (size_t i=0;i<((size_t)me.p2)/elem_size; ++i) {
			owner->constantize(err, ptr_dst, ptr_src);
			ptr_src += elem_size;
			ptr_dst += elem_size;
		}

		auto val = Compiler::current()->constant_manager()->register_string_literal(data);

		if (target) {
			memcpy(target, val.first.data(), val.first.size());
		}
		else {
			ILBuilder::build_const_slice(Compiler::current()->scope(), val.second, val.first.size());
		}
	}


	// ==========================================================================   RVALUE
	
	bool Type::rvalue_stacked() {
		return false;
	}
	
	bool TypeSlice::rvalue_stacked() {
		return false;
	}

	bool TypeStructureInstance::rvalue_stacked() {
		return owner->structure_type == StructureInstanceType::normal_structure;
	}

	bool TypeTraitInstance::rvalue_stacked() {
		return false;
	}
	
	ILDataType Type::rvalue() {
		return ILDataType::none;
	}

	ILDataType TypeStructureInstance::rvalue() {
		return owner->rvalue;
	}

	ILDataType TypeReference::rvalue() {
		return ILDataType::word;
	}

	ILDataType TypeArray::rvalue() {
		return ILDataType::word;
	}

	ILDataType TypeTraitInstance::rvalue() {
		return ILDataType::dword;
	}

	ILDataType TypeSlice::rvalue() {
		return ILDataType::dword;
	}

	ILDataType TypeFunction::rvalue() {
		return ILDataType::word;
	}

	ILDataType TypeTemplate::rvalue() {
		return ILDataType::word;
	}

	// ==============================================================================================



	TypeReference* Type::generate_reference() {
		if (reference == nullptr) {
			reference = std::make_unique<TypeReference>();
			reference->owner = this;
		}

		return reference.get();
	}
	
	TypeSlice* Type::generate_slice() {
		if (slice == nullptr) {
			slice = std::make_unique<TypeSlice>();
			slice->owner = this;
		}

		return slice.get();
	}


	TypeArray* Type::generate_array(uint32_t count) {
		
		auto f = arrays.find(count);
		if (f == arrays.end()) {
			std::unique_ptr<TypeArray> ti = std::make_unique<TypeArray>();
			ti->owner = this;
			
			ti->table = Compiler::current()->global_module()->register_array_table();
			Compiler::current()->global_module()->array_tables[ti->table].count = count;
			Compiler::current()->global_module()->array_tables[ti->table].element = size();
			TypeArray* rt = ti.get();
			arrays[count] = std::move(ti);
			return rt;
		}
		else {
			return f->second.get();
		}
	}



	// ==============================================================================================  PRINT

	void Type::print(std::ostream& os) {
		os << "?";
	}

	void TypeStructureInstance::print(std::ostream& os) {
		os << ((AstStructureNode*)owner->ast_node)->name_string;
	}

	void TypeTraitInstance::print(std::ostream& os) {
		os << owner->ast_node->name_string;
	}

	void TypeStructureTemplate::print(std::ostream& os) {
		os << owner->ast_node->name_string;
	}

	void TypeFunctionTemplate::print(std::ostream& os) {
		os << owner->ast_node->name_string;
	}

	void TypeTraitTemplate::print(std::ostream& os) {
		os << owner->ast_node->name_string;
	}

	void TypeReference::print(std::ostream& os) {
		os << "&";
		owner->print(os);
	}

	void TypeSlice::print(std::ostream& os) {
		os << "[]";
		owner->print(os);
	}

	void TypeArray::print(std::ostream& os) {
		os << "[" << Compiler::current()->global_module()->array_tables[table].count << "]";
		owner->print(os);
	}

	void TypeFunction::print(std::ostream& os) {
		os << "fn";
		if (ptr_context == ILContext::compile) {
			os << " compile";
		}
		else if (ptr_context == ILContext::runtime) {
			os << " runtime";
		}
		os << "(";
		std::vector<Type*> args = Compiler::current()->types()->argument_array_storage.get(argument_array_id);
		for (auto arg = args.begin(); arg != args.end(); arg++) {
			if (arg != args.begin())
				os << ", ";
			(*arg)->print(os);
		}
		os << ") ";
		return_type->print(os);
	}

	void TypeTemplate::print(std::ostream& os) {
		os << "type(";
		std::vector<Type*> args = owner->argument_array_storage.get(argument_array_id);
		for (auto arg = args.begin(); arg != args.end(); arg++) {
			if (arg != args.begin())
				os << ", ";
			(*arg)->print(os);
		}
		os << ")";
	}

	// ==============================================================================================  SIZE/ALIGNMENT

	ILSize Type::size() {
		return { ILSizeType::absolute,0 };
	}


	ILSize TypeStructureInstance::size() {
		return owner->size;
	}

	ILSize TypeTraitInstance::size() {
		return ILSize::double_ptr;
	}


	ILSize TypeReference::size() {
		return ILSize::single_ptr;
	}

	ILSize TypeSlice::size() {
		return ILSize::double_ptr;
	}


	ILSize TypeFunction::size() {
		return ILSize::single_ptr;
	}
	ILSize TypeTemplate::size() {
		return ILSize::single_ptr;
	}

	ILSize TypeArray::size() {
		return  ILSize(ILSizeType::array, table);
	}


	// ==============================================================================================  CONTEXT


	ILContext Type::context() { return ILContext::compile; }

	ILContext TypeFunction::context() { return ptr_context; }

	ILContext TypeReference::context() { return owner->context(); }

	ILContext TypeSlice::context() { return owner->context(); }

	ILContext TypeArray::context() { return owner->context(); }

	ILContext TypeTraitInstance::context() { return owner->ast_node->context; }

	ILContext TypeStructureInstance::context() {
		return owner->context;
	}

}