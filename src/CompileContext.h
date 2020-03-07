#pragma once
#ifndef _compile_context_crs_h
#define _compile_context_crs_h
#include <variant>
#include <vector>
#include "IL/IL.h"

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
		ILModule*				module = nullptr;
	};

	struct CompileContextExt {
		CompileContext					basic;
		ILFunction*						function = nullptr;
		ILBlock*						block = nullptr;
		Declaration*					unit = nullptr;
	};

	struct CompileValue {
		const Type*		t = nullptr;
		bool			lvalue;
	};
}


#endif