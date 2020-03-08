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


	AbstractType::AbstractType() {}
	AbstractType::AbstractType(ILDataType rv) : rvalue(rv) {}


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