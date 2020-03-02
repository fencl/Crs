#include "Operand.h"
#include "Error.h"
#include "svtoi.h"
#include "PredefinedTypes.h"
#include "Expression.h"

#include <iostream>

namespace Corrosive {
	CompileValue Operand::Parse(Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		CompileValue ret;
		ret.lvalue = false;
		ret.t = nullptr;
		ret.v = nullptr;

		if (c.Tok() == RecognizedToken::OpenParenthesis) {
			c.Move();
			ret = Expression::Parse(c, ctx, cpt);
			if (c.Tok() != RecognizedToken::CloseParenthesis) {
				ThrowWrongTokenError(c, "')'");
			}
			c.Move();
		}
		else if (c.Tok() == RecognizedToken::Symbol) {
			if (c.Data() == "true") {
				c.Move();

				ret.lvalue = false;
				ret.t = t_bool;
				ret.v = LLVMConstInt(LLVMInt1Type(), true, false);
			}
			else if (c.Data() == "false") {
				c.Move();

				ret.lvalue = false;
				ret.t = t_bool;
				ret.v = LLVMConstInt(LLVMInt1Type(), false, false);
			}
			else {
				Cursor pack;
				Cursor name = c;
				c.Move();
				if (c.Tok() == RecognizedToken::DoubleColon) {
					c.Move();
					if (c.Tok() != RecognizedToken::Symbol) {
						ThrowNotANameError(c);
					}
					pack = name;
					name = c;
					c.Move();
				}
			}
		}
		else if (c.Tok() == RecognizedToken::Number || c.Tok() == RecognizedToken::UnsignedNumber) {
			bool usg = c.Tok() == RecognizedToken::UnsignedNumber;

			std::string_view ndata;
			if (usg)
				ndata = c.Data().substr(0, c.Data().size() - 1);
			else
				ndata = c.Data();

			unsigned long long d = svtoi(ndata);
			c.Move();

			if (cpt != CompileType::ShortCircuit)
				ret.v = LLVMConstInt(LLVMInt32Type(),d,!usg);
			
			ret.t = usg?Corrosive::t_u32: Corrosive::t_i32;
			ret.lvalue = false;
		}
		else if (c.Tok() == RecognizedToken::LongNumber || c.Tok() == RecognizedToken::UnsignedLongNumber) {
			bool usg = c.Tok() == RecognizedToken::UnsignedLongNumber;

			std::string_view ndata;
			if (usg)
				ndata = c.Data().substr(0, c.Data().size() - 2);
			else
				ndata = c.Data().substr(0, c.Data().size() - 1);

			unsigned long long d = svtoi(ndata);
			c.Move();

			if (cpt != CompileType::ShortCircuit)
				ret.v = LLVMConstInt(LLVMInt64Type(), d, !usg);
			
			ret.t = usg ? Corrosive::t_u64 : Corrosive::t_i64;
			ret.lvalue = false;
		}
		else if (c.Tok() == RecognizedToken::FloatNumber || c.Tok() == RecognizedToken::DoubleNumber) {
			bool dbl = c.Tok() == RecognizedToken::DoubleNumber;

			std::string_view ndata;
			if (dbl)
				ndata = c.Data().substr(0, c.Data().size() - 1);
			else
				ndata = c.Data();

			double d = svtod(ndata);
			c.Move();

			if (cpt != CompileType::ShortCircuit)
				ret.v = LLVMConstReal(dbl?LLVMDoubleType():LLVMFloatType(), d);
			
			ret.t = dbl?Corrosive::t_f64:Corrosive::t_f32;
			ret.lvalue = false;
		}
		else {
			ThrowSpecificError(c, "Expected to parse operand");
		}


		return ret;
	}
}