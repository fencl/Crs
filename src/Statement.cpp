#include "Statement.h"
#include "Expression.h"
#include "Operand.h"
#include "Error.h"
#include "StackManager.h"
#include <iostream>

namespace Corrosive {


	void Statement::parse_inner_block(Cursor& c, bool& terminated, bool exit_returns, Cursor* err) {

		StackItem sitm;
		ILBlock* b_exit = Ctx::workspace_function()->create_block();
		b_exit->alias = "exit";
		Ctx::push_scope_exit(b_exit);
		std::vector<uint16_t> tmp_local_drop;

		terminated = false;
		while (c.tok != RecognizedToken::CloseBrace) {

			if (c.src != nullptr) {
				Source* src = (Source*)c.src;
				ILBuilder::build_debug(Ctx::scope(),src->debug_id, c.top);
			}
			else {

				ILBuilder::build_debug(Ctx::scope(), UINT16_MAX, c.top);
			}

			Statement::parse(c, CompileType::compile, terminated);

			if (terminated && c.tok != RecognizedToken::CloseBrace) {
				throw_specific_error(c, "Instruction after the current branch has been terminated");
			}

			while (Ctx::temp_stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) {
				if (sitm.value.t->has_special_destructor()) {
					ILBuilder::build_local(Ctx::scope(), sitm.id);
					sitm.value.t->build_drop();
				}
				tmp_local_drop.push_back(sitm.id);
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
		for (auto tmp_local_drop_id = tmp_local_drop.begin(); tmp_local_drop_id != tmp_local_drop.end(); ++tmp_local_drop_id) {
			Ctx::workspace_function()->drop_local(*tmp_local_drop_id);
		}

		while (Ctx::stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) {
			if (sitm.value.t->has_special_destructor()) {
				ILBuilder::build_local(Ctx::scope(), sitm.id);
				sitm.value.t->build_drop();
			}
			Ctx::workspace_function()->drop_local(sitm.id);
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
					if (err != nullptr) {
						throw_specific_error(*err, "Function does not always return value");
					}
					else {
						throw std::exception("Compiler error, Function does not always return value, error cursor not provided");
					}
				}
				else {
					ILBuilder::build_ret(b_exit, ILDataType::none);
				}
			}
		}

		
		Ctx::stack()->pop_block();
	}

	void Statement::parse(Cursor& c, CompileType cmp, bool& terminated) {

		if (cmp == CompileType::eval) {
			throw_specific_error(c, "Statement is used in compiletime context. Compiler should not allow it.");
		}

		switch (c.tok) {
			case RecognizedToken::Symbol:
			{
				if (c.buffer == "return") {
					terminated = true;
					parse_return(c);
					return; 
				}else if (c.buffer == "make") {
					parse_make(c);
					return;
				}
				else if (c.buffer == "let") {
					parse_let(c);
					return;
				}
				else if (c.buffer == "if") {
					parse_if(c, terminated);
					return;
				}
				else if (c.buffer == "while") {
					parse_while(c, terminated);
					return;
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

				parse_inner_block(c, terminated);
				Ctx::workspace_function()->append_block(continue_block);
				return;
			}break;
		}

		//fallthrough if every other check fails, statement is plain expression

		CompileValue ret_val;
		Expression::parse(c, ret_val, cmp, false);

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
		}
		c.move();
	}

	void Statement::parse_if(Cursor& c, bool& terminated) {
		c.move();

		CompileValue test_value;
		Cursor err = c;

		Expression::parse(c, test_value, CompileType::compile);
		Operand::cast(err, test_value, Ctx::types()->t_bool, CompileType::compile, false);
		Expression::rvalue(test_value, CompileType::compile);


		if (c.tok != RecognizedToken::OpenBrace) {
			throw_wrong_token_error(c, "'{'");
		}
		c.move();

		ILBlock* block_from = Ctx::scope();
		ILBuilder::build_yield(block_from, ILDataType::none);



		ILBlock* continue_block = Ctx::workspace_function()->create_block();

		Ctx::pop_scope();
		Ctx::push_scope(continue_block);


		ILBlock* block = Ctx::workspace_function()->create_and_append_block();
		block->alias = "true";
		Ctx::push_scope(block);
		Ctx::stack()->push_block();
		ILBuilder::build_accept(Ctx::scope(), ILDataType::none);
		bool term = false;
		Statement::parse_inner_block(c, term);



		if (c.buffer == "else") {
			c.move();
			
			if (c.tok == RecognizedToken::OpenBrace) {
				c.move();

				ILBlock* else_block = Ctx::workspace_function()->create_and_append_block();
				else_block->alias = "false";
				Ctx::push_scope(else_block);
				Ctx::stack()->push_block();
				ILBuilder::build_accept(Ctx::scope(), ILDataType::none);
				bool term2 = false;
				Statement::parse_inner_block(c, term2);
				if (term && term2) { terminated = true; }

				ILBuilder::build_jmpz(block_from, else_block, block);
			}
			else if (c.buffer == "if") {
				bool term2 = false;
				parse_if(c, term2);
				if (term && term2) { terminated = true; }

				ILBuilder::build_jmpz(block_from, continue_block, block);
			}
			else {
				throw_specific_error(c,"else can be followed only by { or if");
			}
		}
		else {
			ILBuilder::build_jmpz(block_from, continue_block, block);
		}

		Ctx::workspace_function()->append_block(continue_block);
	}


	void Statement::parse_while(Cursor& c, bool& terminated) {
		c.move();


		ILBlock* test_block = Ctx::workspace_function()->create_and_append_block();
		ILBuilder::build_yield(Ctx::scope(), ILDataType::none);
		ILBuilder::build_jmp(Ctx::scope(), test_block);
		Ctx::pop_scope();
		Ctx::push_scope(test_block);
		ILBuilder::build_accept(Ctx::scope(), ILDataType::none);

		CompileValue test_value;
		Cursor err = c;

		Expression::parse(c, test_value, CompileType::compile);
		Operand::cast(err, test_value, Ctx::types()->t_bool, CompileType::compile, false);
		Expression::rvalue(test_value, CompileType::compile);


		if (c.tok != RecognizedToken::OpenBrace) {
			throw_wrong_token_error(c, "'{'");
		}
		c.move();

		ILBuilder::build_yield(test_block, ILDataType::none);
		ILBlock* continue_block = Ctx::workspace_function()->create_block();



		ILBlock* block = Ctx::workspace_function()->create_and_append_block();
		block->alias = "while";
		Ctx::push_scope(block);
		Ctx::stack()->push_block();
		ILBuilder::build_accept(block, ILDataType::none);
		bool term = false;
		Statement::parse_inner_block(c, term);



		ILBuilder::build_jmpz(test_block, continue_block, block);

		ILBuilder::build_accept(continue_block, ILDataType::none);
		

		Ctx::pop_scope();
		Ctx::push_scope(continue_block);

		Ctx::workspace_function()->append_block(continue_block);
	}




	void Statement::parse_return(Cursor& c) {
		
		c.move();
		CompileValue ret_val;
		Cursor err = c;
		Expression::parse(c, ret_val, CompileType::compile);
		Expression::rvalue(ret_val, CompileType::compile);

		Type* to = Ctx::workspace_return();
		Operand::cast(err, ret_val, to, CompileType::compile, false);


		if (Ctx::workspace_return()->rvalue_stacked()) {
			ILBuilder::build_local(Ctx::scope(), 0);
			ILBuilder::build_load(Ctx::scope(), ILDataType::ptr);
			Expression::move_from_rvalue(Ctx::workspace_return(), CompileType::compile, false);
		}


		if (c.tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}

		c.move();
	}

	void Statement::parse_let(Cursor& c) {

		c.move();
		bool reference = false;
		if (c.tok == RecognizedToken::And) {
			reference = true;
			c.move();
		}

		if (c.tok != RecognizedToken::Symbol) {
			throw_not_a_name_error(c);
		}
		Cursor name = c;
		c.move();

		bool do_copy = true;

		if (!reference) {
			if (c.tok != RecognizedToken::Equals && c.tok != RecognizedToken::BackArrow) {
				throw_wrong_token_error(c, "'=' or '<-'");
			}
		}
		else {
			if (c.tok != RecognizedToken::Equals) {
				throw_wrong_token_error(c, "'='");
			}
		}
		
		c.move();


		Cursor err = c;
		CompileValue val;
		Expression::parse(c, val, CompileType::compile);

		if (!reference) {
			if (!val.lvalue) 
				do_copy = false;
			
			Expression::rvalue(val, CompileType::compile);
		}
		else {
			if (!val.lvalue) {
				throw_specific_error(err, "Cannot create reference to rvalue");
			}
		}
		Type* new_t = val.t;
		new_t->compile();

		if (new_t->context() != ILContext::both && Ctx::scope_context() != new_t->context()) {
			throw_specific_error(err, "Type was not designed for this context");
		}

		if (reference) {
			new_t = new_t->generate_reference();
			val.t = new_t;
			val.lvalue = true;
		}

		uint32_t local_id = Ctx::workspace_function()->register_local(new_t->size());
		Ctx::stack()->push_item(name.buffer, val, local_id, StackItemTag::regular);
		if (new_t->has_special_constructor()) {
			ILBuilder::build_local(Ctx::scope(), local_id);
			new_t->build_construct();
		}


		ILBuilder::build_local(Ctx::scope(), local_id);

		if (do_copy) {
			Expression::copy_from_rvalue(new_t, CompileType::compile, false);
		}
		else {
			Expression::move_from_rvalue(new_t, CompileType::compile, false);
		}

		if (c.tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}
		c.move();
	}

	void Statement::parse_make(Cursor& c) {
		c.move();
		if (c.tok != RecognizedToken::Symbol) {
			throw_not_a_name_error(c);
		}
		Cursor name = c;
		c.move();
		if (c.tok != RecognizedToken::Colon) {
			throw_wrong_token_error(c, "':'");
		}
		c.move();
		Cursor err = c;
		CompileValue val;

		Ctx::push_scope_context(ILContext::compile);

		Expression::parse(c, val, CompileType::eval);
		Expression::rvalue(val, CompileType::eval);

		if (val.t != Ctx::types()->t_type) {
			throw_specific_error(err, "Exprected type");
		}
		Ctx::pop_scope_context();

		Type* new_t = Ctx::eval()->pop_register_value<Type*>();
		val.lvalue = true;
		val.t = new_t;
		new_t->compile();

		if (new_t->context() != ILContext::both && Ctx::scope_context() != new_t->context()) {
			throw_specific_error(err, "Type was not designed for this context");
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
		}
		c.move();
	}
}