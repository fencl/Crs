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
#include <llvm/Core.h>
#include "ir/IR.h"

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


		bool ref = false;
		bool is_heavy = false;
		bool compiled = false;
		IRType* irtype = nullptr;
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

		virtual int		id		() const;
		virtual int		cmp		(const Type& t2) const;
		virtual size_t	hash	() const;

		virtual const Type*		resolve_package			(CompileContext& ctx) const;
		virtual void			compile					(CompileContext& ctx) const;
		virtual void			pre_compile				(CompileContext& ctx) const;
		virtual bool			can_simple_cast_into	(const Type* into) const;
		virtual const Type*		clone_ref				(bool r) const;

		bool can_simple_cast_into_ignore_this(const Type* into) const;

		const Type*						returns = nullptr;
		const std::vector<const Type*>* arguments;
	};




	class ArrayType : public Type {
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


		unsigned int size = 0;
		const Type*	 base= nullptr;
	};




	class TupleType : public Type {
	public:

		virtual void print() const;

		virtual int		id		() const;
		virtual int		cmp		(const Type& t2) const;
		virtual size_t	hash		() const;

		virtual const Type*		resolve_package			(CompileContext& ctx) const;
		virtual void			compile					(CompileContext& ctx) const;
		virtual void			pre_compile				(CompileContext& ctx) const;
		virtual bool			can_simple_cast_into	(const Type* into) const;
		virtual const Type*		clone_ref				(bool r) const;

		const std::vector<const Type*>* types;
	};




	class InterfaceType : public Type {
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
