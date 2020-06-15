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
#include "StackManager.h"
#include "ConstantManager.h"
#include <memory>

namespace Corrosive {
	const ILArchitecture compiler_arch = ILArchitecture::x86_64;

	int crs_main() {
		std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

		ILModule m;
		DefaultTypes dt;
		Namespace gn;
		std::unique_ptr<ILEvaluator> e = std::make_unique<ILEvaluator>();
		StackManager rts;
		StackManager cps;
		StackManager tms;
		ConstantManager cmgr;

		m.insintric_function[(unsigned char)ILInsintric::build_array] = &Operand::priv_build_array;
		m.insintric_function_name[(unsigned char)ILInsintric::build_array] = "array";
		m.insintric_function[(unsigned char)ILInsintric::build_reference] = &Operand::priv_build_reference;
		m.insintric_function_name[(unsigned char)ILInsintric::build_reference] = "reference";
		m.insintric_function[(unsigned char)ILInsintric::push_template] = &Operand::priv_build_push_template;
		m.insintric_function_name[(unsigned char)ILInsintric::push_template] = "push_template";
		m.insintric_function[(unsigned char)ILInsintric::build_template] = &Operand::priv_build_build_template;
		m.insintric_function_name[(unsigned char)ILInsintric::build_template] = "build_template";
		m.insintric_function[(unsigned char)ILInsintric::template_cast] = &Operand::priv_type_template_cast;
		m.insintric_function_name[(unsigned char)ILInsintric::template_cast] = "dynamic_cast";
		m.insintric_function[(unsigned char)ILInsintric::build_slice] = &Operand::priv_build_slice;
		m.insintric_function_name[(unsigned char)ILInsintric::build_slice] = "slice";
		m.insintric_function[(unsigned char)ILInsintric::type_size] = &Operand::priv_type_size;
		m.insintric_function_name[(unsigned char)ILInsintric::type_size] = "type_size";

		m.architecture = ILArchitecture::x86_64;
		e->parent = &m;


		Ctx::init(&m, &dt, e.get(), &gn, &rts, &cps, &tms,&cmgr);

		Ctx::eval()->sandbox_begin();

		try {
			

			Source src;
			src.load("..\\test\\test.crs");
			src.pair_braces();
			src.register_debug();
			Cursor c = src.read_first();


			Ctx::types()->setup();

			ILFunction* main = nullptr;

			Declaration::parse_global(c, &gn);
			auto mainfun = gn.subfunctions.find("main");
			if (mainfun != gn.subfunctions.end()) {
				FunctionInstance* finst;
				mainfun->second->generate(nullptr, finst);
				finst->compile();
				main = finst->func;
			}
			else {
				std::cerr << "main not found\n";
			}


			if (main != nullptr) {
				Ctx::eval()->debug_file = UINT16_MAX;
				Ctx::eval()->debug_line = UINT16_MAX;
				ILBuilder::eval_fnptr(Ctx::eval(), main);
				ILBuilder::eval_callstart(Ctx::eval());
				ILBuilder::eval_call(Ctx::eval(), ILDataType::u64, 0);
				uint64_t ret_val = Ctx::eval()->pop_register_value<uint64_t>();
				std::cout << "========= TEST =========\ntest result was: " << ret_val << "\n\n";

				auto lr = (size_t)(Ctx::eval()->register_stack_pointer - Ctx::eval()->register_stack);
				if (lr>0)
					std::cout << "leaked registers: " << lr << "\n";
			}
			else {
				std::cerr << "main was null\n";
			}
		}
		catch (std::exception& e) {
			std::cerr << e.what()<<"\n";
		}

		Ctx::eval()->sandbox_end();


		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();

		std::cout << "\nelapsed: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]\n" << std::endl;

		return 0;
	}
}

int main() {
	return Corrosive::crs_main();
}