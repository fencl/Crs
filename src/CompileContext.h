#pragma once
#ifndef _compile_context_crs_h
#define _compile_context_crs_h
#include <variant>
#include "IL/IL.h"
#include "Type.h"
#include <vector>
#include <memory>

namespace Corrosive {

	enum class CompileType {
		compile, eval/*, short_circuit*/
	};

	struct CompileValue {
		Type* type;
		bool lvalue;
	};

}
#include "PredefinedTypes.h"
#include "Declaration.h"
#include "Type.h"

#endif