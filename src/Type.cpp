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
		return owner->compare(eval, p1, p2);
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
	
	int TypeTemplate::compare(ILEvaluator* eval,  unsigned char* p1,  unsigned char* p2) {
		return memcmp(p1, p2, eval->get_compile_pointer_size());
	}


	void Type::move(ILEvaluator* eval,  unsigned char* src,  unsigned char* dst) {
		std::cout << "adsf";
	}


	void TypeArray::move(ILEvaluator* eval,  unsigned char* src,  unsigned char* dst) {
		size_t os = owner->compile_size(eval);

		for (uint64_t i = 0; i < count; i++) {
			owner->move(eval, src, dst);

			src += os;
			src += os;
		}
	}


	void TypeStructureInstance::move(ILEvaluator* eval,  unsigned char* src,  unsigned char* dst) {
		owner->move(eval, src, dst);
	}

	void TypeTraitInstance::move(ILEvaluator* eval, unsigned char* src, unsigned char* dst) {
		owner->move(eval, src, dst);
	}

	void TypeReference::move(ILEvaluator* eval,  unsigned char* src,  unsigned char* dst) {
		memcpy(dst, src, eval->get_compile_pointer_size());
	}

	void TypeTemplate::move(ILEvaluator* eval,  unsigned char* src,  unsigned char* dst) {
		memcpy(dst, src, eval->get_compile_pointer_size());
	}

	bool Type::rvalue_stacked() {
		return false;
	}

	bool TypeStructureInstance::rvalue_stacked() {
		return owner->generator->rvalue_stacked;
	}


	bool TypeTraitInstance::rvalue_stacked() {
		return true;
	}

	TypeReference* Type::generate_reference() {
		if (reference == nullptr) {
			reference = std::make_unique<TypeReference>();
			reference->owner = this;
			reference->rvalue = ILDataType::ptr;
		}

		return reference.get();
	}


	TypeArray* Type::generate_array(unsigned int count) {
		
		auto f = arrays.find(count);
		if (f == arrays.end()) {
			std::unique_ptr<TypeArray> ti = std::make_unique<TypeArray>();
			ti->owner = this;
			ti->rvalue = ILDataType::ptr;
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
		return owner->size;
	}
	uint32_t TypeTraitInstance::alignment(ILEvaluator* eval) {
		return owner->alignment;
	}

	uint32_t TypeTraitInstance::compile_size(ILEvaluator* eval) {
		return owner->compile_size;
	}
	uint32_t TypeTraitInstance::compile_alignment(ILEvaluator* eval) {
		return owner->compile_alignment;
	}

	uint32_t TypeReference::size(ILEvaluator* eval) {
		return eval->get_pointer_size();
	}
	uint32_t TypeReference::alignment(ILEvaluator* eval) {
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
		os << "fn(";
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
}