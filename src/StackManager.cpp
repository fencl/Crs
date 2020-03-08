#include "StackManager.h"


namespace Corrosive {


	unsigned long StackManager::stack_state(){
		return (unsigned long)stack.size();
	}

	void StackManager::stack_restore(unsigned long state)
	{
		for (unsigned long i = (unsigned long)stack.size() - 1; i > state; i--) {
			stack_pop();
		}
	}


	std::pair<std::unordered_map<std::string_view, unsigned long>, std::vector<StackItem>> StackManager::move_stack_out() {
		return std::move(std::make_pair(std::move(stack_namespace), std::move(stack)));
	}
	void StackManager::move_stack_in(std::pair<std::unordered_map<std::string_view, unsigned long>, std::vector<StackItem>> s) {
		stack_namespace = std::move(s.first);
		stack = std::move(s.second);
	}


	void StackManager::stack_pop() {
		StackItem sitm = stack.back();
		if (sitm.previous == ULONG_MAX) {
			stack_namespace.erase(sitm.name);
		}
		else {
			stack_namespace[sitm.name] = sitm.previous;
		}

		stack.pop_back();
	}


	StackItem& StackManager::stack_push(std::string_view name, CompileValue value, unsigned int ir_local)
	{
		StackItem sitm;
		sitm.name = name;
		sitm.value = value;
		sitm.ir_local = ir_local;
		auto prev = stack_namespace.find(name);
		if (prev == stack_namespace.end()) {
			sitm.previous = ULONG_MAX;
		}
		else {
			sitm.previous = prev->second;
		}

		stack.push_back(sitm);
		stack_namespace[name] = (unsigned long)stack.size() - 1;
		return stack[stack.size() - 1];
	}



	StackItem* StackManager::stack_find(std::string_view name) {
		auto f = stack_namespace.find(name);
		if (f != stack_namespace.end()) {
			return &stack[f->second];
		}
		else return nullptr;
	}


	std::unordered_map<std::string_view, unsigned long> StackManager::stack_namespace;
	std::vector<StackItem> StackManager::stack;

}