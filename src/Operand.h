#pragma once
#ifndef _operand_crs_h
#define _operand_crs_h
#include "Cursor.h"
#include "CompileContext.h"

namespace Corrosive {

	class Operand {
	public:
		static bool parse(Cursor& c, CompileValue& res, CompileType copm_type);
		static bool cast(Cursor& err, CompileValue& from, Type*& to, CompileType copm_type, bool implicit);

		template<typename T, typename S>
		static bool parse_generate_template(Cursor& c, T* st, S*& out);

		static bool priv_type_template_cast(ILEvaluator* eva);
		static bool priv_type_template_cast_crsr(ILEvaluator* eval, Cursor& err);
		static bool priv_build_array(ILEvaluator* eval);
		static bool priv_build_reference(ILEvaluator* eval);
		static bool priv_build_slice(ILEvaluator* eval);
		static bool priv_build_push_template(ILEvaluator* eval);
		static bool priv_build_build_template(ILEvaluator* eval);
		static bool priv_malloc(ILEvaluator* eval);
		static bool priv_memcpy(ILEvaluator* eval);

		static Type* template_stack[1024];
		static uint16_t template_sp;

		static bool parse_const_type_function(Cursor& c, FunctionInstance*& func,Type*& type);
		static bool parse_expression(CompileValue& ret, Cursor& c, CompileType copm_type);
		static bool parse_reference(CompileValue& ret, Cursor& c, CompileType copm_type);
		static bool parse_array_type(CompileValue& ret, Cursor& c, CompileType copm_type);
		static bool parse_symbol(CompileValue& ret, Cursor& c, CompileType copm_type);
		static bool parse_number(CompileValue& ret, Cursor& c, CompileType copm_type);
		static bool parse_long_number(CompileValue& ret, Cursor& c, CompileType copm_type);
		static bool parse_float_number(CompileValue& ret, Cursor& c, CompileType copm_type);
		static bool parse_call_operator(CompileValue& ret, Cursor& c, CompileType copm_type);
		static bool parse_array_operator(CompileValue& ret, Cursor& c, CompileType copm_type);
		static bool parse_dot_operator(CompileValue& ret, Cursor& c, CompileType copm_type);
		static bool parse_double_colon_operator(CompileValue& ret, Cursor& c, CompileType copm_type);
	};
}


#endif