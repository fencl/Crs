#include "Statement.h"
#include "Expression.h"
#include "Operand.h"
#include "Error.h"

namespace Corrosive {



	bool Statement::parse(Cursor& c, CompileContext& ctx, CompileValue& res, CompileType cmp) {
		if (cmp == CompileType::eval) {
			throw_specific_error(c, "Statement is used in compiletime context. Compiler should not allow it.");
			return false;
		}
		switch (c.tok) {
			case RecognizedToken::Symbol:
			{
				if (c.buffer == "return") {
					return parse_return(c, ctx, res, cmp);
				}
				else {
					throw_specific_error(c,"Not implemented yet");
					return false;
				}
			}break;
			default:
			{
				throw_specific_error(c, "Expected statement");
				return false;
			}
		}

	}


	bool Statement::parse_return(Cursor& c, CompileContext& ctx, CompileValue& res, CompileType cmp) {
		c.move();
		CompileValue ret_val;
		Cursor err = c;
		if (!Expression::parse(c, ctx, ret_val, cmp)) return false;
		if (!Operand::cast(err, ctx, ret_val, ctx.function->returns, cmp)) return false;

		ILBuilder::build_yield(ctx.scope, ret_val.t->rvalue);
		ILBuilder::build_jmp(ctx.scope, ctx.scope_exit);
		res.lvalue = false;
		res.t = ctx.default_types->t_void;
		return true;
	}
}