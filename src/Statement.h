#ifndef _statement_crs_h
#define _statement_crs_h

#include "Cursor.h"
#include "CompileContext.h"

namespace Corrosive {

	enum class BlockTermination {
		terminated, needs_exit, no_exit
	};

	class Statement {
	public:

		static void parse(Cursor& c, CompileType copm_type, BlockTermination& termination);
		static void parse_inner_block(Cursor& c, BlockTermination& termination, bool exit_returns = false, Cursor* err = nullptr);

		static void parse_if(Cursor& c, BlockTermination& termination);
		static void parse_while(Cursor& c, BlockTermination& termination);

		static void parse_return(Cursor& c);
		static void parse_make(Cursor& c);
		static void parse_let(Cursor& c);
	};

}

#endif