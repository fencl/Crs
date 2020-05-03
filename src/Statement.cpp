#include "Statement.h"
#include "Expression.h"
#include "Operand.h"
#include "Error.h"

namespace Corrosive {


	bool Statement::parse_inner_block(Cursor& c, CompileValue& res, CompileType cmp, bool& terminated) {
		CompileContext& nctx = CompileContext::get();
		res.t = nctx.default_types->t_void;
		res.lvalue = false;

		terminated = false;
		while (c.tok != RecognizedToken::CloseBrace) {
			if (!parse(c, res, cmp,terminated)) return false;
			if (terminated && c.tok != RecognizedToken::CloseBrace) {
				throw_specific_error(c, "Instruction after the current branch has been terminated");
				return false;
			}
		}
		c.move();
		return true;
	}

	bool Statement::parse(Cursor& c, CompileValue& res, CompileType cmp, bool& terminated) {

		CompileContext& nctx = CompileContext::get();
		if (cmp == CompileType::eval) {
			throw_specific_error(c, "Statement is used in compiletime context. Compiler should not allow it.");
			return false;
		}
		switch (c.tok) {
			case RecognizedToken::Symbol:
			{
				if (c.buffer == "return") {
					terminated = true;
					return parse_return(c, res, cmp);
				}
			}break;
			case RecognizedToken::OpenBrace: {
				c.move();
				return parse_inner_block(c, res, cmp, terminated);
			}break;
		}

		//fallthrough if every other check fails, statement is plain expression

		CompileValue ret_val;
		if (!Expression::parse(c, ret_val, cmp)) return false;
		if (cmp == CompileType::compile) {
			ILBuilder::build_forget(nctx.scope, ret_val.t->rvalue());
		}
		else {
			ILBuilder::eval_forget(nctx.eval, ret_val.t->rvalue());
		}
		if (c.tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
			return false;
		}
		c.move();

		return true;

	}


	bool Statement::parse_return(Cursor& c, CompileValue& res, CompileType cpt) {
		CompileContext& nctx = CompileContext::get();
		c.move();
		CompileValue ret_val;
		Cursor err = c;
		if (!Expression::parse(c, ret_val, cpt)) return false;
		if (!Operand::cast(err, ret_val, nctx.function_returns, cpt,false)) return false;

		if (nctx.function_returns->rvalue_stacked()) {
			if (cpt == CompileType::compile) {
				ILBuilder::build_local(nctx.scope, 0);
				ILBuilder::build_load(nctx.scope, ILDataType::ptr);
				nctx.function_returns->build_move();
			}
		}


		res.lvalue = false;
		res.t = nctx.default_types->t_void;
		if (c.tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
			return false;
		}
		c.move();

		res = ret_val;
		return true;
	}
}