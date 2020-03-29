#include "StackManager.h"


namespace Corrosive {
	std::unordered_map<std::string_view, size_t> StackManager::stack_namespace[2];
	std::vector<StackItem> StackManager::stack[2];
	uint32_t StackManager::stack_memory_size[2];
}