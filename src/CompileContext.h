#pragma once
#ifndef _compile_context_crs_h
#define _compile_context_crs_h
#include <variant>
#include <vector>
#include "ir/IR.h"

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
		StructDeclaration*		parent_struct;
		NamespaceDeclaration*	parent_namespace = nullptr;
		const TemplateContext*	template_ctx = nullptr;
		IRModule*				module = nullptr;
	};

	struct CompileContextExt {
		CompileContext					basic;
		IRFunction*						function = nullptr;
		IRBlock*						block = nullptr;
		Declaration*					unit = nullptr;
		IRBlock*						fallback = nullptr;
	};

	struct CompileValue {
		const Type*		t = nullptr;
		bool			lvalue;
	};
}


#endif