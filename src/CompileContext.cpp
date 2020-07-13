#include <iostream>
#include <vector>
#include <memory>
#include "IL/IL.h"
#include <memory>
#include "CompileContext.h"
#include "StackManager.h"
#include "Declaration.h"
#include "ConstantManager.h"

namespace Corrosive {
	void Ctx::init(ILModule* global_mod, DefaultTypes* def_types, ILEvaluator* evaluator, Namespace* global_nspc, StackManager* rt_stack, StackManager* cp_stack, StackManager* tmp_stack, ConstantManager* cmgr) {
		il_module = global_mod;
		il_eval = evaluator;
		global = global_nspc;
		default_types = def_types;
		runtime_stack = rt_stack;
		compile_stack = cp_stack;
		temporary_stack = tmp_stack;
		constant_mgr = cmgr;
	}


	ConstantManager* Ctx::const_mgr() {
		return constant_mgr;
	}

	StackManager* Ctx::stack() {
		return runtime_stack;
	}

	StackManager* Ctx::eval_stack() {
		return compile_stack;
	}

	StackManager* Ctx::temp_stack() {
		return temporary_stack;
	}

	DefaultTypes* Ctx::types() {
		return default_types;
	}
	Namespace* Ctx::global_namespace() {
		return global;
	}

	ILModule* Ctx::global_module() {
		return il_module;
	}

	ILEvaluator* Ctx::eval() {
		return il_eval;
	}

	Namespace* Ctx::workspace() {
		return inside.back();
	}

	ILBytecodeFunction* Ctx::workspace_function() {
		return il_function.back();
	}

	Type* Ctx::workspace_return() {
		return function_returns.back();
	}

	ILContext Ctx::scope_context() {
		return scope_ctx.back();
	}

	ILBlock* Ctx::scope() {
		if (il_scope.size() == 0) return nullptr;
		return il_scope.back();
	}

	ILBlock* Ctx::scope_exit() {
		if (il_scope_exit.size() == 0) return nullptr;
		return il_scope_exit.back();
	}

	void Ctx::push_workspace(Namespace* wrkplc) { inside.push_back(wrkplc); }
	void Ctx::pop_workspace() { inside.pop_back(); }
	void Ctx::push_scope_context(ILContext ctx) { scope_ctx.push_back(ctx); }
	void Ctx::pop_scope_context() { scope_ctx.pop_back(); }
	void Ctx::push_function(ILBytecodeFunction* ilf, Type* ret) { il_function.push_back(ilf); function_returns.push_back(ret); }
	void Ctx::pop_function() { il_function.pop_back(); function_returns.pop_back(); }
	void Ctx::push_scope(ILBlock* s) { il_scope.push_back(s); }
	void Ctx::pop_scope() { il_scope.pop_back(); }
	void Ctx::push_scope_exit(ILBlock* s_e) { il_scope_exit.push_back(s_e); }
	void Ctx::pop_scope_exit() { il_scope_exit.pop_back(); }

	DefaultTypes* Ctx::default_types = nullptr;
	StackManager* Ctx::runtime_stack = nullptr;
	StackManager* Ctx::compile_stack = nullptr;
	StackManager* Ctx::temporary_stack = nullptr;
	ILModule* Ctx::il_module = nullptr;
	ILEvaluator* Ctx::il_eval = nullptr;
	Namespace* Ctx::global = nullptr;
	ConstantManager* Ctx::constant_mgr = nullptr;

	std::vector<Namespace*> Ctx::inside;
	std::vector < ILBytecodeFunction* >Ctx::il_function;
	std::vector < Type* > Ctx::function_returns;
	std::vector < ILContext	> Ctx::scope_ctx;
	std::vector < ILBlock* > Ctx::il_scope;
	std::vector < ILBlock* > Ctx::il_scope_exit;


	void Ctx::register_ext_function(std::initializer_list<const char*> path, void(*ptr)(ILEvaluator*)) {
		Namespace* nspc = Ctx::global_namespace();
		FunctionTemplate* func = nullptr;

		for (auto && p : path) {
			Namespace* next_nspc = nullptr;
			StructureTemplate* next_struct = nullptr;
			FunctionTemplate* next_func = nullptr;
			TraitTemplate* next_trait = nullptr;

			nspc->find_name(p, next_nspc, next_struct, next_func, next_trait);
			

			if (next_nspc && !func) {
				nspc = next_nspc;
			}
			else if (next_func && !func) {
				func = next_func;
			}
			else {
				throw std::exception("Wrong path provided to register_ext_function function.");
			}
		}

		FunctionInstance* finst;
		func->generate(nullptr, finst);
		finst->compile();
		((ILExtFunction*)finst->func)->ptr = ptr;
	}

}