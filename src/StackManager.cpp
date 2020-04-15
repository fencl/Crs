#include "StackManager.h"


namespace Corrosive {
	std::unordered_map<std::string_view, size_t> StackManager::stack_namespace[2];
	std::vector<StackItem> StackManager::stack[2];
	uint16_t StackManager::stack_memory_size[2];
	uint16_t StackManager::stack_memory_compile_size[2];


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
}