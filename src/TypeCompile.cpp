#include "Type.h"
#include "Declaration.h"
#include "Error.h"
#include "Contents.h"
#include "PredefinedTypes.h"
#include "Utilities.h"
#include <iostream>
#include "Expression.h"

namespace Corrosive {


	bool AbstractType::compile(CompileContext& ctx) {
		return true;
	}

	bool InstanceType::compile(CompileContext& ctx) {
		if (!owner->compile(ctx)) return false;
		return true;
	}

	bool DirectType::compile(CompileContext& ctx) {
		if (!owner->compile(ctx)) return false;
		return true;
	}


	bool Type::compile(CompileContext& ctx) {
		if (ref_count == 0) {
			if (!type->compile(ctx)) return false;
		}

		return true;
	}
}