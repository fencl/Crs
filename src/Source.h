#pragma once
#ifndef _source_crs_h
#define _source_crs_h
#include "Cursor.h"
namespace Corrosive {
	class Source {
	public:
		std::string_view const data() const;
		void load(const char* file);
		void read(Cursor& out, size_t offset, unsigned int left, unsigned int top) const;
		void read_after(Cursor& out, const Cursor& c) const;
		Cursor read_first() const;
	private:
		std::string buffer;
	};

}
#endif