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
#include "Compiler.h"
#include "Ast.h"

namespace Corrosive {
	const ILArchitecture compiler_arch = (sizeof(void*) == 8) ? ILArchitecture::bit64 : ILArchitecture::bit32;

	uint64_t print_test(uint64_t a, double b) {
		std::cout << a << ", " << b << "\n";
		return 42;
	}

	void test_fun_provider(ILEvaluator* eval) {
		eval->write_register_value<void*>(print_test);
	}


	int crs_main() {
		static_assert(sizeof(void*) == sizeof(size_t), "Error, size_t and void* must be the same size");
		static_assert(sizeof(double) == 8, "Error, double must be 64bit"); // TODO lets maybe create wrapper class to ensure correct format
		static_assert(sizeof(float) == 4, "Error, float must be 32bit");   //      on architectures with different floating point format

		//TODO ENDIANNESS !!!

		switch (compiler_arch)
		{
			case ILArchitecture::bit32:
				std::cout << "32bit arch\n\n"; break;
			case ILArchitecture::bit64:
				std::cout << "64bit arch\n\n"; break;
		}

		
		ILEvaluator::sandbox_begin();

		try {
			std::unique_ptr<ILModule> compiled_module = nullptr;
			ILFunction* main = nullptr;

			std::chrono::steady_clock::time_point compile_begin = std::chrono::steady_clock::now();
			{
				auto compiler = Compiler::create();
				Compiler::push_compiler(compiler.get());

				Source::require("../test/test2.crs");
				Compiler::current()->register_ext_function({ "std","test" }, test_fun_provider);
				auto res = compiler->find_name("main");

				if (auto fun = res.get_function()) {
					FunctionInstance* finst;
					fun->generate(nullptr, finst);
					finst->compile();
					main = finst->func;
				}
				else {
					std::cerr << "main not found\n";
				}

				Compiler::pop_compiler();

				compiled_module = compiler->finalize();
			}
			std::chrono::steady_clock::time_point compile_end = std::chrono::steady_clock::now();


			if (main && compiled_module) {
				
				std::cout << "========= TEST =========\n";

				std::chrono::steady_clock::time_point runtime_start = std::chrono::steady_clock::now();
				compiled_module->run(main);
				std::chrono::steady_clock::time_point runtime_end = std::chrono::steady_clock::now();


				std::cout << "========= ==== =========\n";
				std::cout << "\ncompile time: " << std::chrono::duration_cast<std::chrono::milliseconds>(compile_end - compile_begin).count() << "[ms]\n";
				std::cout << "runtime: " << std::chrono::duration_cast<std::chrono::milliseconds>(runtime_end - compile_end).count() << "[ms]\n" << std::endl;
			}
			else {
				std::cerr << "main was null\n";
			}
		}
		catch (std::exception& e) {
			std::cerr << e.what()<<"\n";
		}

		ILEvaluator::sandbox_end();


		return 0;
	}
}

int main() {
	return Corrosive::crs_main();
}