#include "Error.h"
#include <iostream>
#include "Type.h"
#include "Source.h"
#include <algorithm>

namespace Corrosive {
	void throw_error_header(const Cursor& c) {
		std::cerr << "\n | Error (" << (c.top+1) << "):\n | \t";

		Cursor cc = c;
		if (cc.tok != RecognizedToken::Eof) {
			Source* src = (Source*)cc.src;
			std::string_view data = src->data();
			size_t from = std::min(cc.offset, data.size() - 1);
			size_t to = std::min(cc.offset, data.size() - 1);

			while (from > 0 && (from == 1 || data[from - 1] != '\n') && (cc.offset - from) < 20) {
				from--;
			}

			while (to < data.size() - 1 && (to == data.size() - 2 || data[to + 1] != '\n') && (to - cc.offset) < 20) {
				to++;
			}
			int req_offset = 4;
			std::string_view line = data.substr(from, to - from);
			std::cerr << "... ";
			for (int i = 0; i < line.length(); i++) {
				if (line[i] == '\t') {
					std::cerr << "    ";
					if (i < cc.offset - from)
						req_offset += 4;
				}
				else {
					std::cerr << line[i];
					if (i < cc.offset - from)
						req_offset += 1;
				}
			}
			std::cerr << " ...\n | \t";

			for (int i = 0; i < req_offset; i++)
				std::cerr << " ";

			for (int i = 0; i < cc.buffer.length(); i++)
				std::cerr << "^";
		}

		std::cerr << "\n | \t";
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