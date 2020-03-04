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
#include "ir/IR.h"

int main() {
	auto start = std::chrono::system_clock::now();

	Corrosive::Source src;
	src.load("..\\test\\test.crs");
	
	std::unique_ptr<Corrosive::IRModule> m = std::make_unique<Corrosive::IRModule>();

	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << "load: " << elapsed.count() << "ms" << std::endl;
	start = end;

	Corrosive::Cursor c = src.read_first();
	std::vector<std::unique_ptr<Corrosive::Declaration>> decls;
	Corrosive::init_predefined_types(decls);

	while (c.tok != Corrosive::RecognizedToken::Eof)
		Corrosive::Declaration::parse(c, decls, nullptr,nullptr);


	end = std::chrono::system_clock::now();
	elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << "parse: " << elapsed.count() << "ms" << std::endl;
	start = end;
	Corrosive::CompileContext ctx;

	
	ctx.module = m.get();
	ctx.parent_namespace = Corrosive::Contents::entry_point->parent_pack;
	ctx.parent_struct = Corrosive::Contents::entry_point->parent_struct();
	ctx.template_ctx = nullptr;

	Corrosive::Contents::entry_point->compile(ctx);
		

	for (auto&& it : Corrosive::Contents::StaticStructures) {
		ctx.parent_namespace = it->parent_pack;
		ctx.parent_struct = it;
		ctx.template_ctx = it->template_ctx;
		it->compile(ctx);
	}

	end = std::chrono::system_clock::now();
	elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << "compile: " << elapsed.count() << "ms" << std::endl;
	start = end;

	/*LLVMDumpValue(Corrosive::Contents::entry_point->function);
	std::cout << std::endl << std::endl;


	for (auto&& it : Corrosive::Contents::StaticStructures) {
		LLVMTypeRef t = it->LLVMType();
		if (t != nullptr) {
			LLVMDumpType(t);
			std::cout << "\n";
		}
	}*/

	std::cout << "\n";
	for (auto&& f : m->functions) {
		f->assert_flow();
		f->dump();
		std::cout << "\n";
		std::cout << "\n";
	}



	if (false) {

		std::cout << std::endl;
		for (auto it = decls.begin(); it != decls.end(); it++) {
			auto s = dynamic_cast<Corrosive::StructDeclaration*>(it->get());
			if (s == nullptr || !s->is_extending)
				it->get()->print(0);
		}

		std::cout << std::endl << std::endl;
	}

	std::cout << "allocated: \n\ttypes: " << Corrosive::Contents::AllTypes.size()
		<< "\n\ttype arrays: " << Corrosive::Contents::TypeArrays.size() 
		<< "\n\tgeneric arrays: " << Corrosive::Contents::GenericArrays.size()
		<< std::endl;

	if (false) {
		for (auto&& t : Corrosive::Contents::AllTypes) {
			t->print_ln();
		}
	}	

	return 0;
}
