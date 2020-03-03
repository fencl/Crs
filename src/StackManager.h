#pragma once
#ifndef _stack_manager_crs_h
#define _stack_manager_crs_h

#include <vector>
#include <unordered_map>
#include "CompileContext.h"

namespace Corrosive {
	struct StackItem {
		CompileValue value;
		std::string_view name;
		unsigned long previous = ULONG_MAX;
	};

	class StackManager {
	public:
		static unsigned long stack_state();
		static void stack_restore(unsigned long state);
		static StackItem& stack_push(std::string_view name, CompileValue value);
		static StackItem* stack_find(std::string_view name);

	private:
		static void stack_pop();
		static std::unordered_map<std::string_view, unsigned long> stack_namespace;
		static std::vector<StackItem> stack;
	};

}

#endif
