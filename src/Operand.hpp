#ifndef _operand_crs_h
#define _operand_crs_h
#include "Cursor.hpp"
#include "CompileContext.hpp"

namespace Corrosive {

	class Operand {
	public:
		static errvoid parse(Cursor& c, CompileValue& res, CompileType copm_type, bool targets_defer, Type* request = nullptr);
		static errvoid parse_const_decl(CompileValue& ret, Cursor& c, CompileType copm_type, Type* request);
		static errvoid cast(Cursor& err, CompileValue& from, Type*& to, CompileType copm_type, bool implicit);

		template<typename T, typename S>
		static errvoid parse_generate_template(Cursor& c, T* st, S*& out);
		
		static errvoid type_template_cast_crsr(ILEvaluator* eval, Cursor& err);
		static errvoid type_template_cast(ILEvaluator* eval);
		static errvoid push_template(ILEvaluator* eval);
		static errvoid build_template(ILEvaluator* eval);

		static Type* template_stack[1024];
		static std::uint16_t template_sp;

		static errvoid deref(CompileValue& val, CompileType cpt);
		static errvoid function_call(CompileValue& ret, Cursor& c, CompileType cpt, unsigned int argi, bool targets_defer);
		static errvoid structure_element_offset(CompileValue& ret, tableelement_t id, CompileType cpt);
		static errvoid parse_const_type_function(Cursor& c, CompileValue& res);
		static errvoid parse_expression(CompileValue& ret, Cursor& c, CompileType copm_type, Type* request);
		static errvoid parse_reference(CompileValue& ret, Cursor& c, CompileType copm_type, bool targets_defer);
		static errvoid parse_array_type(CompileValue& ret, Cursor& c, CompileType copm_type, bool targets_defer);
		static errvoid parse_symbol(CompileValue& ret, Cursor& c, CompileType copm_type, bool targets_defer);
		static errvoid parse_number(CompileValue& ret, Cursor& c, CompileType copm_type);
		static errvoid parse_long_number(CompileValue& ret, Cursor& c, CompileType copm_type);
		static errvoid parse_float_number(CompileValue& ret, Cursor& c, CompileType copm_type);
		static errvoid parse_call_operator(CompileValue& ret, Cursor& c, CompileType copm_type, bool targets_defer);
		static errvoid parse_array_operator(CompileValue& ret, Cursor& c, CompileType copm_type);
		static errvoid parse_dot_operator(CompileValue& ret, Cursor& c, CompileType copm_type, bool targets_defer);
		static errvoid parse_double_colon_operator(CompileValue& ret, Cursor& c, CompileType copm_type, bool targets_defer);
		static errvoid parse_string_literal(CompileValue& ret, Cursor& c, CompileType copm_type);
		static errvoid read_arguments(Cursor& c, unsigned int& argi, TypeFunction* ft, CompileType cpt);

		static bool is_numeric_value(Type* type);
	};
}


#endif