#include "Source.h"
#include <iostream>
#include "Type.h"
#include "Declaration.h"
#include <chrono>
#include <vector>
#include <memory>
#include "PredefinedTypes.h"
#include "Operand.h"
#include "Expression.h"
#include "IL/IL.h"

namespace Corrosive {
	int crs_main() {
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

		bool alive = true;
		auto start = std::chrono::system_clock::now();

		Source src;
		src.load("..\\test\\test.crs");

		std::unique_ptr<ILModule> m = std::make_unique<ILModule>();
		std::unique_ptr<DefaultTypes> dt = std::make_unique<DefaultTypes>();
		std::unique_ptr<Namespace> gn = std::make_unique<Namespace>();
		std::unique_ptr<ILEvaluator> e = std::make_unique<ILEvaluator>();
		m->insintric_function[(unsigned char)ILInsintric::build_array] = &Operand::priv_build_array;
		m->insintric_function_name[(unsigned char)ILInsintric::build_array] = "array";
		m->insintric_function[(unsigned char)ILInsintric::build_reference] = &Operand::priv_build_reference;
		m->insintric_function_name[(unsigned char)ILInsintric::build_reference] = "reference";
		m->insintric_function[(unsigned char)ILInsintric::push_template] = &Operand::priv_build_push_template;
		m->insintric_function_name[(unsigned char)ILInsintric::push_template] = "push_template";
		m->insintric_function[(unsigned char)ILInsintric::build_template] = &Operand::priv_build_build_template;
		m->insintric_function_name[(unsigned char)ILInsintric::build_template] = "build_template";
		m->insintric_function[(unsigned char)ILInsintric::template_cast] = &Operand::priv_type_template_cast;
		m->insintric_function_name[(unsigned char)ILInsintric::template_cast] = "dynamic_cast";
		m->insintric_function[(unsigned char)ILInsintric::malloc] = &Operand::priv_malloc;
		m->insintric_function_name[(unsigned char)ILInsintric::malloc] = "malloc";
		m->insintric_function[(unsigned char)ILInsintric::memcpy] = &Operand::priv_memcpy;
		m->insintric_function_name[(unsigned char)ILInsintric::memcpy] = "memcpy";
		m->insintric_function[(unsigned char)ILInsintric::build_slice] = &Operand::priv_build_slice;
		m->insintric_function_name[(unsigned char)ILInsintric::build_slice] = "slice";
		m->insintric_function[(unsigned char)ILInsintric::debug_cursor] = &DefaultTypes::priv_debug_cursor;
		m->insintric_function_name[(unsigned char)ILInsintric::debug_cursor] = "debug";

		m->architecture = ILArchitecture::x86_64;
		e->parent = m.get();

		Cursor c = src.read_first();


		CompileContext ctx;
		ctx.module = m.get();
		ctx.eval = e.get();
		ctx.default_types = dt.get();
		ctx.global = gn.get();

		CompileContext::push(ctx);

		ctx.default_types->setup(ctx);

		ILFunction* ilf = nullptr;

		if (Declaration::parse_global(c, gn.get())) {
			if (gn->subtemplates["B"]->compile()) {
				auto& sfcs = gn->subtemplates["I"]->singe_instance->subfunctions;

				/*auto f_r = sfcs.find("equals");
				if (f_r != sfcs.end()) {
					FunctionInstance* finst;
					if (f_r->second->generate(nullptr, finst)) finst->compile();
				}

				f_r = sfcs.find("equals2");
				if (f_r != sfcs.end()) {
					FunctionInstance* finst;
					if (f_r->second->generate(nullptr, finst)) finst->compile();
				}*/
			
				auto f_r = sfcs.find("main");
				if (f_r != sfcs.end()) {
					FunctionInstance* finst;
					if (f_r->second->generate(nullptr, finst)) {
						if (finst->compile()) {
							ilf = finst->func;
						}
					}
				}
				else {
					std::cerr << "main not found\n";
				}
			}
		}

		if (ilf != nullptr) {
			ILBuilder::eval_fnptr(ctx.eval, ilf);
			ILBuilder::eval_callstart(ctx.eval);
			ILBuilder::eval_call(ctx.eval, ILDataType::u64, 0);
			uint64_t ret_val = ctx.eval->pop_register_value<uint64_t>();
			std::cout << "\n\n========= TEST =========\ntest result was: " << ret_val << "\n\n";
		}


		CompileContext::pop();

		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

		std::cout << "\nelapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]\n" << std::endl;

		return 0;
	}
}

int main() {
	return Corrosive::crs_main();
}