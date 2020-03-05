#include "Error.h"
#include <iostream>

namespace Corrosive {
	void throw_error_header(const Cursor& c) {
		std::cerr << "Error (" << (c.left+1) << ":" << (c.top+1) << ")\n\t";
	}

	void throw_ir_wrong_data_flow_error() {
		std::cerr << "Compiler Error:\n\tWrong data flow inside compiler IR";
		throw_exit();
	}

	void throw_ir_nothing_on_stack_error() {
		std::cerr << "Compiler Error:\n\tInstruction requires more argumens than the number of arguments on the stack";
		throw_exit();
	}

	void throw_ir_wrong_type_error() {
		std::cerr << "Compiler Error:\n\tPassed broken type";
		throw_exit();
	}


	void throw_ir_remaining_stack_error() {
		std::cerr << "Compiler Error:\n\tStack is not empty after terminator instruction";
		throw_exit();
	}

	void throw_ir_wrong_arguments_error() {
		std::cerr << "Compiler Error:\n\tInstruction cannot use argument(s) on the stack";
		throw_exit();
	}

	void throw_specific_error(const Cursor& c, std::string_view text) {
		throw_error_header(c);
		std::cerr << text;
		throw_exit();
	}
	void throw_eof_error(const Cursor& c, std::string_view during) {
		throw_error_header(c);
		std::cerr << "End of file found during " << during;
		throw_exit();
	}
	void throw_not_a_name_error(const Cursor& c) {
		throw_error_header(c);
		std::cerr << "Symbol '" << c.buffer << "' is not a valid name";
		throw_exit();
	}

	void throw_variable_not_found_error(const Cursor& c) {
		throw_error_header(c);
		std::cerr << "Variable with the name '" << c.buffer << "' was not found";
		throw_exit();
	}

	void throw_wrong_token_error(const Cursor& c, std::string_view expected) {
		throw_error_header(c);
		std::cerr << "Token '" << c.buffer << "' found but parser expected " << expected;
		throw_exit();
	}

	void throw_exit() {
		std::cerr << std::endl;
		exit(1);
	}
}