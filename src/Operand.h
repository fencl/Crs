#pragma once
#ifndef _operand_crs_h
#define _operand_crs_h
#include <llvm/Core.h>
#include "Cursor.h"
#include "CompileContext.h"

namespace Corrosive {

	class Operand {
	public:
		static bool parse(Cursor& c, CompileContextExt& ctx, CompileValue& res, CompileType copm_type);

	private:
		static bool parse_expression(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType copm_type);
		static bool parse_symbol(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType copm_type);
		static bool parse_number(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType copm_type);
		static bool parse_long_number(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType copm_type);
		static bool parse_float_number(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType copm_type);
		static bool parse_array_operator(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType copm_type);
		static bool parse_dot_operator(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType copm_type);
	};
}


#endif