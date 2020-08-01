#pragma once
#ifndef _expression_crs_h
#define _expression_crs_h
#include "Cursor.h"
#include "CompileContext.h"
#include "Type.h"
#include "Compiler.h"

namespace Corrosive {
	class Expression {
	public:
		static void parse(Compiler& compiler, Cursor& c,RecognizedToken& tok, CompileValue& res, CompileType comp_type, bool require_output = true);
		static void rvalue(Compiler& compiler, CompileValue& value, CompileType cpt);

		static void copy_from_rvalue(Type* me, CompileType cpt, bool me_top = true);

		static ILDataType arithmetic_type(Type* type);
		static Type* arithmetic_result(Compiler& compiler, Type* type_left, Type* type_right);

	private:
		static void parse_and(Compiler& compiler, Cursor& c, RecognizedToken& tok, CompileValue& res, CompileType comp_type);
		static void parse_or(Compiler& compiler, Cursor& c, RecognizedToken& tok, CompileValue& res, CompileType comp_type);
		static void parse_operators(Compiler& compiler, Cursor& c, RecognizedToken& tok, CompileValue& res, CompileType comp_type);
		static void emit(Compiler& compiler, Cursor& c, CompileValue& res, int l, int op, CompileValue left, CompileValue right, CompileType cpt, int next_l, int next_op);
	};
}

#endif