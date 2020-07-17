#include "Statement.h"
#include "Expression.h"
#include "Operand.h"
#include "Error.h"
#include "StackManager.h"
#include <iostream>

namespace Corrosive {


	void Statement::parse_inner_block(Cursor& c, BlockTermination& termination, bool exit_returns, Cursor* err) {

		StackItem sitm;
		ILBlock* b_exit = Ctx::workspace_function()->create_block();
		b_exit->alias = "exit";
		Ctx::push_scope_exit(b_exit);


		termination = BlockTermination::no_exit;

		BlockTermination term;
		while (c.tok != RecognizedToken::CloseBrace) {

			if (c.src != nullptr) {
				Source* src = (Source*)c.src;
				ILBuilder::build_debug(Ctx::scope(),src->debug_id, c.top);
			}
			else {

				ILBuilder::build_debug(Ctx::scope(), UINT16_MAX, c.top);
			}

			Statement::parse(c, CompileType::compile, term);

			if (term == BlockTermination::terminated && c.tok != RecognizedToken::CloseBrace) {
				throw_specific_error(c, "Instruction after the current branch has been terminated");
			}

			if (term == BlockTermination::terminated) {
				termination = BlockTermination::terminated;
			}
			else if (term == BlockTermination::needs_exit) {
				termination = BlockTermination::needs_exit;
			}


			int d = 0;
			while (Ctx::temp_stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) {
				if (sitm.type->has_special_destructor()) {
					ILBuilder::build_local(Ctx::scope(), sitm.id);
					sitm.type->build_drop();
				}
				d++;
			}

			if (d>0)
				Ctx::workspace_function()->local_stack_lifetime.pop();
			else
				Ctx::workspace_function()->local_stack_lifetime.discard_push();
			
		}
		c.move();

		auto ret = Ctx::workspace_return();
		ILBuilder::build_accept(b_exit, ret->rvalue_stacked()?ILDataType::none:ret->rvalue());



		while (Ctx::stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) {
			if (sitm.type->has_special_destructor()) {

				if (termination != BlockTermination::terminated) {
					ILBuilder::build_local(Ctx::scope(), sitm.id);
					sitm.type->build_drop();
				}

				if (termination != BlockTermination::no_exit) {
					Ctx::push_scope(Ctx::scope_exit());
					ILBuilder::build_local(Ctx::scope(), sitm.id);
					sitm.type->build_drop();
					Ctx::pop_scope();
				}
			}
		}

		Ctx::workspace_function()->local_stack_lifetime.pop();


		auto prev_scope = Ctx::scope();
		Ctx::pop_scope();
		Ctx::pop_scope_exit();

		if (termination == BlockTermination::terminated) {

			Ctx::workspace_function()->append_block(b_exit);


			auto ret = Ctx::workspace_return();
			ILBuilder::build_yield(prev_scope, ret->rvalue_stacked() ? ILDataType::none : ret->rvalue());
			ILBuilder::build_jmp(prev_scope, b_exit);

			if (!exit_returns) {
				auto ret = Ctx::workspace_return();
				ILBuilder::build_yield(b_exit, ret->rvalue_stacked() ? ILDataType::none : ret->rvalue());
				ILBuilder::build_jmp(b_exit, Ctx::scope_exit());
			}
			else {
				auto ret = Ctx::workspace_return();
				ILBuilder::build_ret(b_exit, ret->rvalue_stacked() ? ILDataType::none : ret->rvalue());
			}
		}
		else if (termination == BlockTermination::needs_exit) {

			Ctx::workspace_function()->append_block(b_exit);

			ILBuilder::build_yield(Ctx::scope(), ILDataType::none);

			if (!exit_returns) {
				ILBuilder::build_jmp(prev_scope, Ctx::scope());


				auto ret = Ctx::workspace_return();
				ILBuilder::build_yield(b_exit, ret->rvalue_stacked() ? ILDataType::none : ret->rvalue());
				ILBuilder::build_jmp(b_exit, Ctx::scope_exit());
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

					ILBuilder::build_ret(prev_scope, ILDataType::none);
					ILBuilder::build_ret(b_exit, ILDataType::none);
				}
			}
		}
		else if (termination == BlockTermination::no_exit) {
			ILBuilder::build_yield(Ctx::scope(), ILDataType::none);

			if (!exit_returns) {
				ILBuilder::build_jmp(prev_scope, Ctx::scope());
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
					ILBuilder::build_ret(prev_scope, ILDataType::none);
				}
			}
		}

		
		Ctx::stack()->pop_block();
	}

	void Statement::parse(Cursor& c, CompileType cmp, BlockTermination& termination) {

		if (cmp == CompileType::eval) {
			throw_specific_error(c, "Statement is used in compiletime context. Compiler should not allow it.");
		}

		switch (c.tok) {
			case RecognizedToken::Symbol:
			{
				if (c.buffer == "return") {
					Ctx::workspace_function()->local_stack_lifetime.push();// temp lifetime push
					termination = BlockTermination::terminated;
					parse_return(c);
					return; 
				}else if (c.buffer == "make") {
					parse_make(c);

					Ctx::workspace_function()->local_stack_lifetime.push();// temp lifetime push
					return;
				}
				else if (c.buffer == "let") {
					parse_let(c);
					return;
				}
				else if (c.buffer == "if") {
					Ctx::workspace_function()->local_stack_lifetime.push();// temp lifetime push
					parse_if(c, termination);
					return;
				}
				else if (c.buffer == "while") {
					Ctx::workspace_function()->local_stack_lifetime.push();// temp lifetime push
					parse_while(c, termination);
					return;
				}
			}break;
			case RecognizedToken::OpenBrace: {
				Ctx::workspace_function()->local_stack_lifetime.push(); // temp lifetime push
				c.move();
				ILBlock* block = Ctx::workspace_function()->create_and_append_block();

				ILBuilder::build_yield(Ctx::scope(), ILDataType::none);
				ILBuilder::build_jmp(Ctx::scope(), block);

				ILBlock* continue_block = Ctx::workspace_function()->create_block();

				Ctx::pop_scope();
				Ctx::push_scope(continue_block);
				Ctx::push_scope(block);
				Ctx::workspace_function()->local_stack_lifetime.push(); // block lifetime push
				Ctx::stack()->push_block();
				ILBuilder::build_accept(Ctx::scope(), ILDataType::none);

				parse_inner_block(c, termination);
				Ctx::workspace_function()->append_block(continue_block);
				return;
			}break;
		}

		//fallthrough if every other check fails, statement is plain expression

		Ctx::workspace_function()->local_stack_lifetime.push();// temp lifetime push
		CompileValue ret_val;
		Expression::parse(c, ret_val, cmp, false);

		if (ret_val.t->rvalue() != ILDataType::none) {
			if (cmp == CompileType::compile) {

				if (ret_val.lvalue || ret_val.t->rvalue_stacked()) {
					if (ret_val.t->has_special_destructor()) {
						ret_val.t->build_drop();
					}
					else {
						ILBuilder::build_forget(Ctx::scope(), ILDataType::word);
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
						ILBuilder::eval_forget(Ctx::eval(), ILDataType::word);
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

	void Statement::parse_if(Cursor& c, BlockTermination& termination) {
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
		Ctx::workspace_function()->local_stack_lifetime.push();// block lifetime push
		Ctx::stack()->push_block();
		ILBuilder::build_accept(Ctx::scope(), ILDataType::none);
		BlockTermination term = BlockTermination::no_exit;
		Statement::parse_inner_block(c, term);



		if (c.buffer == "else") {
			c.move();
			
			if (c.tok == RecognizedToken::OpenBrace) {
				c.move();

				ILBlock* else_block = Ctx::workspace_function()->create_and_append_block();
				else_block->alias = "false";
				Ctx::push_scope(else_block);
				Ctx::workspace_function()->local_stack_lifetime.push();// block lifetime push
				Ctx::stack()->push_block();
				ILBuilder::build_accept(Ctx::scope(), ILDataType::none);
				BlockTermination term2 = BlockTermination::no_exit;
				Statement::parse_inner_block(c, term2);
				if (term== BlockTermination::no_exit && term2== BlockTermination::no_exit) { termination = BlockTermination::no_exit; }
				else if (term == BlockTermination::terminated && term2 == BlockTermination::terminated) { termination = BlockTermination::terminated; }
				else { termination = BlockTermination::needs_exit; }

				ILBuilder::build_jmpz(block_from, else_block, block);
			}
			else if (c.buffer == "if") {
				BlockTermination term2 = BlockTermination::no_exit;
				parse_if(c, term2);

				if (term == BlockTermination::no_exit && term2 == BlockTermination::no_exit) { termination = BlockTermination::no_exit; }
				else if (term == BlockTermination::terminated && term2 == BlockTermination::terminated) { termination = BlockTermination::terminated; }
				else { termination = BlockTermination::needs_exit; }

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


	void Statement::parse_while(Cursor& c, BlockTermination& termination) {
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
		Ctx::workspace_function()->local_stack_lifetime.push();// block lifetime push
		Ctx::stack()->push_block();
		ILBuilder::build_accept(block, ILDataType::none);
		bool term = false;
		Statement::parse_inner_block(c, termination);



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
		Type* to = Ctx::workspace_return();
		Operand::cast(err, ret_val, to, CompileType::compile, false);
		Expression::rvalue(ret_val, CompileType::compile);



		if (Ctx::workspace_return()->rvalue_stacked()) {
			ILBuilder::build_local(Ctx::scope(), 0);
			ILBuilder::build_load(Ctx::scope(), ILDataType::word);
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

			do_copy = c.tok != RecognizedToken::BackArrow;
		}
		else {
			if (c.tok != RecognizedToken::Equals) {
				throw_wrong_token_error(c, "'='");
			}
		}
		
		c.move();

		size_t let_holder;
		uint32_t local_id = Ctx::workspace_function()->local_stack_lifetime.append_unknown(let_holder);

		Ctx::workspace_function()->local_stack_lifetime.push(); // temp lifetime push

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
		}

		Ctx::workspace_function()->local_stack_lifetime.resolve_unknown(let_holder,new_t->size());

		Ctx::stack()->push_item(name.buffer, new_t, local_id, StackItemTag::regular);
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
		bool construct = true;
		if (c.tok == RecognizedToken::ExclamationMark) {
			construct = false;
			c.move();
		}

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
		new_t->compile();

		if (new_t->context() != ILContext::both && Ctx::scope_context() != new_t->context()) {
			throw_specific_error(err, "Type was not designed for this context");
		}


		ILSize s = new_t->size();
		uint32_t local_id = Ctx::workspace_function()->local_stack_lifetime.append(s);
		Ctx::stack()->push_item(name.buffer, new_t, local_id, StackItemTag::regular);

		if (construct && new_t->has_special_constructor()) {
			ILBuilder::build_local(Ctx::scope(), local_id);
			new_t->build_construct();
		}

		if (c.tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}
		c.move();
	}
}