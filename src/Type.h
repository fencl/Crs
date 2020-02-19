#pragma once
#ifndef _type_crs_h
#define _type_crs_h

#include <string_view>
#include <memory>
#include <vector>
#include "Cursor.h"
#include <variant>
#include <unordered_set>
#include "CompileContext.h"

namespace Corrosive {
	
	class Type {
	public:
		virtual ~Type();

		void PrintLn() const;
		virtual void Print() const;

		virtual int ID() const;
		virtual int Cmp(const Type& t2) const;
		virtual size_t Hash() const;

		virtual const Type* ResolvePackage(CompileContext& ctx) const;
		
		virtual void Compile(CompileContext& ctx) const;
		virtual void PreCompile(CompileContext& ctx) const;

		virtual bool CanPrimCastInto(const Type* into) const;

		unsigned int Ref() const;
		void Ref(unsigned int r);

		static const Type* Parse(Cursor& c, std::vector<Cursor>* argnames = nullptr);
		static const Type* ParseDirect(CompileContext& ctx, Cursor& c, std::vector<Cursor>* argnames = nullptr);

		virtual const Type* CloneRef(unsigned int r) const;

		static bool ResolvePackageInPlace(const Type*& t, CompileContext& ctx);
		bool HeavyType() const;

		LLVMTypeRef LLVMType() const;
		LLVMTypeRef LLVMTypeLValue() const;
		LLVMTypeRef LLVMTypeRValue() const;

	protected:
		unsigned int ref = 0;
		LLVMTypeRef llvm_type = nullptr;
		LLVMTypeRef llvm_lvalue = nullptr;
		LLVMTypeRef llvm_rvalue = nullptr;
		bool heavy_type = false;
		unsigned int rc = 0;
		bool compiled = false;
	};

	bool operator == (const Type& t1, const Type& t2);
	bool operator != (const Type& t1, const Type& t2);
	bool operator > (const Type& t1, const Type& t2);
	bool operator < (const Type& t1, const Type& t2);

	class PrimitiveType : public Type {
	public:
		virtual void Print() const;

		virtual int ID() const;
		virtual int Cmp(const Type& t2) const;
		virtual size_t Hash() const;
		virtual const Type* ResolvePackage(CompileContext& ctx) const;
	

		virtual void Compile(CompileContext& ctx) const;
		virtual void PreCompile(CompileContext& ctx) const;

		virtual bool CanPrimCastInto(const Type* into) const;

		std::string_view const Pack() const;
		void Pack(std::string_view);


		virtual const Type* CloneRef(unsigned int r) const;

		Cursor const Name() const;
		void Name(Cursor);

		StructDeclaration* Structure() const;


		const TemplateContext* const& Templates() const;
		const TemplateContext*& Templates();
	protected:
		Cursor name;
		std::string_view package;
		StructDeclaration* structure_cache = nullptr;

		const TemplateContext* templates;
	};




	class FunctionType : public Type {
	public:
		
		virtual void Print() const;

		virtual int ID() const;
		virtual int Cmp(const Type& t2) const;
		virtual size_t Hash() const;
		virtual const Type* ResolvePackage(CompileContext& ctx) const;

		virtual void Compile(CompileContext& ctx) const;
		virtual void PreCompile(CompileContext& ctx) const;

		virtual bool CanPrimCastInto(const Type* into) const;
		bool CanPrimCastIntoIgnoreThis(const Type* into) const;


		virtual const Type* CloneRef(unsigned int r) const;

		const Type* Returns() const;
		void Returns(const Type*);

		const std::vector<const Type*>* const & Args() const;
		const std::vector<const Type*>*& Args();
	protected:
		const Type* returns = nullptr;
		const std::vector<const Type*>* arguments;
	};




	class ArrayType : public Type {
	public:
		virtual void Print() const;

		virtual int ID() const;
		virtual int Cmp(const Type& t2) const;
		virtual size_t Hash() const;
		virtual const Type* ResolvePackage(CompileContext& ctx) const;

		virtual void Compile(CompileContext& ctx) const;
		virtual void PreCompile(CompileContext& ctx) const;

		virtual bool CanPrimCastInto(const Type* into) const;


		virtual const Type* CloneRef(unsigned int r) const;

		const Type* Base() const;
		void Base(const Type*);

		Cursor Size() const;
		void Size(Cursor);

		void ActualSize(unsigned int) const;

		bool HasSimpleSize() const;
		void HasSimpleSize(bool b);

	protected:
		Cursor size;
		unsigned int actual_size = 0;
		bool simple_size = true;
		const Type* base = nullptr;
	};




	class TupleType : public Type {
	public:

		virtual void Print() const;

		virtual int ID() const;
		virtual int Cmp(const Corrosive::Type& t2) const;
		virtual size_t Hash() const;
		virtual const Type* ResolvePackage(CompileContext& ctx) const;

		virtual void Compile(CompileContext& ctx) const;
		virtual void PreCompile(CompileContext& ctx) const;

		virtual const Type* CloneRef(unsigned int r) const;

		virtual bool CanPrimCastInto(const Type* into) const;

		const std::vector<const Type*>*& Types();
		const std::vector<const Type*>* const& Types() const;

	protected:
		const std::vector<const Type*>* types;
	};




	class InterfaceType : public Type {
	public:
		virtual void Print() const;

		virtual int ID() const;
		virtual int Cmp(const Corrosive::Type& t2) const;
		virtual size_t Hash() const;
		virtual const Type* ResolvePackage(CompileContext& ctx) const;

		virtual void Compile(CompileContext& ctx) const;
		virtual void PreCompile(CompileContext& ctx) const;

		virtual const Type* CloneRef(unsigned int r) const;

		virtual bool CanPrimCastInto(const Type* into) const;

		const std::vector<const Type*>*& Types();
		const std::vector<const Type*>* const& Types() const;

	protected:
		const std::vector<const Type*>* types;
	};
}

namespace std {
	
	template<> struct hash<Corrosive::Type>
	{
		size_t operator()(const Corrosive::Type& t) const;
	};
}

#endif
