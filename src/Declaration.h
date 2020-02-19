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

namespace Corrosive {

	class StructDeclaration;
	class FunctionDeclaration;
	class TypedefDeclaration;

	class Declaration {
	public:
		virtual ~Declaration();

		virtual std::unique_ptr<Declaration> Clone();

		Cursor Name() const;
		void Name(Cursor c);

		std::string_view const Pack() const;
		void Pack(std::string_view);

		const Declaration* Parent() const;
		void Parent(Declaration*);

		NamespaceDeclaration* ParentPack() const;
		void ParentPack(NamespaceDeclaration*);

		StructDeclaration* ParentStruct() const;

		static void Parse(Cursor& c, std::vector<std::unique_ptr<Declaration>>& into, Declaration* parent,NamespaceDeclaration* pack);

		virtual void Print(unsigned int offset) const;


		virtual void Compile(CompileContext& ctx);
		virtual void PreCompile(CompileContext& ctx);

	protected:
		int llvm_compile_progress = 0;
		Cursor name;
		std::string_view package = "g";
		Declaration* parent = nullptr;
		NamespaceDeclaration* parent_pack = nullptr;
	};

	class VariableDeclaration : public Declaration {
	public:
		const Corrosive::Type*& Type();
		void Type(const Corrosive::Type*);

		virtual std::unique_ptr<Declaration> Clone();

		virtual void Print(unsigned int offset) const;


		virtual void Compile(CompileContext& ctx);
		virtual void PreCompile(CompileContext& ctx);
	protected:
		const Corrosive::Type* type;
	};

	class TypedefDeclaration : public Declaration {
	public:
		const Corrosive::Type*& Type();
		void Type(const Corrosive::Type*);

		virtual void Print(unsigned int offset) const;
		const Corrosive::Type* ResolveType();
	protected:
		const Corrosive::Type* type;
		int resolve_progress = 0;
	};

	class FunctionDeclaration : public Declaration {
	public:

		virtual std::unique_ptr<Declaration> Clone();

		const Corrosive::Type*& Type();

		void Type(const Corrosive::Type*);

		Cursor Block() const;
		void Block(Cursor c);

		bool HasBlock() const;
		void HasBlock(bool b);

		std::vector<Cursor>* Argnames();

		virtual void Compile(CompileContext& ctx);
		virtual void PreCompile(CompileContext& ctx);

		virtual void Print(unsigned int offset) const;

		bool Static() const;
		void Static(bool b);

	protected:
		bool isstatic = false;
		const Corrosive::Type* type;
		std::vector<Cursor> argnames;
		Cursor block;
		bool hasBlock = false;
	};

	class GenericFunctionDeclaration : public FunctionDeclaration {
	public:
		virtual void Print(unsigned int offset) const;
		const std::map<std::string_view, int>& Generics() const;
		std::map<std::string_view, int>& Generics();

	protected:
		std::map<std::string_view, int> generic_typenames;
	};


	enum class StructDeclarationType {
		Declared,t_u8,t_u16,t_u32,t_u64, t_i8, t_i16, t_i32, t_i64, t_f32, t_f64,t_ptr,t_bool,t_array,t_tuple,t_string
	};

	class StructDeclaration : public Declaration {
	public:

		std::vector<std::unique_ptr<Declaration>> Members;

		virtual void Print(unsigned int offset) const;

		bool Class() const;
		void Class(bool c);

		virtual bool Generic();

		virtual void Compile(CompileContext& ctx);
		virtual void PreCompile(CompileContext& ctx);

		void TestInterfaceComplete();
		void BuildLookupTable();

		int GenID() const;
		void GenID(int id);

		void Template(const TemplateContext* tc);
		const TemplateContext* Template() const;

		bool Extending();
		void Extending(bool);

		const std::vector<std::pair<StructDeclaration*, const Corrosive::Type*>>& Extends() const;
		std::vector<std::pair<StructDeclaration*, const Corrosive::Type*>>& Extends();

		StructDeclarationType DeclType() const;
		void DeclType(StructDeclarationType t);

		LLVMTypeRef LLVMType();

		std::vector<std::pair<Cursor, Cursor>> Aliases;
		std::map<std::string_view, std::tuple<Declaration*, unsigned int, std::string_view>> LookupTable;

	protected:
		std::vector<StructDeclaration*> extends_structures;
		std::vector<std::pair<StructDeclaration*, const Corrosive::Type*>> extends;
		bool isextending = false;
		StructDeclarationType decl_type = StructDeclarationType::Declared;
		const TemplateContext* template_ctx = nullptr;

		bool isClass = false;
		bool compiled = false;
		int gen_id = 0;
		LLVMTypeRef llvm_type = nullptr;
	};

	class GenericStructDeclaration : public StructDeclaration {
	public:
		const std::map<std::string_view, int>& Generics() const;
		std::map<std::string_view, int>& Generics();

		StructDeclaration* CreateTemplate(CompileContext& ctx);

		std::map<const TemplateContext*, std::unique_ptr<StructDeclaration>>& Generated();
		const std::map<const TemplateContext*, std::unique_ptr<StructDeclaration>>& Generated() const;

		virtual void Print(unsigned int offset) const;

		virtual bool Generic();
	protected:
		std::map<const TemplateContext*,std::unique_ptr<StructDeclaration>> generated;
		std::map<std::string_view,int> generic_typenames;
	};

	class NamespaceDeclaration : public Declaration {
	public:
		std::vector<std::unique_ptr<Declaration>> Members;
		virtual void Print(unsigned int offset) const;
		std::vector<std::string_view> queue;
	};
}

#endif