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
	

	const ILArchitecture compiler_arch = (sizeof(void*)==8)? ILArchitecture::bit64 : ILArchitecture::bit32;

	void print_provider(ILEvaluator* eval) {
		auto ptr = eval->pop_register_value<size_t*>();
		const char* text = *(const char**)ptr;
		size_t size = ptr[1];
		std::basic_string_view<char> sv(text, size);
		std::cout << sv;
	}

	void malloc_provider(ILEvaluator* eval) {
		auto size = eval->pop_register_value<size_t>();
		auto ref = malloc(size);
		eval->write_register_value(ref);
	}

	void free_provider(ILEvaluator* eval) {
		auto ref = eval->pop_register_value<void*>();
		free(ref);
	}

	int crs_main() {

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
		m.insintric_function[(unsigned char)ILInsintric::build_subtype] = &Operand::priv_build_subtype;
		m.insintric_function_name[(unsigned char)ILInsintric::build_subtype] = "subtype";
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

		e->parent = &m;

		Ctx::init(&m, &dt, e.get(), &gn, &rts, &cps, &tms,&cmgr);

		Ctx::eval()->sandbox_begin();

		try {

			std::chrono::steady_clock::time_point compile_begin = std::chrono::steady_clock::now();

			Ctx::types()->setup();

			Source std_src;
			std_src.load("..\\test\\std.crs");
			std_src.pair_braces();
			std_src.register_debug();
			Cursor c_std = std_src.read_first();
			Declaration::parse_global(c_std, &gn);
			
			Source src;
			src.load("..\\test\\test.crs");
			src.pair_braces();
			src.register_debug();
			Cursor c_src = src.read_first();
			Declaration::parse_global(c_src, &gn);
			


			ILFunction* main = nullptr;

			Ctx::register_ext_function({ "std","print_slice" }, print_provider);
			Ctx::register_ext_function({ "std","malloc" }, malloc_provider);
			Ctx::register_ext_function({ "std","free" }, free_provider);

			Namespace* f_nspc;
			StructureTemplate* f_stemp;
			FunctionTemplate* f_ftemp;
			TraitTemplate* f_ttemp;

			gn.find_name("main", f_nspc,f_stemp,f_ftemp,f_ttemp);

			if (f_ftemp) {
				FunctionInstance* finst;
				f_ftemp->generate(nullptr, finst);
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
				Ctx::push_scope_context(ILContext::runtime);
				Ctx::eval()->debug_file = UINT16_MAX;
				Ctx::eval()->debug_line = UINT16_MAX;
				ILBuilder::eval_fnptr(Ctx::eval(), main);
				ILBuilder::eval_callstart(Ctx::eval());
				ILBuilder::eval_call(Ctx::eval(), main->decl_id);
				uint64_t ret_val = Ctx::eval()->pop_register_value<uint64_t>();

				std::chrono::steady_clock::time_point runtime_end = std::chrono::steady_clock::now();

				std::cout << "\ntest result was: " << ret_val << "\n\n";

				auto lr1b = (size_t)(Ctx::eval()->register_stack_1b - Ctx::eval()->register_stack_1b);
				
				std::cout << "leaked 1 byte registers: " << lr1b << "\n";
				auto lr2b = (size_t)(Ctx::eval()->register_stack_2b - Ctx::eval()->register_stack_2b);
				std::cout << "leaked 2 byte registers: " << lr2b << "\n";
				auto lr4b = (size_t)(Ctx::eval()->register_stack_4b - Ctx::eval()->register_stack_4b);
				std::cout << "leaked 4 byte registers: " << lr4b << "\n";
				auto lr8b = (size_t)(Ctx::eval()->register_stack_8b - Ctx::eval()->register_stack_8b);
				std::cout << "leaked 8 byte registers: " << lr8b << "\n";

				Ctx::pop_scope_context();


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

		Ctx::eval()->sandbox_end();
		return 0;
	}
}

int main() {
	return Corrosive::crs_main();
}