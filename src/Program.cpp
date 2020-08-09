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


	bool crs(std::string file) {
		static_assert(sizeof(void*) == sizeof(size_t), "Error, size_t and void* must be the same size");
		static_assert(sizeof(double) == 8, "Error, double must be 64bit"); // TODO lets maybe create wrapper class to ensure correct format
		static_assert(sizeof(float) == 4, "Error, float must be 32bit");   //      on architectures with different floating point format

		//TODO ENDIANNESS !!!		
		ILEvaluator::sandbox_begin();

		try {
			std::unique_ptr<ILModule> compiled_module = nullptr;

			std::chrono::steady_clock::time_point compile_begin = std::chrono::steady_clock::now();
			{
				auto compiler = Compiler::create();
				Compiler::push_compiler(compiler.get());

				Source::require("../test/test.crs");
				//Compiler::current()->register_ext_function({ "std","test" }, test_fun_provider);
				
				Compiler::pop_compiler();

				compiled_module = compiler->finalize();
			}
			std::chrono::steady_clock::time_point compile_end = std::chrono::steady_clock::now();
	
			/*for (auto&& fun : compiled_module->functions) {
				if (auto bcfun = dynamic_cast<ILBytecodeFunction*>(fun.get())) {
					bcfun->dump();
					std::cout << "\n\n";
				}
			}*/


			std::cout << "========= TEST =========\n";

			std::chrono::steady_clock::time_point runtime_start = std::chrono::steady_clock::now();
			compiled_module->run(compiled_module->entry_point);
			std::chrono::steady_clock::time_point runtime_end = std::chrono::steady_clock::now();


			std::cout << "========= ==== =========\n";
			std::cout << "\ncompile time: " << std::chrono::duration_cast<std::chrono::milliseconds>(compile_end - compile_begin).count() << "[ms]\n";
			std::cout << "runtime: " << std::chrono::duration_cast<std::chrono::milliseconds>(runtime_end - runtime_start).count() << "[ms]\n" << std::endl;
			
		}
		catch (std::exception& e) {
			std::cerr << e.what()<<"\n";
		}

		release_jit_code();
		ILEvaluator::sandbox_end();


		return 0;
	}
}

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "undefined target file\ncrs [file]";
		return 2;
	}

	return Corrosive::crs(argv[1]) ? 0 : 1;
}