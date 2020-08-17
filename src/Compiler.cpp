#include "Compiler.hpp"
#include "Operand.hpp"

namespace Corrosive {
	errvoid Compiler::setup() {
		target_module->insintric_function[(unsigned char)ILInsintric::push_template] = &Operand::push_template;
		target_module->insintric_function_name[(unsigned char)ILInsintric::push_template] = "push_template";
		target_module->insintric_function[(unsigned char)ILInsintric::build_template] = &Operand::build_template;
		target_module->insintric_function_name[(unsigned char)ILInsintric::build_template] = "build_template";
		target_module->insintric_function[(unsigned char)ILInsintric::type_dynamic_cast] = &Operand::type_template_cast;
		target_module->insintric_function_name[(unsigned char)ILInsintric::type_dynamic_cast] = "dynamic_cast";
		compiler_evaluator->parent = target_module.get();
		constant_stack_manager->compiler = this;
		if (!default_types->setup()) return pass();
		outer_namespace_stack.push_back(target_global_namespace.get());
		initialized = true;
		return errvoid();
	}


	errvoid Compiler::register_native_function(FunctionInstance*& r, std::initializer_list<const char*> path, void* ptr) {

		Namespace* nspc = global_namespace();
		FunctionTemplate* func = nullptr;

		for (auto&& p : path) {
			
			auto res = nspc->find_name(p);


			if (res.type() == FindNameResultType::Namespace && !func) {
				nspc = res.get_namespace();
			}
			else if (res.type() == FindNameResultType::Function && !func) {
				func = res.get_function();
			}
			else {
				r = nullptr;
				return errvoid();
			}
		}

		FunctionInstance* finst;
		if (!func->generate(nullptr, finst)) return pass();
		if (!finst->compile()) return pass();
		((ILNativeFunction*)finst->func)->ptr = ptr;
		r= finst;
		return errvoid();
	}

	thread_local std::vector<Compiler*> Compiler::c;
	void Compiler::push_compiler(Compiler* compiler) { c.push_back(compiler); }
	void Compiler::pop_compiler() { c.pop_back(); }
	Compiler* Compiler::current() { return c.back(); }
	std::unique_ptr<Compiler> Compiler::create() {
		auto cmp = std::make_unique<Compiler>();
		return std::move(cmp);
	}



	stackid_t Compiler::mask_local(unsigned char* ptr) {
		auto& ls = local_stack_offsets.back();
		ls.push_back(ptr);
		return (stackid_t)(ls.size() - 1);
	}

	void Compiler::pop_mask_local() {
		local_stack_offsets.pop_back();
	}

	// TODO align?
	stackid_t Compiler::push_local(ILSize size) {
		auto& lss = local_stack_size.back();
		auto& lsb = local_stack_base.back();
		auto& ls = local_stack_offsets.back();

		std::size_t sz = size.eval(target_module.get(), compiler_arch);

		ls.push_back(lsb + lss);
		lss += sz;

		return (stackid_t)(ls.size() - 1);
	}

	void Compiler::pop_local(ILSize size) {
		auto& lss = local_stack_size.back();
		std::size_t sz = size.eval(target_module.get(), compiler_arch);
		lss -= sz;
		local_stack_offsets.pop_back();
	}


	void Compiler::stack_push(std::size_t align) {
		if (local_stack_base.size() == 0) {
			std::size_t new_base = (std::size_t)(memory_stack);
			new_base = align_up(new_base, align);
			local_stack_base.push_back((unsigned char*)new_base);
		}
		else {
			std::size_t new_base = (std::size_t)(local_stack_base.back() + local_stack_size.back());
			new_base = align_up(new_base, align);
			local_stack_base.push_back((unsigned char*)new_base);
		}

		local_stack_size.push_back(0);
		local_push_sizes.push_back(std::vector<std::pair<std::size_t,std::size_t>>());
		local_stack_offsets.push_back(std::move(decltype(local_stack_offsets)::value_type()));
		stack_push_block();
	}


	void Compiler::stack_push_block() {
		local_push_sizes.back().push_back(std::make_pair(local_stack_size.back(), local_stack_offsets.back().size()));
	}

	void Compiler::stack_pop_block() {
		auto b = local_push_sizes.back().back();
		local_stack_size.back() = b.first;
		local_stack_offsets.back().resize(b.second);
		local_push_sizes.back().pop_back();
	}

	void Compiler::stack_pop() {
		stack_pop_block();
		local_push_sizes.pop_back();
		local_stack_base.pop_back();
		local_stack_size.pop_back();
		local_stack_offsets.pop_back();
	}

	unsigned char* Compiler::stack_ptr(stackid_t id) {
		return local_stack_offsets.back()[id];
	}


	void Compiler::eval_local(stackid_t id) {
		compiler_evaluator->write_register_value<void*>(stack_ptr(id));
	}

	std::unique_ptr<ILModule> Compiler::finalize() {
		target_module->strip_unused_content();
		release_jit_code();
		if (ILEvaluator::sandboxed()) build_sandbox();
		return std::move(target_module);
	}

	ScopeState::~ScopeState() {
		Compiler* c = Compiler::current();

		if (set_workspace) { c->outer_namespace_stack.pop_back(); }
		if (set_function) { 
			c->working_function_stack.pop_back(); 
			c->return_type_stack.pop_back();
			c->compile_loop_state_stack.pop_back();
			c->defers.pop_back();
		}
		if (set_context) { c->scope_context_stack.pop_back(); }
		if (set_stack) { c->stack()->pop(); }
		if (set_compiler_stack) { c->compiler_stack()->pop(); c->stack_pop(); }

		set_function = set_context = set_workspace = set_compiler_stack = set_stack = false;
	}

	ScopeState::ScopeState() {}
	ScopeState::ScopeState(ScopeState& state) {
		set_workspace = state.set_workspace;
		set_function = state.set_function;
		set_stack = state.set_stack;
		set_context = state.set_context;
		set_compiler_stack = state.set_compiler_stack;

		state.set_context = state.set_function = state.set_workspace = state.set_compiler_stack = state.set_stack = false;
	}

	ScopeState::ScopeState(ScopeState&& state) {
		set_workspace = state.set_workspace;
		set_function = state.set_function;
		set_context = state.set_context;
		set_compiler_stack = state.set_compiler_stack;
		set_stack = state.set_stack;

		state.set_context = state.set_function = state.set_workspace = state.set_compiler_stack = state.set_stack = false;
	}
	
	ScopeState& ScopeState::workspace(Namespace* nspc) {
		if (!set_workspace) { 
			set_workspace = true;
			Compiler::current()->outer_namespace_stack.push_back(nspc);
		} return *this;
	}
	
	ScopeState& ScopeState::function(ILBytecodeFunction* fun, Type* return_type) { 
		if (!set_function) { 
			set_function = true; 
			Compiler* c = Compiler::current();
			c->working_function_stack.push_back(fun);
			c->return_type_stack.push_back(return_type);
			c->compile_loop_state_stack.push_back(std::vector<CompileTimeBlockState*>());
			c->defers.push_back(std::vector<std::vector<TypeFunction*>>());
		} return *this;
	}

	ScopeState& ScopeState::context(ILContext ctx) { 
		if (!set_context) { 
			set_context = true;
			Compiler::current()->scope_context_stack.push_back(ctx);
		} return *this; 
	}
	ScopeState& ScopeState::stack() { if (!set_stack) { set_stack = true; Compiler::current()->stack()->push(); } return *this; }
	ScopeState& ScopeState::compiler_stack() { if (!set_compiler_stack) { set_compiler_stack = true; Compiler::current()->compiler_stack()->push(); Compiler::current()->stack_push(); } return *this; }
}