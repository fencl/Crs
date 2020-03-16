#include "StackManager.h"


namespace Corrosive {
	std::unordered_map<std::string_view, unsigned long> StackManager::stack_namespace[2];
	std::vector<StackItem> StackManager::stack[2];
}