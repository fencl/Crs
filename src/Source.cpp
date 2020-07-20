#include "Source.h"
#include <iostream>
#include <fstream>
#include <ctype.h>
#include <algorithm>
#include <string>
#include <string_view>
#include "Utilities.h"
#include "CompileContext.h"
#include "Error.h"
#include "ConstantManager.h"
#include "Compiler.h"

namespace Corrosive {

	void Source::register_debug(Compiler& compiler) {
		if (debug_id == UINT16_MAX) {
			debug_id = compiler.evaluator()->register_debug_source(name);
		}
	}

	std::string_view const Source::data() const {
		return std::string_view(buffer);
	}	

	void Source::load(const char* file) {

		std::ifstream in(file, std::ios::in | std::ios::binary);
		if (in)
		{
			in.seekg(0, std::ios::end);
			buffer.resize((const unsigned int)in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&buffer[0], buffer.size());
			in.close();
		}
		else {
			throw std::exception("File not found");
		}

		name = file;
		const size_t last_slash_idx = name.find_last_of("\\/");
		if (std::string::npos != last_slash_idx)
		{
			name.erase(0, last_slash_idx + 1);
		}
	}

	void Source::load_data(const char* data, const char* nm) {
		buffer = data;
		name = nm;
	}

	void Source::read_after(Cursor& out, const Cursor& c) const {
		read(out, c.offset + c.buffer.length(), c.left + (unsigned int)c.buffer.length(), c.top);
	}

	Cursor Source::read_first() const {
		Cursor c;
		read(c, 0, 0, 0);
		return c;
	}

	bool operator < (const Cursor& c1, const Cursor& c2) {
		if (c1.src < c2.src) return true;
		if (c1.src > c2.src) return false;
		if (c1.offset < c2.offset) return true;

		return false;
	}

	Cursor Cursor::next() const {
		Cursor c;
		((Corrosive::Source*)src)->read_after(c, *this);
		return c;
	}

	void Cursor::move() {
		((Corrosive::Source*)src)->read_after(*this, *this);
	}

	void Source::read(Cursor& out, size_t offset, unsigned int left, unsigned int top) const {
		while (true) {
			while (offset < buffer.size() && isspace(buffer[offset]))
			{
				if (buffer[offset] == '\n')
				{
					left = 0;
					top++;
				}

				offset++;
			}

			if (offset < buffer.size() - 1 && buffer[offset] == '/' && buffer[offset + 1] == '*') {
				offset += 3;

				while (offset < buffer.size() && (buffer[offset] != '/' || buffer[offset - 1] != '*'))
				{
					if (buffer[offset] == '\n')
					{
						left = 0;
						top++;
					}

					offset++;
				}
				offset++;
			}
			else if (offset < buffer.size() - 1 && buffer[offset] == '/' && buffer[offset + 1] == '/') {
				offset += 2;

				while (offset < buffer.size())
				{
					if (buffer[offset] == '\n')
					{
						left = 0;
						top++;
						break;
					}

					offset++;
				}

				offset++;
			}
			else break;
		}

		if (offset < buffer.size())
		{
			if (isalpha(buffer[offset]) || buffer[offset] == '_')
			{
				size_t start = offset;
				unsigned int sleft = left;

				while (isalnum(buffer[offset]) || buffer[offset] == '_')
				{
					offset++;
					left++;
				}
				out.src = this;
				out.offset = start;
				out.left = sleft;
				out.top = top;
				out.buffer = data().substr(start, offset - start);
				out.tok = RecognizedToken::Symbol;

				return;
			}
			else if (isdigit(buffer[offset]))
			{
				bool floatt = false;
				bool doublet = false;
				bool islong = false;
				bool isusg = false;

				size_t start = offset;
				unsigned int sleft = left;

				while (isdigit(buffer[offset]) || buffer[offset] == '.')
				{
					if (buffer[offset] == '.')
						floatt = true;

					offset++;
					left++;
				}

				if (buffer[offset] == 'd' && floatt) {
					doublet = true;
					offset++;
					left++;
				}

				if (buffer[offset] == 'u' && !floatt) {
					isusg = true;
					offset++;
					left++;
				}

				if (buffer[offset] == 'l' && !floatt) {
					islong = true;
					offset++;
					left++;
				}

				out.src = this;
				out.offset = start;
				out.left = sleft;
				out.top = top;
				out.buffer = data().substr(start, offset - start);
				if (floatt) {
					if (doublet)
						out.tok = (RecognizedToken::DoubleNumber);
					else
						out.tok = (RecognizedToken::FloatNumber);
				}
				else if (islong) {
					if (isusg)
						out.tok = (RecognizedToken::UnsignedLongNumber);
					else
						out.tok = (RecognizedToken::LongNumber);
				}
				else {
					if (isusg)
						out.tok = (RecognizedToken::UnsignedNumber);
					else
						out.tok = (RecognizedToken::Number);
				}


				return;
			} else if (buffer[offset] == '"') {
				size_t start = offset;
				unsigned int sleft = left;

				bool escaped = false;
				while (true) {
					offset++;
					char boff = buffer[offset];

					if (offset >= buffer.size() || boff == '\n') {
						out.src = this;
						out.offset = start;
						out.left = sleft;
						out.top = top;
						out.buffer = data().substr(start, offset - start);
						throw_specific_error(out, "String literal not closed");
					}
					else if (boff == '"' && !escaped) {
						break;
					}

					if (boff == '\\' && !escaped) {
						escaped = true;
					}
					else {
						escaped = false;
					}

				}

				offset++;

				left += (unsigned int)(offset - start);

				out.tok = RecognizedToken::String;
				out.src = this;
				out.offset = start;
				out.left = sleft;
				out.top = top;
				out.buffer = data().substr(start, offset - start);
			}
			else
			{
				size_t start = offset;
				unsigned int sleft = left;

				char c = buffer[offset++];
				char nc = '\0';
				if (offset < buffer.size())
				{
					nc = buffer[offset];
				}

				switch (c)
				{
					case '@': out.tok = (RecognizedToken::At); break;
					case '[': out.tok = (RecognizedToken::OpenBracket); break;
					case ']': out.tok = (RecognizedToken::CloseBracket); break;
					case '{': out.tok = (RecognizedToken::OpenBrace); break;
					case '}': out.tok = (RecognizedToken::CloseBrace); break;
					case '(': out.tok = (RecognizedToken::OpenParenthesis); break;
					case ')': out.tok = (RecognizedToken::CloseParenthesis); break;
					case '+': switch (nc)
					{
						case '=': offset++; out.tok = (RecognizedToken::PlusEquals); break;
						default: out.tok = (RecognizedToken::Plus); break;
					} break;
					case '-': switch (nc)
					{
						case '=': offset++; out.tok = (RecognizedToken::MinusEquals); break;
						case '>': offset++; out.tok = (RecognizedToken::Arrow); break;
						default: out.tok = (RecognizedToken::Minus); break;
					}break;
					case '*': switch (nc)
					{
						case '=': offset++; out.tok = (RecognizedToken::StarEquals); break;
						default: out.tok = (RecognizedToken::Star); break;
					} break;
					case '/': switch (nc)
					{
						case '=': offset++; out.tok = (RecognizedToken::SlashEquals); break;
						default: out.tok = (RecognizedToken::Slash); break;
					} break;
					case ';': out.tok = (RecognizedToken::Semicolon); break;
					case ',': out.tok = (RecognizedToken::Comma); break;
					case '.': out.tok = (RecognizedToken::Dot); break;
					case '%': out.tok = (RecognizedToken::Percent); break;
					case '^': out.tok = (RecognizedToken::Xor); break;
					case '\\': out.tok = (RecognizedToken::Backslash); break;
					case '?': out.tok = (RecognizedToken::QestionMark); break;
					case '!': switch (nc)
						{
						case '=': offset++; out.tok = (RecognizedToken::NotEquals); break;
						default: out.tok = (RecognizedToken::ExclamationMark); break;
						} break;
					case '>': switch (nc)
						{
						case '=': offset++; out.tok = (RecognizedToken::GreaterOrEqual); break;
						default: out.tok = (RecognizedToken::GreaterThan); break;
						}break;
					case '<': switch (nc)
						{
						case '=': offset++; out.tok = (RecognizedToken::LessOrEqual); break;
						case '-': offset++; out.tok = (RecognizedToken::BackArrow); break;
						default: out.tok = (RecognizedToken::LessThan); break;
						}break;
					case ':': switch (nc)
						{
						case ':': offset++; out.tok = (RecognizedToken::DoubleColon); break;
						case '=': offset++; out.tok = (RecognizedToken::ColonEquals); break;
						default: out.tok = (RecognizedToken::Colon); break;
						}break;
					case '|': switch (nc)
						{
						case '|': offset++; out.tok = (RecognizedToken::DoubleOr); break;
						default: out.tok = (RecognizedToken::Or); break;
						}break;
					case '&': switch (nc)
						{
						case '&': offset++; out.tok = (RecognizedToken::DoubleAnd); break;
						default: out.tok = (RecognizedToken::And); break;
						}break;
					case '=': switch (nc)
						{
						case '=': offset++; out.tok = (RecognizedToken::DoubleEquals); break;
						default: out.tok = (RecognizedToken::Equals); break;
						}break;

					default: out.tok = (RecognizedToken::Unknown); break;
				}


				left += (unsigned int)(offset - start);

				out.src = this;
				out.offset = start;
				out.left = sleft;
				out.top = top;
				out.buffer = data().substr(start, offset - start);
				return;
			}
		}
		else {
			out.tok = (RecognizedToken::Eof);
			out.src = this;
			out.offset = offset+1;
			out.left = left+1;
			out.top = top;
			out.buffer = "<eof>";
		}

	}

	void Cursor::move_matching() {
		if (src != nullptr && tok == RecognizedToken::OpenBrace) {
			src->move_matching(*this);
		}
	}

	void Source::move_matching(Cursor& c) const {
		c = brace_pair.find(c.offset)->second;
	}

	void Source::pair_braces() {
		Cursor c = read_first();
		int level = 0;
		std::vector<Cursor> open_braces;

		while (c.tok != RecognizedToken::Eof) {
			
			if (c.tok == RecognizedToken::OpenBrace) {
				open_braces.push_back(c);
				level++;
			}
			else if (c.tok == RecognizedToken::CloseBrace) {
				if (level > 0) {
					brace_pair[open_braces.back().offset] = c;
					open_braces.pop_back();
					level--;
				}
				else {
					throw_specific_error(c, "There was no '{' to match this brace");
				}
			}

			c.move();
		}

		if (level != 0) {
			throw_specific_error(open_braces.back(), "There was no '}' to close this block");
		}
	}

	std::map<std::filesystem::path, std::unique_ptr<Source>> Source::included_sources;

	void Source::require(Compiler& compiler, std::filesystem::path file, const Source* src) 
	{
		std::filesystem::path abs;
		if (src) {
			abs = src->path.parent_path();
			abs += abs.preferred_separator;
			abs += file;
			abs = std::filesystem::absolute(abs);
		}
		else {
			abs = std::filesystem::absolute(file);
		}

		auto f = included_sources.find(abs);
		if (f == included_sources.end()) {
			auto std_src = std::make_unique<Source>();
			std_src->path = abs;
			std_src->load(abs.generic_string().c_str());
			std_src->pair_braces();
			std_src->register_debug(compiler);
			Cursor c_std = std_src->read_first();
			included_sources[std::move(abs)] = std::move(std_src);
			Declaration::parse_global(compiler,c_std, compiler.global_namespace());
		}
	}

}