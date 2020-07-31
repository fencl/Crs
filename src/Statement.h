#ifndef _statement_crs_h
#define _statement_crs_h

#include "Cursor.h"
#include "CompileContext.h"
#include "Compiler.h"

namespace Corrosive {

	enum class BlockTermination {
		terminated, needs_exit, no_exit
	};

	class Statement {
	public:

		static void parse(Compiler& compiler, Cursor& c, RecognizedToken& tok, CompileType copm_type, BlockTermination& termination);
		static void parse_inner_block(Compiler& compiler, Cursor& c, RecognizedToken& tok, BlockTermination& termination, bool exit_returns = false, Cursor* err = nullptr);

		static void parse_if(Compiler& compiler, Cursor& c, RecognizedToken& tok, BlockTermination& termination);
		static void parse_while(Compiler& compiler, Cursor& c, RecognizedToken& tok, BlockTermination& termination);

		static void parse_return(Compiler& compiler, Cursor& c, RecognizedToken& tok);
		static void parse_make(Compiler& compiler, Cursor& c, RecognizedToken& tok);
		static void parse_let(Compiler& compiler, Cursor& c, RecognizedToken& tok);
	};

}

#endif