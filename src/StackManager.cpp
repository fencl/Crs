#include "StackManager.h"


namespace Corrosive {

	void Ctx::init(ILModule* global_mod, DefaultTypes* def_types, ILEvaluator* evaluator, Namespace* global_nspc, StackManager* rt_stack, StackManager* cp_stack, StackManager* tmp_stack) {
		il_module = global_mod;
		il_eval = evaluator;
		global = global_nspc;
		default_types = def_types;
		runtime_stack = rt_stack;
		compile_stack = cp_stack;
		temporary_stack = tmp_stack;
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

	ILFunction* Ctx::workspace_function() {
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
	void Ctx::push_function(ILFunction* ilf, Type* ret) { il_function.push_back(ilf); function_returns.push_back(ret); }
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

	std::vector<Namespace*> Ctx::inside;
	std::vector < ILFunction* >Ctx::il_function;
	std::vector < Type* > Ctx::function_returns;
	std::vector < ILContext	> Ctx::scope_ctx;
	std::vector < ILBlock* > Ctx::il_scope;
	std::vector < ILBlock* > Ctx::il_scope_exit;

	






	void StackManager::push() {
		stack.push_back(std::move(decltype(stack)::value_type()));
		stack_namespace.push_back(std::move(decltype(stack_namespace)::value_type()));
		stack_state.push_back(std::move(decltype(stack_state)::value_type()));

		push_block();
	}

	void StackManager::pop() {
		pop_block();

		stack.pop_back();
		stack_namespace.pop_back();
		stack_state.pop_back();
	}

	void StackManager::push_block() {
		stack_state.back().push_back(stack.back().size());
	}
	
	void StackManager::pop_block() {
		StackItem sitm;
		while (pop_item(sitm)) {}
		stack_state.back().pop_back();
	}

	void StackManager::push_item(std::string_view name, CompileValue cval, uint16_t id, StackItemTag tag) {
		size_t previous = SIZE_MAX;

		auto prev = stack_namespace.back().find(name);
		if (prev != stack_namespace.back().end()) {
			previous = prev->second;
		}

		stack.back().push_back({cval,name,previous,id,tag});
		stack_namespace.back()[name] = stack.back().size() - 1;
	}

	bool StackManager::pop_item(StackItem& sitm) {
		auto& sback = stack.back();

		if (sback.size() > stack_state.back().back()) {
			sitm = sback.back();
			sback.pop_back();

			if (sitm.previous == SIZE_MAX) {
				stack_namespace.back().erase(sitm.name);
			}
			else {
				stack_namespace.back()[sitm.name] = sitm.previous;
			}
			return true;
		}
		else {
			return false;
		}
	}


	bool StackManager::find(std::string_view name, StackItem& sitm) {
		if (stack_namespace.size() == 0) return false;

		auto f = stack_namespace.back().find(name);
		if (f != stack_namespace.back().end()) {
			sitm = stack.back()[f->second];
			return true;
		}
		else return false;
	}
}