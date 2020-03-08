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
		owner->compile(ctx);
		return true;
	}

	bool DirectType::compile(CompileContext& ctx) {
		owner->compile(ctx);
		return true;
	}
}