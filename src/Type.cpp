#include "Type.h"
#include "Error.h"
#include <iostream>
#include "Declaration.h"
#include "Contents.h"
#include "PredefinedTypes.h"
#include "Utilities.h"

namespace Corrosive {


	int TypeInstance::compare(ILEvaluator* eval, void* p1, void* p2) {
		switch (type)
		{
			case Corrosive::TypeInstanceType::type_instance:
				return ((StructureInstance*)owner_ptr)->compare(eval, p1, p2);
			default:
				exit(1);
				return 0;
		}
	}

	void TypeInstance::move(CompileContext& ctx, void* src, void* dst) {
		switch (type)
		{
			case Corrosive::TypeInstanceType::type_instance:
				((StructureInstance*)owner_ptr)->move(ctx, src, dst);
				break;
			default:
				exit(1);
		}
	}

	int Type::compare(ILEvaluator* eval, void* p1, void* p2) {
		if (ref_count > 0) {
			return memcmp(p1, p2, compile_time_size(eval));
		}
		else {
			return type->compare(eval,p1, p2);
		}
	}


	bool TypeInstance::rvalue_stacked() { 
		switch (type)
		{
			case Corrosive::TypeInstanceType::type_instance:
				return ((StructureInstance*)owner_ptr)->generator->rvalue_stacked;
			default:
				exit(1);
				return 0;
		}
	}

	void Type::move(CompileContext& ctx, void* src, void* dst) {
		if (ref_count > 0) {
			memcpy(dst, src, ctx.eval->compile_time_register_size(ILDataType::ptr));
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


	void TypeInstance::print(std::ostream& os) {
		switch (type)
		{
			case Corrosive::TypeInstanceType::type_instance:
				os << ((StructureInstance*)owner_ptr)->generator->name.buffer;
				break;
			case Corrosive::TypeInstanceType::type_template:
				os << ((StructureTemplate*)owner_ptr)->name.buffer;
				os << "(inst)";
				break;
			default:
				exit(1);
		}
	}

	size_t TypeInstance::compile_time_size(ILEvaluator* eval) {
		switch (type)
		{
			case Corrosive::TypeInstanceType::type_instance:
				return ((StructureInstance*)owner_ptr)->iltype->size_in_bytes;
				break;
			default:
				exit(1);
		}
	}

	bool Type::rvalue_stacked() {
		if (ref_count > 0)
			return true;
		else
			return type->rvalue_stacked();
	}

	size_t Type::compile_time_size(ILEvaluator* eval) {
		if (ref_count > 0) {
			return eval->compile_time_register_size(ILDataType::ptr);
		}
		else {
			return type->compile_time_size(eval);
		}
	}

	Type Type::null = { nullptr,0 };
}