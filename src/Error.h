#pragma once
#ifndef _error_crs_h
#define _error_crs_h
#include "Cursor.h"
namespace Corrosive {
	void throw_error_header(const Cursor& c);
	void throw_eof_error(const Cursor& c, std::string_view during);
	void throw_specific_error(const Cursor& c, std::string_view text);
	void throw_not_a_name_error(const Cursor& c);
	void throw_variable_not_found_error(const Cursor& c);
	void throw_wrong_token_error(const Cursor& c, std::string_view expected);
	void throw_exit();

	void throw_ir_wrong_data_flow_error();
	void throw_ir_nothing_on_stack_error();
	void throw_ir_remaining_stack_error();
	void throw_ir_wrong_arguments_error();
	void throw_ir_wrong_type_error();
}
#endif