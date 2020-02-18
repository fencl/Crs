#pragma once
#ifndef _expression_crs_h
#define _expression_crs_h
#include "Cursor.h"
#include "CompileContext.h"
#include "Type.h"

namespace Corrosive {
	class Expression {
	public:
		static CompileValue Parse(Cursor& c, CompileContextExt& ctx, CompileType comp_type);

		static CompileValue EmitOperator(Cursor& c,CompileContextExt& ctx , int l, int op, CompileValue left, CompileValue right, CompileType cpt);
		static bool ArithCast(CompileValue& left, CompileValue& right,bool& isfloat, bool& issigned);
		static int ArithValue(const PrimitiveType* pt);
		static void ArithConstPromote(CompileValue& value,int from,int to);
		
	};
}

#endif