#include "Type.h"
#include "Error.h"
#include <iostream>
#include "Declaration.h"
#include "PredefinedTypes.h"
#include "Utilities.h"
#include <csetjmp>

namespace Corrosive {

	extern jmp_buf sandbox;

	int8_t Type::compare(unsigned char* me, unsigned char* to) {
		if (setjmp(sandbox) == 0) {
			return (int8_t)memcmp(me, to, size().eval(Ctx::global_module(), compiler_arch));
		}
		else {
			throw_runtime_handler_exception(Ctx::eval());
		}

		return 0;
	}

	void Type::move(unsigned char* me, unsigned char* from) {
		if (setjmp(sandbox) == 0) {
			memcpy(me, from, size().eval(Ctx::global_module(), compiler_arch));
		}
		else {
			throw_runtime_handler_exception(Ctx::eval());
		}
	}

	void Type::copy(unsigned char* me, unsigned char* from) {
		if (setjmp(sandbox) == 0) {
			memcpy(me, from, size().eval(Ctx::global_module(), compiler_arch));
		}
		else {
			throw_runtime_handler_exception(Ctx::eval());
		}
	}


	void Type::construct(unsigned char* me) {
		// EMPTY
	}

	void Type::drop(unsigned char* me) {
		// EMPTY
	}


	// ========================================================================   STRUCTURE EVAL COPY/MOVE/CMP/CTOR/DROP

	void TypeStructureInstance::construct(unsigned char* me) {
		if (has_special_constructor()) {
			ILEvaluator* eval = Ctx::eval();
			ILBuilder::eval_fnptr(eval, owner->auto_constructor);
			ILBuilder::eval_callstart(eval);
			ILBuilder::eval_const_ptr(eval, me);
			ILBuilder::eval_call(eval, ILDataType::none, 1);
		}
	}

	void TypeStructureInstance::drop(unsigned char* me) {
		if (has_special_constructor()) {
			ILEvaluator* eval = Ctx::eval();
			ILBuilder::eval_fnptr(eval, owner->auto_destructor);
			ILBuilder::eval_callstart(eval);
			ILBuilder::eval_const_ptr(eval, me);
			ILBuilder::eval_call(eval, ILDataType::none, 1);
		}
	}

	void TypeStructureInstance::move(unsigned char* me,  unsigned char* from) {
		if (has_special_move()) {

			ILEvaluator* eval = Ctx::eval();
			ILBuilder::eval_fnptr(eval, owner->auto_move);
			ILBuilder::eval_callstart(eval);
			ILBuilder::eval_const_ptr(eval, me);
			ILBuilder::eval_const_ptr(eval, from);
			ILBuilder::eval_call(eval, ILDataType::none, 2);
		}
		else {
			Type::move(me, from);
		}
	}

	void TypeStructureInstance::copy(unsigned char* me,  unsigned char* from) {
		if (has_special_copy()) {

			ILEvaluator* eval = Ctx::eval();
			ILBuilder::eval_fnptr(eval, owner->auto_copy);
			ILBuilder::eval_callstart(eval);
			ILBuilder::eval_const_ptr(eval, me);
			ILBuilder::eval_const_ptr(eval, from);
			ILBuilder::eval_call(eval, ILDataType::none, 2);
		}
		else {
			Type::copy(me, from);
		}
	}

	int8_t TypeStructureInstance::compare(unsigned char* me, unsigned char* to) {
		if (has_special_compare()) {

			ILEvaluator* eval = Ctx::eval();
			ILBuilder::eval_fnptr(eval, owner->auto_compare);
			ILBuilder::eval_callstart(eval);
			ILBuilder::eval_const_ptr(eval, me);
			ILBuilder::eval_const_ptr(eval, to);
			ILBuilder::eval_call(eval, ILDataType::u8, 2);
			return eval->pop_register_value<int8_t>();

		}
		else {
			return Type::compare(me, to);
		}
	}

	// ========================================================================   ARRAY EVAL COPY/MOVE/CMP/CTOR/DROP

	void TypeArray::construct(unsigned char* me) {
		if (has_special_constructor()) {
			uint32_t count = Ctx::global_module()->array_tables[table].count;
			size_t elem_stride = align_up(owner->size().eval(Ctx::global_module(), compiler_arch), owner->size().alignment(Ctx::global_module(), compiler_arch));

			for (uint32_t i = 0; i < count; i++) {
				owner->construct(me);
				me += elem_stride;
			}
		}
	}

	void TypeArray::drop(unsigned char* me) {
		if (has_special_constructor()) {
			uint32_t count = Ctx::global_module()->array_tables[table].count;
			size_t elem_stride = align_up(owner->size().eval(Ctx::global_module(), compiler_arch), owner->size().alignment(Ctx::global_module(), compiler_arch));

			for (uint32_t i = 0; i < count; i++) {
				owner->drop(me);
				me += elem_stride;
			}
		}
	}

	void TypeArray::move(unsigned char* me, unsigned char* from) {
		if (has_special_move()) {

			uint32_t count = Ctx::global_module()->array_tables[table].count;
			size_t elem_stride = align_up(owner->size().eval(Ctx::global_module(), compiler_arch), owner->size().alignment(Ctx::global_module(), compiler_arch));

			for (uint32_t i = 0; i < count; i++) {
				owner->move(me, from);
				me += elem_stride;
				from += elem_stride;
			}
		}
		else {
			Type::move(me, from);
		}
	}

	void TypeArray::copy(unsigned char* me, unsigned char* from) {
		if (has_special_copy()) {
			uint32_t count = Ctx::global_module()->array_tables[table].count;
			size_t elem_stride = align_up(owner->size().eval(Ctx::global_module(), compiler_arch), owner->size().alignment(Ctx::global_module(), compiler_arch));

			for (uint32_t i = 0; i < count; i++) {
				owner->copy(me, from);
				me += elem_stride;
				from += elem_stride;
			}
		}
		else {
			Type::move(me, from);
		}
	}

	int8_t TypeArray::compare(unsigned char* me, unsigned char* to) {
		if (has_special_compare()) {
			uint32_t count = Ctx::global_module()->array_tables[table].count;
			size_t elem_stride = align_up(owner->size().eval(Ctx::global_module(), compiler_arch), owner->size().alignment(Ctx::global_module(), compiler_arch));

			for (uint32_t i = 0; i < count; i++) {
				int8_t cmpval = owner->compare(me, to);
				if (cmpval != 0) return cmpval;
				me += elem_stride;
				to += elem_stride;
			}

			return 0;
		}
		else {
			return Type::compare(me, to);
		}
	}




	// ==========================================================================   RVALUE

	bool Type::rvalue_stacked() {
		return false;
	}
	
	bool TypeSlice::rvalue_stacked() {
		return true;
	}

	bool TypeStructureInstance::rvalue_stacked() {
		return owner->structure_type == StructureInstanceType::normal_structure;
	}

	bool TypeTraitInstance::rvalue_stacked() {
		return true;
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
		return ILDataType::word;
	}

	ILDataType TypeSlice::rvalue() {
		return ILDataType::word;
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


	TypeArray* Type::generate_array(unsigned int count) {
		
		auto f = arrays.find(count);
		if (f == arrays.end()) {
			std::unique_ptr<TypeArray> ti = std::make_unique<TypeArray>();
			ti->owner = this;
			
			ti->table = Ctx::global_module()->register_array_table();
			Ctx::global_module()->array_tables[ti->table].count = count;
			Ctx::global_module()->array_tables[ti->table].element = size();
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
		os << owner->name.buffer;
	}

	void TypeTraitInstance::print(std::ostream& os) {
		os << owner->name.buffer;
	}

	void TypeStructureTemplate::print(std::ostream& os) {
		os << owner->name.buffer;
	}

	void TypeFunctionTemplate::print(std::ostream& os) {
		os << owner->name.buffer;
	}

	void TypeTraitTemplate::print(std::ostream& os) {
		os << owner->name.buffer;
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
		os << "[" << Ctx::global_module()->array_tables[table].count << "]";
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
		std::vector<Type*> args = owner->argument_array_storage.get(argument_array_id);
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

	ILContext TypeTraitInstance::context() { return owner->context; }

	ILContext TypeStructureInstance::context() {
		return owner->context;
	}
}