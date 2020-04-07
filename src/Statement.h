#ifndef _statement_crs_h
#define _statement_crs_h

#include "Cursor.h"
#include "CompileContext.h"

namespace Corrosive {

	class Statement {
	public:

		static bool parse(Cursor& c, CompileContext& ctx, CompileValue& res, CompileType copm_type);

		static bool parse_return(Cursor& c, CompileContext& ctx, CompileValue& res, CompileType copm_type);
	};

}

#endif