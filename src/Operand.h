#pragma once
#ifndef _operand_crs_h
#define _operand_crs_h
#include <llvm/Core.h>
#include "Cursor.h"
#include "CompileContext.h"

namespace Corrosive {

	class Operand {
	public:
		static CompileValue parse(Cursor& c, CompileContextExt& ctx, CompileType copm_type);

	private:
		static void parse_expression(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType copm_type);
		static void parse_symbol(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType copm_type);
		static void parse_number(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType copm_type);
		static void parse_long_number(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType copm_type);
		static void parse_float_number(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType copm_type);

		static void parse_array_operator(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType copm_type);
		static void parse_dot_operator(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType copm_type);
	};
}


#endif