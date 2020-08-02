#include "Compiler.h"
#include "Operand.h"

namespace Corrosive {
	void Compiler::setup() {
		target_module->insintric_function[(unsigned char)ILInsintric::push_template] = &Operand::priv_build_push_template;
		target_module->insintric_function_name[(unsigned char)ILInsintric::push_template] = "push_template";
		target_module->insintric_function[(unsigned char)ILInsintric::build_template] = &Operand::priv_build_build_template;
		target_module->insintric_function_name[(unsigned char)ILInsintric::build_template] = "build_template";
		target_module->insintric_function[(unsigned char)ILInsintric::type_dynamic_cast] = &Operand::priv_type_template_cast;
		target_module->insintric_function_name[(unsigned char)ILInsintric::type_dynamic_cast] = "dynamic_cast";
		compiler_evaluator->parent = target_module.get();
		constant_stack_manager.compiler = this;
		default_types->setup();
		push_workspace(target_global_namespace.get());
		initialized = true;
	}

	FunctionInstance* Compiler::register_ext_function(std::initializer_list<const char*> path, void(*ptr)(ILEvaluator*)) {

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
				return nullptr;
			}
		}

		FunctionInstance* finst;
		func->generate(nullptr, finst);
		finst->compile();
		((ILExtFunction*)finst->func)->ptr = ptr;
		return finst;
	}

	thread_local std::vector<Compiler*> Compiler::c;
	void Compiler::push_compiler(Compiler* compiler) { c.push_back(compiler); if (!compiler->initialized) { compiler->setup(); } }
	void Compiler::pop_compiler() { c.pop_back(); }
	Compiler* Compiler::current() { return c.back(); }
	std::unique_ptr<Compiler> Compiler::create() {
		auto cmp = std::make_unique<Compiler>();
		return std::move(cmp);
	}



	uint16_t Compiler::mask_local(unsigned char* ptr) {
		auto& ls = local_stack_offsets.back();
		ls.push_back(ptr);
		return (uint16_t)(ls.size() - 1);
	}

	void Compiler::pop_mask_local() {
		local_stack_offsets.pop_back();
	}

	uint16_t Compiler::push_local(ILSize size) {
		auto& lss = local_stack_size.back();
		auto& lsb = local_stack_base.back();
		auto& ls = local_stack_offsets.back();

		size_t sz = size.eval(target_module.get(), compiler_arch);

		ls.push_back(lsb + lss);
		lss += sz;

		return (uint16_t)(ls.size() - 1);
	}

	void Compiler::pop_local(ILSize size) {
		auto& lss = local_stack_size.back();
		size_t sz = size.eval(target_module.get(), compiler_arch);
		lss -= sz;
		local_stack_offsets.pop_back();
	}


	void Compiler::stack_push(size_t align) {
		if (local_stack_base.size() == 0) {
			size_t new_base = (size_t)(memory_stack);
			new_base = _align_up(new_base, align);
			local_stack_base.push_back((unsigned char*)new_base);
		}
		else {
			size_t new_base = (size_t)(local_stack_base.back() + local_stack_size.back());
			new_base = _align_up(new_base, align);
			local_stack_base.push_back((unsigned char*)new_base);
		}

		local_stack_size.push_back(0);
		local_stack_offsets.push_back(std::move(decltype(local_stack_offsets)::value_type()));
	}

	void Compiler::stack_pop() {
		local_stack_base.pop_back();
		local_stack_size.pop_back();
		local_stack_offsets.pop_back();
	}

	unsigned char* Compiler::stack_ptr(uint16_t id) {
		return local_stack_offsets.back()[id];
	}


	void Compiler::eval_local(uint16_t id) {
		compiler_evaluator->write_register_value<void*>(stack_ptr(id));
	}


	std::unique_ptr<ILModule> Compiler::finalize() {
		return std::move(target_module);
	}

	ScopeState::~ScopeState() {
		if (set_workspace) { Compiler::current()->pop_workspace(); }
		if (set_function) { Compiler::current()->pop_function(); }
		if (set_context) { Compiler::current()->pop_scope_context(); }
		if (set_stack) { Compiler::current()->stack()->pop(); }
		if (set_compiler_stack) { Compiler::current()->compiler_stack()->pop(); Compiler::current()->stack_pop(); }

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

	ScopeState& ScopeState::workspace(Namespace* nspc) { if (!set_workspace) { set_workspace = true; Compiler::current()->push_workspace(nspc); } return *this; }
	ScopeState& ScopeState::function(ILBytecodeFunction* fun, Type* return_type) { if (!set_function) { set_function = true; Compiler::current()->push_function(fun,return_type); } return *this; }
	ScopeState& ScopeState::context(ILContext ctx) { if (!set_context) { set_context = true; Compiler::current()->push_scope_context(ctx); } return *this; }
	ScopeState& ScopeState::stack() { if (!set_stack) { set_stack = true; Compiler::current()->stack()->push(); } return *this; }
	ScopeState& ScopeState::compiler_stack() { if (!set_compiler_stack) { set_compiler_stack = true; Compiler::current()->compiler_stack()->push(); Compiler::current()->stack_push(); } return *this; }
}