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
		Or, DoubleOr, And, DoubleAnd, Open, At, Xor, Backslash, BackArrow, Percent, Arrow
	};

	class Cursor {
	public:
		Cursor next() const;
		void move();


		const void* src;
		RecognizedToken tok;
		std::string_view buffer;
		unsigned int top = 0;
		unsigned int left = 0;
		size_t offset = 0;

	private:
	};

	bool operator < (const Cursor& c1, const Cursor& c2);
}
#endif