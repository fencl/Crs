#pragma once
#ifndef _expression_crs_h
#define _expression_crs_h
#include "Cursor.h"
#include "CompileContext.h"
#include "Type.h"

namespace Corrosive {
	class Expression {
	public:
		static bool parse(Cursor& c, CompileContextExt& ctx, CompileValue& res, CompileType comp_type);

	private:
		static bool parse_and(Cursor& c, CompileContextExt& ctx, CompileValue& res, CompileType comp_type);
		static bool parse_or(Cursor& c, CompileContextExt& ctx, CompileValue& res, CompileType comp_type);
		static bool parse_operators(Cursor& c, CompileContextExt& ctx, CompileValue& res, CompileType comp_type);
		static bool rvalue(CompileContextExt& ctx, CompileValue& value, CompileType cpt);
		static bool emit(Cursor& c,CompileContextExt& ctx, CompileValue& res, int l, int op, CompileValue left, CompileValue right, CompileType cpt, int next_l,int next_op);
		static bool arith_cast(CompileContextExt& ctx, CompileValue& left, CompileValue& right,bool& isfloat, bool& issigned,bool& res);
		static bool arith_value(CompileContextExt& ctx, const PrimitiveType* pt,int& res);
		static void arith_promote(CompileValue& value,int from,int to);
	};
}

#endif