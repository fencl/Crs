#include "Source.h"
#include <iostream>
#include <fstream>
#include <ctype.h>
#include <algorithm>
#include <string>
#include <string_view>
#include "Utilities.h"

namespace Corrosive {



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
		Read(out, c.offset + c.data.length(), c.left + (unsigned int)c.data.length(), c.top);
	}


	Cursor Source::ReadFirst() const {
		Cursor c;
		Read(c, 0, 0, 0);
		return c;
	}


	Cursor Cursor::next() const {
		Cursor c;
		((Corrosive::Source*)src)->ReadAfter(c, *this);
		return c;
	}

	void Cursor::move() {
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
				out.src = this;
				out.offset = start;
				out.left = sleft;
				out.top = top;
				out.data = Data().substr(start, offset - start);
				out.tok = RecognizedToken::Symbol;

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

				out.src = this;
				out.offset = start;
				out.left = sleft;
				out.top = top;
				out.data = Data().substr(start, offset - start);
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
					case '@': out.tok = (RecognizedToken::At); break;
					case '[': out.tok = (RecognizedToken::OpenBracket); break;
					case ']': out.tok = (RecognizedToken::CloseBracket); break;
					case '{': out.tok = (RecognizedToken::OpenBrace); break;
					case '}': out.tok = (RecognizedToken::CloseBrace); break;
					case '(': out.tok = (RecognizedToken::OpenParenthesis); break;
					case ')': out.tok = (RecognizedToken::CloseParenthesis); break;
					case '+': out.tok = (RecognizedToken::Plus); break;
					case '-': switch (nc)
					{
						case '>': offset++; out.tok = (RecognizedToken::Arrow); break;
						default: out.tok = (RecognizedToken::Minus); break;
					}break;
					case '*': out.tok = (RecognizedToken::Star); break;
					case '/': out.tok = (RecognizedToken::Slash); break;
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
				out.data = Data().substr(start, offset - start);
				return;
			}
		}
		else {
			out.tok = (RecognizedToken::Eof);
			out.src = this;
			out.offset = offset;
			out.left = left;
			out.top = top;
		}

	}
}