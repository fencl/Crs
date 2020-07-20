#pragma once
#ifndef _operand_crs_h
#define _operand_crs_h
#include "Cursor.h"
#include "CompileContext.h"

namespace Corrosive {

	class Operand {
	public:
		static void parse(Compiler& compiler, Cursor& c, CompileValue& res, CompileType copm_type);
		static void cast(Compiler& compiler, Cursor& err, CompileValue& from, Type*& to, CompileType copm_type, bool implicit);

		template<typename T, typename S>
		static void parse_generate_template(Compiler& compiler, Cursor& c, T* st, S*& out);

		static void priv_type_template_cast(ILEvaluator* eval);
		static void priv_type_template_cast_crsr(ILEvaluator* eval, Cursor& err);
		static void priv_build_array(ILEvaluator* eval);
		static void priv_build_reference(ILEvaluator* eval);
		static void priv_build_subtype(ILEvaluator* eval);
		static void priv_build_slice(ILEvaluator* eval);
		static void priv_build_push_template(ILEvaluator* eval);
		static void priv_build_build_template(ILEvaluator* eval);
		static void priv_type_size(ILEvaluator* eval);

		static Type* template_stack[1024];
		static uint16_t template_sp;

		static void function_call(Compiler& compiler, CompileValue& ret, Cursor& c, CompileType cpt, unsigned int argi);
		static void structure_element_offset(Compiler& compiler, CompileValue& ret, uint16_t id, CompileType cpt);
		static void parse_const_type_function(Compiler& compiler, Cursor& c, FunctionInstance*& func,Type*& type,ILSize& type_size);
		static void parse_expression(Compiler& compiler, CompileValue& ret, Cursor& c, CompileType copm_type);
		static void parse_reference(Compiler& compiler, CompileValue& ret, Cursor& c, CompileType copm_type);
		static void parse_array_type(Compiler& compiler, CompileValue& ret, Cursor& c, CompileType copm_type);
		static void parse_symbol(Compiler& compiler, CompileValue& ret, Cursor& c, CompileType copm_type);
		static void parse_number(Compiler& compiler, CompileValue& ret, Cursor& c, CompileType copm_type);
		static void parse_long_number(Compiler& compiler, CompileValue& ret, Cursor& c, CompileType copm_type);
		static void parse_float_number(Compiler& compiler, CompileValue& ret, Cursor& c, CompileType copm_type);
		static void parse_call_operator(Compiler& compiler, CompileValue& ret, Cursor& c, CompileType copm_type);
		static void parse_array_operator(Compiler& compiler, CompileValue& ret, Cursor& c, CompileType copm_type);
		static void parse_dot_operator(Compiler& compiler, CompileValue& ret, Cursor& c, CompileType copm_type);
		static void parse_double_colon_operator(Compiler& compiler, CompileValue& ret, Cursor& c, CompileType copm_type);
		static void parse_string_literal(Compiler& compiler, CompileValue& ret, Cursor& c, CompileType copm_type);
		static void read_arguments(Compiler& compiler, Cursor& c, unsigned int& argi, TypeFunction* ft, CompileType cpt);

		static bool is_numeric_value(Type* type);
	};
}


#endif