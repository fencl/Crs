#pragma once
#ifndef _expression_crs_h
#define _expression_crs_h
#include "Cursor.h"
#include "CompileContext.h"
#include "Type.h"

namespace Corrosive {
	class Expression {
	public:
		static bool parse(Cursor& c, CompileContext& ctx, CompileValue& res, CompileType comp_type);

	private:
		static bool parse_and(Cursor& c, CompileContext& ctx, CompileValue& res, CompileType comp_type);
		static bool parse_or(Cursor& c, CompileContext& ctx, CompileValue& res, CompileType comp_type);
		static bool parse_operators(Cursor& c, CompileContext& ctx, CompileValue& res, CompileType comp_type);
		static bool rvalue(CompileContext& ctx, CompileValue& value, CompileType cpt);
		static bool emit(Cursor& c, CompileContext& ctx, CompileValue& res, int l, int op, CompileValue left, CompileValue right, CompileType cpt, int next_l,int next_op);
	};
}

#endif