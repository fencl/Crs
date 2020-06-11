#pragma once
#ifndef _error_crs_h
#define _error_crs_h
#include "Cursor.h"
#include "Type.h"

namespace Corrosive {

	void throw_eof_error(const Cursor& c, std::string_view during);
	void throw_specific_error(const Cursor& c, std::string_view text);
	void throw_not_a_name_error(const Cursor& c);
	void throw_variable_not_found_error(const Cursor& c);
	void throw_wrong_token_error(const Cursor& c, std::string_view expected);
	void throw_cannot_cast_error(const Cursor& c, Type* from, Type* to);
	void throw_cannot_implicit_cast_error(const Cursor& c, Type* from, Type* to);

	void throw_il_wrong_data_flow_error();
	void throw_il_nothing_on_stack_error();
	void throw_il_remaining_stack_error();
	void throw_il_wrong_arguments_error();
	void throw_il_wrong_type_error();
}
#endif