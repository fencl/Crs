#pragma once
#ifndef _expression_crs_h
#define _expression_crs_h
#include "Cursor.h"
#include "CompileContext.h"
#include "Type.h"

namespace Corrosive {
	class Expression {
	public:
		static bool parse(Cursor& c, CompileValue& res, CompileType comp_type);
		static bool rvalue(CompileValue& value, CompileType cpt);

	private:
		static bool parse_and(Cursor& c, CompileValue& res, CompileType comp_type);
		static bool parse_or(Cursor& c, CompileValue& res, CompileType comp_type);
		static bool parse_operators(Cursor& c, CompileValue& res, CompileType comp_type);
		static bool emit(Cursor& c, CompileValue& res, int l, int op, CompileValue left, CompileValue right, CompileType cpt, int next_l,int next_op);
	};
}

#endif