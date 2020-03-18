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
	enum class TypeInstanceType {
		Structure,StructureInstance,Array
	};

	class TypeInstance {
	public:

		ILDataType rvalue;
		TypeInstanceType type;
		void* owner_ptr;

		bool compile(CompileContext& ctx);
		int compare(ILEvaluator* ctx, void* p1, void* p2);
		void move(CompileContext& ctx, void* src, void* dst);
		bool rvalue_stacked();
		void print(std::ostream& os);
		size_t compile_time_size(ILEvaluator* eval);
	};


	class Type {
	public:
		TypeInstance* type;
		uint32_t ref_count = 0;

		static Type null;
		size_t compile_time_size(ILEvaluator* eval);
		bool compile(CompileContext& ctx);
		int compare(ILEvaluator* ctx, void* p1, void* p2);
		void move(CompileContext& ctx, void* src, void* dst);
		bool rvalue_stacked();
		void print(std::ostream& os);
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
