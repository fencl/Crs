#ifndef _statement_crs_h
#define _statement_crs_h

#include "Cursor.h"
#include "CompileContext.h"
#include "Compiler.h"

namespace Corrosive {

	enum class BlockTermination {
		terminated, continued, breaked
	};

	class Statement {
	public:

		static void parse(Cursor& c, RecognizedToken& tok, CompileType copm_type, BlockTermination& termination);

		static void parse_inner_block_start(ILBlock* block);
		static void parse_inner_block(Cursor& c, RecognizedToken& tok, BlockTermination& termination, bool exit_returns = false, Cursor* err = nullptr);

		static void parse_if(Cursor& c, RecognizedToken& tok, BlockTermination& termination);
		static void parse_while(Cursor& c, RecognizedToken& tok, BlockTermination& termination);
		static void parse_for(Cursor& c, RecognizedToken& tok, BlockTermination& termination);

		static void parse_return(Cursor& c, RecognizedToken& tok);
		static void parse_make(Cursor& c, RecognizedToken& tok);
		static void parse_let(Cursor& c, RecognizedToken& tok);
	};

}

#endif