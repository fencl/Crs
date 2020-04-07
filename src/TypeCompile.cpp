#include "Type.h"
#include "Declaration.h"
#include "Error.h"
#include "PredefinedTypes.h"
#include "Utilities.h"
#include <iostream>
#include "Expression.h"

namespace Corrosive {

	bool Type::compile(CompileContext& ctx) {
		return true;
	}

	bool TypeInstance::compile(CompileContext& ctx) {
		return owner->compile(ctx);
	}

	bool TypeStructure::compile(CompileContext& ctx) {
		return owner->compile(ctx);
	}

	bool TypeArray::compile(CompileContext& ctx) {
		return owner->compile(ctx);
	}
}