#pragma once
#ifndef _expression_crs_h
#define _expression_crs_h
#include "Cursor.h"
#include "CompileContext.h"
#include "Type.h"

namespace Corrosive {
	class Expression {
	public:
		static void parse(Cursor& c, CompileValue& res, CompileType comp_type, bool require_output = true);
		static void rvalue(CompileValue& value, CompileType cpt);

		static void copy_from_rvalue(Type* me, CompileType cpt, bool me_top = true);
		static void move_from_rvalue(Type* me, CompileType cpt, bool me_top = true);

	private:
		static void parse_and(Cursor& c, CompileValue& res, CompileType comp_type);
		static void parse_or(Cursor& c, CompileValue& res, CompileType comp_type);
		static void parse_operators(Cursor& c, CompileValue& res, CompileType comp_type);
		static void emit(Cursor& c, CompileValue& res, int l, int op, CompileValue left, CompileValue right, CompileType cpt, int next_l,int next_op);
	};
}

#endif