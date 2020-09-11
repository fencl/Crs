#ifndef _expression_crs_h
#define _expression_crs_h
#include "Cursor.hpp"
#include "CompileContext.hpp"
#include "Type.hpp"
#include "Compiler.hpp"

namespace Crs {
	class Expression {
	public:
		static errvoid parse(Cursor& c, CompileValue& res, CompileType comp_type, bool require_output = true, Type* request = nullptr);
		static errvoid rvalue(CompileValue& value, CompileType cpt);

		static errvoid copy_from_rvalue(Type* me, CompileType cpt);
		static errvoid copy_from_rvalue_reverse(Type* me, CompileType cpt);

		static ILDataType arithmetic_type(Type* type);
		static Type* arithmetic_result(Type* type_left, Type* type_right);

	private:
		static errvoid parse_and(Cursor& c, CompileValue& res, CompileType comp_type, Type* request);
		static errvoid parse_or(Cursor& c, CompileValue& res, CompileType comp_type, Type* request);
		static errvoid parse_operators(Cursor& c, CompileValue& res, CompileType comp_type, Type* request);
		static errvoid emit(Cursor& c, CompileValue& res, int l, int op, CompileValue left, CompileValue right, CompileType cpt, int next_l, int next_op);
	};
}

#endif