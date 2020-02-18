#include "Operand.h"
#include "Error.h"
#include "svtoi.h"
#include "PredefinedTypes.h"
#include "Expression.h"

#include <iostream>

namespace Corrosive {
	CompileValue Operand::Parse(Cursor& c, CompileContextExt& ctx, CompileType cpt) {

		if (c.Tok() == RecognizedToken::OpenParenthesis) {
			c.Move();
			CompileValue ret = Expression::Parse(c, ctx, cpt);
			if (c.Tok() != RecognizedToken::CloseParenthesis) {
				ThrowWrongTokenError(c, "')'");
			}
			c.Move();
			return ret;
		}
		else if (c.Tok() == RecognizedToken::Symbol && c.Data() == "const") {
			c.Move();
			if (c.Tok() != RecognizedToken::OpenParenthesis) {
				ThrowWrongTokenError(c, "'('");
			}
			c.Move();

			if (c.Tok() != RecognizedToken::Symbol) {
				ThrowNotANameError(c);
			}
			
			if (cpt != CompileType::ShortCircuit) {
				auto gs = dynamic_cast<GenericStructDeclaration*>(ctx.basic.parent_struct);

				auto ind = gs->Generics().find(c.Data());
				if (ind != gs->Generics().end()) {
					auto& val = (*ctx.basic.template_ctx)[ind->second];
					if (val.index() == 0) {
						unsigned int v = std::get<0>(val);

						LLVMValueRef cst = nullptr;
						if (cpt != CompileType::ShortCircuit)
							cst = LLVMConstInt(LLVMInt32Type(), v, true);
						CompileValue cv;
						cv.v = cst;
						cv.t = Corrosive::t_i32;
						cv.lvalue = false;
						return cv;
					}
					else {
						ThrowSpecificError(c, "Generic identifier points to a type, const requires it to be integer");
					}
				}
				else {
					ThrowSpecificError(c, "Identifier not found in the generic declaration");
				}
			}

			c.Move();

			if (c.Tok() != RecognizedToken::CloseParenthesis) {
				ThrowWrongTokenError(c, "')'");
			}
			c.Move();

		}
		else if (c.Tok() == RecognizedToken::Symbol) {
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
		else if (c.Tok() == RecognizedToken::Number || c.Tok() == RecognizedToken::UnsignedNumber) {
			bool usg = c.Tok() == RecognizedToken::UnsignedNumber;

			std::string_view ndata;
			if (usg)
				ndata = c.Data().substr(0, c.Data().size() - 1);
			else
				ndata = c.Data();

			unsigned long long d = svtoi(ndata);
			c.Move();

			LLVMValueRef cst = nullptr;
			if (cpt != CompileType::ShortCircuit)
				cst = LLVMConstInt(LLVMInt32Type(),d,!usg);
			CompileValue cv;
			cv.v = cst;
			cv.t = usg?Corrosive::t_u32: Corrosive::t_i32;
			cv.lvalue = false;
			return cv;
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
			LLVMValueRef cst = nullptr;
			if (cpt != CompileType::ShortCircuit)
				cst = LLVMConstInt(LLVMInt64Type(), d, !usg);
			CompileValue cv;
			cv.v = cst;
			cv.t = usg ? Corrosive::t_u64 : Corrosive::t_i64;
			cv.lvalue = false;
			return cv;
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

			LLVMValueRef cst = nullptr;
			if (cpt != CompileType::ShortCircuit)
				cst = LLVMConstReal(dbl?LLVMDoubleType():LLVMFloatType(), d);
			CompileValue cv;
			cv.v = cst;
			cv.t = dbl?Corrosive::t_f64:Corrosive::t_f32;
			cv.lvalue = false;
			return cv;
		}
		else {
			ThrowSpecificError(c, "Expected to parse operand");
		}


		CompileValue cv;
		cv.v = nullptr;
		cv.t = nullptr;
		cv.lvalue = false;
		return cv;
	}
}