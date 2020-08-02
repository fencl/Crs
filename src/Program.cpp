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

// for library test
#include <Windows.h>

namespace Corrosive {
	const ILArchitecture compiler_arch = (sizeof(void*)==8)? ILArchitecture::bit64 : ILArchitecture::bit32;

	void print_provider(ILEvaluator* eval) {
		auto ptr = eval->pop_register_value<size_t*>();
		const char* text = *(const char**)ptr;
		size_t size = ptr[1];
		std::basic_string_view<char> sv(text, size);
		std::cout << sv;
	}

	size_t allocated_counter = 0;

	void malloc_provider(ILEvaluator* eval) {
		auto size = eval->pop_register_value<size_t>();
		auto ref = malloc(size);
		eval->write_register_value(ref);
		++allocated_counter;
	}

	void free_provider(ILEvaluator* eval) {
		auto ref = eval->pop_register_value<void*>();
		free(ref);
		--allocated_counter;
	}

	uint64_t print_test(uint64_t a, double b) {
		std::cout << a << ", " << b << "\n";
		return 42;
	}

	void test_fun_provider(ILEvaluator* eval) {
		eval->write_register_value<void*>(print_test);
	}

	void share_provider(ILEvaluator* eval) {
		auto ptr = eval->pop_register_value<size_t*>();
		const char* text = *(const char**)ptr;
		size_t size = ptr[1];
		std::basic_string_view<char> sv(text, size);
		void* lib = LoadLibraryA(std::string(sv).c_str());
		eval->write_register_value<void*>(lib);
	}

	void function_provider(ILEvaluator* eval) {
		auto ptr = eval->pop_register_value<size_t*>();
		const char* text = *(const char**)ptr;
		size_t size = ptr[1];
		std::basic_string_view<char> sv(text, size);
		auto lib = eval->pop_register_value<void*>();
		eval->write_register_value<void*>(GetProcAddress((HMODULE)lib,std::string(sv).c_str()));
	}


	void release_provider(ILEvaluator* eval) {
		auto lib = eval->pop_register_value<void*>();
		FreeLibrary((HMODULE)lib);
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

		
		auto compiler = Compiler::create();
		Compiler::push_compiler(compiler.get());

		ILEvaluator::sandbox_begin();

		try {

			std::chrono::steady_clock::time_point compile_begin = std::chrono::steady_clock::now();


			Source::require("../test/test2.crs");

			ILFunction* main = nullptr;

			compiler->register_ext_function({ "std","print_slice" }, print_provider);
			compiler->register_ext_function({ "std","malloc" }, malloc_provider);
			compiler->register_ext_function({ "std","free" }, free_provider);
			compiler->register_ext_function({ "std","test" }, test_fun_provider);
			compiler->register_ext_function({ "std","library","share" }, share_provider);
			compiler->register_ext_function({ "std","library","function" }, function_provider);
			compiler->register_ext_function({ "std","library","release" }, release_provider);
			


			auto res = compiler->gn.find_name("main");


			if (auto fun = res.get_function()) {
				FunctionInstance* finst;
				fun->generate(nullptr, finst);
				finst->compile();
				main = finst->func;
			}
			else {
				std::cerr << "main not found\n";
			}


			std::chrono::steady_clock::time_point compile_end = std::chrono::steady_clock::now();

			if (main != nullptr) {
				
				std::cout << "========= TEST =========\n";

				std::chrono::steady_clock::time_point runtime_start = std::chrono::steady_clock::now();
				compiler->push_scope_context(ILContext::runtime);
				compiler->evaluator()->debug_file = UINT16_MAX;
				compiler->evaluator()->debug_line = UINT16_MAX;
				ILBuilder::eval_fncall(compiler->evaluator(), main);
				uint64_t ret_val = compiler->evaluator()->pop_register_value<uint64_t>();

				std::chrono::steady_clock::time_point runtime_end = std::chrono::steady_clock::now();

				std::cout << "\ntest result was: " << ret_val << "\n\n";

				auto lr1b = (size_t)(compiler->evaluator()->register_stack_pointer_1b - compiler->evaluator()->register_stack_1b);
				
				std::cout << "leaked 1 byte registers: " << lr1b << "\n";
				auto lr2b = (size_t)(compiler->evaluator()->register_stack_pointer_2b - compiler->evaluator()->register_stack_2b);
				std::cout << "leaked 2 byte registers: " << lr2b << "\n";
				auto lr4b = (size_t)(compiler->evaluator()->register_stack_pointer_4b - compiler->evaluator()->register_stack_4b);
				std::cout << "leaked 4 byte registers: " << lr4b << "\n";
				auto lr8b = (size_t)(compiler->evaluator()->register_stack_pointer_8b - compiler->evaluator()->register_stack_8b);
				std::cout << "leaked 8 byte registers: " << lr8b << "\n\n";

				std::cout << "leaked allocations: " << allocated_counter << "\n";

				compiler->pop_scope_context();

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

		Compiler::pop_compiler();

		return 0;
	}
}

int main() {
	return Corrosive::crs_main();
}