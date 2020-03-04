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
	
	LLVMModuleRef m = LLVMModuleCreateWithName("module");
	LLVMSetTarget(m, "x86_64-pc-win32");
	
	LLVMTargetDataRef t_l = LLVMGetModuleDataLayout(m);

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

	
	ctx.module = m;
	ctx.target_layout = t_l;
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
	LLVMDumpModule(m);
	std::cout << "\n";



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

	std::cout << std::endl << std::endl<<"IR Test\n\n";
	std::unique_ptr<Corrosive::IRModule> irm = std::make_unique<Corrosive::IRModule>();
	Corrosive::IRFunction* irf = irm->create_function(Corrosive::IRDataType::f64);

	Corrosive::IRBlock* irentry_clear = irf->create_block(Corrosive::IRDataType::i32);
	Corrosive::IRBlock* irentry_clear2 = irf->create_block(Corrosive::IRDataType::i32);
	Corrosive::IRBlock* irentry = irf->create_block(Corrosive::IRDataType::none);
	irf->append_block(irentry);
	irf->append_block(irentry_clear);
	irf->append_block(irentry_clear2);



	Corrosive::IRBuilder::build_accept(irentry_clear);
	Corrosive::IRBuilder::build_const_u64(irentry_clear, 17);
	Corrosive::IRBuilder::build_add(irentry_clear);
	Corrosive::IRBuilder::build_const_f64(irentry_clear, 56.54);
	Corrosive::IRBuilder::build_div(irentry_clear);
	Corrosive::IRBuilder::build_yield(irentry_clear);
	Corrosive::IRBuilder::build_ret(irentry_clear);


	Corrosive::IRBuilder::build_accept(irentry_clear2);
	Corrosive::IRBuilder::build_const_f64(irentry_clear2, 156.32);
	Corrosive::IRBuilder::build_div(irentry_clear2);
	Corrosive::IRBuilder::build_yield(irentry_clear2);
	Corrosive::IRBuilder::build_ret(irentry_clear2);


	Corrosive::IRBuilder::build_discard(irentry);
	Corrosive::IRBuilder::build_const_u32(irentry, 42);
	Corrosive::IRBuilder::build_const_u32(irentry, 24);
	Corrosive::IRBuilder::build_gt(irentry);
	Corrosive::IRBuilder::build_const_i32(irentry, 13);
	Corrosive::IRBuilder::build_yield(irentry);
	Corrosive::IRBuilder::build_jmpz(irentry, irentry_clear, irentry_clear2);

	irf->assert_flow();

	irf->dump();



	return 0;
}
