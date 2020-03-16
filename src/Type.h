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
		virtual int compare(void* p1, void* p2);
		virtual void move(CompileContext& ctx, void* src, void* dst);
		virtual bool rvalue_stacked();
	};

	class Structure;
	class DirectType : public AbstractType {
	public:
		Structure* owner;
		virtual bool compile(CompileContext& ctx);
		virtual int compare(void* p1, void* p2);
		virtual void move(CompileContext& ctx, void* src, void* dst);
		virtual bool rvalue_stacked();
	};

	class StructureInstance;
	class InstanceType : public AbstractType {
	public:
		StructureInstance* owner;
		virtual bool compile(CompileContext& ctx);
		virtual int compare(void* p1, void* p2);
		virtual void move(CompileContext& ctx, void* src, void* dst);
		virtual bool rvalue_stacked();
	};

	class Type {
	public:
		AbstractType* type;
		uint32_t ref_count = 0;

		static Type null;
		size_t size(CompileContext& ctx);
		bool compile(CompileContext& ctx);
		int compare(size_t s, void* p1, void* p2);
		void move(CompileContext& ctx, void* src, void* dst);
		bool rvalue_stacked();
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
