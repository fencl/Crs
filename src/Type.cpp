#include "Type.h"
#include "Error.h"
#include <iostream>
#include "Declaration.h"
#include "Contents.h"
#include "PredefinedTypes.h"
#include "Utilities.h"

namespace Corrosive {


	int Type::compare(CompileContext& ctx, ILPtr p1, ILPtr p2) {
		return 0;
	}

	int TypeInstance::compare(CompileContext& ctx, ILPtr p1, ILPtr p2) {
		owner->compare(ctx, p1, p2);
		return 0;
	}

	int TypeArray::compare(CompileContext& ctx, ILPtr p1, ILPtr p2) {
		size_t os = owner->size(ctx);

		for (uint64_t i = 0; i < count; i++) {
			owner->compare(ctx, p1, p2);
			
			p1 += os;
			p2 += os;
		}
		return 0;
	}


	int TypeReference::compare(CompileContext& ctx, ILPtr p1, ILPtr p2) {
		return memcmp(ctx.eval->map(p1), ctx.eval->map(p2), ctx.eval->compile_time_register_size(ILDataType::ptr));
	}


	void Type::move(CompileContext& ctx, ILPtr src, ILPtr dst) {}


	void TypeArray::move(CompileContext& ctx, ILPtr src, ILPtr dst) {
		size_t os = owner->size(ctx);

		for (uint64_t i = 0; i < count; i++) {
			owner->move(ctx, src, dst);

			src += os;
			dst += os;
		}
	}


	void TypeInstance::move(CompileContext& ctx, ILPtr src, ILPtr dst) {
		owner->move(ctx, src, dst);
	}

	void TypeReference::move(CompileContext& ctx, ILPtr src, ILPtr dst) {
		memcpy(ctx.eval->map(dst), ctx.eval->map(src), ctx.eval->compile_time_register_size(ILDataType::ptr));
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