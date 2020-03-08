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
		unsigned long ir_local = 0;
	};

	class StackManager {
	public:
		static unsigned long stack_state();
		static void stack_restore(unsigned long state);
		static StackItem& stack_push(std::string_view name, CompileValue value, unsigned int ir_local);
		static StackItem* stack_find(std::string_view name);
		static std::pair<std::unordered_map<std::string_view, unsigned long>, std::vector<StackItem>> move_stack_out();
		static void move_stack_in(std::pair<std::unordered_map<std::string_view, unsigned long>, std::vector<StackItem>> s);

	private:
		static void stack_pop();
		static std::unordered_map<std::string_view, unsigned long> stack_namespace;
		static std::vector<StackItem> stack;
	};

}

#endif
