#pragma once
#ifndef _error_crs_h
#define _error_crs_h
#include "Cursor.hpp"
#include "Type.hpp"

namespace Crs {
	errvoid throw_eof_error(Cursor& c, std::string_view during);
	errvoid throw_specific_error(Cursor& c, std::string_view text);
	errvoid throw_not_a_name_error(Cursor& c);
	errvoid throw_variable_not_found_error(Cursor& c);
	errvoid throw_wrong_token_error(Cursor& c, std::string_view expected);
	errvoid throw_cannot_cast_error(Cursor& c, Type* from, Type* to);
	errvoid throw_cannot_implicit_cast_error(Cursor& c, Type* from, Type* to);

	void throw_il_wrong_data_flow_error();
	void throw_il_nothing_on_stack_error();
	void throw_il_remaining_stack_error();
	void throw_il_wrong_arguments_error();
	void throw_il_wrong_type_error();
}
#endif