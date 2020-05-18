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
	struct CompileContext;
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

		virtual bool compile();

		virtual int compare(ILEvaluator* eval, unsigned char* p1, unsigned char* p2);
		virtual void move(ILEvaluator* eval, unsigned char* src, unsigned char* dst);
		virtual void copy(ILEvaluator* eval, unsigned char* src, unsigned char* dst);

		virtual void construct(ILEvaluator* eval, unsigned char* ptr);
		virtual void drop(ILEvaluator* eval, unsigned char* ptr);

		virtual bool has_special_constructor();
		virtual bool has_special_destructor();

		virtual uint32_t size(ILEvaluator* eval);
		virtual uint32_t alignment(ILEvaluator* eval);
		virtual uint32_t compile_size(ILEvaluator* eval);
		virtual uint32_t compile_alignment(ILEvaluator* eval);

		virtual void build_construct();
		virtual void build_move();
		virtual void build_copy();
		virtual void build_drop();

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

		virtual ILContext context();
		virtual bool compile();
		virtual int compare(ILEvaluator* eval,  unsigned char* p1,  unsigned char* p2);
		virtual void move(ILEvaluator* eval,  unsigned char* src,  unsigned char* dst);
		virtual void copy(ILEvaluator* eval, unsigned char* src, unsigned char* dst);
		virtual uint32_t size(ILEvaluator* eval);
		virtual uint32_t alignment(ILEvaluator* eval);
		virtual uint32_t compile_size(ILEvaluator* eval);
		virtual uint32_t compile_alignment(ILEvaluator* eval);

		virtual void construct(ILEvaluator* eval, unsigned char* ptr);
		virtual void drop(ILEvaluator* eval, unsigned char* ptr);

		virtual bool has_special_constructor();
		virtual bool has_special_destructor();

		virtual void build_construct();
		virtual void build_move();
		virtual void build_copy();
		virtual void build_drop();

		virtual bool rvalue_stacked();
		virtual void print(std::ostream& os);
	};

	class TypeStructureTemplate : public Type {
	public:
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_structure_template; }

		StructureTemplate* owner;

		virtual bool compile();
		virtual void print(std::ostream& os);
	};

	class TypeFunctionTemplate : public Type {
	public:
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_function_template; }

		FunctionTemplate* owner;

		virtual bool compile();
		virtual void print(std::ostream& os);
	};

	class TypeTraitInstance : public Type {
	public:
		TraitInstance* owner;

		virtual ILDataType rvalue();

		inline virtual TypeInstanceType type() { return TypeInstanceType::type_trait; }

		virtual ILContext context();
		virtual int compare(ILEvaluator* eval, unsigned char* p1, unsigned char* p2);
		virtual void move(ILEvaluator* eval, unsigned char* src, unsigned char* dst);
		virtual void copy(ILEvaluator* eval, unsigned char* src, unsigned char* dst);
		virtual uint32_t size(ILEvaluator* eval);
		virtual uint32_t alignment(ILEvaluator* eval);
		virtual uint32_t compile_size(ILEvaluator* eval);
		virtual uint32_t compile_alignment(ILEvaluator* eval);

		virtual bool rvalue_stacked();
		virtual void print(std::ostream& os);
	};

	class TypeTraitTemplate : public Type {
	public:
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_trait_template; }

		TraitTemplate* owner;

		virtual bool compile();
		virtual void print(std::ostream& os);
	};

	class TypeArray : public Type {
	public:
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_array; }

		virtual ILDataType rvalue();

		Type* owner;
		uint32_t count;
		virtual ILContext context();
		virtual bool compile();
		virtual int compare(ILEvaluator* eval,  unsigned char* p1,  unsigned char* p2);
		virtual void move(ILEvaluator* eval,  unsigned char* src,  unsigned char* dst);
		virtual void copy(ILEvaluator* eval, unsigned char* src, unsigned char* dst);
		virtual uint32_t size(ILEvaluator* eval);
		virtual uint32_t alignment(ILEvaluator* eval);
		virtual uint32_t compile_size(ILEvaluator* eval);
		virtual uint32_t compile_alignment(ILEvaluator* eval);
		virtual void print(std::ostream& os);


		virtual bool has_special_constructor();
		virtual bool has_special_destructor();
	};
	
	class TypeReference : public Type {
	public:
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_reference; }

		virtual ILDataType rvalue();

		Type* owner;
		virtual ILContext context();
		virtual int compare(ILEvaluator* eval,  unsigned char* p1,  unsigned char* p2);
		virtual void move(ILEvaluator* eval,  unsigned char* src,  unsigned char* dst);
		virtual uint32_t size(ILEvaluator* eval);
		virtual uint32_t alignment(ILEvaluator* eval);
		virtual uint32_t compile_size(ILEvaluator* eval);
		virtual uint32_t compile_alignment(ILEvaluator* eval);
		virtual void print(std::ostream& os);
	};

	class TypeSlice : public Type {
	public:
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_slice; }
		Type* owner;

		virtual ILDataType rvalue();

		virtual bool rvalue_stacked();

		virtual ILContext context();
		virtual int compare(ILEvaluator* eval, unsigned char* p1, unsigned char* p2);
		virtual void move(ILEvaluator* eval, unsigned char* src, unsigned char* dst);
		virtual uint32_t size(ILEvaluator* eval);
		virtual uint32_t alignment(ILEvaluator* eval);
		virtual uint32_t compile_size(ILEvaluator* eval);
		virtual uint32_t compile_alignment(ILEvaluator* eval);
		virtual void print(std::ostream& os);
	};

	class TypeFunction : public Type {
	public:
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_function; }

		virtual ILDataType rvalue();

		ILContext ptr_context;

		DefaultTypes* owner;
		size_t argument_array_id;
		Type* return_type;
		virtual ILContext context();

		virtual int compare(ILEvaluator* eval, unsigned char* p1, unsigned char* p2);
		virtual void move(ILEvaluator* eval, unsigned char* src, unsigned char* dst);

		uint32_t size(ILEvaluator* eval);
		uint32_t alignment(ILEvaluator* eval);
		virtual uint32_t compile_size(ILEvaluator* eval);
		virtual uint32_t compile_alignment(ILEvaluator* eval);
		virtual void print(std::ostream& os);
	};

	class TypeTemplate : public Type {
	public:
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_template; }

		virtual ILDataType rvalue();

		DefaultTypes* owner;
		size_t argument_array_id;

		virtual int compare(ILEvaluator* eval, unsigned char* p1, unsigned char* p2);
		virtual void move(ILEvaluator* eval, unsigned char* src, unsigned char* dst);
		virtual uint32_t compile_size(ILEvaluator* eval);
		virtual uint32_t compile_alignment(ILEvaluator* eval);
		virtual void print(std::ostream& os);
	};
}

#include "PredefinedTypes.h"
#include "CompileContext.h"

#endif
