#ifndef _constant_manager_crs_h
#define _constant_manager_crs_h
#include "Cursor.hpp"
#include "CompileContext.hpp"
#include "Type.hpp"
#include <unordered_map>
#include <map>
#include <string>

namespace Crs {

	struct basic_string_view_u8_hash {
		std::size_t operator() (const std::basic_string_view<std::uint8_t>& key) const;
	};

	class ConstantManager {
	public:
		Compiler* compiler;
		std::pair<const std::string_view, std::uint32_t> register_constant(std::string string, ILSize size);
		std::pair<const std::string_view, std::uint32_t> register_string_literal(Cursor& c);
		errvoid register_generic_storage(std::uint8_t*& r,std::uint8_t* ptr, std::size_t size, Type* of);

	private:
		std::unordered_map<std::string, std::uint32_t> string_literals;
		std::map<Cursor, std::pair<const std::string_view,std::uint32_t>> string_holders;
		std::unordered_map<std::basic_string_view<std::uint8_t>, std::size_t,basic_string_view_u8_hash> generic_storage_map;
		std::vector<std::unique_ptr<std::uint8_t[]>> generic_storage;
	};
}

#endif