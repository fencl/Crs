#ifndef _expression_crs_h
#define _expression_crs_h
#include "Cursor.hpp"
#include "CompileContext.hpp"
#include "Type.hpp"
#include "Compiler.hpp"

namespace Corrosive {
	class Expression {
	public:
		static void parse(Cursor& c,RecognizedToken& tok, CompileValue& res, CompileType comp_type, bool require_output = true);
		static void rvalue(CompileValue& value, CompileType cpt);

		static void copy_from_rvalue(Type* me, CompileType cpt);
		static void copy_from_rvalue_reverse(Type* me, CompileType cpt);

		static ILDataType arithmetic_type(Type* type);
		static Type* arithmetic_result(Type* type_left, Type* type_right);

	private:
		static void parse_and(Cursor& c, RecognizedToken& tok, CompileValue& res, CompileType comp_type);
		static void parse_or(Cursor& c, RecognizedToken& tok, CompileValue& res, CompileType comp_type);
		static void parse_operators(Cursor& c, RecognizedToken& tok, CompileValue& res, CompileType comp_type);
		static void emit(Cursor& c, CompileValue& res, int l, int op, CompileValue left, CompileValue right, CompileType cpt, int next_l, int next_op);
	};
}

#endif