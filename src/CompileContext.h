#pragma once
#ifndef _compile_context_crs_h
#define _compile_context_crs_h
#include <llvm/Core.h>
#include <llvm/Target.h>
#include <variant>
#include <vector>

namespace Corrosive {
	class Declaration;
	class StructDeclaration;
	class NamespaceDeclaration;
	class Type;

	enum class CompileType {
		compile, Eval, ShortCircuit
	};

	using TemplateContext = std::vector<const Type*>;

	struct CompileContext {
		StructDeclaration* parent_struct;
		NamespaceDeclaration* parent_namespace;
		const TemplateContext* template_ctx;
		LLVMModuleRef module;
		LLVMTargetDataRef target_layout;
	};

	struct CompileContextExt {
		CompileContext basic;
		LLVMValueRef function = nullptr;
		LLVMBasicBlockRef block = nullptr;
		Declaration* unit = nullptr;
		LLVMBuilderRef builder = nullptr;
		LLVMBasicBlockRef fallback_and = nullptr;
		std::vector<LLVMBasicBlockRef> incoming_blocks_and;
		std::vector<LLVMValueRef> incoming_values_and;

		LLVMBasicBlockRef fallback_or = nullptr;
		std::vector<LLVMBasicBlockRef> incoming_blocks_or;
		std::vector<LLVMValueRef> incoming_values_or;
	};

	struct CompileValue {
		LLVMValueRef v;
		const Type* t;
		bool lvalue;
	};
}


#endif