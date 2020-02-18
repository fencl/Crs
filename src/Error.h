#pragma once
#ifndef _error_crs_h
#define _error_crs_h
#include "Cursor.h"
namespace Corrosive {
	void ThrowErrorHeader(const Cursor& c);
	void ThrowEofError(const Cursor& c, std::string_view during);
	void ThrowSpecificError(const Cursor& c, std::string_view text);
	void ThrowNotANameError(const Cursor& c);
	void ThrowWrongTokenError(const Cursor& c, std::string_view expected);
	void ThrowErrorExit();
}
#endif