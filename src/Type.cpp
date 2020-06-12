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
			return (int8_t)memcmp(me, to, size().eval(compiler_arch));
		}
		else {
			throw_runtime_handler_exception(Ctx::eval());
		}

		return 0;
	}

	void Type::move(unsigned char* me, unsigned char* from) {
		if (setjmp(sandbox) == 0) {
			memcpy(me, from, size().eval(compiler_arch));
		}
		else {
			throw_runtime_handler_exception(Ctx::eval());
		}
	}

	void Type::copy(unsigned char* me, unsigned char* from) {
		if (setjmp(sandbox) == 0) {
			memcpy(me, from, size().eval(compiler_arch));
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
			for (uint32_t i = 0; i < count; i++) {
				owner->construct(me);
				me += owner->size().eval(compiler_arch);
			}
		}
	}

	void TypeArray::drop(unsigned char* me) {
		if (has_special_constructor()) {
			for (uint32_t i = 0; i < count; i++) {
				owner->drop(me);
				me += owner->size().eval(compiler_arch);
			}
		}
	}

	void TypeArray::move(unsigned char* me, unsigned char* from) {
		if (has_special_move()) {
			for (uint32_t i = 0; i < count; i++) {
				owner->move(me, from);
				me += owner->size().eval(compiler_arch);
				from += owner->size().eval(compiler_arch);
			}
		}
		else {
			Type::move(me, from);
		}
	}

	void TypeArray::copy(unsigned char* me, unsigned char* from) {
		if (has_special_copy()) {
			for (uint32_t i = 0; i < count; i++) {
				owner->copy(me, from);
				me += owner->size().eval(compiler_arch);
				from += owner->size().eval(compiler_arch);
			}
		}
		else {
			Type::move(me, from);
		}
	}

	int8_t TypeArray::compare(unsigned char* me, unsigned char* to) {
		if (has_special_compare()) {
			for (uint32_t i = 0; i < count; i++) {
				int8_t cmpval = owner->compare(me, to);
				if (cmpval != 0) return cmpval;
				me += owner->size().eval(compiler_arch);
				to += owner->size().eval(compiler_arch);
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
		return ILDataType::ptr;
	}

	ILDataType TypeArray::rvalue() {
		return ILDataType::ptr;
	}

	ILDataType TypeTraitInstance::rvalue() {
		return ILDataType::ptr;
	}

	ILDataType TypeSlice::rvalue() {
		return ILDataType::ptr;
	}

	ILDataType TypeFunction::rvalue() {
		return ILDataType::ptr;
	}

	ILDataType TypeTemplate::rvalue() {
		return ILDataType::ptr;
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
			ti->count = count;
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
		os << "[" << count << "]";
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
		return { 0,0 };
	}
	ILSize Type::alignment() {
		return { 0,0 };
	}


	ILSize TypeStructureInstance::size() {
		return owner->size;
	}
	ILSize TypeStructureInstance::alignment() {
		return owner->alignment;
	}

	ILSize TypeTraitInstance::size() {
		return ILSize::double_ptr;
	}

	ILSize TypeTraitInstance::alignment() {
		return ILSize::single_ptr;
	}


	ILSize TypeReference::size() {
		return ILSize::single_ptr;
	}

	ILSize TypeReference::alignment() {
		return ILSize::single_ptr;
	}

	ILSize TypeSlice::size() {
		return ILSize::double_ptr;
	}

	ILSize TypeSlice::alignment() {
		return ILSize::single_ptr;
	}


	ILSize TypeFunction::size() {
		return ILSize::single_ptr;
	}

	ILSize TypeFunction::alignment() {
		return ILSize::single_ptr;
	}

	ILSize TypeTemplate::size() {
		return ILSize::single_ptr;
	}

	ILSize TypeTemplate::alignment() {
		return ILSize::single_ptr;
	}

	ILSize TypeArray::size() {
		return  owner->size() * count;
	}

	ILSize TypeArray::alignment() {
		return owner->alignment();
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