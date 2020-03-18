#pragma once
#ifndef _compile_context_crs_h
#define _compile_context_crs_h
#include <variant>
#include <vector>
#include "IL/IL.h"
#include "Type.h"

namespace Corrosive {

	enum class CompileType {
		compile, eval, short_circuit
	};

	class Namespace;
	class DefaultTypes;

	struct CompileContext {
		DefaultTypes*	default_types = nullptr;
		Namespace*		inside = nullptr;
		Namespace*		global = nullptr;
		ILModule*		module = nullptr;
		ILEvaluator*	eval = nullptr;
		ILFunction*		function = nullptr;
		ILBlock*		block = nullptr;
	};

	struct CompileValue {
		Type t;
		bool lvalue;
	};
}
#include "PredefinedTypes.h"
#include "Declaration.h"
#include "Type.h"

#endif