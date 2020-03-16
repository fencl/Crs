#include "Type.h"
#include "Error.h"
#include <iostream>
#include "Declaration.h"
#include "Contents.h"
#include "PredefinedTypes.h"
#include "Utilities.h"

namespace Corrosive {
	AbstractType::~AbstractType() {

	}


	AbstractType::AbstractType() : rvalue(ILDataType::undefined) {}
	AbstractType::AbstractType(ILDataType rv) : rvalue(rv) {}


	int AbstractType::compare(void* p1, void* p2) {
		exit(0);
		return -1;
	}

	int DirectType::compare(void* p1, void* p2) {
		exit(0);
		return -1; // not shure what this should mean, maybe it will be some feature later
	}

	int InstanceType::compare(void* p1, void* p2) {
		return owner->compare(p1, p2);
	}

	void AbstractType::move(CompileContext& ctx, void* src, void* dst) {
		exit(0);
	}

	void DirectType::move(CompileContext& ctx, void* src, void* dst) {
		exit(0);
	}

	void InstanceType::move(CompileContext& ctx, void* src, void* dst) {
		owner->move(ctx, src, dst);
	}

	int Type::compare(size_t s, void* p1, void* p2) {
		if (ref_count > 0) {
			return memcmp(p1, p2, s);
		}
		else {
			return type->compare(p1, p2);
		}
	}


	bool AbstractType::rvalue_stacked() { return false; }
	bool DirectType::rvalue_stacked() { return false; }
	bool InstanceType::rvalue_stacked() { return owner->generator->rvalue_stacked; }

	void Type::move(CompileContext& ctx, void* src, void* dst) {
		if (ref_count > 0) {
			memcpy(dst, src, ctx.eval->register_size(ILDataType::ptr));
		}
		else {
			type->move(ctx, src, dst);
		}
	}

	void Type::print(std::ostream& os) {
		for (unsigned int i = 0; i < ref_count; i++)
			os << "&";
		type->print(os);
	}



	void AbstractType::print(std::ostream& os) { os << "<error>"; }
	void DirectType::print(std::ostream& os) { os << owner->name.buffer; }
	void InstanceType::print(std::ostream& os) { os << owner->generator->name.buffer; if (owner->generator->is_generic) { std::cout << "(...)"; } }

	bool Type::rvalue_stacked() {
		if (ref_count > 0)
			return true;
		else
			return type->rvalue_stacked();
	}

	size_t Type::size(CompileContext& ctx) {
		if (ref_count > 0) {
			return ctx.eval->register_size(ILDataType::ptr);
		}
		else {
			if (auto it = dynamic_cast<InstanceType*>(type)) {
				return it->owner->iltype->size_in_bytes;
			}
			else {
				return 0;
			}
		}
	}

	Type Type::null = {nullptr,0};
}