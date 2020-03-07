#pragma once
#ifndef _declaration_crs_h
#define _declaration_crs_h


#include "CompileContext.h"
#include <memory>
#include <vector>
#include "Cursor.h"
#include "Type.h"
#include <unordered_map>
#include <map>
#include "IL/IL.h"

#include <llvm/Core.h>
namespace Corrosive {

	class StructDeclaration;
	class FunctionDeclaration;
	class TypedefDeclaration;

	enum class StructDeclarationType {
		Declared, t_u8, t_u16, t_u32, t_u64, t_i8, t_i16, t_i32, t_i64, t_f32, t_f64, t_ptr, t_bool, t_array, t_tuple, t_string
	};

	class Declaration {
	public:
		virtual ~Declaration();
		static bool parse(Cursor& c, std::vector<std::unique_ptr<Declaration>>& into, Declaration* parent,NamespaceDeclaration* pack);

		virtual std::unique_ptr<Declaration> clone();

		virtual void print			(unsigned int offset) const;
		virtual bool compile		(CompileContext& ctx);
		virtual bool pre_compile	(CompileContext& ctx);

		Cursor					name;
		std::string_view		package				= "g";
		int						compile_progress	= 0;
		Declaration*			parent				= nullptr;
		NamespaceDeclaration*	parent_pack			= nullptr;
		StructDeclaration*		parent_struct() const;
	};




	class VariableDeclaration : public Declaration {
	public:
		const Corrosive::Type* type;

		virtual std::unique_ptr<Declaration> clone();

		virtual void print			(unsigned int offset) const;
		virtual bool compile		(CompileContext& ctx);
		virtual bool pre_compile	(CompileContext& ctx);
	};




	class TypedefDeclaration : public Declaration {
	public:
		bool resolve_type(const Type*& into);

		virtual void print(unsigned int offset) const;

		const Corrosive::Type*	type;
		int	resolve_progress = 0;
	};




	class FunctionDeclaration : public Declaration {
	public:

		virtual std::unique_ptr<Declaration> clone();

		virtual bool compile		(CompileContext& ctx);
		virtual bool pre_compile	(CompileContext& ctx);
		virtual void print			(unsigned int offset) const;

		ILFunction* function = nullptr;

		const Corrosive::Type*	type;
		std::vector<Cursor>		argnames;
		Cursor					block;
		bool					has_block = false;
		bool					is_static = false;
	};

	class GenericFunctionDeclaration : public FunctionDeclaration {
	public:
		virtual void print(unsigned int offset) const;

		std::map<std::string_view, int> generic_typenames;
	};




	class StructDeclaration : public Declaration {
	public:
		virtual void print			(unsigned int offset) const;
		virtual bool compile		(CompileContext& ctx);
		virtual bool pre_compile	(CompileContext& ctx);

		bool test_interface_complete();
		bool build_lookup_table();

		Declaration* FindDeclarationOfMember(std::string_view name);

		std::vector<std::unique_ptr<Declaration>>	members;
		std::vector<std::pair<Cursor, Cursor>>		aliases;

		std::map<std::string_view, std::tuple<Declaration*, unsigned int, std::string_view>> lookup_table;
		std::vector<std::pair<StructDeclaration*, const Corrosive::Type*>> implements;


		virtual bool is_generic();

		bool	has_lookup_table = false;
		bool	is_trait = false;
		bool	compiled = false;
		int		gen_id = 0; 
		bool	is_extending = false;

		StructDeclarationType			decl_type = StructDeclarationType::Declared;
		const TemplateContext*			template_ctx = nullptr;
		std::vector<StructDeclaration*> implements_structures;

		ILType* iltype = nullptr;
	};

	class GenericStructDeclaration : public StructDeclaration {
	public:
		bool create_template(CompileContext& ctx, StructDeclaration*& into);
		virtual void print(unsigned int offset) const;
		virtual bool is_generic();

		std::map<const TemplateContext*, std::unique_ptr<StructDeclaration>> generated;
		std::map<std::string_view, int> generic_typenames;
	};




	class NamespaceDeclaration : public Declaration {
	public:
		virtual void print(unsigned int offset) const;

		std::vector<std::unique_ptr<Declaration>>	members;
		std::vector<std::string_view>				queue;
	};
}

#endif