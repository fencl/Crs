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
	class Type;
	class DefaultTypes;

	enum class TypeInstanceType {
		type_template,type_instance,type_array,type_reference,type_function,type_undefined
	};
	class TypeArray;
	class TypeReference;

	class Type {
	public:
		inline virtual ~Type() {}
		ILDataType rvalue;
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_undefined; }

		virtual bool compile(CompileContext& ctx);

		virtual int compare(CompileContext& ctx, ILPtr p1, ILPtr p2);
		virtual void move(CompileContext& ctx, ILPtr src, ILPtr dst);
		virtual unsigned int size(CompileContext& ctx);
		virtual unsigned int alignment(CompileContext& ctx);

		virtual bool rvalue_stacked();
		virtual void print(std::ostream& os);

		TypeArray* generate_array(unsigned int count);
		TypeReference* generate_reference();

		std::map<uint64_t, std::unique_ptr<TypeArray>> arrays;
		std::unique_ptr<TypeReference> reference = nullptr;
	};

	class TypeInstance : public Type {
	public:
		StructureInstance* owner;

		inline virtual TypeInstanceType type() { return TypeInstanceType::type_instance; }

		virtual bool compile(CompileContext& ctx);
		virtual int compare(CompileContext& ctx, ILPtr p1, ILPtr p2);
		virtual void move(CompileContext& ctx, ILPtr src, ILPtr dst);
		virtual unsigned int size(CompileContext& ctx);
		virtual unsigned int alignment(CompileContext& ctx);

		virtual bool rvalue_stacked();
		virtual void print(std::ostream& os);
	};

	class TypeStructure : public Type {
	public:
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_template; }

		StructureTemplate* owner;

		virtual bool compile(CompileContext& ctx);
		virtual void print(std::ostream& os);
	};

	class TypeArray : public Type {
	public:
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_array; }

		Type* owner;
		uint32_t count;

		virtual bool compile(CompileContext& ctx);
		virtual int compare(CompileContext& ctx, ILPtr p1, ILPtr p2);
		virtual void move(CompileContext& ctx, ILPtr src, ILPtr dst);
		virtual unsigned int size(CompileContext& ctx);
		virtual unsigned int alignment(CompileContext& ctx);
		virtual void print(std::ostream& os);
	};
	
	class TypeReference : public Type {
	public:
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_reference; }

		Type* owner;
		virtual int compare(CompileContext& ctx, ILPtr p1, ILPtr p2);
		virtual void move(CompileContext& ctx, ILPtr src, ILPtr dst);
		virtual unsigned int size(CompileContext& ctx);
		virtual unsigned int alignment(CompileContext& ctx);
		virtual void print(std::ostream& os);
	};

	class TypeFunction : public Type {
	public:
		inline virtual TypeInstanceType type() { return TypeInstanceType::type_function; }

		DefaultTypes* owner;
		size_t argument_array_id;
		Type* return_type;

		virtual void print(std::ostream& os);
	};
}

#include "PredefinedTypes.h"
#include "CompileContext.h"

#endif
