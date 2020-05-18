#include "Type.h"
#include "Error.h"
#include <iostream>
#include "Declaration.h"
#include "PredefinedTypes.h"
#include "Utilities.h"

namespace Corrosive {


	int Type::compare(ILEvaluator* eval,  unsigned char* p1,  unsigned char* p2) {
		return 0;
	}

	int TypeStructureInstance::compare(ILEvaluator* eval,  unsigned char* p1,  unsigned char* p2) {
		return owner->compare(eval, p1, p2);
	}

	int TypeTraitInstance::compare(ILEvaluator* eval,  unsigned char* p1,  unsigned char* p2) {
		return memcmp(p1, p2, eval->get_compile_pointer_size() * 2);
	}

	int TypeArray::compare(ILEvaluator* eval,  unsigned char* p1,  unsigned char* p2) {
		size_t os = owner->compile_size(eval);
		for (uint64_t i = 0; i < count; i++) {
			owner->compare(eval, p1, p2);
			
			p1 += os;
			p2 += os;
		}
		return 0;
	}


	int TypeReference::compare(ILEvaluator* eval,  unsigned char* p1,  unsigned char* p2) {
		return memcmp(p1, p2, eval->get_compile_pointer_size());
	}
	
	int TypeFunction::compare(ILEvaluator* eval,  unsigned char* p1,  unsigned char* p2) {
		return memcmp(p1, p2, eval->get_compile_pointer_size());
	}
	
	int TypeSlice::compare(ILEvaluator* eval,  unsigned char* p1,  unsigned char* p2) {
		return memcmp(p1, p2, eval->get_compile_pointer_size()*2);
	}
	
	int TypeTemplate::compare(ILEvaluator* eval,  unsigned char* p1,  unsigned char* p2) {
		return memcmp(p1, p2, eval->get_compile_pointer_size());
	}


	void Type::move(ILEvaluator* eval,  unsigned char* src,  unsigned char* dst) {
		std::cout << "adsf";
	}

	void Type::copy(ILEvaluator* eval, unsigned char* src, unsigned char* dst) {
		move(eval, src,dst);
	}


	void TypeArray::move(ILEvaluator* eval,  unsigned char* src,  unsigned char* dst) {
		size_t os = owner->compile_size(eval);

		for (uint64_t i = 0; i < count; i++) {
			owner->move(eval, src, dst);

			src += os;
			src += os;
		}
	}


	void Type::construct(ILEvaluator* eval, unsigned char* ptr) {

	}

	void Type::drop(ILEvaluator* eval, unsigned char* ptr) {

	}


	void TypeStructureInstance::construct(ILEvaluator* eval, unsigned char* ptr) {
		ILBuilder::eval_fnptr(eval, owner->auto_constructor);
		ILBuilder::eval_callstart(eval);
		ILBuilder::eval_const_ptr(eval, ptr);
		ILBuilder::eval_call(eval, ILDataType::none, 1);
	}

	void TypeStructureInstance::drop(ILEvaluator* eval, unsigned char* ptr) {
		ILBuilder::eval_fnptr(eval, owner->auto_destructor);
		ILBuilder::eval_callstart(eval);
		ILBuilder::eval_const_ptr(eval, ptr);
		ILBuilder::eval_call(eval, ILDataType::none, 1);
	}

	void TypeArray::copy(ILEvaluator* eval, unsigned char* src, unsigned char* dst) {
		size_t os = owner->compile_size(eval);

		for (uint64_t i = 0; i < count; i++) {
			owner->copy(eval, src, dst);

			src += os;
			src += os;
		}
	}

	void TypeStructureInstance::move(ILEvaluator* eval,  unsigned char* src,  unsigned char* dst) {
		owner->move(eval, src, dst);
	}
	void TypeStructureInstance::copy(ILEvaluator* eval,  unsigned char* src,  unsigned char* dst) {
		owner->copy(eval, src, dst);
	}

	void TypeTraitInstance::move(ILEvaluator* eval, unsigned char* src, unsigned char* dst) {
		memcpy(dst, src, eval->get_compile_pointer_size() * 2);
	}

	void TypeTraitInstance::copy(ILEvaluator* eval, unsigned char* src, unsigned char* dst) {
		memcpy(dst, src, eval->get_compile_pointer_size() * 2);
	}
	



	void TypeReference::move(ILEvaluator* eval,  unsigned char* src,  unsigned char* dst) {
		memcpy(dst, src, eval->get_compile_pointer_size());
	}
	
	void TypeFunction::move(ILEvaluator* eval,  unsigned char* src,  unsigned char* dst) {
		memcpy(dst, src, eval->get_compile_pointer_size());
	}
	
	void TypeSlice::move(ILEvaluator* eval,  unsigned char* src,  unsigned char* dst) {
		memcpy(dst, src, eval->get_compile_pointer_size()*2);
	}

	void TypeTemplate::move(ILEvaluator* eval,  unsigned char* src,  unsigned char* dst) {
		memcpy(dst, src, eval->get_compile_pointer_size());
	}



	bool Type::rvalue_stacked() {
		return false;
	}

	
	bool TypeSlice::rvalue_stacked() {
		return true;
	}

	bool TypeStructureInstance::rvalue_stacked() {
		return owner->structure_type == StructureInstanceType::normal_structure;
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

	bool TypeTraitInstance::rvalue_stacked() {
		return true;
	}

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

	void Type::print(std::ostream& os) {
		os << "?";
	}

	void TypeStructureInstance::print(std::ostream& os) {
		os << owner->generator->name.buffer;

		if (owner->generator->is_generic) {
			os << "(...)";
		}
	}

	void TypeTraitInstance::print(std::ostream& os) {
		os << owner->generator->name.buffer;

		if (owner->generator->is_generic) {
			os << "(...)";
		}
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

	uint32_t Type::size(ILEvaluator* eval) {
		return 0;
	}
	uint32_t Type::alignment(ILEvaluator* eval) {
		return 0;
	}

	uint32_t Type::compile_size(ILEvaluator* eval) {
		return 0;
	}
	uint32_t Type::compile_alignment(ILEvaluator* eval) {
		return 0;
	}




	uint32_t TypeStructureInstance::size(ILEvaluator* eval) {
		return owner->size;
	}
	uint32_t TypeStructureInstance::alignment(ILEvaluator* eval) {
		return owner->alignment;
	}

	uint32_t TypeStructureInstance::compile_size(ILEvaluator* eval) {
		return owner->compile_size;
	}
	uint32_t TypeStructureInstance::compile_alignment(ILEvaluator* eval) {
		return owner->compile_alignment;
	}

	uint32_t TypeTraitInstance::size(ILEvaluator* eval) {
		return eval->get_pointer_size() * 2;
	}
	uint32_t TypeTraitInstance::alignment(ILEvaluator* eval) {
		return eval->get_pointer_size();
	}

	uint32_t TypeTraitInstance::compile_size(ILEvaluator* eval) {
		return eval->get_compile_pointer_size() * 2;
	}
	uint32_t TypeTraitInstance::compile_alignment(ILEvaluator* eval) {
		return eval->get_compile_pointer_size();
	}

	uint32_t TypeReference::size(ILEvaluator* eval) {
		return eval->get_pointer_size();
	}
	uint32_t TypeReference::alignment(ILEvaluator* eval) {
		return eval->get_pointer_size();
	}
	
	uint32_t TypeSlice::size(ILEvaluator* eval) {
		return eval->get_pointer_size();
	}
	uint32_t TypeSlice::alignment(ILEvaluator* eval) {
		return eval->get_pointer_size();
	}

	uint32_t TypeTemplate::compile_size(ILEvaluator* eval) {
		return eval->get_compile_pointer_size();
	}
	uint32_t TypeTemplate::compile_alignment(ILEvaluator* eval) {
		return eval->get_compile_pointer_size();
	}

	uint32_t TypeReference::compile_size(ILEvaluator* eval) {
		return eval->get_compile_pointer_size();
	}
	uint32_t TypeReference::compile_alignment(ILEvaluator* eval) {
		return eval->get_compile_pointer_size();
	}

	uint32_t TypeSlice::compile_size(ILEvaluator* eval) {
		return eval->get_compile_pointer_size()*2;
	}
	uint32_t TypeSlice::compile_alignment(ILEvaluator* eval) {
		return eval->get_compile_pointer_size()*2;
	}

	uint32_t TypeFunction::compile_size(ILEvaluator* eval) {
		return eval->get_compile_pointer_size();
	}
	uint32_t TypeFunction::compile_alignment(ILEvaluator* eval) {
		return eval->get_compile_pointer_size();
	}

	uint32_t TypeFunction::size(ILEvaluator* eval) {
		return eval->get_pointer_size();
	}
	uint32_t TypeFunction::alignment(ILEvaluator* eval) {
		return eval->get_pointer_size();
	}

	uint32_t TypeArray::size(ILEvaluator* eval) {
		return owner->size(eval)*count;
	}
	uint32_t TypeArray::alignment(ILEvaluator* eval) {
		return owner->alignment(eval);
	}

	uint32_t TypeArray::compile_size(ILEvaluator* eval) {
		return owner->compile_size(eval) * count;
	}
	uint32_t TypeArray::compile_alignment(ILEvaluator* eval) {
		return owner->compile_alignment(eval);
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