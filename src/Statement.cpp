#include "Statement.h"
#include "Expression.h"
#include "Operand.h"
#include "Error.h"
#include "StackManager.h"
#include <iostream>

namespace Corrosive {


	bool Statement::parse_inner_block(Cursor& c, CompileValue& res, bool& terminated, bool exit_returns) {	

		StackItem sitm;
		ILBlock* b_exit = Ctx::workspace_function()->create_block();
		b_exit->alias = "exit";
		Ctx::push_scope_exit(b_exit);

		res.t = Ctx::types()->t_void;
		res.lvalue = false;

		terminated = false;
		while (c.tok != RecognizedToken::CloseBrace) {
			if (!parse(c, res, CompileType::compile,terminated)) return false;
			if (terminated && c.tok != RecognizedToken::CloseBrace) {
				throw_specific_error(c, "Instruction after the current branch has been terminated");
				return false;
			}

			while (Ctx::temp_stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) {
				if (sitm.value.t->has_special_destructor()) {
					ILBuilder::build_local(Ctx::scope(), sitm.id);
					sitm.value.t->build_drop();
				}
			}
		}
		c.move();

		Ctx::workspace_function()->append_block(b_exit);

		if (terminated) {
			ILBuilder::build_yield(Ctx::scope(), Ctx::workspace_function()->returns);
			ILBuilder::build_jmp(Ctx::scope(), b_exit);

			ILBuilder::build_accept(Ctx::scope_exit(), Ctx::workspace_function()->returns);
		}
		else {
			ILBuilder::build_yield(Ctx::scope(), ILDataType::none);
			ILBuilder::build_jmp(Ctx::scope(), b_exit);

			ILBuilder::build_accept(Ctx::scope_exit(), ILDataType::none);
		}


		Ctx::push_scope(Ctx::scope_exit());
		while (Ctx::stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) {
			if (sitm.value.t->has_special_destructor()) {
				ILBuilder::build_local(Ctx::scope(), sitm.id);
				sitm.value.t->build_drop();
			}
		}
		Ctx::pop_scope();



		Ctx::pop_scope();
		Ctx::pop_scope_exit();

		if (terminated) {
			if (!exit_returns) {
				ILBuilder::build_yield(b_exit, Ctx::workspace_function()->returns);
				ILBuilder::build_jmp(b_exit, Ctx::scope_exit());
			}
			else {
				ILBuilder::build_ret(b_exit, Ctx::workspace_function()->returns);
			}
		}
		else {
			if (!exit_returns) {
				ILBuilder::build_yield(b_exit, ILDataType::none);
				ILBuilder::build_jmp(b_exit, Ctx::scope());
			}
			else {
				if (Ctx::workspace_return() != Ctx::types()->t_void) {
					throw_specific_error(c, "Function does not always return value");
					return false;
				}
				else {
					ILBuilder::build_ret(b_exit, ILDataType::none);
				}
			}
		}

		
		Ctx::stack()->pop_block();

		return true;
	}

	bool Statement::parse(Cursor& c, CompileValue& res, CompileType cmp, bool& terminated) {

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
				else if (c.buffer == "let") {
					return parse_let(c, res);
				}
			}break;
			case RecognizedToken::OpenBrace: {
				c.move();
				ILBlock* block = Ctx::workspace_function()->create_and_append_block();

				ILBuilder::build_yield(Ctx::scope(), ILDataType::none);
				ILBuilder::build_jmp(Ctx::scope(), block);

				ILBlock* continue_block = Ctx::workspace_function()->create_block();

				Ctx::pop_scope();
				Ctx::push_scope(continue_block);
				Ctx::push_scope(block);

				Ctx::stack()->push_block();
				ILBuilder::build_accept(Ctx::scope(), ILDataType::none);

				bool result = parse_inner_block(c, res, terminated);
				Ctx::workspace_function()->append_block(continue_block);
				return result;

			}break;
		}

		//fallthrough if every other check fails, statement is plain expression

		CompileValue ret_val;
		if (!Expression::parse(c, ret_val, cmp, false)) return false;

		if (ret_val.t->rvalue() != ILDataType::none) {
			if (cmp == CompileType::compile) {

				if (ret_val.lvalue || ret_val.t->rvalue_stacked()) {
					if (ret_val.t->has_special_destructor()) {
						ret_val.t->build_drop();
					}
					else {
						ILBuilder::build_forget(Ctx::scope(), ILDataType::ptr);
					}
				}
				else {
					ILBuilder::build_forget(Ctx::scope(), ret_val.t->rvalue());
				}
			}
			else {
				if (ret_val.lvalue || ret_val.t->rvalue_stacked()) {
					if (ret_val.t->has_special_destructor()) {
						unsigned char* me = Ctx::eval()->pop_register_value<unsigned char*>();
						ret_val.t->drop(me);
					}
					else {
						ILBuilder::eval_forget(Ctx::eval(), ILDataType::ptr);
					}
				} else {
					ILBuilder::eval_forget(Ctx::eval(), ret_val.t->rvalue());
				}
			}
		}

		if (c.tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
			return false;
		}
		c.move();

		return true;

	}


	bool Statement::parse_return(Cursor& c, CompileValue& res) {
		
		c.move();
		CompileValue ret_val;
		Cursor err = c;
		if (!Expression::parse(c, ret_val, CompileType::compile)) return false;
		Type* to = Ctx::workspace_return();
		if (!Operand::cast(err, ret_val, to, CompileType::compile,false)) return false;

		if (!Expression::rvalue(ret_val, CompileType::compile)) return false;

		if (Ctx::workspace_return()->rvalue_stacked()) {
			ILBuilder::build_local(Ctx::scope(), 0);
			ILBuilder::build_load(Ctx::scope(), ILDataType::ptr);
			if (!Expression::move_from_rvalue(Ctx::workspace_return(), CompileType::compile, false)) return false;
		}


		res.lvalue = false;
		res.t = Ctx::types()->t_void;
		if (c.tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
			return false;
		}
		c.move();

		res = ret_val;
		return true;
	}

	bool Statement::parse_let(Cursor& c, CompileValue& res) {

		c.move();
		if (c.tok != RecognizedToken::Symbol) {
			throw_not_a_name_error(c);
			return false;
		}
		Cursor name = c;
		c.move();
		if (c.tok != RecognizedToken::Equals) {
			throw_wrong_token_error(c, "'='");
			return false;
		}
		c.move();


		Cursor err = c;
		CompileValue val;
		if (!Expression::parse(c, val, CompileType::compile)) return false;
		if (!Expression::rvalue(val, CompileType::compile)) return false;

		Type* new_t = val.t;

		if (!new_t->compile()) return false;
		if (new_t->context() != ILContext::both && Ctx::scope_context() != new_t->context()) {
			throw_specific_error(err, "Type was not designed for this context");
			return false;
		}


		uint32_t local_id = Ctx::workspace_function()->register_local(new_t->size());
		Ctx::stack()->push_item(name.buffer, val, local_id, StackItemTag::regular);
		if (new_t->has_special_constructor()) {
			ILBuilder::build_local(Ctx::scope(), local_id);
			new_t->build_construct();
		}


		ILBuilder::build_local(Ctx::scope(), local_id);

		if (!Expression::copy_from_rvalue(new_t,CompileType::compile, false)) return false;

		if (c.tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
			return false;
		}
		c.move();
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

		Ctx::push_scope_context(ILContext::compile);

		if (!Expression::parse(c, val, CompileType::eval)) return false;
		if (!Expression::rvalue(val, CompileType::eval)) return false;

		if (val.t != Ctx::types()->t_type) {
			throw_specific_error(err, "Exprected type");
			return false;
		}
		Ctx::pop_scope_context();

		Type* new_t = Ctx::eval()->pop_register_value<Type*>();
		val.lvalue = true;
		val.t = new_t;

		if (!new_t->compile()) return false;

		if (new_t->context() != ILContext::both && Ctx::scope_context() != new_t->context()) {
			throw_specific_error(err, "Type was not designed for this context");
			return false;
		}


		ILSize s = new_t->size();
		uint32_t local_id = Ctx::workspace_function()->register_local(s);
		Ctx::stack()->push_item(name.buffer, val, local_id, StackItemTag::regular);

		if (new_t->has_special_constructor()) {
			ILBuilder::build_local(Ctx::scope(), local_id);
			new_t->build_construct();
		}

		if (c.tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
			return false;
		}
		c.move();
		return true;
	}
}