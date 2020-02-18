#pragma once
#ifndef _cursor_crs_h
#define _cursor_crs_h
#include <string_view>
#include <memory>
namespace Corrosive {
	enum class RecognizedToken {
		Number, FloatNumber, DoubleNumber, UnsignedNumber, LongNumber, UnsignedLongNumber, Symbol, OpenBracket, CloseBracket, OpenBrace, CloseBrace, OpenParenthesis, CloseParenthesis, Eof, Unknown,
		Plus, Minus, Slash, Star, Semicolon, Comma, ExclamationMark, QestionMark, GreaterThan, LessThan, Dot, Colon,
		Equals, DoubleEquals, NotEquals, GreaterOrEqual, LessOrEqual, DoubleGreaterThan, DoubleLessThan, DoubleColon,
		Or, DoubleOr, And, DoubleAnd, Open, At, Xor, Backslash, BackArrow, Percent
	};

	class Cursor {
	public:
		std::string_view const Data() const;
		void Data(std::string_view);

		RecognizedToken const Tok() const;
		void Tok(RecognizedToken);

		void Pos(const void* src, size_t offset, unsigned int left, unsigned int top);
		size_t Offset() const;
		unsigned int Left() const;
		unsigned int Top() const;

		Cursor Next() const;
		void Move();

		const void* Source() const;
	private:
		const void* src;
		RecognizedToken token;
		std::string_view data;
		unsigned int top = 0;
		unsigned int left = 0;
		size_t offset = 0;
	};
}
#endif