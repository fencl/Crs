#include "Statement.h"
#include "Expression.h"
#include "Operand.h"
#include "Error.h"
#include "StackManager.h"
#include <iostream>

namespace Corrosive {

	void Statement::parse_inner_block(Cursor& c,RecognizedToken& tok, BlockTermination& termination, bool exit_returns, Cursor* err) {
		StackItem sitm;

		termination = BlockTermination::continued;
		while (tok != RecognizedToken::CloseBrace) {

			//TODO
			/*if (c.src != nullptr) {
				Source* src = (Source*)c.src;
				ILBuilder::build_debug(Compiler::current()->scope(),src->debug_id, top);
			}
			else {

				ILBuilder::build_debug(Compiler::current()->scope(), UINT16_MAX, top);
			}*/

			Statement::parse(c, tok, CompileType::compile, termination);

			if (termination != BlockTermination::continued && tok != RecognizedToken::CloseBrace) {
				throw_specific_error(c, "Instruction after the current branch has been terminated");
			}

			int d = 0;
			while (Compiler::current()->temp_stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) {
				d++;
			}

			if (d > 0)
				Compiler::current()->target()->local_stack_lifetime.pop();
			else
				Compiler::current()->target()->local_stack_lifetime.discard_push();

		}
		c.move(tok);

		auto ret = Compiler::current()->return_type();


		int d = 0;
		while (Compiler::current()->stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) { d++; }
		if (d > 0)
			Compiler::current()->target()->local_stack_lifetime.pop();
		else
			Compiler::current()->target()->local_stack_lifetime.discard_push();

		Compiler::current()->target()->local_stack_lifetime.pop();


		auto prev_scope = Compiler::current()->scope();
		Compiler::current()->pop_scope();

		if (termination == BlockTermination::continued) {
			if (!exit_returns) {
				ILBuilder::build_jmp(prev_scope, Compiler::current()->scope());
			}
			else {
				if (Compiler::current()->return_type() != Compiler::current()->types()->t_void) {
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
		Compiler::current()->stack()->pop_block();
	}

	void Statement::parse(Cursor& c,RecognizedToken& tok, CompileType cmp, BlockTermination& termination) {

		if (cmp == CompileType::eval) {
			throw_specific_error(c, "Statement is used in compiletime context. Compiler should not allow it.");
		}

		switch (tok) {
			case RecognizedToken::Symbol:
			{
				auto buf = c.buffer();
				if (buf == "return") {
					Compiler::current()->target()->local_stack_lifetime.push();// temp lifetime push
					termination = BlockTermination::terminated;
					parse_return(c,tok);
					return; 
				}else if (buf == "make") {
					parse_make(c,tok);

					Compiler::current()->target()->local_stack_lifetime.push();// temp lifetime push
					return;
				}
				else if (buf == "let") {
					parse_let(c,tok);
					return;
				}
				else if (buf == "if") {
					Compiler::current()->target()->local_stack_lifetime.push();// temp lifetime push
					parse_if(c,tok, termination);
					return;
				}
				else if (buf == "while") {
					Compiler::current()->target()->local_stack_lifetime.push();// temp lifetime push
					parse_while(c,tok, termination);
					return;
				}
				else if (buf == "for") {
					Compiler::current()->target()->local_stack_lifetime.push();// temp lifetime push
					parse_for(c,tok, termination);
					return;
				}
				else if (buf == "break") {
					if (!Compiler::current()->has_loop()) {
						throw_specific_error(c, "Break must be called from loop");
					}

					ILBuilder::build_jmp(Compiler::current()->scope(), Compiler::current()->loop_break());

					c.move(tok);
					if (tok != RecognizedToken::Semicolon) {
						throw_wrong_token_error(c, "';'");
					}
					c.move(tok);
					termination = BlockTermination::breaked;
					return;
				}
				else if (buf == "continue") {
					if (!Compiler::current()->has_loop()) {
						throw_specific_error(c, "Continue must be called from loop");
					}

					ILBuilder::build_jmp(Compiler::current()->scope(), Compiler::current()->loop_continue());
					termination = BlockTermination::breaked;
					c.move(tok);
					if (tok != RecognizedToken::Semicolon) {
						throw_wrong_token_error(c, "';'");
					}
					c.move(tok);
					return;
				}

			}break;
			case RecognizedToken::OpenBrace: {
				Compiler::current()->target()->local_stack_lifetime.push(); // temp lifetime push
				c.move(tok);
				ILBlock* block = Compiler::current()->target()->create_and_append_block();

				ILBuilder::build_yield(Compiler::current()->scope(), ILDataType::none);
				ILBuilder::build_jmp(Compiler::current()->scope(), block);

				ILBlock* continue_block = Compiler::current()->target()->create_block();

				Compiler::current()->pop_scope();
				Compiler::current()->push_scope(continue_block);
				Compiler::current()->push_scope(block);
				Compiler::current()->target()->local_stack_lifetime.push(); // block lifetime push
				Compiler::current()->stack()->push_block();
				ILBuilder::build_accept(Compiler::current()->scope(), ILDataType::none);

				parse_inner_block(c,tok, termination);
				Compiler::current()->target()->append_block(continue_block);
				return;
			}break;
		}

		//fallthrough if every other check fails, statement is plain expression

		Compiler::current()->target()->local_stack_lifetime.push();// temp lifetime push
		CompileValue ret_val;
		Expression::parse(c,tok, ret_val, cmp, false);

		if (tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}
		c.move(tok);
	}

	void Statement::parse_if(Cursor& c,RecognizedToken& tok, BlockTermination& termination) {
		c.move(tok);

		CompileValue test_value;
		Cursor err = c;

		Expression::parse(c,tok, test_value, CompileType::compile);
		Operand::cast(err, test_value, Compiler::current()->types()->t_bool, CompileType::compile, false);
		Expression::rvalue(test_value, CompileType::compile);


		if (tok != RecognizedToken::OpenBrace) {
			throw_wrong_token_error(c, "'{'");
		}
		c.move(tok);

		ILBlock* block_from = Compiler::current()->scope();
		ILBuilder::build_yield(block_from, ILDataType::none);



		ILBlock* continue_block = Compiler::current()->target()->create_block();

		Compiler::current()->pop_scope();
		Compiler::current()->push_scope(continue_block);


		ILBlock* block = Compiler::current()->target()->create_and_append_block();
		block->alias = "true";
		Compiler::current()->push_scope(block);
		Compiler::current()->target()->local_stack_lifetime.push();// block lifetime push
		Compiler::current()->stack()->push_block();
		ILBuilder::build_accept(Compiler::current()->scope(), ILDataType::none);
		BlockTermination term = BlockTermination::continued;
		Statement::parse_inner_block(c,tok, term);



		if (c.buffer() == "else") {
			c.move(tok);
			
			if (tok == RecognizedToken::OpenBrace) {
				c.move(tok);

				ILBlock* else_block = Compiler::current()->target()->create_and_append_block();
				else_block->alias = "false";
				Compiler::current()->push_scope(else_block);
				Compiler::current()->target()->local_stack_lifetime.push();// block lifetime push
				Compiler::current()->stack()->push_block();
				ILBuilder::build_accept(Compiler::current()->scope(), ILDataType::none);
				BlockTermination term2 = BlockTermination::continued;
				Statement::parse_inner_block(c,tok, term2);
				if (term== BlockTermination::terminated && term2== BlockTermination::terminated) { termination = BlockTermination::terminated; }
				else { termination = BlockTermination::continued; }

				ILBuilder::build_jmpz(block_from, else_block, block);
			}
			else if (c.buffer() == "if") {
				BlockTermination term2 = BlockTermination::continued;
				parse_if(c,tok, term2);

				if (term == BlockTermination::terminated && term2 == BlockTermination::terminated) { termination = BlockTermination::terminated; }
				else { termination = BlockTermination::continued; }

				ILBuilder::build_jmpz(block_from, continue_block, block);
			}
			else {
				termination = term;
				throw_specific_error(c,"else can be followed only by { or if");
			}
		}
		else {
			ILBuilder::build_jmpz(block_from, continue_block, block);
		}

		Compiler::current()->target()->append_block(continue_block);
	}


	void Statement::parse_while(Cursor& c, RecognizedToken& tok, BlockTermination& termination) {
		c.move(tok);

		ILBlock* test_block = Compiler::current()->target()->create_and_append_block();
		test_block->alias = "while_test_block";

		ILBuilder::build_jmp(Compiler::current()->scope(), test_block);
		Compiler::current()->switch_scope(test_block);

		CompileValue test_value;
		Cursor err = c;

		Expression::parse(c,tok, test_value, CompileType::compile);
		Operand::cast(err, test_value, Compiler::current()->types()->t_bool, CompileType::compile, false);
		Expression::rvalue(test_value, CompileType::compile);

		if (tok != RecognizedToken::OpenBrace) {
			throw_wrong_token_error(c, "'{'");
		}
		c.move(tok);

		ILBlock* continue_block = Compiler::current()->target()->create_block();
		continue_block->alias = "while_continue_block";

		Compiler::current()->push_loop_blocks(continue_block, test_block);


		ILBlock* block = Compiler::current()->target()->create_and_append_block();
		block->alias = "while_block";
		Compiler::current()->push_scope(block);
		Compiler::current()->target()->local_stack_lifetime.push();// block lifetime push
		Compiler::current()->stack()->push_block();
		bool term = false;
		Statement::parse_inner_block(c,tok, termination);
		if (termination == BlockTermination::breaked) termination = BlockTermination::continued;

		ILBuilder::build_jmpz(test_block, continue_block, block);

		Compiler::current()->pop_loop_blocks();
		Compiler::current()->switch_scope(continue_block);

		Compiler::current()->target()->append_block(continue_block);
	}


	void Statement::parse_for(Cursor& c, RecognizedToken& tok, BlockTermination& termination) {
		c.move(tok);
		if (tok != RecognizedToken::OpenParenthesis) {
			throw_wrong_token_error(c, "'('");
		}


		Compiler::current()->target()->local_stack_lifetime.push(); // temp lifetime push
		Compiler::current()->target()->local_stack_lifetime.push(); // block lifetime push
		Compiler::current()->stack()->push_block();


		Cursor cc = c;
		Cursor err = cc;
		RecognizedToken tok2;
		cc.move(tok2);
		Statement::parse(cc, tok2, CompileType::compile, termination);
		

		ILBlock* test_block = Compiler::current()->target()->create_and_append_block();
		ILBuilder::build_jmp(Compiler::current()->scope(), test_block);
		Compiler::current()->pop_scope();
		Compiler::current()->push_scope(test_block);

		CompileValue test_value;

		err = cc;
		Expression::parse(cc, tok2, test_value, CompileType::compile);
		Operand::cast(err, test_value, Compiler::current()->types()->t_bool, CompileType::compile, false);
		Expression::rvalue(test_value, CompileType::compile);

		if (tok2 != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}
		cc.move(tok2);


		c.move_matching(tok);
		c.move(tok);
		if (tok != RecognizedToken::OpenBrace) {
			throw_wrong_token_error(c, "'{'");
		}
		c.move(tok);

		ILBlock* continue_block = Compiler::current()->target()->create_block();
		continue_block->alias = "for_continue_block";

		ILBlock* increment_block = Compiler::current()->target()->create_and_append_block();
		increment_block->alias = "for_increment";
		Compiler::current()->push_scope(increment_block);

		Expression::parse(cc, tok2, test_value, CompileType::compile, false);
		ILBuilder::build_jmp(increment_block, test_block);

		Compiler::current()->push_loop_blocks(continue_block, increment_block);

		ILBlock* block = Compiler::current()->target()->create_and_append_block();
		block->alias = "for";
		Compiler::current()->push_scope(block);
		Compiler::current()->target()->local_stack_lifetime.push(); // block lifetime push
		Compiler::current()->stack()->push_block();
		bool term = false;
		Statement::parse_inner_block(c, tok, termination);
		if (termination == BlockTermination::breaked) termination = BlockTermination::continued;


		StackItem sitm;
		int d = 0;
		while (Compiler::current()->temp_stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) { d++;  }
		if (d > 0)
			Compiler::current()->target()->local_stack_lifetime.pop();
		else
			Compiler::current()->target()->local_stack_lifetime.discard_push();

		d = 0;
		while (Compiler::current()->stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) { d++; }
		if (d > 0)
			Compiler::current()->target()->local_stack_lifetime.pop();
		else
			Compiler::current()->target()->local_stack_lifetime.discard_push();


		Compiler::current()->pop_loop_blocks();
		Compiler::current()->pop_scope();

		ILBuilder::build_jmpz(test_block, continue_block, block);

		Compiler::current()->pop_scope();
		Compiler::current()->push_scope(continue_block);

		Compiler::current()->target()->append_block(continue_block);
	}




	void Statement::parse_return(Cursor& c, RecognizedToken& tok) {
		
		c.move(tok);
		CompileValue ret_val;
		Cursor err = c;
		Expression::parse(c,tok, ret_val, CompileType::compile);
		Type* to = Compiler::current()->return_type();
		Operand::cast(err, ret_val, to, CompileType::compile, false);
		Expression::rvalue(ret_val, CompileType::compile);

		if (Compiler::current()->return_type()->rvalue_stacked()) {
			ILBuilder::build_local(Compiler::current()->scope(), 0);
			ILBuilder::build_load(Compiler::current()->scope(), ILDataType::word);
			Expression::copy_from_rvalue(Compiler::current()->return_type(), CompileType::compile, false);
			ILBuilder::build_ret(Compiler::current()->scope(), ILDataType::none);
		}
		else {
			ILBuilder::build_ret(Compiler::current()->scope(), Compiler::current()->return_type()->rvalue());
		}


		if (tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}

		c.move(tok);
	}

	void Statement::parse_let(Cursor& c, RecognizedToken& tok) {

		c.move(tok);
		bool reference = false;
		if (tok == RecognizedToken::And) {
			reference = true;
			c.move(tok);
		}

		if (tok != RecognizedToken::Symbol) {
			throw_not_a_name_error(c);
		}
		Cursor name = c;
		c.move(tok);

		if (!reference) {
			if (tok != RecognizedToken::Equals) {
				throw_wrong_token_error(c, "'='");
			}
		}
		else {
			if (tok != RecognizedToken::Equals) {
				throw_wrong_token_error(c, "'='");
			}
		}
		
		c.move(tok);

		size_t let_holder;
		uint32_t local_id = Compiler::current()->target()->local_stack_lifetime.append_unknown(let_holder);

		Compiler::current()->target()->local_stack_lifetime.push(); // temp lifetime push

		Cursor err = c;
		CompileValue val;
		Expression::parse(c,tok, val, CompileType::compile);

		if (!reference) {			
			Expression::rvalue(val, CompileType::compile);
		}
		else {
			if (!val.lvalue) {
				throw_specific_error(err, "Cannot create reference to rvalue");
			}
		}
		Type* new_t = val.type;
		new_t->compile();

		if (new_t->context() != ILContext::both && Compiler::current()->scope_context() != new_t->context()) {
			throw_specific_error(err, "Type was not designed for this context");
		}

		if (reference) {
			new_t = new_t->generate_reference();
		}

		Compiler::current()->target()->local_stack_lifetime.resolve_unknown(let_holder,new_t->size());

		Compiler::current()->stack()->push_item(name.buffer(), new_t, local_id, StackItemTag::regular);
		/*if (new_t->has_special_constructor()) {
			ILBuilder::build_local(Compiler::current()->scope(), local_id);
			new_t->build_construct();
		}*/


		ILBuilder::build_local(Compiler::current()->scope(), local_id);
		Expression::copy_from_rvalue(new_t, CompileType::compile, false);
		
		if (tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}
		c.move(tok);
	}

	void Statement::parse_make(Cursor& c, RecognizedToken& tok) {
		c.move(tok);

		if (tok != RecognizedToken::Symbol) {
			throw_not_a_name_error(c);
		}
		Cursor name = c;
		c.move(tok);
		if (tok != RecognizedToken::Colon) {
			throw_wrong_token_error(c, "':'");
		}
		c.move(tok);
		Cursor err = c;
		CompileValue val;

		Compiler::current()->push_scope_context(ILContext::compile);

		Expression::parse(c,tok, val, CompileType::eval);
		Expression::rvalue(val, CompileType::eval);

		if (val.type != Compiler::current()->types()->t_type) {
			throw_specific_error(err, "Exprected type");
		}
		Compiler::current()->pop_scope_context();

		Type* new_t = Compiler::current()->evaluator()->pop_register_value<Type*>();
		new_t->compile();

		if (new_t->context() != ILContext::both && Compiler::current()->scope_context() != new_t->context()) {
			throw_specific_error(err, "Type was not designed for this context");
		}

		ILSize s = new_t->size();
		uint32_t local_id = Compiler::current()->target()->local_stack_lifetime.append(s);
		Compiler::current()->stack()->push_item(name.buffer(), new_t, local_id, StackItemTag::regular);

		if (tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}
		c.move(tok);
	}
}