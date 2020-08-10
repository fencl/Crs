#ifndef _constant_manager_crs_h
#define _constant_manager_crs_h
#include "Cursor.hpp"
#include "CompileContext.hpp"
#include "Type.hpp"
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace Corrosive {

	class ConstantManager {
	public:
		Compiler* compiler;
		std::pair<const std::string_view, uint32_t> register_string_literal(std::string string);
		std::pair<const std::string_view, uint32_t> register_string_literal(Cursor& c);
		uint8_t* register_generic_storage(uint8_t* ptr, size_t size, Type* of);

	private:
		std::unordered_map<std::string, uint32_t> string_literals;
		std::map<Cursor, std::pair<const std::string_view,uint32_t>> string_holders;

		std::unordered_map<std::basic_string_view<uint8_t>, size_t> generic_storage_map;
		std::vector<std::unique_ptr<uint8_t[]>> generic_storage;
	};
}

#endif