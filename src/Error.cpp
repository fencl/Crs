#include "Error.h"
#include <iostream>

namespace Corrosive {
	void ThrowErrorHeader(const Cursor& c) {
		std::cerr << "Error (" << (c.Left()+1) << ":" << (c.Top()+1) << ")\n\t";
	}

	void ThrowSpecificError(const Cursor& c, std::string_view text) {
		ThrowErrorHeader(c);
		std::cerr << text;
		ThrowErrorExit();
	}
	void ThrowEofError(const Cursor& c, std::string_view during) {
		ThrowErrorHeader(c);
		std::cerr << "End of file found during " << during;
		ThrowErrorExit();
	}
	void ThrowNotANameError(const Cursor& c) {
		ThrowErrorHeader(c);
		std::cerr << "Symbol '" << c.Data() << "' is not a valid name";
		ThrowErrorExit();
	}

	void ThrowVariableNotFound(const Cursor& c) {
		ThrowErrorHeader(c);
		std::cerr << "Variable with the name '" << c.Data() << "' was not found";
		ThrowErrorExit();
	}
	void ThrowWrongTokenError(const Cursor& c, std::string_view expected) {
		ThrowErrorHeader(c);
		std::cerr << "Token '" << c.Data() << "' found but parser expected " << expected;
		ThrowErrorExit();
	}

	void ThrowErrorExit() {
		std::cerr << std::endl;
		exit(1);
	}
}