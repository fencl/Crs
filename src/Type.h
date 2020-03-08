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

namespace Corrosive {
	struct CompileContext;

	class AbstractType {
	public:
		AbstractType();
		AbstractType(ILDataType rv);
		virtual ~AbstractType();
		ILDataType rvalue;

		virtual bool compile(CompileContext& ctx);
	};

	class Structure;
	class DirectType : public AbstractType {
	public:
		Structure* owner;
		virtual bool compile(CompileContext& ctx);
	};

	class StructureInstance;
	class InstanceType : public AbstractType {
	public:
		StructureInstance* owner;
		virtual bool compile(CompileContext& ctx);
	};

	class Type {
	public:
		AbstractType* type;
		uint32_t ref_count = 0;

		static Type null;
		size_t size(CompileContext& ctx);
	};


	inline bool operator == (const Type& t1, const Type& t2) {
		return t1.type == t2.type && t1.ref_count == t2.ref_count;
	}
	inline bool operator != (const Type& t1, const Type& t2) {
		return !(t1 == t2);
	}
}

#include "CompileContext.h"
#endif
