#include "Compiler.h"
#include "Operand.h"

namespace Corrosive {
	void Compiler::setup() {
		m.insintric_function[(unsigned char)ILInsintric::push_template] = &Operand::priv_build_push_template;
		m.insintric_function_name[(unsigned char)ILInsintric::push_template] = "push_template";
		m.insintric_function[(unsigned char)ILInsintric::build_template] = &Operand::priv_build_build_template;
		m.insintric_function_name[(unsigned char)ILInsintric::build_template] = "build_template";
		m.insintric_function[(unsigned char)ILInsintric::type_dynamic_cast] = &Operand::priv_type_template_cast;
		m.insintric_function_name[(unsigned char)ILInsintric::type_dynamic_cast] = "dynamic_cast";
		e->parent = &m;
		cmgr.compiler = this;
		dt.setup();
		push_workspace(&gn);
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
}