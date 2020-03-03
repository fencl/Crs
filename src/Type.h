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

		void			print_ln() const;
		virtual void	print() const;
		virtual int		id() const;
		virtual int		cmp(const Type& t2) const;
		virtual size_t	hash() const;

		static bool resolve_package_in_place(const Type*& t, CompileContext& ctx);

		virtual const Type* clone_ref				(bool r) const;
		virtual const Type* resolve_package			(CompileContext& ctx) const;
		virtual void		compile					(CompileContext& ctx) const;
		virtual void		pre_compile				(CompileContext& ctx) const;
		virtual bool		can_simple_cast_into	(const Type* into) const;

		static const Type* parse		(Cursor& c, std::vector<Cursor>* argnames = nullptr);
		static const Type* parse_direct	(CompileContext& ctx, Cursor& c, std::vector<Cursor>* argnames = nullptr);

		LLVMTypeRef LLVMType() const;
		LLVMTypeRef LLVMTypeLValue() const;
		LLVMTypeRef LLVMTypeRValue() const;

		bool ref = false;
		bool is_heavy = false;
		bool compiled = false;

	protected:
		LLVMTypeRef llvm_type = nullptr;
		LLVMTypeRef llvm_lvalue = nullptr;
		LLVMTypeRef llvm_rvalue = nullptr;
	};

	bool operator == (const Type& t1, const Type& t2);
	bool operator != (const Type& t1, const Type& t2);
	bool operator > (const Type& t1, const Type& t2);
	bool operator < (const Type& t1, const Type& t2);

	class PrimitiveType : public Type {
	public:
		virtual void print() const;

		virtual int		id		() const;
		virtual int		cmp		(const Type& t2) const;
		virtual size_t	hash	() const;

		virtual const Type*		resolve_package			(CompileContext& ctx) const;
		virtual void			compile					(CompileContext& ctx) const;
		virtual void			pre_compile				(CompileContext& ctx) const;
		virtual bool			can_simple_cast_into	(const Type* into) const;
		virtual const Type*		clone_ref				(bool r) const;


		Cursor					name;
		std::string_view		package;
		StructDeclaration*		structure = nullptr;
		const TemplateContext*	templates;
	};




	class FunctionType : public Type {
	public:
		
		virtual void print() const;

		virtual int id() const;
		virtual int cmp(const Type& t2) const;
		virtual size_t hash() const;
		virtual const Type* resolve_package(CompileContext& ctx) const;

		virtual void compile(CompileContext& ctx) const;
		virtual void pre_compile(CompileContext& ctx) const;

		virtual bool can_simple_cast_into(const Type* into) const;
		bool CanPrimCastIntoIgnoreThis(const Type* into) const;


		virtual const Type* clone_ref(bool r) const;

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
		virtual void print() const;

		virtual int id() const;
		virtual int cmp(const Type& t2) const;
		virtual size_t hash() const;
		virtual const Type* resolve_package(CompileContext& ctx) const;

		virtual void compile(CompileContext& ctx) const;
		virtual void pre_compile(CompileContext& ctx) const;

		virtual bool can_simple_cast_into(const Type* into) const;


		virtual const Type* clone_ref(bool r) const;

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

		virtual void print() const;

		virtual int id() const;
		virtual int cmp(const Corrosive::Type& t2) const;
		virtual size_t hash() const;
		virtual const Type* resolve_package(CompileContext& ctx) const;

		virtual void compile(CompileContext& ctx) const;
		virtual void pre_compile(CompileContext& ctx) const;

		virtual const Type* clone_ref(bool r) const;

		virtual bool can_simple_cast_into(const Type* into) const;

		const std::vector<const Type*>*& Types();
		const std::vector<const Type*>* const& Types() const;

	protected:
		const std::vector<const Type*>* types;
	};




	class InterfaceType : public Type {
	public:
		virtual void print() const;

		virtual int id() const;
		virtual int cmp(const Corrosive::Type& t2) const;
		virtual size_t hash() const;
		virtual const Type* resolve_package(CompileContext& ctx) const;

		virtual void compile(CompileContext& ctx) const;
		virtual void pre_compile(CompileContext& ctx) const;

		virtual const Type* clone_ref(bool r) const;

		virtual bool can_simple_cast_into(const Type* into) const;

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
