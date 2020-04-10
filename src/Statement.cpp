#include "Statement.h"
#include "Expression.h"
#include "Operand.h"
#include "Error.h"

namespace Corrosive {


	bool Statement::parse_inner_block(Cursor& c, CompileContext& ctx, CompileValue& res, CompileType cmp, bool& terminated) {
		res.t = ctx.default_types->t_void;
		res.lvalue = false;

		terminated = false;
		while (c.tok != RecognizedToken::CloseBrace) {
			if (!parse(c, ctx, res, cmp,terminated)) return false;
			if (terminated && c.tok != RecognizedToken::CloseBrace) {
				throw_specific_error(c, "Instruction after the current branch has been terminated");
				return false;
			}
		}
		c.move();
		return true;
	}

	bool Statement::parse(Cursor& c, CompileContext& ctx, CompileValue& res, CompileType cmp, bool& terminated) {
		if (cmp == CompileType::eval) {
			throw_specific_error(c, "Statement is used in compiletime context. Compiler should not allow it.");
			return false;
		}
		switch (c.tok) {
			case RecognizedToken::Symbol:
			{
				if (c.buffer == "return") {
					terminated = true;
					return parse_return(c, ctx, res, cmp);
				}
			}break;
			case RecognizedToken::OpenBrace: {
				c.move();
				return parse_inner_block(c, ctx, res, cmp, terminated);
			}break;
		}

		//fallthrough if every other check fails, statement is plain expression

		CompileValue ret_val;
		if (!Expression::parse(c, ctx, ret_val, cmp)) return false;
		if (cmp == CompileType::compile) {
			ILBuilder::build_forget(ctx.scope, ret_val.t->rvalue);
		}
		else {
			ILBuilder::eval_forget(ctx.eval, ret_val.t->rvalue);
		}
		if (c.tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
			return false;
		}
		c.move();

		return true;

	}


	bool Statement::parse_return(Cursor& c, CompileContext& ctx, CompileValue& res, CompileType cmp) {
		c.move();
		CompileValue ret_val;
		Cursor err = c;
		if (!Expression::parse(c, ctx, ret_val, cmp)) return false;
		if (!Operand::cast(err, ctx, ret_val, ctx.function->returns, cmp)) return false;

		res.lvalue = false;
		res.t = ctx.default_types->t_void;
		if (c.tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
			return false;
		}
		c.move();

		res = ret_val;
		return true;
	}
}