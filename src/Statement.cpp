#include "Statement.h"
#include "Expression.h"
#include "Operand.h"
#include "Error.h"
#include "StackManager.h"

namespace Corrosive {


	bool Statement::parse_inner_block(Cursor& c, CompileValue& res, bool& terminated) {
		CompileContext& nctx = CompileContext::get();
		res.t = nctx.default_types->t_void;
		res.lvalue = false;

		terminated = false;
		while (c.tok != RecognizedToken::CloseBrace) {
			if (!parse(c, res, CompileType::compile,terminated)) return false;
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
					return parse_return(c, res);
				}else if (c.buffer == "make") {
					return parse_make(c, res);
				}
			}break;
			case RecognizedToken::OpenBrace: {
				c.move();
				return parse_inner_block(c, res, terminated);
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


	bool Statement::parse_return(Cursor& c, CompileValue& res) {
		CompileContext& nctx = CompileContext::get();
		c.move();
		CompileValue ret_val;
		Cursor err = c;
		if (!Expression::parse(c, ret_val, CompileType::compile)) return false;
		if (!Operand::cast(err, ret_val, nctx.function_returns, CompileType::compile,false)) return false;

		if (nctx.function_returns->rvalue_stacked()) {
			//if (cpt == CompileType::compile) {
				ILBuilder::build_local(nctx.scope, 0);
				ILBuilder::build_load(nctx.scope, ILDataType::ptr);
				nctx.function_returns->build_move();
			//}
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

	bool Statement::parse_make(Cursor& c, CompileValue& res) {
		c.move();
		if (c.tok != RecognizedToken::Symbol) {
			throw_not_a_name_error(c);
			return false;
		}
		Cursor name = c;
		c.move();
		if (c.tok != RecognizedToken::Colon) {
			throw_wrong_token_error(c, "':'");
			return false;
		}
		c.move();
		Cursor err = c;
		CompileValue val;

		CompileContext new_ctx = CompileContext::get();
		new_ctx.scope_context = ILContext::compile;
		CompileContext::push(new_ctx);
		CompileContext& nctx = CompileContext::get();

		if (!Expression::parse(c, val, CompileType::eval)) return false;
		if (val.t != nctx.default_types->t_type) {
			throw_specific_error(err, "Exprected type");
			return false;
		}
		CompileContext::pop();

		Type* new_t = nctx.eval->pop_register_value<Type*>();
		val.lvalue = true;
		val.t = new_t;

		if (new_t->context() != ILContext::both && nctx.scope_context != new_t->context()) {
			throw_specific_error(err, "Type was not designed for this context");
			return false;
		}

		uint32_t local_id = nctx.function->register_local(new_t->compile_size(nctx.eval), new_t->size(nctx.eval));
		StackItem local_stack_item = StackManager::stack_push<0>(nctx.eval, name.buffer, val, local_id);

		if (c.tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
			return false;
		}
		c.move();

		return true;
	}
}