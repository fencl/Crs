#include "Type.h"
#include "Error.h"
#include <iostream>
#include "Declaration.h"
#include "PredefinedTypes.h"
#include "Utilities.h"

namespace Corrosive {


	int Type::compare(CompileContext& ctx,  unsigned char* p1,  unsigned char* p2) {
		return 0;
	}

	int TypeInstance::compare(CompileContext& ctx,  unsigned char* p1,  unsigned char* p2) {
		return owner->compare(ctx, p1, p2);
	}

	int TypeTraitInstance::compare(CompileContext& ctx,  unsigned char* p1,  unsigned char* p2) {
		return owner->compare(ctx, p1, p2);
	}

	int TypeArray::compare(CompileContext& ctx,  unsigned char* p1,  unsigned char* p2) {
		size_t os = owner->compile_size(ctx);
		for (uint64_t i = 0; i < count; i++) {
			owner->compare(ctx, p1, p2);
			
			p1 += os;
			p2 += os;
		}
		return 0;
	}


	int TypeReference::compare(CompileContext& ctx,  unsigned char* p1,  unsigned char* p2) {
		return memcmp(p1, p2, ctx.default_types->t_ptr->compile_size(ctx));
	}


	void Type::move(CompileContext& ctx,  unsigned char* src,  unsigned char* dst) {
		std::cout << "adsf";
	}


	void TypeArray::move(CompileContext& ctx,  unsigned char* src,  unsigned char* dst) {
		size_t os = owner->compile_size(ctx);

		for (uint64_t i = 0; i < count; i++) {
			owner->move(ctx, src, dst);

			src += os;
			src += os;
		}
	}


	void TypeInstance::move(CompileContext& ctx,  unsigned char* src,  unsigned char* dst) {
		owner->move(ctx, src, dst);
	}

	void TypeTraitInstance::move(CompileContext& ctx, unsigned char* src, unsigned char* dst) {
		owner->move(ctx, src, dst);
	}

	void TypeReference::move(CompileContext& ctx,  unsigned char* src,  unsigned char* dst) {
		memcpy(dst, src, ctx.default_types->t_ptr->compile_size(ctx));
	}

	bool Type::rvalue_stacked() {
		return true;
	}

	bool TypeInstance::rvalue_stacked() {
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

	void TypeInstance::print(std::ostream& os) {
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

	void TypeStructure::print(std::ostream& os) {
		os << owner->name.buffer;
	}


	void TypeTrait::print(std::ostream& os) {
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

	uint32_t Type::size(CompileContext& ctx) {
		return 0;
	}
	uint32_t Type::alignment(CompileContext& ctx) {
		return 0;
	}

	uint32_t Type::compile_size(CompileContext& ctx) {
		return 0;
	}
	uint32_t Type::compile_alignment(CompileContext& ctx) {
		return 0;
	}


	uint32_t TypeInstance::size(CompileContext& ctx) {
		return owner->size;
	}
	uint32_t TypeInstance::alignment(CompileContext& ctx) {
		return owner->alignment;
	}

	uint32_t TypeInstance::compile_size(CompileContext& ctx) {
		return owner->compile_size;
	}
	uint32_t TypeInstance::compile_alignment(CompileContext& ctx) {
		return owner->compile_alignment;
	}

	uint32_t TypeTraitInstance::size(CompileContext& ctx) {
		return owner->size;
	}
	uint32_t TypeTraitInstance::alignment(CompileContext& ctx) {
		return owner->alignment;
	}

	uint32_t TypeTraitInstance::compile_size(CompileContext& ctx) {
		return owner->compile_size;
	}
	uint32_t TypeTraitInstance::compile_alignment(CompileContext& ctx) {
		return owner->compile_alignment;
	}

	uint32_t TypeReference::size(CompileContext& ctx) {
		return ctx.default_types->t_ptr->size(ctx);
	}
	uint32_t TypeReference::alignment(CompileContext& ctx) {
		return ctx.default_types->t_ptr->alignment(ctx);
	}

	uint32_t TypeReference::compile_size(CompileContext& ctx) {
		return ctx.default_types->t_ptr->compile_size(ctx);
	}
	uint32_t TypeReference::compile_alignment(CompileContext& ctx) {
		return ctx.default_types->t_ptr->compile_alignment(ctx);
	}

	uint32_t TypeArray::size(CompileContext& ctx) {
		return owner->size(ctx)*count;
	}
	uint32_t TypeArray::alignment(CompileContext& ctx) {
		return owner->alignment(ctx);
	}

	uint32_t TypeArray::compile_size(CompileContext& ctx) {
		return owner->compile_size(ctx) * count;
	}
	uint32_t TypeArray::compile_alignment(CompileContext& ctx) {
		return owner->compile_alignment(ctx);
	}

	void TypeFunction::print(std::ostream& os) {
		os << "fn(";
		std::vector<Type*> args = owner->argument_array_storage.get(argument_array_id);
		for (auto arg = args.begin(); arg != args.end(); arg++) {
			(*arg)->print(os);
		}
		os << ") ";
		return_type->print(os);
	}
}