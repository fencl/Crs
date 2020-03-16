#include "Source.h"
#include <iostream>
#include "Type.h"
#include "Declaration.h"
#include <chrono>
#include <vector>
#include <memory>
#include "Contents.h"
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
		m->architecture = ILArchitecture::x86_64;
		m->build_default_types();
		e->parent = m.get();

		Cursor c = src.read_first();
		CompileContext ctx;
		ctx.module = m.get();
		ctx.eval = e.get();
		ctx.default_types = dt.get();
		ctx.global = gn.get();

		ctx.default_types->setup(ctx);

		Declaration::parse_global(c,ctx, *gn.get());

		((Structure*)gn->subnamespaces["B"].get())->compile(ctx);

		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();


		std::cout << "\nelapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]" << std::endl;

		return 0;
	}
}

int main() {
	return Corrosive::crs_main();
}