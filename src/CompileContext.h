#pragma once
#ifndef _compile_context_crs_h
#define _compile_context_crs_h
#include <llvm/Core.h>
#include <variant>
#include <vector>

namespace Corrosive {
	class Declaration;
	class StructDeclaration;
	class NamespaceDeclaration;
	class Type;

	enum class CompileType {
		Compile, Eval, ShortCircuit
	};

	using TemplateContext = std::vector<const Type*>;

	struct CompileContext {
		StructDeclaration* parent_struct;
		NamespaceDeclaration* parent_namespace;
		const TemplateContext* template_ctx;

	};

	struct CompileContextExt {
		CompileContext basic;
		LLVMValueRef function = nullptr;
		LLVMBasicBlockRef block = nullptr;
		Declaration* unit = nullptr;
		LLVMBuilderRef builder = nullptr;
		LLVMBasicBlockRef fallback = nullptr;
		std::vector<LLVMBasicBlockRef> incoming_blocks;
		std::vector<LLVMValueRef> incoming_values;
	};

	struct CompileValue {
		LLVMValueRef v;
		const Type* t;
		bool lvalue;
	};
}


#endif