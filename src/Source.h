#pragma once
#ifndef _source_crs_h
#define _source_crs_h
#include "Cursor.h"
#include <unordered_map> 
#include <map> 
#include <string>
#include <filesystem>

namespace Corrosive {
	class Source {
	public:
		uint16_t debug_id = UINT16_MAX;
		void register_debug();

		std::string name;
		std::filesystem::path path;

		std::string_view const data() const;
		void load(const char* file);
		void load_data(const char* data, const char* name);
		void read(Cursor& out, size_t offset, unsigned int left, unsigned int top) const;
		void read_after(Cursor& out, const Cursor& c) const;
		Cursor read_first() const;

		void pair_braces();
		void move_matching(Cursor& c) const;

		static std::map<std::filesystem::path, std::unique_ptr<Source>> included_sources;
		static void require(std::filesystem::path file, const Source* base=nullptr);
		inline static void release() { included_sources.clear(); }
	private:
		std::string buffer;
		std::unordered_map<size_t, Cursor> brace_pair;
	};

}
#endif