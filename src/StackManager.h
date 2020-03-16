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
		template<unsigned int N> static inline unsigned long stack_state() {
			return (unsigned long)stack[N].size();
		}
		template<unsigned int N> static inline void stack_restore(unsigned long state) {
			for (unsigned long i = (unsigned long)stack[N].size() - 1; i > state; i--) {
				stack_pop<N>();
			}
		}
		template<unsigned int N> static inline StackItem& stack_push(std::string_view name, CompileValue value, unsigned int ir_local) {
			StackItem sitm;
			sitm.name = name;
			sitm.value = value;
			sitm.ir_local = ir_local;
			auto prev = stack_namespace[N].find(name);
			if (prev == stack_namespace[N].end()) {
				sitm.previous = ULONG_MAX;
			}
			else {
				sitm.previous = prev->second;
			}

			stack[N].push_back(sitm);
			stack_namespace[N][name] = (unsigned long)stack[N].size() - 1;
			return stack[N][stack[N].size() - 1];
		}
		template<unsigned int N> static inline StackItem* stack_find(std::string_view name) {
			auto f = stack_namespace[N].find(name);
			if (f != stack_namespace[N].end()) {
				return &stack[N][f->second];
			}
			else return nullptr;
		}
		template<unsigned int N> static inline std::pair<std::unordered_map<std::string_view, unsigned long>, std::vector<StackItem>> move_stack_out() {
			return std::move(std::make_pair(std::move(stack_namespace[N]), std::move(stack[N])));
		}
		template<unsigned int N> static inline void move_stack_in(std::pair<std::unordered_map<std::string_view, unsigned long>, std::vector<StackItem>> s) {
			stack_namespace[N] = std::move(s.first);
			stack[N] = std::move(s.second);
		}

	private:
		template<unsigned int N> static inline void stack_pop() {
			StackItem sitm = stack[N].back();
			if (sitm.previous == ULONG_MAX) {
				stack_namespace[N].erase(sitm.name);
			}
			else {
				stack_namespace[N][sitm.name] = sitm.previous;
			}

			stack[N].pop_back();
		}

		static std::unordered_map<std::string_view, unsigned long> stack_namespace[2];
		static std::vector<StackItem> stack[2];
	};

}

#endif
