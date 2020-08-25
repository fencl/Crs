#include "Source.hpp"
#include <iostream>
#include <fstream>
#include <ctype.h>
#include <algorithm>
#include <string>
#include <string_view>
#include "CompileContext.hpp"
#include "Error.hpp"
#include "ConstantManager.hpp"
#include "Compiler.hpp"
#include "Ast.hpp"
#include "Statement.hpp"

namespace Corrosive {


	bool operator < (const SourceRange& l, const SourceRange& r) {
		return l.offset+l.length <= r.offset;
	}
	
	bool operator == (const SourceRange& l, const SourceRange& r) {
		return l.offset <= r.offset + r.length && r.offset <= l.offset+l.length;
	}

	std::size_t Cursor::line() {
		if (src == nullptr) return 0;

		return src->get_line(*this);
	}

	std::size_t Source::get_line(Cursor c) {
		if (c.src != this) {
			return 0;
		}

		SourceRange range;
		range.length = c.length;
		range.offset = c.offset;
		return lines[range];
	}

	void Source::register_debug() {
		if (debug_id == UINT16_MAX) {
			debug_id = Compiler::current()->evaluator()->register_debug_source(name);
		}

		std::size_t l = 0;
		std::size_t off = 0;
		std::string_view src = data();
		bool next = true;
		while (next) {
			std::size_t pos = src.find("\n", off);
			if (pos == src.npos) {
				pos = src.length()-1;
				next = false;
			}
			else {
				++pos;
			}


			SourceRange range;
			range.length = pos - off;
			range.offset = off;

			if (range.length > 0) {
				lines[range] = l;
			}

			++l;
			off = pos;
		}
	}

	std::string_view const Source::data() {
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
			throw string_exception("File not found");
		}

		name = file;
		const std::size_t last_slash_idx = name.find_last_of("\\/");
		if (std::string::npos != last_slash_idx)
		{
			name.erase(0, last_slash_idx + 1);
		}
	}

	void Source::load_data(const char* data, const char* nm) {
		buffer = data;
		name = nm;
	}

	void Source::read_after(Cursor& out, const Cursor& c) {
		read(out, c.offset + c.length,c.x,c.y);
	}

	Cursor Source::read_first() {
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

	std::string_view Cursor::buffer() const {
		return src->data().substr(offset, length);
	}

	Cursor Cursor::next() const {
		Cursor c;
		((Corrosive::Source*)src)->read_after(c, *this);
		return c;
	}

	void Cursor::move() {
		((Corrosive::Source*)src)->read_after(*this, *this);
	}

	void Source::read(Cursor& out, std::size_t offset, std::size_t x, std::size_t y) {
		while (true) {
			while (offset < buffer.size() && isspace(buffer[offset]))
			{
				if (buffer[offset] == '\n')
				{
					x = 0;
					y++;
				}

				offset++;
			}

			if (offset < buffer.size() - 1 && buffer[offset] == '/' && buffer[offset + 1] == '*') {
				offset += 3;

				while (offset < buffer.size() && (buffer[offset] != '/' || buffer[offset - 1] != '*'))
				{
					if (buffer[offset] == '\n')
					{
						x = 0;
						y++;
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
						x = 0;
						y++;
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
				std::size_t start = offset;
				std::size_t start_x = x;

				while (isalnum(buffer[offset]) || buffer[offset] == '_')
				{
					offset++;
					x++;
				}
				out.x = start_x;
				out.y = y;
				out.src = this;
				out.offset = start;
				out.length = offset - start;

				out.tok = RecognizedToken::Symbol;

				return;
			}
			else if (isdigit(buffer[offset]))
			{
				bool floatt = false;
				bool doublet = false;
				bool islong = false;
				bool isusg = false;

				std::size_t start = offset;
				std::size_t start_x = x;

				while (isdigit(buffer[offset]) || buffer[offset] == '.')
				{
					if (buffer[offset] == '.')
						floatt = true;

					offset++;
					x++;
				}

				if (buffer[offset] == 'd' && floatt) {
					doublet = true;
					offset++;
					x++;
				}

				if (buffer[offset] == 'u' && !floatt) {
					isusg = true;
					offset++;
					x++;
				}

				if (buffer[offset] == 'l' && !floatt) {
					islong = true;
					offset++;
					x++;
				}

				out.src = this;
				out.offset = start;
				out.length = offset - start;
				out.y = y;
				out.x = start_x;
				
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
				std::size_t start = offset;
				std::size_t start_x = x;

				bool escaped = false;
				while (true) {
					offset++;
					char boff = buffer[offset];

					if (offset >= buffer.size() || boff == '\n') {
						out.src = this;
						out.offset = start;
						out.length = offset - start;
						out.x = start_x;
						out.y = y;
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

				x += offset-start;

				out.tok = RecognizedToken::String;
				out.src = this;
				out.offset = start;
				out.length = offset - start;
				out.x = start_x;
				out.y = y;
			}
			else
			{
				std::size_t start = offset;
				std::size_t start_x = x;

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
					case '.': switch (nc)
					{
						case '.': offset++; out.tok = (RecognizedToken::DoubleDot); break;
						default: out.tok = (RecognizedToken::Dot); break;
					} break;
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

				x+= offset - start;

				out.src = this;
				out.offset = start;
				out.length = offset - start;
				out.x = start_x;
				out.y = y;
				return;
			}
		}
		else {
			out.tok = RecognizedToken::Eof;
			out.src = this;
			out.offset = offset+1;
			out.x = x+1;
			out.y = y;
			out.length = 0;
		}
	}

	void Cursor::move_matching() {
		if (src != nullptr && (tok == RecognizedToken::OpenBrace || tok == RecognizedToken::OpenParenthesis)) {
			src->move_matching(*this);
			if (tok == RecognizedToken::OpenBrace) tok = RecognizedToken::CloseBrace;
			if (tok == RecognizedToken::OpenParenthesis) tok = RecognizedToken::CloseParenthesis;
		}
	}

	void Source::move_matching(Cursor& c) const {
		c = token_pair.find(c.offset)->second;
	}

	errvoid Source::pair_tokens() {
		Cursor c = read_first();
		int level_braces = 0;
		int level_parenthesies = 0;
		std::vector<Cursor> open_braces;
		std::vector<Cursor> open_parenthesies;

		while (c.tok != RecognizedToken::Eof) {
			
			switch (c.tok)
			{
				case RecognizedToken::OpenBrace:
					open_braces.push_back(c);
					level_braces++;
					break;
				case RecognizedToken::OpenParenthesis:
					open_parenthesies.push_back(c);
					level_parenthesies++;
					break;
				case RecognizedToken::CloseBrace:
					if (level_braces > 0) {
						token_pair[open_braces.back().offset] = c;
						open_braces.pop_back();
						level_braces--;
					}
					else {
						return throw_specific_error(c, "There was no '}' to match this brace");
					}
					break;

				case RecognizedToken::CloseParenthesis:
					if (level_parenthesies > 0) {
						token_pair[open_parenthesies.back().offset] = c;
						open_parenthesies.pop_back();
						level_parenthesies--;
					}
					else {
						return throw_specific_error(c, "There was no ')' to match this parenthesis");
					}
					break;
			}

			c.move();
		}

		if (level_braces != 0) {
			return throw_specific_error(open_braces.back(), "There was no '}' to close this block");
		}
		if (level_parenthesies != 0) {
			return throw_specific_error(open_parenthesies.back(), "There was no ')' to close this block");
		}
		return err::ok;
	}

	ILBytecodeFunction* compile_build_block(Cursor& c) {
		Compiler* compiler = Compiler::current();
		compiler->types()->t_build_script->compile();

		auto func = compiler->global_module()->create_function(ILContext::compile);
		func->decl_id = compiler->types()->t_build_script->il_function_decl;

		ILBlock* b = func->create_and_append_block();

		auto scope = ScopeState().function(func, compiler->types()->t_void).context(ILContext::compile).stack().compiler_stack();

		Statement::parse_inner_block_start(b);
		Cursor name = c;
		BlockTermination term;
		c.move();
		Statement::parse_inner_block(c, term, true, &name);
		return func;
	}

	errvoid Source::require(std::filesystem::path file, Source* src) 
	{
		Compiler* compiler = Compiler::current();
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

		auto f = compiler->included_sources.find(abs);
		if (f == compiler->included_sources.end()) {
			auto new_src = std::make_unique<Source>();
			new_src->path = abs;
			new_src->load(abs.generic_string().c_str());
			new_src->register_debug();
			if (!new_src->pair_tokens()) return err::fail;
			auto ptr = new_src.get();
			compiler->included_sources[std::move(abs)] = std::move(new_src);
			std::unique_ptr<AstRootNode> node;
			if (!AstRootNode::parse(node,ptr)) return err::fail;
			ptr->root_node = std::move(node);
			if (!ptr->root_node->populate()) return err::fail;

			compiler->source_stack.push_back(ptr);

			for (auto&& r : ptr->root_node->compile) {
				auto scope = ScopeState().context(ILContext::compile).compiler_stack().function(nullptr,nullptr);

				Cursor c = load_cursor(r, ptr);

				BlockTermination termination;
				if (!Statement::parse(c, termination, ForceCompile::single)) return err::fail;
			}

			compiler->source_stack.pop_back();
		}

		return err::ok;
	}

	void Source::require_wrapper(dword_t slice)
	{
		std::basic_string_view<char> data_string((char*)slice.p1, (std::size_t)slice.p2);
		if (!Source::require(data_string, Compiler::current()->source())) { ILEvaluator::ex_throw(); return; }
	}

}