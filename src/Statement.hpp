#ifndef _statement_crs_h
#define _statement_crs_h

#include "Cursor.hpp"
#include "CompileContext.hpp"
#include "Compiler.hpp"

namespace Crs {

	enum class BlockTermination {
		terminated, continued, breaked
	};


	enum class ForceCompile {
		force, single, no, inlineblock
	};

	class Statement {
	public:

		static bool runtime(ForceCompile t) { return t == ForceCompile::no || t == ForceCompile::inlineblock; }

		static errvoid parse(Cursor& c, BlockTermination& termination, ForceCompile force_compile = ForceCompile::no);

		static errvoid parse_inner_block_start(ILBlock* block, ForceCompile force_compile = ForceCompile::no);
		static errvoid parse_inner_block(Cursor& c, BlockTermination& termination, bool exit_returns = false, Cursor* err = nullptr, ForceCompile force_compile = ForceCompile::no);

		static errvoid parse_if(Cursor& c, BlockTermination& termination, ForceCompile force_compile, bool do_next=true);
		static errvoid parse_while(Cursor& c, BlockTermination& termination, ForceCompile force_compile);
		static errvoid parse_for(Cursor& c, BlockTermination& termination, ForceCompile force_compile);

		static errvoid parse_return(Cursor& c);
		static errvoid parse_make(Cursor& c, ForceCompile force_compile);
		static errvoid parse_let(Cursor& c, ForceCompile force_compile);
	};

}

#endif