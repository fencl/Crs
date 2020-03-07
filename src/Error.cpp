#include "Error.h"
#include <iostream>

namespace Corrosive {
	void throw_error_header(const Cursor& c) {
		std::cerr << "Error (" << (c.left+1) << ":" << (c.top+1) << ")\n\t";
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
	}
}