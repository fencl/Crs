#pragma once
#ifndef _source_crs_h
#define _source_crs_h
#include "Cursor.h"
namespace Corrosive {
	class Source {
	public:
		std::string_view const Data() const;
		void Load(const char* file);
		void Read(Cursor& out, size_t offset, unsigned int left, unsigned int top) const;
		void ReadAfter(Cursor& out, const Cursor& c) const;
		Cursor ReadFirst() const;
	private:
		std::string data;
	};
}
#endif