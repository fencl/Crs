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
	class StackManager;

	struct CompileContext {
		DefaultTypes*	default_types = nullptr;
		Namespace*		inside = nullptr;
		Namespace*		global = nullptr;
		ILFunction*     function = nullptr;
		Type*			function_returns = nullptr;
		ILContext		scope_context = ILContext::both;

		ILModule*		module = nullptr;
		ILEvaluator*	eval = nullptr;
		ILBlock*		scope = nullptr;
		//ILBlock*		scope_exit = nullptr;

		StackManager*	runtime_stack = nullptr;
		StackManager*	compile_stack = nullptr;


		static CompileContext context_stack[1024];
		static uint32_t context_sp;
		static CompileContext& get();
		static void push(CompileContext ctx);
		static void pop();
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