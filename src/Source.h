#pragma once
#ifndef _source_crs_h
#define _source_crs_h
#include "Cursor.h"
#include <unordered_map> 


namespace Corrosive {
	class Source {
	public:
		uint16_t debug_id = UINT16_MAX;
		void register_debug();

		std::string name;

		std::string_view const data() const;
		void load(const char* file);
		void load_data(const char* data, const char* name);
		void read(Cursor& out, size_t offset, unsigned int left, unsigned int top) const;
		void read_after(Cursor& out, const Cursor& c) const;
		Cursor read_first() const;

		void pair_braces();
		void move_matching(Cursor& c) const;
	private:
		std::string buffer;
		std::unordered_map<size_t, Cursor> brace_pair;

	};

}
#endif