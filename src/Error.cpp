#include "Error.h"
#include <iostream>
#include "Type.h"
#include "Source.h"
#include <algorithm>

namespace Corrosive {
	void throw_error_header(const Cursor& c) {
		std::cerr << "Error (" << (c.top+1) << "):\n\t";

		Cursor cc = c;
		Source* src = (Source*)cc.src;
		std::string_view data = src->data();
		size_t from = cc.offset;
		size_t to = cc.offset;

		while (from > 0 && data[from] != '\n' && (cc.offset-from)<20) {
			from--;
		}

		while (to < data.size()-1 && data[to] != '\n' && (to - cc.offset) < 20) {
			to++;
		}

		std::cerr <<"... "<< data.substr(from+1, to - from-2) << " ...\n\t";

		for (int i = 0; i < cc.offset - from+3; i++)
			std::cerr << " ";

		for (int i = 0; i < cc.buffer.length(); i++)
			std::cerr << "^";

		std::cerr << "\n\t";
	}

	void throw_specific_error(const Cursor& c, std::string_view text) {
		throw_error_header(c);
		std::cerr << text;
		throw_exit();
	}


	void throw_cannot_cast_error(const Cursor& c, Type from, Type to) {
		throw_error_header(c);
		std::cerr << "Cannot cast from '";
		from.print(std::cerr);
		std::cerr << "' to '";
		to.print(std::cerr);
		std::cerr << "'";

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