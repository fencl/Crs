#pragma once
#ifndef _stack_manager_crs_h
#define _stack_manager_crs_h

#include <vector>
#include <unordered_map>
#include "CompileContext.h"

namespace Corrosive {

	enum class StackItemTag {
		regular, alias
	};

	struct StackItem {
		CompileValue value;
		std::string_view name;
		size_t previous;
		uint16_t id;
		StackItemTag tag;
	};

	class StackManager {
	public:

		void push();
		void pop();

		void push_block();
		void pop_block();

		bool pop_item(StackItem& sitm);
		void push_item(std::string_view name, CompileValue cval, uint16_t id, StackItemTag tag);
		bool find(std::string_view name, StackItem& sitm);

		

	private:

		std::vector<std::unordered_map<std::string_view, size_t>> stack_namespace;
		std::vector<std::vector<StackItem>> stack;
		std::vector<std::vector<size_t>> stack_state;
	};

}

#endif
