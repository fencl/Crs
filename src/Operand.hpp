#ifndef _operand_crs_h
#define _operand_crs_h
#include "Cursor.hpp"
#include "CompileContext.hpp"

namespace Corrosive {

	class Operand {
	public:
		static void parse(Cursor& c, RecognizedToken& tok, CompileValue& res, CompileType copm_type, bool targets_defer);
		static void cast(Cursor& err, CompileValue& from, Type*& to, CompileType copm_type, bool implicit);

		template<typename T, typename S>
		static void parse_generate_template(Cursor& c, RecognizedToken& tok, T* st, S*& out);
		
		static void type_template_cast_crsr(ILEvaluator* eval, Cursor& err);
		static void type_template_cast(ILEvaluator* eval);
		static void push_template(ILEvaluator* eval);
		static void build_template(ILEvaluator* eval);

		static Type* template_stack[1024];
		static uint16_t template_sp;

		static void deref(CompileValue& val, CompileType cpt);
		static void function_call(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType cpt, unsigned int argi, bool targets_defer);
		static void structure_element_offset(CompileValue& ret, tableelement_t id, CompileType cpt);
		static void parse_const_type_function(Cursor& c, RecognizedToken& tok, CompileValue& res);
		static void parse_expression(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType copm_type);
		static void parse_reference(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType copm_type, bool targets_defer);
		static void parse_array_type(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType copm_type, bool targets_defer);
		static void parse_symbol(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType copm_type, bool targets_defer);
		static void parse_number(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType copm_type);
		static void parse_long_number(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType copm_type);
		static void parse_float_number(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType copm_type);
		static void parse_call_operator(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType copm_type, bool targets_defer);
		static void parse_array_operator(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType copm_type);
		static void parse_dot_operator(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType copm_type, bool targets_defer);
		static void parse_double_colon_operator(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType copm_type, bool targets_defer);
		static void parse_string_literal(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType copm_type);
		static void read_arguments(Cursor& c, RecognizedToken& tok, unsigned int& argi, TypeFunction* ft, CompileType cpt);

		static bool is_numeric_value(Type* type);
	};
}


#endif