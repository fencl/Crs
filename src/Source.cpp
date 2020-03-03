#include "Source.h"
#include <iostream>
#include <fstream>
#include <ctype.h>
#include <algorithm>
#include <string>
#include <string_view>
#include "Utilities.h"

namespace Corrosive {
	std::string_view const Cursor::Data() const {
		return data;
	}

	void Cursor::Data(std::string_view d) {
		data = d;
	}


	const void* Cursor::Source() const { return src; }

	RecognizedToken const Cursor::Tok() const {
		return token;
	}

	void Cursor::Tok(RecognizedToken t) {
		token = t;
	}

	size_t Cursor::Offset() const { return offset; }
	unsigned int Cursor::Left() const { return left; }
	unsigned int Cursor::Top() const { return top; }

	void Cursor::Pos(const void* s, size_t o, unsigned int l, unsigned int t) {
		src = s;
		offset = o;
		left = l;
		top = t;
	}

	std::string_view const Source::Data() const {
		return std::string_view(data);
	}


	void Source::Load(const char* file) {

		std::ifstream in(file, std::ios::in | std::ios::binary);
		if (in)
		{
			in.seekg(0, std::ios::end);
			data.resize((const unsigned int)in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&data[0], data.size());
			in.close();
		}
	}

	void Source::ReadAfter(Cursor& out, const Cursor& c) const {
		Read(out, c.Offset() + c.Data().length(), c.Left() + (unsigned int)c.Data().length(), c.Top());
	}


	Cursor Source::ReadFirst() const {
		Cursor c;
		Read(c, 0, 0, 0);
		return c;
	}


	Cursor Cursor::Next() const {
		Cursor c;
		((Corrosive::Source*)src)->ReadAfter(c, *this);
		return c;
	}

	void Cursor::Move() {
		((Corrosive::Source*)src)->ReadAfter(*this, *this);
	}

	void Source::Read(Cursor& out, size_t offset, unsigned int left, unsigned int top) const {
		while (true) {
			while (offset < data.size() && isspace(data[offset]))
			{
				if (data[offset] == '\n')
				{
					left = 0;
					top++;
				}

				offset++;
			}

			if (offset < data.size() - 1 && data[offset] == '/' && data[offset + 1] == '*') {
				offset += 3;

				while (offset < data.size() && (data[offset] != '/' || data[offset - 1] != '*'))
				{
					if (data[offset] == '\n')
					{
						left = 0;
						top++;
					}

					offset++;
				}
				offset++;
			}
			else if (offset < data.size() - 1 && data[offset] == '/' && data[offset + 1] == '/') {
				offset += 2;

				while (offset < data.size())
				{
					if (data[offset] == '\n')
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

		if (offset < data.size())
		{
			if (isalpha(data[offset]) || data[offset] == '_')
			{
				size_t start = offset;
				unsigned int sleft = left;

				while (isalnum(data[offset]) || data[offset] == '_')
				{
					offset++;
					left++;
				}

				out.Pos(this, start, sleft, top);
				out.Data(Data().substr(start, offset - start));
				out.Tok(RecognizedToken::Symbol);

				return;
			}
			else if (isdigit(data[offset]))
			{
				bool floatt = false;
				bool doublet = false;
				bool islong = false;
				bool isusg = false;

				size_t start = offset;
				unsigned int sleft = left;

				while (isdigit(data[offset]) || data[offset] == '.')
				{
					if (data[offset] == '.')
						floatt = true;

					offset++;
					left++;
				}

				if (data[offset] == 'd' && floatt) {
					doublet = true;
					offset++;
					left++;
				}

				if (data[offset] == 'u' && !floatt) {
					isusg = true;
					offset++;
					left++;
				}

				if (data[offset] == 'l' && !floatt) {
					islong = true;
					offset++;
					left++;
				}

				out.Pos(this, start, sleft, top);
				out.Data(Data().substr(start, offset - start));
				if (floatt) {
					if (doublet)
						out.Tok(RecognizedToken::DoubleNumber);
					else
						out.Tok(RecognizedToken::FloatNumber);
				}
				else if (islong) {
					if (isusg)
						out.Tok(RecognizedToken::UnsignedLongNumber);
					else
						out.Tok(RecognizedToken::LongNumber);
				}
				else {
					if (isusg)
						out.Tok(RecognizedToken::UnsignedNumber);
					else
						out.Tok(RecognizedToken::Number);
				}


				return;
			}
			else
			{
				size_t start = offset;
				unsigned int sleft = left;

				char c = data[offset++];
				char nc = '\0';
				if (offset < data.size())
				{
					nc = data[offset];
				}

				switch (c)
				{
					case '@': out.Tok(RecognizedToken::At); break;
					case '[': out.Tok(RecognizedToken::OpenBracket); break;
					case ']': out.Tok(RecognizedToken::CloseBracket); break;
					case '{': out.Tok(RecognizedToken::OpenBrace); break;
					case '}': out.Tok(RecognizedToken::CloseBrace); break;
					case '(': out.Tok(RecognizedToken::OpenParenthesis); break;
					case ')': out.Tok(RecognizedToken::CloseParenthesis); break;
					case '+': out.Tok(RecognizedToken::Plus); break;
					case '-': switch (nc)
					{
						case '>': offset++; out.Tok(RecognizedToken::Arrow); break;
						default: out.Tok(RecognizedToken::Minus); break;
					}break;
					case '*': out.Tok(RecognizedToken::Star); break;
					case '/': out.Tok(RecognizedToken::Slash); break;
					case ';': out.Tok(RecognizedToken::Semicolon); break;
					case ',': out.Tok(RecognizedToken::Comma); break;
					case '.': out.Tok(RecognizedToken::Dot); break;
					case '%': out.Tok(RecognizedToken::Percent); break;
					case '^': out.Tok(RecognizedToken::Xor); break;
					case '\\': out.Tok(RecognizedToken::Backslash); break;
					case '?': out.Tok(RecognizedToken::QestionMark); break;
					case '!': switch (nc)
						{
						case '=': offset++; out.Tok(RecognizedToken::NotEquals); break;
						default: out.Tok(RecognizedToken::ExclamationMark); break;
						} break;
					case '>': switch (nc)
						{
						case '=': offset++; out.Tok(RecognizedToken::GreaterOrEqual); break;
						default: out.Tok(RecognizedToken::GreaterThan); break;
						}break;
					case '<': switch (nc)
						{
						case '=': offset++; out.Tok(RecognizedToken::LessOrEqual); break;
						case '-': offset++; out.Tok(RecognizedToken::BackArrow); break;
						default: out.Tok(RecognizedToken::LessThan); break;
						}break;
					case ':': switch (nc)
						{
						case ':': offset++; out.Tok(RecognizedToken::DoubleColon); break;
						default: out.Tok(RecognizedToken::Colon); break;
						}break;
					case '|': switch (nc)
						{
						case '|': offset++; out.Tok(RecognizedToken::DoubleOr); break;
						default: out.Tok(RecognizedToken::Or); break;
						}break;
					case '&': switch (nc)
						{
						case '&': offset++; out.Tok(RecognizedToken::DoubleAnd); break;
						default: out.Tok(RecognizedToken::And); break;
						}break;
					case '=': switch (nc)
						{
						case '=': offset++; out.Tok(RecognizedToken::DoubleEquals); break;
						default: out.Tok(RecognizedToken::Equals); break;
						}break;

					default: out.Tok(RecognizedToken::Unknown); break;
				}


				left += (unsigned int)(offset - start);

				out.Pos(this, start, sleft, top);
				out.Data(Data().substr(start, offset - start));
				return;
			}
		}
		else {
			out.Tok(RecognizedToken::Eof);
			out.Pos(this, offset, left, top);
		}

	}
}