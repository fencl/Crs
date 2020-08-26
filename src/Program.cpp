#include "Source.hpp"
#include <iostream>
#include "Type.hpp"
#include "Declaration.hpp"
#include <chrono>
#include <vector>
#include <memory>
#include "BuiltIn.hpp"
#include "Operand.hpp"
#include "Expression.hpp"
#include "IL/IL.hpp"
#include "StackManager.hpp"
#include "ConstantManager.hpp"
#include <memory>
#include "Compiler.hpp"
#include "Ast.hpp"
#include <fstream>
#include <csetjmp>
#include <cstring>

namespace Corrosive {
	bool crs(std::string file, bool debug) {

		ILEvaluator::sandbox_begin();
		
		std::unique_ptr<ILModule> compiled_module = nullptr;
		std::chrono::steady_clock::time_point compile_begin = std::chrono::steady_clock::now();
		{
			auto compiler = Compiler::create();
			Compiler::push_compiler(compiler.get());
			if (compiler->setup()) {
				if (Source::require(file)) {
					compiled_module = compiler->finalize();
				}
			}
			Compiler::pop_compiler();
		}
		std::chrono::steady_clock::time_point compile_end = std::chrono::steady_clock::now();

		if (compiled_module && compiled_module->entry_point) {
			if (debug) {
				std::cout << "========= TEST =========\n";
			}
			/*std::chrono::steady_clock::time_point saveload_start = std::chrono::steady_clock::now();
			{
				std::ofstream file("output.bin", std::ios::binary);
				ILOutputStream stream(&file);
				compiled_module->save(stream);
			}

			{
				std::ifstream file("output.bin", std::ios::binary);
				ILInputStream stream(file);
				compiled_module->load(stream);
				StandardLibraryCode::link(compiled_module.get());
			}
			std::chrono::steady_clock::time_point saveload_end = std::chrono::steady_clock::now();*/


			std::chrono::steady_clock::time_point runtime_start = std::chrono::steady_clock::now();
			compiled_module->run(compiled_module->entry_point);
			std::chrono::steady_clock::time_point runtime_end = std::chrono::steady_clock::now();

			if (debug) {
				std::cout << "========= ==== =========\n\n";
				std::cout << "compile time: " << std::chrono::duration_cast<std::chrono::milliseconds>(compile_end - compile_begin).count() << "[ms]\n";
				//std::cout << "save and load time: " << std::chrono::duration_cast<std::chrono::milliseconds>(saveload_end - saveload_start).count() << "[ms]\n";
				std::cout << "runtime: " << std::chrono::duration_cast<std::chrono::milliseconds>(runtime_end - runtime_start).count() << "[ms]\n" << std::endl;
			}
		}
		else {
			if (debug) {
				std::cout << "compile time: " << std::chrono::duration_cast<std::chrono::milliseconds>(compile_end - compile_begin).count() << "[ms]\n";
			}
		}

		ILEvaluator::sandbox_end();
		
		return true;
	}
}

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "undefined target file\ncrs [file]";
		return 2;
	}

	bool debug = false;
	for (std::size_t i = 2; i < argc; ++i) {
		if (strcmp(argv[i],"-d") == 0) {
			debug = true;
		} else {
			std::cerr << "wrong argument found: " << argv[i] << "\n\t-d\tshow debug output";
			return 2;
		}
	}

	return Corrosive::crs(argv[1], debug) ? 0 : 1;
}