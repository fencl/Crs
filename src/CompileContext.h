#pragma once
#ifndef _compile_context_crs_h
#define _compile_context_crs_h
#include <variant>
#include <vector>
#include "IL/IL.h"
#include "Type.h"

namespace Corrosive {

	enum class CompileType {
		compile, eval/*, short_circuit*/
	};

	class Namespace;
	class DefaultTypes;
	class FunctionInstance;

	struct CompileContext {
		DefaultTypes*	default_types = nullptr;
		Namespace*		inside = nullptr;
		Namespace*		global = nullptr;
		FunctionInstance* function = nullptr;

		ILModule*		module = nullptr;
		ILEvaluator*	eval = nullptr;
		ILBlock*		scope = nullptr;
		//ILBlock*		scope_exit = nullptr;
	};

	struct CompileValue {
		Type* t;
		bool lvalue;
	};
}
#include "PredefinedTypes.h"
#include "Declaration.h"
#include "Type.h"

#endif