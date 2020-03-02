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
		static unsigned long StackState();
		static void StackStateRestore(unsigned long state);
		static StackItem& StackPush(std::string_view name, CompileValue value);
		static StackItem* StackFind(std::string_view name);

	private:
		static void StackPop();
		static std::unordered_map<std::string_view, unsigned long> stack_namespace;
		static std::vector<StackItem> stack;
	};

}

#endif
