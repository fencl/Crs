#pragma once
#ifndef _constant_manager_crs_h
#define _constant_manager_crs_h
#include "Cursor.h"
#include "CompileContext.h"
#include "Type.h"
#include <unordered_map>
#include <string>

namespace Corrosive {

	class ConstantManager {
	public:
		uint32_t register_string_literal(std::string_view string);

	private:
		std::unordered_map<std::string_view, uint32_t> string_literals;
	};
}

#endif