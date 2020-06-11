#ifndef _statement_crs_h
#define _statement_crs_h

#include "Cursor.h"
#include "CompileContext.h"

namespace Corrosive {

	class Statement {
	public:

		static void parse(Cursor& c, CompileType copm_type,bool& terminated);
		static void parse_inner_block(Cursor& c, bool& terminated, bool exit_returns = false);

		static void parse_if(Cursor& c, bool& terminated);

		static void parse_return(Cursor& c);
		static void parse_make(Cursor& c);
		static void parse_let(Cursor& c);
	};

}

#endif