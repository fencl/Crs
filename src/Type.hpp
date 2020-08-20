#pragma once
#ifndef _type_crs_h
#define _type_crs_h

#include <string_view>
#include <memory>
#include <vector>
#include "Cursor.hpp"
#include <variant>
#include <unordered_set>
#include "IL/IL.hpp"
#include <ostream>
#include <map>

namespace Corrosive {
	class Compiler;
	class StructureInstance;
	class StructureTemplate;
	class TraitTemplate;
	class FunctionTemplate;
	class FunctionInstance;
	class TraitInstance;
	class Type;
	class BuiltInTypes;

	enum class TypeInstanceType {
		type_structure_template,type_structure_instance,type_array,type_reference,type_function,type_trait_template,type_trait,type_function_template,type_template,type_slice,type_function_instance,type_undefined
	};
	class TypeArray;
	class TypeReference;
	class TypeSlice;

	const std::uint64_t type_magic_mask = 0xffffffffffffff00u;
	const std::uint64_t type_magic = 9223372036854775807u & type_magic_mask;

	class Type {
	public:
		static errvoid assert(Cursor&c, Type* t);
		static errvoid assert(Type* t);
		virtual std::uint64_t magic();

		inline virtual ~Type() {}
		virtual ILDataType rvalue();
		virtual ILContext context();

		inline virtual TypeInstanceType type() { return TypeInstanceType::type_undefined; }

		virtual errvoid compile();

		virtual errvoid compare_for_generic_storage(std::int8_t& res,unsigned char* me, unsigned char* to);
		virtual errvoid copy_to_generic_storage(unsigned char* me, unsigned char* to);

		virtual errvoid constantize(Cursor& err, unsigned char* target, unsigned char* source);

		virtual ILSize size();
		virtual bool rvalue_stacked();

		virtual void print(std::ostream& os);

		TypeArray* generate_array(std::uint32_t count);
		TypeReference* generate_reference();
		TypeSlice* generate_slice();

		std::map<std::uint64_t, std::unique_ptr<TypeArray>> arrays;
		std::unique_ptr<TypeReference> reference = nullptr;
		std::unique_ptr<TypeSlice> slice = nullptr;
	};

	class TypeStructureInstance : public Type {
	public:
		virtual std::uint64_t magic();

		StructureInstance* owner;

		virtual ILDataType rvalue();

		inline virtual TypeInstanceType type() { return TypeInstanceType::type_structure_instance; }


		virtual errvoid constantize(Cursor& err, unsigned char* target, unsigned char* source);
		
		virtual ILContext context();
		virtual errvoid compile();

		virtual ILSize size();

		virtual bool rvalue_stacked();
		virtual void print(std::ostream& os);
	};

	class TypeStructureTemplate : public Type {
	public:
		virtual std::uint64_t magic();
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_structure_template; }

		StructureTemplate* owner;

		
		virtual errvoid compile();
		virtual void print(std::ostream& os);
	};

	class TypeFunctionTemplate : public Type {
	public:
		virtual std::uint64_t magic();
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_function_template; }

		FunctionTemplate* owner;

		
		virtual errvoid compile();
		virtual void print(std::ostream& os);
	};

	class TypeTraitInstance : public Type {
	public:
		virtual std::uint64_t magic();
		TraitInstance* owner;

		virtual ILDataType rvalue();

		inline virtual TypeInstanceType type() { return TypeInstanceType::type_trait; }

		virtual ILContext context();
		virtual ILSize size();

		virtual bool rvalue_stacked();
		virtual void print(std::ostream& os);		
	};

	class TypeTraitTemplate : public Type {
	public:
		virtual std::uint64_t magic();
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_trait_template; }

		TraitTemplate* owner;

		virtual errvoid compile();
		virtual void print(std::ostream& os);

		
	};

	class TypeArray : public Type {
	public:
		virtual std::uint64_t magic();
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_array; }

		virtual ILDataType rvalue();

		virtual errvoid constantize(Cursor& err, unsigned char* target, unsigned char* source);

		Type* owner;
		//tableid_t table;
		ILSize size_value;
		std::uint32_t count;

		virtual ILContext context();
		virtual errvoid compile();
		virtual ILSize size();
		virtual void print(std::ostream& os);
		virtual bool rvalue_stacked();
	};
	
	class TypeReference : public Type {
	public:
		virtual std::uint64_t magic();
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_reference; }

		virtual ILDataType rvalue();

		Type* owner;
		virtual ILContext context();
		virtual ILSize size();
		virtual void print(std::ostream& os);
		
	};

	class TypeSlice : public Type {
	public:
		virtual std::uint64_t magic();
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_slice; }
		Type* owner;

		virtual errvoid compare_for_generic_storage(std::int8_t& res,unsigned char* me, unsigned char* to);
		virtual errvoid copy_to_generic_storage(unsigned char* me, unsigned char* to);

		virtual errvoid constantize(Cursor& err, unsigned char* target, unsigned char* source);

		virtual ILDataType rvalue();

		virtual ILContext context();
		virtual ILSize size();
		virtual void print(std::ostream& os);
	};

	class TypeFunction : public Type {
	public:
		virtual std::uint64_t magic();
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_function; }

		virtual ILDataType rvalue();

		ILContext ptr_context;

		std::size_t argument_array_id;
		Type* return_type;
		std::uint32_t il_function_decl;
		ILCallingConvention call_conv;

		virtual ILContext context();
		virtual ILSize size();
		virtual void print(std::ostream& os);

		virtual errvoid compile();
	};

	class TypeFunctionInstance : public Type {
	public:
		virtual std::uint64_t magic();
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_function_instance; }

		FunctionInstance* owner;
		TypeFunction* function_type;

		virtual void print(std::ostream& os);
		virtual errvoid compile();
	};

	class TypeTemplate : public Type {
	public:
		virtual std::uint64_t magic();
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_template; }
		virtual ILDataType rvalue();
		BuiltInTypes* owner;
		std::size_t argument_array_id;
		virtual ILSize size();
		virtual void print(std::ostream& os);		
	};
}

#endif
