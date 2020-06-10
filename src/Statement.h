#ifndef _statement_crs_h
#define _statement_crs_h

#include "Cursor.h"
#include "CompileContext.h"

namespace Corrosive {

	class Statement {
	public:

		static bool parse(Cursor& c, CompileType copm_type,bool& terminated);
		static bool parse_inner_block(Cursor& c, bool& terminated, bool exit_returns = false);

		static bool parse_if(Cursor& c, bool& terminated);

		static bool parse_return(Cursor& c);
		static bool parse_make(Cursor& c);
		static bool parse_let(Cursor& c);
	};

}

#endif