#include "Error.h"
#include <iostream>
#include "Type.h"
#include "Source.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>

namespace Corrosive {
	std::stringstream throw_error_header(Cursor& c) {
		std::stringstream cerr;
		if (c.src != nullptr) {
			cerr << "\n | Error ("<<((Source*)c.src)->name<<": " << (c.line() + 1) << "):\n | \t";
		}
		else {
			cerr << "\n | Error (" << (c.line() + 1) << "):\n | \t";
		}

		Cursor cc = c;
		if (cc.length > 0 && cc.src!=nullptr) {
			Source* src = (Source*)cc.src;
			std::string_view data = src->data();
			size_t from = std::min(cc.offset, data.size() - 1);
			size_t to = std::min(cc.offset, data.size() - 1);

			while (from > 0 && (from == 1 || data[from - 1] != '\n') && (cc.offset - from) < 40) {
				from--;
			}

			while (to < data.size() - 1 && (to == data.size() - 2 || data[to + 1] != '\n') && (to - cc.offset) < 40) {
				to++;
			}
			int req_offset = 4;
			std::string_view line = data.substr(from, to - from);
			cerr << "... ";
			bool remove_whitespace = true;
			for (size_t i = 0; i < line.length(); i++) {

				if (remove_whitespace) {
					if (isspace(line[i]))
						continue;
					else
						remove_whitespace = false;
				}

				if (line[i] == '\t') {
					cerr << "    ";
					if (i < cc.offset - from)
						req_offset += 4;
				}
				else {
					cerr << line[i];
					if (i < cc.offset - from)
						req_offset += 1;
				}
			}
			cerr << " ...\n | \t";

			for (int i = 0; i < req_offset; i++)
				cerr << " ";

			for (size_t i = 0; i < cc.length; i++)
				cerr << "^";
		}

		cerr << "\n | \t";
		return cerr;
	}

	void throw_specific_error(Cursor& c, std::string_view text) {

		std::stringstream cerr = throw_error_header(c);
		cerr << text;
		throw string_exception(std::move(cerr.str()));
	}


	void throw_cannot_cast_error(Cursor& c, Type* from, Type* to) {

		std::stringstream cerr = throw_error_header(c);
		cerr << "Cannot cast from '";
		from->print(cerr);
		cerr << "' to '";
		to->print(cerr);
		cerr << "'";
		throw string_exception(std::move(cerr.str()));
	}


	void throw_cannot_implicit_cast_error(Cursor& c, Type* from, Type* to) {

		std::stringstream cerr = throw_error_header(c);
		cerr << "Cannot implicitly cast from '";
		from->print(cerr);
		cerr << "' to '";
		to->print(cerr);
		cerr << "'\n |\tplease, use explicit cast(...) and be careful";
		throw string_exception(std::move(cerr.str()));
	}


	void throw_eof_error(Cursor& c, std::string_view during) {

		std::stringstream cerr = throw_error_header(c);
		cerr << "End of file found during " << during;
		throw string_exception(std::move(cerr.str()));
	}
	void throw_not_a_name_error(Cursor& c) {

		std::stringstream cerr = throw_error_header(c);
		cerr << "Symbol '" << c.buffer() << "' is not a valid name";
		throw string_exception(std::move(cerr.str()));
	}

	void throw_variable_not_found_error(Cursor& c) {

		std::stringstream cerr = throw_error_header(c);
		cerr << "Variable with the name '" << c.buffer() << "' was not found";
		throw string_exception(std::move(cerr.str()));
	}

	void throw_wrong_token_error(Cursor& c, std::string_view expected) {

		std::stringstream cerr = throw_error_header(c);
		cerr << "Token '" << c.buffer() << "' found but parser expected " << expected;
		throw string_exception(std::move(cerr.str()));
	}

}