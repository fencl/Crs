#ifndef _compile_context_crs_h
#define _compile_context_crs_h
#include <variant>
#include "Type.hpp"
#include <vector>
#include <memory>

namespace Corrosive {

	enum class CompileType {
		compile, eval
	};

	struct CompileValue {
		Type* type = nullptr;
		bool lvalue = false;
		bool reflock = false;
	};

}

#endif