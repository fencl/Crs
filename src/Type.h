#pragma once
#ifndef _type_crs_h
#define _type_crs_h

#include <string_view>
#include <memory>
#include <vector>
#include "Cursor.h"
#include <variant>
#include <unordered_set>
#include "IL/IL.h"
#include <ostream>
#include <map>

namespace Corrosive {
	class Compiler;
	class StructureInstance;
	class StructureTemplate;
	class TraitTemplate;
	class FunctionTemplate;
	class TraitInstance;
	class Type;
	class DefaultTypes;

	enum class TypeInstanceType {
		type_structure_template,type_structure_instance,type_array,type_reference,type_function,type_trait_template,type_trait,type_function_template,type_template,type_slice,type_undefined
	};
	class TypeArray;
	class TypeReference;
	class TypeSlice;

	class Type {
	public:
		inline virtual ~Type() {}
		virtual ILDataType rvalue();
		virtual ILContext context();

		inline virtual TypeInstanceType type() { return TypeInstanceType::type_undefined; }

		virtual void compile();

		virtual Compiler* compiler();

		virtual int8_t compare(unsigned char* me, unsigned char* p2);
		virtual void move(unsigned char* me, unsigned char* from);
		virtual void copy(unsigned char* me, unsigned char* from);

		virtual void construct(unsigned char* me);
		virtual void drop(unsigned char* me);

		virtual bool has_special_constructor();
		virtual bool has_special_destructor();
		virtual bool has_special_copy();
		virtual bool has_special_move();
		virtual bool has_special_compare();

		virtual ILSize size();

		virtual void build_construct();
		virtual void build_drop();
		virtual void build_move();
		virtual void build_copy();
		virtual void build_compare();

		virtual bool rvalue_stacked();

		virtual void print(std::ostream& os);

		TypeArray* generate_array(unsigned int count);
		TypeReference* generate_reference();
		TypeSlice* generate_slice();

		std::map<uint64_t, std::unique_ptr<TypeArray>> arrays;
		std::unique_ptr<TypeReference> reference = nullptr;
		std::unique_ptr<TypeSlice> slice = nullptr;
	};

	class TypeStructureInstance : public Type {
	public:
		StructureInstance* owner;

		virtual ILDataType rvalue();

		inline virtual TypeInstanceType type() { return TypeInstanceType::type_structure_instance; }


		virtual Compiler* compiler();
		virtual ILContext context();
		virtual void compile();
		virtual int8_t compare(unsigned char* me,  unsigned char* to);
		virtual void move(unsigned char* me,  unsigned char* from);
		virtual void copy(unsigned char* me, unsigned char* from);
		virtual void construct(unsigned char* me);
		virtual void drop(unsigned char* me);

		virtual ILSize size();

		virtual bool has_special_constructor();
		virtual bool has_special_destructor();
		virtual bool has_special_copy();
		virtual bool has_special_move();
		virtual bool has_special_compare();

		virtual void build_construct();
		virtual void build_drop();
		virtual void build_move();
		virtual void build_copy();
		virtual void build_compare();

		virtual bool rvalue_stacked();
		virtual void print(std::ostream& os);
	};

	class TypeStructureTemplate : public Type {
	public:
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_structure_template; }

		StructureTemplate* owner;

		virtual Compiler* compiler();
		virtual void compile();
		virtual void print(std::ostream& os);
	};

	class TypeFunctionTemplate : public Type {
	public:
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_function_template; }

		FunctionTemplate* owner;

		virtual Compiler* compiler();
		virtual void compile();
		virtual void print(std::ostream& os);
	};

	class TypeTraitInstance : public Type {
	public:
		TraitInstance* owner;

		virtual ILDataType rvalue();

		inline virtual TypeInstanceType type() { return TypeInstanceType::type_trait; }

		virtual ILContext context();
		virtual ILSize size();

		virtual bool rvalue_stacked();
		virtual void print(std::ostream& os);

		virtual Compiler* compiler();
	};

	class TypeTraitTemplate : public Type {
	public:
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_trait_template; }

		TraitTemplate* owner;

		virtual void compile();
		virtual void print(std::ostream& os);

		virtual Compiler* compiler();
	};

	class TypeArray : public Type {
	public:
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_array; }

		virtual ILDataType rvalue();

		Type* owner;
		uint32_t table;

		virtual ILContext context();
		virtual void compile();

		virtual void construct(unsigned char* me);
		virtual void drop(unsigned char* me);
		virtual int8_t compare(unsigned char* me,  unsigned char* to);
		virtual void move(unsigned char* me,  unsigned char* from);
		virtual void copy(unsigned char* me, unsigned char* from);

		virtual ILSize size();
		virtual void print(std::ostream& os);


		virtual bool has_special_constructor();
		virtual bool has_special_destructor();
		virtual bool has_special_copy();
		virtual bool has_special_move();
		virtual bool has_special_compare();

		virtual Compiler* compiler();
	};
	
	class TypeReference : public Type {
	public:
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_reference; }

		virtual ILDataType rvalue();

		Type* owner;
		virtual ILContext context();
		virtual ILSize size();
		virtual void print(std::ostream& os);
		virtual Compiler* compiler();
	};

	class TypeSlice : public Type {
	public:
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_slice; }
		Type* owner;

		virtual ILDataType rvalue();

		virtual bool rvalue_stacked();

		virtual ILContext context();
		virtual ILSize size();
		virtual void print(std::ostream& os);

		virtual Compiler* compiler();
	};

	class TypeFunction : public Type {
	public:
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_function; }

		virtual ILDataType rvalue();

		ILContext ptr_context;

		Compiler* owner;
		size_t argument_array_id;
		Type* return_type;
		uint32_t il_function_decl;
		ILCallingConvention call_conv;

		virtual ILContext context();
		virtual ILSize size();
		virtual void print(std::ostream& os);

		virtual void compile();

		virtual Compiler* compiler();
	};

	class TypeTemplate : public Type {
	public:
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_template; }
		virtual ILDataType rvalue();
		DefaultTypes* owner;
		size_t argument_array_id;
		virtual ILSize size();
		virtual void print(std::ostream& os);

		virtual Compiler* compiler();
	};
}

#include "PredefinedTypes.h"
#include "CompileContext.h"

#endif
