#pragma once
#ifndef _operand_crs_h
#define _operand_crs_h
#include "Cursor.h"
#include "CompileContext.h"

namespace Corrosive {

	class Operand {
	public:
		static bool parse(Cursor& c, CompileContext& ctx, CompileValue& res, CompileType copm_type);
		static bool cast(Cursor& err, CompileContext& ctx, CompileValue& from, Type*& to, CompileType copm_type);

		template<typename T, typename S>
		static bool parse_generate_template(Cursor& c, CompileContext& ctx, T* st, S*& out);

		static bool priv_build_array(ILEvaluator* eval);
		static bool priv_build_reference(ILEvaluator* eval);

		static bool parse_const_type_function(Cursor& c, CompileContext& ctx, FunctionInstance*& func,Type*& type);
		static bool parse_expression(CompileValue& ret, Cursor& c, CompileContext& ctx, CompileType copm_type);
		static bool parse_reference(CompileValue& ret, Cursor& c, CompileContext& ctx, CompileType copm_type);
		static bool parse_array_type(CompileValue& ret, Cursor& c, CompileContext& ctx, CompileType copm_type);
		static bool parse_symbol(CompileValue& ret, Cursor& c, CompileContext& ctx, CompileType copm_type);
		static bool parse_number(CompileValue& ret, Cursor& c, CompileContext& ctx, CompileType copm_type);
		static bool parse_long_number(CompileValue& ret, Cursor& c, CompileContext& ctx, CompileType copm_type);
		static bool parse_float_number(CompileValue& ret, Cursor& c, CompileContext& ctx, CompileType copm_type);
		static bool parse_call_operator(CompileValue& ret, Cursor& c, CompileContext& ctx, CompileType copm_type);
		static bool parse_array_operator(CompileValue& ret, Cursor& c, CompileContext& ctx, CompileType copm_type);
		static bool parse_dot_operator(CompileValue& ret, Cursor& c, CompileContext& ctx, CompileType copm_type);
		static bool parse_double_colon_operator(CompileValue& ret, Cursor& c, CompileContext& ctx, CompileType copm_type);
	};
}


#endif