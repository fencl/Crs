#ifndef _statement_crs_h
#define _statement_crs_h

#include "Cursor.h"
#include "CompileContext.h"

namespace Corrosive {

	class Statement {
	public:

		static bool parse(Cursor& c, CompileValue& res, CompileType copm_type,bool& terminated);
		static bool parse_inner_block(Cursor& c, CompileValue& res, bool& terminated);

		static bool parse_return(Cursor& c, CompileValue& res);
		static bool parse_make(Cursor& c, CompileValue& res);
		static bool parse_let(Cursor& c, CompileValue& res);
	};

}

#endif