#include "Compiler.h"
#include "Operand.h"

namespace Corrosive {
	Compiler::Compiler() {
		m.insintric_function[(unsigned char)ILInsintric::push_template] = &Operand::priv_build_push_template;
		m.insintric_function_name[(unsigned char)ILInsintric::push_template] = "push_template";
		m.insintric_function[(unsigned char)ILInsintric::build_template] = &Operand::priv_build_build_template;
		m.insintric_function_name[(unsigned char)ILInsintric::build_template] = "build_template";
		m.insintric_function[(unsigned char)ILInsintric::type_dynamic_cast] = &Operand::priv_type_template_cast;
		m.insintric_function_name[(unsigned char)ILInsintric::type_dynamic_cast] = "dynamic_cast";
		e->parent = &m;
		cmgr.compiler = this;
		dt.setup(*this);
	}

	FunctionInstance* Compiler::register_ext_function(std::initializer_list<const char*> path, void(*ptr)(ILEvaluator*)) {

		Namespace* nspc = global_namespace();
		FunctionTemplate* func = nullptr;

		for (auto&& p : path) {
			Namespace* next_nspc = nullptr;
			StructureTemplate* next_struct = nullptr;
			FunctionTemplate* next_func = nullptr;
			TraitTemplate* next_trait = nullptr;


			nspc->find_name(p, next_nspc, next_struct, next_func, next_trait);


			if (next_nspc && !func) {
				nspc = next_nspc;
			}
			else if (next_func && !func) {
				func = next_func;
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
}