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

	size_t Cursor::line() {
		if (src == nullptr) return 0;

		return src->get_line(*this);
	}

	size_t Source::get_line(Cursor c) {
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

		size_t l = 0;
		size_t off = 0;
		std::string_view src = data();
		bool next = true;
		while (next) {
			size_t pos = src.find("\n", off);
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

	void Source::read_after(Cursor& out, const Cursor& c, RecognizedToken& tok) {
		read(out, c.offset + c.length,tok);
	}

	Cursor Source::read_first(RecognizedToken& tok) {
		Cursor c;
		read(c, 0, tok);
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

	Cursor Cursor::next(RecognizedToken& tok) const {
		Cursor c;
		((Corrosive::Source*)src)->read_after(c, *this, tok);
		return c;
	}

	void Cursor::move(RecognizedToken& tok) {
		((Corrosive::Source*)src)->read_after(*this, *this, tok);
	}

	void Source::read(Cursor& out, size_t offset, RecognizedToken& tok) {
		while (true) {
			while (offset < buffer.size() && isspace(buffer[offset]))
			{
				offset++;
			}

			if (offset < buffer.size() - 1 && buffer[offset] == '/' && buffer[offset + 1] == '*') {
				offset += 3;

				while (offset < buffer.size() && (buffer[offset] != '/' || buffer[offset - 1] != '*'))
				{
					offset++;
				}
				offset++;
			}
			else if (offset < buffer.size() - 1 && buffer[offset] == '/' && buffer[offset + 1] == '/') {
				offset += 2;

				while (offset < buffer.size())
				{
					if (buffer[offset] == '\n') break;
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

				while (isalnum(buffer[offset]) || buffer[offset] == '_')
				{
					offset++;
				}
				out.src = this;
				out.offset = start;
				out.length = offset - start;

				tok = RecognizedToken::Symbol;

				return;
			}
			else if (isdigit(buffer[offset]))
			{
				bool floatt = false;
				bool doublet = false;
				bool islong = false;
				bool isusg = false;

				size_t start = offset;

				while (isdigit(buffer[offset]) || buffer[offset] == '.')
				{
					if (buffer[offset] == '.')
						floatt = true;

					offset++;
				}

				if (buffer[offset] == 'd' && floatt) {
					doublet = true;
					offset++;
				}

				if (buffer[offset] == 'u' && !floatt) {
					isusg = true;
					offset++;
				}

				if (buffer[offset] == 'l' && !floatt) {
					islong = true;
					offset++;
				}

				out.src = this;
				out.offset = start;
				out.length = offset - start;
				
				if (floatt) {
					if (doublet)
						tok = (RecognizedToken::DoubleNumber);
					else
						tok = (RecognizedToken::FloatNumber);
				}
				else if (islong) {
					if (isusg)
						tok = (RecognizedToken::UnsignedLongNumber);
					else
						tok = (RecognizedToken::LongNumber);
				}
				else {
					if (isusg)
						tok = (RecognizedToken::UnsignedNumber);
					else
						tok = (RecognizedToken::Number);
				}


				return;
			} else if (buffer[offset] == '"') {
				size_t start = offset;

				bool escaped = false;
				while (true) {
					offset++;
					char boff = buffer[offset];

					if (offset >= buffer.size() || boff == '\n') {
						out.src = this;
						out.offset = start;
						out.length = offset - start;
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


				tok = RecognizedToken::String;
				out.src = this;
				out.offset = start;
				out.length = offset - start;
			}
			else
			{
				size_t start = offset;

				char c = buffer[offset++];
				char nc = '\0';
				if (offset < buffer.size())
				{
					nc = buffer[offset];
				}

				switch (c)
				{
					case '@': tok = (RecognizedToken::At); break;
					case '[': tok = (RecognizedToken::OpenBracket); break;
					case ']': tok = (RecognizedToken::CloseBracket); break;
					case '{': tok = (RecognizedToken::OpenBrace); break;
					case '}': tok = (RecognizedToken::CloseBrace); break;
					case '(': tok = (RecognizedToken::OpenParenthesis); break;
					case ')': tok = (RecognizedToken::CloseParenthesis); break;
					case '+': switch (nc)
					{
						case '=': offset++; tok = (RecognizedToken::PlusEquals); break;
						default: tok = (RecognizedToken::Plus); break;
					} break;
					case '-': switch (nc)
					{
						case '=': offset++; tok = (RecognizedToken::MinusEquals); break;
						case '>': offset++; tok = (RecognizedToken::Arrow); break;
						default: tok = (RecognizedToken::Minus); break;
					}break;
					case '*': switch (nc)
					{
						case '=': offset++; tok = (RecognizedToken::StarEquals); break;
						default: tok = (RecognizedToken::Star); break;
					} break;
					case '/': switch (nc)
					{
						case '=': offset++; tok = (RecognizedToken::SlashEquals); break;
						default: tok = (RecognizedToken::Slash); break;
					} break;
					case ';': tok = (RecognizedToken::Semicolon); break;
					case ',': tok = (RecognizedToken::Comma); break;
					case '.': tok = (RecognizedToken::Dot); break;
					case '%': tok = (RecognizedToken::Percent); break;
					case '^': tok = (RecognizedToken::Xor); break;
					case '\\': tok = (RecognizedToken::Backslash); break;
					case '?': tok = (RecognizedToken::QestionMark); break;
					case '!': switch (nc)
						{
						case '=': offset++; tok = (RecognizedToken::NotEquals); break;
						default: tok = (RecognizedToken::ExclamationMark); break;
						} break;
					case '>': switch (nc)
						{
						case '=': offset++; tok = (RecognizedToken::GreaterOrEqual); break;
						default: tok = (RecognizedToken::GreaterThan); break;
						}break;
					case '<': switch (nc)
						{
						case '=': offset++; tok = (RecognizedToken::LessOrEqual); break;
						case '-': offset++; tok = (RecognizedToken::BackArrow); break;
						default: tok = (RecognizedToken::LessThan); break;
						}break;
					case ':': switch (nc)
						{
						case ':': offset++; tok = (RecognizedToken::DoubleColon); break;
						case '=': offset++; tok = (RecognizedToken::ColonEquals); break;
						default: tok = (RecognizedToken::Colon); break;
						}break;
					case '|': switch (nc)
						{
						case '|': offset++; tok = (RecognizedToken::DoubleOr); break;
						default: tok = (RecognizedToken::Or); break;
						}break;
					case '&': switch (nc)
						{
						case '&': offset++; tok = (RecognizedToken::DoubleAnd); break;
						default: tok = (RecognizedToken::And); break;
						}break;
					case '=': switch (nc)
						{
						case '=': offset++; tok = (RecognizedToken::DoubleEquals); break;
						default: tok = (RecognizedToken::Equals); break;
						}break;

					default: tok = (RecognizedToken::Unknown); break;
				}



				out.src = this;
				out.offset = start;
				out.length = offset - start;
				return;
			}
		}
		else {
			tok = RecognizedToken::Eof;
			out.src = this;
			out.offset = offset+1;
			out.length = 0;
		}

	}

	void Cursor::move_matching(RecognizedToken& tok) {
		if (src != nullptr && (tok == RecognizedToken::OpenBrace || tok == RecognizedToken::OpenParenthesis)) {
			src->move_matching(*this);
			if (tok == RecognizedToken::OpenBrace) tok = RecognizedToken::CloseBrace;
			if (tok == RecognizedToken::OpenParenthesis) tok = RecognizedToken::CloseParenthesis;
		}
	}

	void Source::move_matching(Cursor& c) const {
		c = token_pair.find(c.offset)->second;
	}

	void Source::pair_tokens() {
		RecognizedToken tok;
		Cursor c = read_first(tok);
		int level_braces = 0;
		int level_parenthesies = 0;
		std::vector<Cursor> open_braces;
		std::vector<Cursor> open_parenthesies;

		while (tok != RecognizedToken::Eof) {
			
			switch (tok)
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
						throw_specific_error(c, "There was no '}' to match this brace");
					}
					break;

				case RecognizedToken::CloseParenthesis:
					if (level_parenthesies > 0) {
						token_pair[open_parenthesies.back().offset] = c;
						open_parenthesies.pop_back();
						level_parenthesies--;
					}
					else {
						throw_specific_error(c, "There was no ')' to match this parenthesis");
					}
					break;
			}

			c.move(tok);
		}

		if (level_braces != 0) {
			throw_specific_error(open_braces.back(), "There was no '}' to close this block");
		}
		if (level_parenthesies != 0) {
			throw_specific_error(open_parenthesies.back(), "There was no ')' to close this block");
		}
	}

	ILBytecodeFunction* compile_build_block(Cursor& c) {
		Compiler* compiler = Compiler::current();
		compiler->types()->t_build_script->compile();

		auto func = compiler->global_module()->create_function(ILContext::compile);
		func->decl_id = compiler->types()->t_build_script->il_function_decl;
		func->alias = "build_script";

		ILBlock* b = func->create_and_append_block();
		b->alias = "entry";

		auto scope = ScopeState().function(func, compiler->types()->t_void).context(ILContext::compile).stack().compiler_stack();

		Statement::parse_inner_block_start(b);
		RecognizedToken tok;
		Cursor name = c;
		BlockTermination term;
		c.move(tok);
		Statement::parse_inner_block(c, tok, term, true, &name);
		func->assert_flow();
		return func;
	}

	void Source::require(std::filesystem::path file, Source* src) 
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
			new_src->pair_tokens();
			auto ptr = new_src.get();
			compiler->included_sources[std::move(abs)] = std::move(new_src);
			ptr->root_node = AstRootNode::parse(ptr);
			ptr->root_node->populate();

			compiler->source_stack.push_back(ptr);

			for (auto&& r : ptr->root_node->compile) {
				RecognizedToken tok;
				auto scope = ScopeState().context(ILContext::compile).compiler_stack().function(nullptr,nullptr);

				Cursor c = load_cursor(r, ptr,tok);

				BlockTermination termination;
				Statement::parse(c, tok, termination, ForceCompile::single);
				//auto fn = compile_build_block(c);
				//ILBuilder::eval_fncall(compiler->evaluator(), fn);
			}

			compiler->source_stack.pop_back();
		}
	}

	void Source::require_wrapper(dword_t slice)
	{
		std::basic_string_view<char> data_string((char*)slice.p1, (size_t)slice.p2);
		Source::require(data_string, Compiler::current()->source());
	}

}