#include "Type.h"
#include "Error.h"
#include <iostream>
#include "Declaration.h"
#include "Contents.h"
#include "PredefinedTypes.h"
#include "Utilities.h"

namespace Corrosive {


	int Type::compile_time_compare(ILEvaluator* eval, void* p1, void* p2) {
		std::cerr << "unsupported operation" << std::endl;
		exit(1);

		return 0;
	}

	int TypeInstance::compile_time_compare(ILEvaluator* eval, void* p1, void* p2) {
		owner->compare(eval, p1, p2);
		return 0;
	}

	int TypeArray::compile_time_compare(ILEvaluator* eval, void* p1, void* p2) {
		unsigned char* pb1 = (unsigned char*)p1;
		unsigned char* pb2 = (unsigned char*)p2;
		size_t os = owner->compile_time_size(eval);

		for (uint64_t i = 0; i < count; i++) {
			owner->compile_time_compare(eval, pb1, pb2);
			
			pb1 += os;
			pb2 += os;
		}
		return 0;
	}


	int TypeReference::compile_time_compare(ILEvaluator* eval, void* p1, void* p2) {
		return memcmp(p1, p2, sizeof(void*));
	}


	void Type::compile_time_move(ILEvaluator* eval, void* src, void* dst) {
		std::cerr << "unsupported operation" << std::endl;
		exit(1);
	}


	void TypeArray::compile_time_move(ILEvaluator* eval, void* src, void* dst) {
		unsigned char* srcb = (unsigned char*)src;
		unsigned char* dstb = (unsigned char*)dst;
		size_t os = owner->compile_time_size(eval);

		for (uint64_t i = 0; i < count; i++) {
			owner->compile_time_move(eval, srcb, dstb);

			srcb += os;
			dstb += os;
		}
	}


	void TypeInstance::compile_time_move(ILEvaluator* eval, void* src, void* dst) {
		owner->move(eval, src, dst);
	}

	void TypeReference::compile_time_move(ILEvaluator* eval, void* src, void* dst) {
		memcpy(dst, src, sizeof(void*));
	}

	bool Type::rvalue_stacked() {
		return true;
	}

	bool TypeInstance::rvalue_stacked() {
		return owner->generator->rvalue_stacked;
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

	void TypeInstance::print(std::ostream& os) {
		os << owner->generator->name.buffer;

		if (owner->generator->is_generic) {
			os << "(...)";
		}
	}

	void TypeStructure::print(std::ostream& os) {
		os << owner->name.buffer;
	}

	void TypeReference::print(std::ostream& os) {
		os << "&";
		owner->print(os);
	}
	void TypeArray::print(std::ostream& os) {
		owner->print(os);
		os << "["<<count<<"]";
	}

	size_t Type::compile_time_size(ILEvaluator* eval) {
		return 0;
	}

	unsigned int Type::runtime_size(CompileContext& ctx) {
		return 0;
	}
	unsigned int Type::runtime_alignment(CompileContext& ctx) {
		return 0;
	}


	unsigned int TypeInstance::runtime_size(CompileContext& ctx) {
		return owner->runtime_size;
	}
	unsigned int TypeInstance::runtime_alignment(CompileContext& ctx) {
		return owner->runtime_alignment;
	}

	size_t TypeInstance::compile_time_size(ILEvaluator* eval) {
		return owner->compile_time_size_in_bytes;
	}

	size_t TypeReference::compile_time_size(ILEvaluator* eval) {
		return sizeof(void*);
	}

	unsigned int TypeReference::runtime_size(CompileContext& ctx) {
		return ctx.default_types->t_ptr->runtime_size(ctx);
	}
	unsigned int TypeReference::runtime_alignment(CompileContext& ctx) {
		return ctx.default_types->t_ptr->runtime_alignment(ctx);
	}

	size_t TypeArray::compile_time_size(ILEvaluator* eval) {
		return count * owner->compile_time_size(eval);
	}

	unsigned int TypeArray::runtime_size(CompileContext& ctx) {
		return owner->runtime_size(ctx)*count;
	}
	unsigned int TypeArray::runtime_alignment(CompileContext& ctx) {
		return owner->runtime_alignment(ctx);
	}
}