#pragma once
#ifndef _compile_context_crs_h
#define _compile_context_crs_h
#include <variant>
#include <vector>
#include "IL/IL.h"
#include "Type.h"
#include <vector>

namespace Corrosive {

	enum class CompileType {
		compile, eval/*, short_circuit*/
	};

	class Namespace;
	class DefaultTypes;
	class FunctionInstance;
	class StackManager;

	class Ctx {
	public:
		static DefaultTypes* types();
		static Namespace* global_namespace();
		static ILModule* global_module();
		static ILEvaluator* eval();
		
		static Namespace* workspace();
		static ILFunction* workspace_function();
		static Type* workspace_return();
		static ILContext scope_context();
		static ILBlock* scope();
		static ILBlock* scope_exit();
		static StackManager* stack();
		static StackManager* temp_stack();
		static StackManager* eval_stack();

		static void init(ILModule* global_mod, DefaultTypes* def_types, ILEvaluator* evaluator, Namespace* global_nspc, StackManager* rt_stack, StackManager* cp_stack, StackManager* tmp_stack);

		static void push_workspace(Namespace* workspace);
		static void pop_workspace();
		static void push_scope_context(ILContext context);
		static void pop_scope_context();

		static void push_function(ILFunction* ilf, Type* ret);
		static void pop_function();

		static void push_scope(ILBlock* scope);
		static void pop_scope();

		static void push_scope_exit(ILBlock* scope_exit);
		static void pop_scope_exit();

	private:
		static DefaultTypes* default_types;
		static Namespace* global;
		static ILModule* il_module;
		static ILEvaluator* il_eval;
		static StackManager* runtime_stack;
		static StackManager* temporary_stack;
		static StackManager* compile_stack;

		static std::vector<Namespace*> inside;
		static std::vector<ILFunction*> il_function;
		static std::vector<Type*> function_returns;
		static std::vector<ILContext> scope_ctx;
		static std::vector<ILBlock*> il_scope;
		static std::vector<ILBlock*> il_scope_exit;
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