#include "StackManager.h"


namespace Corrosive {


	CompileContext CompileContext::context_stack[1024];
	uint32_t CompileContext::context_sp = 0;
	CompileContext& CompileContext::get() {
		return context_stack[context_sp - 1];
	}

	void CompileContext::push(CompileContext ctx) {
		context_stack[context_sp] = ctx;
		context_sp++;
	}
	
	void CompileContext::pop() {
		context_sp--;
	}



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