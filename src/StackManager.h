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
		size_t previous = SIZE_MAX;
		ILSize local_offset;
		uint16_t id = 0;
	};

	class StackManager {
	public:
		template<unsigned int N> static inline size_t stack_state() {
			return stack[N].size();
		}
		template<unsigned int N> static inline void stack_restore(ILEvaluator* eval, size_t state) {
			for (size_t i = stack[N].size() - 1; i > state; i--) {
				stack_pop<N>(eval);
			}
		}
		template<unsigned int N> static inline StackItem& stack_push(ILEvaluator* eval, std::string_view name, CompileValue value, uint16_t id) {
			StackItem sitm;
			sitm.name = name;
			sitm.value = value;
			sitm.local_offset = stack_memory_size[N];
			sitm.id = id;

			stack_memory_size[N] = stack_memory_size[N] + value.t->size();


			auto prev = stack_namespace[N].find(name);
			if (prev == stack_namespace[N].end()) {
				sitm.previous = SIZE_MAX;
			}
			else {
				sitm.previous = prev->second;
			}

			stack[N].push_back(sitm);
			stack_namespace[N][name] = stack[N].size() - 1;
			return stack[N][stack[N].size() - 1];
		}
		template<unsigned int N> static inline StackItem* stack_find(std::string_view name) {
			auto f = stack_namespace[N].find(name);
			if (f != stack_namespace[N].end()) {
				return &stack[N][f->second];
			}
			else return nullptr;
		}
		template<unsigned int N> static inline std::tuple<std::unordered_map<std::string_view, size_t>, std::vector<StackItem>,ILSize> move_stack_out() {
			auto res = std::move(std::make_tuple(std::move(stack_namespace[N]), std::move(stack[N]), stack_memory_size[N]));
			stack_memory_size[N] = ILSize(0,0);
			return std::move(res);
		}
		template<unsigned int N> static inline void move_stack_in(std::tuple<std::unordered_map<std::string_view, size_t>, std::vector<StackItem>, ILSize> s) {
			stack_namespace[N] = std::move(std::get<0>(s));
			stack[N] = std::move(std::get<1>(s));
			stack_memory_size[N] = std::get<2>(s);
		}

		template<unsigned int N> static inline StackItem stack_pop(ILEvaluator* eval) {
			StackItem sitm = stack[N].back();
			stack_memory_size[N] = stack_memory_size[N] - sitm.value.t->size();
			if (sitm.previous == SIZE_MAX) {
				stack_namespace[N].erase(sitm.name);
			}
			else {
				stack_namespace[N][sitm.name] = sitm.previous;
			}

			stack[N].pop_back();
			return sitm;
		}

	private:

		static std::unordered_map<std::string_view, size_t> stack_namespace[2];
		static std::vector<StackItem> stack[2];
		static ILSize stack_memory_size[2];
	};

}

#endif
