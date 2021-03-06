#ifndef _source_crs_h
#define _source_crs_h
#include "Cursor.hpp"
#include <unordered_map> 
#include <map> 
#include <string>

namespace Crs {
	class Compiler;
	class AstRootNode;

	struct SourceRange {
		std::size_t offset;
		std::size_t length;
	};

	bool operator < (const SourceRange& l, const SourceRange& r);
	bool operator == (const SourceRange& l, const SourceRange& r);

	class Source {
	public:
		std::uint16_t debug_id = UINT16_MAX;
		void register_debug();
		std::map<SourceRange, std::size_t> lines;

		std::size_t get_line(Cursor c) const;

		std::string name;
		std::string path;

		const std::string_view data() const;
		void load(const char* file);
		void load_data(const char* data, const char* name);
		void read(Cursor& out, std::size_t offset, std::size_t x, std::size_t y) const;
		void read_after(Cursor& out, const Cursor& c) const;
		Cursor read_first();

		errvoid pair_tokens();
		void move_matching(Cursor& c) const;
		std::unique_ptr<AstRootNode> root_node;

		static errvoid require(std::string_view file, Source* base=nullptr);
		static void require_wrapper(dword_t slice);
	private:
		std::string buffer;
		std::unordered_map<std::size_t, Cursor> token_pair;
	};

}
#endif