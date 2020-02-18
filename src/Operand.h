#pragma once
#ifndef _operand_crs_h
#define _operand_crs_h
#include <llvm/Core.h>
#include "Cursor.h"
#include "CompileContext.h"

namespace Corrosive {

	class Operand {
	public:
		static CompileValue Parse(Cursor& c, CompileContextExt& ctx, CompileType copm_type);
	};
}


#endif