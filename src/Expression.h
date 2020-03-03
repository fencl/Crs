#pragma once
#ifndef _expression_crs_h
#define _expression_crs_h
#include "Cursor.h"
#include "CompileContext.h"
#include "Type.h"

namespace Corrosive {
	class Expression {
	public:
		static CompileValue parse(Cursor& c, CompileContextExt& ctx, CompileType comp_type);

	private:
		static CompileValue parse_and(Cursor& c, CompileContextExt& ctx, CompileType comp_type);
		static CompileValue parse_or(Cursor& c, CompileContextExt& ctx, CompileType comp_type);
		static CompileValue parse_operators(Cursor& c, CompileContextExt& ctx, CompileType comp_type);
		static void rvalue(CompileContextExt& ctx, CompileValue& value, CompileType cpt);
		static CompileValue emit(Cursor& c,CompileContextExt& ctx , int l, int op, CompileValue left, CompileValue right, CompileType cpt, int next_l,int next_op);
		static bool arith_cast(CompileValue& left, CompileValue& right,bool& isfloat, bool& issigned);
		static int arith_value(const PrimitiveType* pt);
		static void arith_promote(CompileValue& value,int from,int to);
	};
}

#endif