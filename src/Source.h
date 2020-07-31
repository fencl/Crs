#pragma once
#ifndef _source_crs_h
#define _source_crs_h
#include "Cursor.h"
#include <unordered_map> 
#include <map> 
#include <string>
#include <filesystem>

namespace Corrosive {
	class Compiler;
	class AstRootNode;

	struct SourceRange {
		size_t offset;
		size_t length;
	};

	bool operator < (const SourceRange& l, const SourceRange& r);
	bool operator == (const SourceRange& l, const SourceRange& r);

	class Source {
	public:
		uint16_t debug_id = UINT16_MAX;
		void register_debug(Compiler& compiler);
		std::map<SourceRange, size_t> lines;

		size_t get_line(Cursor c);

		std::string name;
		std::filesystem::path path;

		std::string_view const data();
		void load(const char* file);
		void load_data(const char* data, const char* name);
		void read(Cursor& out, size_t offset, RecognizedToken& tok);
		void read_after(Cursor& out, const Cursor& c, RecognizedToken& tok);
		Cursor read_first(RecognizedToken& tok);

		void pair_tokens();
		void move_matching(Cursor& c) const;
		std::unique_ptr<AstRootNode> root_node;

		static void require(Compiler& compiler, std::filesystem::path file, Source* base=nullptr);
	private:
		std::string buffer;
		std::unordered_map<size_t, Cursor> token_pair;
	};

}
#endif