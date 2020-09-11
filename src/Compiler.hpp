#pragma once
#ifndef _compiler_crs_h
#define _compiler_crs_h

#include "IL/IL.hpp"
#include "BuiltIn.hpp"
#include "StackManager.hpp"
#include "ConstantManager.hpp"
#include "Declaration.hpp"

namespace Crs {

	enum class CompileTimeBlockState {
		run, jump_over, jump_back
	};

	class Compiler {
	public:
		bool initialized = false;
		std::vector<FunctionTemplate*> exported_functions;

		ILEvaluator* evaluator() { return compiler_evaluator.get(); }
		ILBlock* scope() { return scope_stack.back(); }
		ILBlock* loop_break() { return loop_block_stack.back().first; }
		ILBlock* loop_continue() { return loop_block_stack.back().second; }
		bool has_loop() { return loop_block_stack.size() > 0; }
		BuiltInTypes* types() { return default_types.get(); }
		ILContext scope_context() { return scope_context_stack.back(); }
		Namespace* workspace() { return outer_namespace_stack.back(); }
		StackManager* stack() { return &runtime_stack_manager; }
		StackManager* compiler_stack() { return &compiler_stack_manager; }
		StackManager* temp_stack() { return &temporary_stack_manager; }
		Namespace* global_namespace() { return target_global_namespace.get(); }
		ILModule* global_module() { return target_module.get(); }
		ConstantManager* constant_manager() { return constant_stack_manager.get(); }
		Source* source() { return source_stack.back(); }
		std::vector<TypeFunction*>& defer_scope() { return defers.back().back(); }
		std::vector<std::vector<TypeFunction*>>& defer_function() { return defers.back(); }
		std::vector<TypeFunction*>& compile_defer_scope() { return compile_defers.back(); }
		ILBytecodeFunction* target() { return working_function_stack.back(); }
		Type* return_type() { return return_type_stack.back(); }

		FindNameResult find_name(std::string_view name) { return target_global_namespace->find_name(name); }

		void push_scope(ILBlock* sc) { scope_stack.push_back(sc); }
		void pop_scope() { scope_stack.pop_back(); }

		void switch_scope(ILBlock* sblock) { scope_stack.back() = sblock; }

		errvoid setup();

		void push_compile_loop_state(CompileTimeBlockState& state) { compile_loop_state_stack.back().push_back(&state); }
		void pop_compile_loop_state() { compile_loop_state_stack.back().pop_back(); }
		CompileTimeBlockState& compile_loop_state() { return *compile_loop_state_stack.back().back(); }
		bool compile_in_loop() { return compile_loop_state_stack.back().size() > 0; }

		std::vector<std::vector<CompileTimeBlockState*>> compile_loop_state_stack;
		std::vector<ILBlock*> scope_stack;
		std::vector<Type*> return_type_stack;
		std::vector<std::pair<ILBlock*,ILBlock*>> loop_block_stack;
		std::vector<ILContext> scope_context_stack;
		std::vector<Namespace*> outer_namespace_stack;
		std::vector<ILBytecodeFunction*> working_function_stack;
		std::vector<Source*> source_stack;
		std::vector<std::vector<std::vector<TypeFunction*>>> defers;
		std::vector<std::vector<TypeFunction*>> compile_defers;

		std::map<std::string, std::unique_ptr<Source>> included_sources;

		std::unique_ptr<ILModule> target_module = std::make_unique<ILModule>();
		std::unique_ptr<BuiltInTypes> default_types = std::make_unique<BuiltInTypes>();
		std::unique_ptr<Namespace> target_global_namespace = std::make_unique<Namespace>();
		std::unique_ptr<ILEvaluator> compiler_evaluator = std::make_unique<ILEvaluator>();
		std::unique_ptr<ConstantManager> constant_stack_manager = std::make_unique<ConstantManager>();

		StackManager runtime_stack_manager;
		StackManager compiler_stack_manager;
		StackManager temporary_stack_manager;

		errvoid precompile_native_function(FunctionInstance*& r,const char* path, void* ptr);

		static thread_local std::vector<Compiler*> c;
		static void push_compiler(Compiler* c);
		static void pop_compiler();
		static Compiler* current();
		static std::unique_ptr<Compiler> create();


		static const inline std::size_t stack_size = 1024 * 4;
		unsigned char memory_stack[stack_size];

		std::vector<std::size_t> local_stack_size;
		std::vector<unsigned char*> local_stack_base;
		std::vector<std::vector<unsigned char*>> local_stack_offsets;
		std::vector<std::vector<std::pair<std::size_t,std::size_t>>> local_push_sizes;

		stackid_t		mask_local(unsigned char* ptr);
		void			pop_mask_local();
		stackid_t		push_local(ILSize size);
		void			pop_local(ILSize size);
		void			stack_push_block();
		void            stack_pop_block();
		void			stack_push(std::size_t align = 1);
		void			stack_pop();
		unsigned char*	stack_ptr(stackid_t id);
		void			eval_local(stackid_t id);

		std::unique_ptr<ILModule> finalize();
	};

	class ScopeState {
	public:
		ScopeState();
		ScopeState(ScopeState& state);
		ScopeState(ScopeState&& state);

		~ScopeState();
		ScopeState& workspace(Namespace* nspc);
		ScopeState& function(ILBytecodeFunction* function, Type* return_type);
		ScopeState& context(ILContext context);
		ScopeState& compiler_stack();
		ScopeState& stack();

	private:
		bool set_workspace = false;
		bool set_function = false;
		bool set_context = false;
		bool set_compiler_stack = false;
		bool set_stack = false;
	};
}
#endif