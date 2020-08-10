#ifndef _cursor_crs_h
#define _cursor_crs_h
#include <string_view>
#include <memory>
#include "IL/IL.hpp"

namespace Corrosive {
	enum class RecognizedToken {
		Number, FloatNumber, DoubleNumber, UnsignedNumber, LongNumber, UnsignedLongNumber, Symbol, OpenBracket, CloseBracket, OpenBrace, CloseBrace, OpenParenthesis, CloseParenthesis, Eof, Unknown,
		Plus, Minus, Slash, Star, Semicolon, Comma, ExclamationMark, QestionMark, GreaterThan, LessThan, Dot, Colon,
		Equals, DoubleEquals, NotEquals, GreaterOrEqual, LessOrEqual, DoubleGreaterThan, DoubleLessThan, DoubleColon,
		Or, DoubleOr, And, DoubleAnd, Open, At, Xor, Backslash, BackArrow, Percent, Arrow, String,
		PlusEquals,MinusEquals,StarEquals,SlashEquals,ColonEquals
	};

	class Source;
	class Cursor {
	public:
		Cursor next(RecognizedToken& tok) const;
		void move(RecognizedToken& tok);


		Source* src = nullptr;
		std::string_view buffer() const;
		size_t offset = 0;
		size_t length = 0;
		size_t line();
		void move_matching(RecognizedToken& tok);
	};

	bool operator < (const Cursor& c1, const Cursor& c2);
}
#endif