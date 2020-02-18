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

int main() {
	auto start = std::chrono::system_clock::now();

	Corrosive::Source src;
	src.Load("..\\test\\test.crs");
	
	LLVMModuleRef m = LLVMModuleCreateWithName("module");
	LLVMSetTarget(m, "x86_64-pc-win32");
	LLVMBuilderRef builder = LLVMCreateBuilder();

	auto end = std::chrono::system_clock::now();
	auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << "load: " << elapsed.count() << "ms" << std::endl;
	start = end;

	Corrosive::Cursor c = src.ReadFirst();
	std::vector<std::unique_ptr<Corrosive::Declaration>> decls;
	Corrosive::InitPredefinedTypes(decls);

	while (c.Tok() != Corrosive::RecognizedToken::Eof)
		Corrosive::Declaration::Parse(c, decls, nullptr,nullptr);


	end = std::chrono::system_clock::now();
	elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << "parse: " << elapsed.count() << "ms" << std::endl;
	start = end;
	Corrosive::CompileContext ctx;

	

	ctx.parent_namespace = Corrosive::Contents::entry_point->ParentPack();
	ctx.parent_struct = Corrosive::Contents::entry_point->ParentStruct();
	ctx.template_ctx = nullptr;
	Corrosive::Contents::entry_point->Compile(ctx);


	
	LLVMValueRef func = LLVMAddFunction(m, "main", LLVMFunctionType(LLVMVoidType(), nullptr, 0, false));
	LLVMBasicBlockRef block = LLVMAppendBasicBlock(func, "entry");
	LLVMPositionBuilderAtEnd(builder, block);

	c = Corrosive::Contents::entry_point->Block();
	Corrosive::CompileContextExt cctx;
	cctx.unit = Corrosive::Contents::entry_point;
	cctx.basic.parent_namespace = Corrosive::Contents::entry_point->ParentPack();
	cctx.basic.parent_struct = nullptr;
	cctx.basic.template_ctx = nullptr;
	cctx.builder = builder;

	Corrosive::CompileValue cv = Corrosive::Expression::Parse(c, cctx, Corrosive::CompileType::Compile);

	LLVMDumpValue(func);
	std::cout << std::endl << std::endl;
	

	for (auto&& it : Corrosive::Contents::StaticStructures) {
		ctx.parent_namespace = it->ParentPack();
		ctx.parent_struct = it;
		ctx.template_ctx = it->Template();
		it->Compile(ctx);
	}

	end = std::chrono::system_clock::now();
	elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
	std::cout << "compile: " << elapsed.count() << "ms" << std::endl;
	start = end;


	for (auto&& it : Corrosive::Contents::StaticStructures) {
		LLVMTypeRef t = it->LLVMType();
		if (t != nullptr) {
			LLVMDumpType(t);
			std::cout << "\n";
		}
	}



	if (false) {

		std::cout << std::endl;
		for (auto it = decls.begin(); it != decls.end(); it++) {
			auto s = dynamic_cast<Corrosive::StructDeclaration*>(it->get());
			if (s == nullptr || !s->Extending())
				it->get()->Print(0);
		}

		std::cout << std::endl << std::endl;
	}

	std::cout << "memory usage: \n\ttypes: " << Corrosive::Contents::AllTypes.size()
		<< "\n\ttype arrays: " << Corrosive::Contents::TypeArrays.size() 
		<< "\n\tgeneric arrays: " << Corrosive::Contents::GenericArrays.size()
		<< std::endl;



	return 0;
}
