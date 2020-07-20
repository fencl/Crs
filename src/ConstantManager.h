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
		Compiler* compiler;
		std::pair<const std::string_view, uint32_t> register_string_literal(std::string string);
		std::pair<const std::string_view, uint32_t> register_string_literal(Cursor& c);

	private:
		std::unordered_map<std::string, uint32_t> string_literals;
		std::map<Cursor, std::pair<const std::string_view,uint32_t>> string_holders;
	};
}

#endif