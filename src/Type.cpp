#include "Type.h"
#include "Error.h"
#include <iostream>
#include "Declaration.h"
#include "Contents.h"
#include "PredefinedTypes.h"
#include "Utilities.h"

namespace Corrosive {


	int Type::compare(CompileContext& ctx, void* p1, void* p2) {
		std::cerr << "unsupported operation" << std::endl;
		exit(1);

		return 0;
	}

	int TypeInstance::compare(CompileContext& ctx, void* p1, void* p2) {
		owner->compare(ctx, p1, p2);
		return 0;
	}

	int TypeArray::compare(CompileContext& ctx, void* p1, void* p2) {
		unsigned char* pb1 = (unsigned char*)p1;
		unsigned char* pb2 = (unsigned char*)p2;
		size_t os = owner->size(ctx);

		for (uint64_t i = 0; i < count; i++) {
			owner->compare(ctx, pb1, pb2);
			
			pb1 += os;
			pb2 += os;
		}
		return 0;
	}


	int TypeReference::compare(CompileContext& ctx, void* p1, void* p2) {
		return memcmp(p1, p2, sizeof(void*));
	}


	void Type::move(CompileContext& ctx, void* src, void* dst) {
		std::cerr << "unsupported operation" << std::endl;
		exit(1);
	}


	void TypeArray::move(CompileContext& ctx, void* src, void* dst) {
		unsigned char* srcb = (unsigned char*)src;
		unsigned char* dstb = (unsigned char*)dst;
		size_t os = owner->size(ctx);

		for (uint64_t i = 0; i < count; i++) {
			owner->move(ctx, srcb, dstb);

			srcb += os;
			dstb += os;
		}
	}


	void TypeInstance::move(CompileContext& ctx, void* src, void* dst) {
		owner->move(ctx, src, dst);
	}

	void TypeReference::move(CompileContext& ctx, void* src, void* dst) {
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

	unsigned int Type::size(CompileContext& ctx) {
		return 0;
	}
	unsigned int Type::alignment(CompileContext& ctx) {
		return 0;
	}


	unsigned int TypeInstance::size(CompileContext& ctx) {
		return owner->size;
	}
	unsigned int TypeInstance::alignment(CompileContext& ctx) {
		return owner->alignment;
	}


	unsigned int TypeReference::size(CompileContext& ctx) {
		return ctx.default_types->t_ptr->size(ctx);
	}
	unsigned int TypeReference::alignment(CompileContext& ctx) {
		return ctx.default_types->t_ptr->alignment(ctx);
	}

	unsigned int TypeArray::size(CompileContext& ctx) {
		return owner->size(ctx)*count;
	}
	unsigned int TypeArray::alignment(CompileContext& ctx) {
		return owner->alignment(ctx);
	}
}