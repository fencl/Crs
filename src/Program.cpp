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
		e->private_fun[1] = &Operand::priv_build_array;
		e->private_fun[2] = &Operand::priv_build_reference;
		e->private_fun[3] = &Operand::priv_build_push_template;
		e->private_fun[4] = &Operand::priv_build_build_template;
		e->private_fun[5] = &Operand::priv_type_template_cast;

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

		if (Declaration::parse_global(c, gn.get())) {
			if (gn->subtemplates["B"]->compile()) {
				auto& sfcs = gn->subtemplates["I"]->singe_instance->subfunctions;

				auto f_r = sfcs.find("equals");
				if (f_r != sfcs.end()) {
					FunctionInstance* finst;
					Cursor cerr;
					if (f_r->second->generate(nullptr, finst)) finst->compile(cerr);
				}

				f_r = sfcs.find("equals2");
				if (f_r != sfcs.end()) {
					FunctionInstance* finst;
					Cursor cerr;
					if (f_r->second->generate(nullptr, finst)) finst->compile(cerr);
				}
			
			}
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