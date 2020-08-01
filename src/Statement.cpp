#include "Statement.h"
#include "Expression.h"
#include "Operand.h"
#include "Error.h"
#include "StackManager.h"
#include <iostream>

namespace Corrosive {

	void Statement::parse_inner_block(Compiler& compiler, Cursor& c,RecognizedToken& tok, BlockTermination& termination, bool exit_returns, Cursor* err) {
		StackItem sitm;

		termination = BlockTermination::continued;
		while (tok != RecognizedToken::CloseBrace) {

			//TODO
			/*if (c.src != nullptr) {
				Source* src = (Source*)c.src;
				ILBuilder::build_debug(compiler.scope(),src->debug_id, top);
			}
			else {

				ILBuilder::build_debug(compiler.scope(), UINT16_MAX, top);
			}*/

			Statement::parse(compiler, c, tok, CompileType::compile, termination);

			if (termination != BlockTermination::continued && tok != RecognizedToken::CloseBrace) {
				throw_specific_error(c, "Instruction after the current branch has been terminated");
			}

			int d = 0;
			while (compiler.temp_stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) {
				d++;
			}

			if (d > 0)
				compiler.target()->local_stack_lifetime.pop();
			else
				compiler.target()->local_stack_lifetime.discard_push();

		}
		c.move(tok);

		auto ret = compiler.return_type();


		int d = 0;
		while (compiler.stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) { d++; }
		if (d > 0)
			compiler.target()->local_stack_lifetime.pop();
		else
			compiler.target()->local_stack_lifetime.discard_push();

		compiler.target()->local_stack_lifetime.pop();


		auto prev_scope = compiler.scope();
		compiler.pop_scope();

		if (termination == BlockTermination::continued) {
			if (!exit_returns) {
				ILBuilder::build_jmp(prev_scope, compiler.scope());
			}
			else {
				if (compiler.return_type() != compiler.types()->t_void) {
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
		compiler.stack()->pop_block();
	}

	void Statement::parse(Compiler& compiler, Cursor& c,RecognizedToken& tok, CompileType cmp, BlockTermination& termination) {

		if (cmp == CompileType::eval) {
			throw_specific_error(c, "Statement is used in compiletime context. Compiler should not allow it.");
		}

		switch (tok) {
			case RecognizedToken::Symbol:
			{
				auto buf = c.buffer();
				if (buf == "return") {
					compiler.target()->local_stack_lifetime.push();// temp lifetime push
					termination = BlockTermination::terminated;
					parse_return(compiler, c,tok);
					return; 
				}else if (buf == "make") {
					parse_make(compiler, c,tok);

					compiler.target()->local_stack_lifetime.push();// temp lifetime push
					return;
				}
				else if (buf == "let") {
					parse_let(compiler, c,tok);
					return;
				}
				else if (buf == "if") {
					compiler.target()->local_stack_lifetime.push();// temp lifetime push
					parse_if(compiler, c,tok, termination);
					return;
				}
				else if (buf == "while") {
					compiler.target()->local_stack_lifetime.push();// temp lifetime push
					parse_while(compiler, c,tok, termination);
					return;
				}
				else if (buf == "for") {
					compiler.target()->local_stack_lifetime.push();// temp lifetime push
					parse_for(compiler, c,tok, termination);
					return;
				}
				else if (buf == "break") {
					if (!compiler.has_loop()) {
						throw_specific_error(c, "Break must be called from loop");
					}

					ILBuilder::build_jmp(compiler.scope(), compiler.loop_break());

					c.move(tok);
					if (tok != RecognizedToken::Semicolon) {
						throw_wrong_token_error(c, "';'");
					}
					c.move(tok);
					termination = BlockTermination::breaked;
					return;
				}
				else if (buf == "continue") {
					if (!compiler.has_loop()) {
						throw_specific_error(c, "Continue must be called from loop");
					}

					ILBuilder::build_jmp(compiler.scope(), compiler.loop_continue());
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
				compiler.target()->local_stack_lifetime.push(); // temp lifetime push
				c.move(tok);
				ILBlock* block = compiler.target()->create_and_append_block();

				ILBuilder::build_yield(compiler.scope(), ILDataType::none);
				ILBuilder::build_jmp(compiler.scope(), block);

				ILBlock* continue_block = compiler.target()->create_block();

				compiler.pop_scope();
				compiler.push_scope(continue_block);
				compiler.push_scope(block);
				compiler.target()->local_stack_lifetime.push(); // block lifetime push
				compiler.stack()->push_block();
				ILBuilder::build_accept(compiler.scope(), ILDataType::none);

				parse_inner_block(compiler, c,tok, termination);
				compiler.target()->append_block(continue_block);
				return;
			}break;
		}

		//fallthrough if every other check fails, statement is plain expression

		compiler.target()->local_stack_lifetime.push();// temp lifetime push
		CompileValue ret_val;
		Expression::parse(compiler, c,tok, ret_val, cmp, false);

		if (tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}
		c.move(tok);
	}

	void Statement::parse_if(Compiler& compiler, Cursor& c,RecognizedToken& tok, BlockTermination& termination) {
		c.move(tok);

		CompileValue test_value;
		Cursor err = c;

		Expression::parse(compiler, c,tok, test_value, CompileType::compile);
		Operand::cast(compiler, err, test_value, compiler.types()->t_bool, CompileType::compile, false);
		Expression::rvalue(compiler, test_value, CompileType::compile);


		if (tok != RecognizedToken::OpenBrace) {
			throw_wrong_token_error(c, "'{'");
		}
		c.move(tok);

		ILBlock* block_from = compiler.scope();
		ILBuilder::build_yield(block_from, ILDataType::none);



		ILBlock* continue_block = compiler.target()->create_block();

		compiler.pop_scope();
		compiler.push_scope(continue_block);


		ILBlock* block = compiler.target()->create_and_append_block();
		block->alias = "true";
		compiler.push_scope(block);
		compiler.target()->local_stack_lifetime.push();// block lifetime push
		compiler.stack()->push_block();
		ILBuilder::build_accept(compiler.scope(), ILDataType::none);
		BlockTermination term = BlockTermination::continued;
		Statement::parse_inner_block(compiler, c,tok, term);



		if (c.buffer() == "else") {
			c.move(tok);
			
			if (tok == RecognizedToken::OpenBrace) {
				c.move(tok);

				ILBlock* else_block = compiler.target()->create_and_append_block();
				else_block->alias = "false";
				compiler.push_scope(else_block);
				compiler.target()->local_stack_lifetime.push();// block lifetime push
				compiler.stack()->push_block();
				ILBuilder::build_accept(compiler.scope(), ILDataType::none);
				BlockTermination term2 = BlockTermination::continued;
				Statement::parse_inner_block(compiler, c,tok, term2);
				if (term== BlockTermination::terminated && term2== BlockTermination::terminated) { termination = BlockTermination::terminated; }
				else { termination = BlockTermination::continued; }

				ILBuilder::build_jmpz(block_from, else_block, block);
			}
			else if (c.buffer() == "if") {
				BlockTermination term2 = BlockTermination::continued;
				parse_if(compiler, c,tok, term2);

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

		compiler.target()->append_block(continue_block);
	}


	void Statement::parse_while(Compiler& compiler, Cursor& c, RecognizedToken& tok, BlockTermination& termination) {
		c.move(tok);

		ILBlock* test_block = compiler.target()->create_and_append_block();
		test_block->alias = "while_test_block";

		ILBuilder::build_jmp(compiler.scope(), test_block);
		compiler.switch_scope(test_block);

		CompileValue test_value;
		Cursor err = c;

		Expression::parse(compiler, c,tok, test_value, CompileType::compile);
		Operand::cast(compiler, err, test_value, compiler.types()->t_bool, CompileType::compile, false);
		Expression::rvalue(compiler, test_value, CompileType::compile);

		if (tok != RecognizedToken::OpenBrace) {
			throw_wrong_token_error(c, "'{'");
		}
		c.move(tok);

		ILBlock* continue_block = compiler.target()->create_block();
		continue_block->alias = "while_continue_block";

		compiler.push_loop_blocks(continue_block, test_block);


		ILBlock* block = compiler.target()->create_and_append_block();
		block->alias = "while_block";
		compiler.push_scope(block);
		compiler.target()->local_stack_lifetime.push();// block lifetime push
		compiler.stack()->push_block();
		bool term = false;
		Statement::parse_inner_block(compiler, c,tok, termination);
		if (termination == BlockTermination::breaked) termination = BlockTermination::continued;

		ILBuilder::build_jmpz(test_block, continue_block, block);

		compiler.pop_loop_blocks();
		compiler.switch_scope(continue_block);

		compiler.target()->append_block(continue_block);
	}


	void Statement::parse_for(Compiler& compiler, Cursor& c, RecognizedToken& tok, BlockTermination& termination) {
		c.move(tok);
		if (tok != RecognizedToken::OpenParenthesis) {
			throw_wrong_token_error(c, "'('");
		}


		compiler.target()->local_stack_lifetime.push(); // temp lifetime push
		compiler.target()->local_stack_lifetime.push(); // block lifetime push
		compiler.stack()->push_block();


		Cursor cc = c;
		Cursor err = cc;
		RecognizedToken tok2;
		cc.move(tok2);
		Statement::parse(compiler, cc, tok2, CompileType::compile, termination);
		

		ILBlock* test_block = compiler.target()->create_and_append_block();
		ILBuilder::build_jmp(compiler.scope(), test_block);
		compiler.pop_scope();
		compiler.push_scope(test_block);

		CompileValue test_value;

		err = cc;
		Expression::parse(compiler, cc, tok2, test_value, CompileType::compile);
		Operand::cast(compiler, err, test_value, compiler.types()->t_bool, CompileType::compile, false);
		Expression::rvalue(compiler, test_value, CompileType::compile);

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

		ILBlock* continue_block = compiler.target()->create_block();
		continue_block->alias = "for_continue_block";

		ILBlock* increment_block = compiler.target()->create_and_append_block();
		increment_block->alias = "for_increment";
		compiler.push_scope(increment_block);

		Expression::parse(compiler, cc, tok2, test_value, CompileType::compile, false);
		ILBuilder::build_jmp(increment_block, test_block);

		compiler.push_loop_blocks(continue_block, increment_block);

		ILBlock* block = compiler.target()->create_and_append_block();
		block->alias = "for";
		compiler.push_scope(block);
		compiler.target()->local_stack_lifetime.push(); // block lifetime push
		compiler.stack()->push_block();
		bool term = false;
		Statement::parse_inner_block(compiler, c, tok, termination);
		if (termination == BlockTermination::breaked) termination = BlockTermination::continued;


		StackItem sitm;
		int d = 0;
		while (compiler.temp_stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) { d++;  }
		if (d > 0)
			compiler.target()->local_stack_lifetime.pop();
		else
			compiler.target()->local_stack_lifetime.discard_push();

		d = 0;
		while (compiler.stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) { d++; }
		if (d > 0)
			compiler.target()->local_stack_lifetime.pop();
		else
			compiler.target()->local_stack_lifetime.discard_push();


		compiler.pop_loop_blocks();
		compiler.pop_scope();

		ILBuilder::build_jmpz(test_block, continue_block, block);

		compiler.pop_scope();
		compiler.push_scope(continue_block);

		compiler.target()->append_block(continue_block);
	}




	void Statement::parse_return(Compiler& compiler, Cursor& c, RecognizedToken& tok) {
		
		c.move(tok);
		CompileValue ret_val;
		Cursor err = c;
		Expression::parse(compiler, c,tok, ret_val, CompileType::compile);
		Type* to = compiler.return_type();
		Operand::cast(compiler, err, ret_val, to, CompileType::compile, false);
		Expression::rvalue(compiler, ret_val, CompileType::compile);

		if (compiler.return_type()->rvalue_stacked()) {
			ILBuilder::build_local(compiler.scope(), 0);
			ILBuilder::build_load(compiler.scope(), ILDataType::word);
			Expression::copy_from_rvalue(compiler.return_type(), CompileType::compile, false);
			ILBuilder::build_ret(compiler.scope(), ILDataType::none);
		}
		else {
			ILBuilder::build_ret(compiler.scope(), compiler.return_type()->rvalue());
		}


		if (tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}

		c.move(tok);
	}

	void Statement::parse_let(Compiler& compiler, Cursor& c, RecognizedToken& tok) {

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
		uint32_t local_id = compiler.target()->local_stack_lifetime.append_unknown(let_holder);

		compiler.target()->local_stack_lifetime.push(); // temp lifetime push

		Cursor err = c;
		CompileValue val;
		Expression::parse(compiler, c,tok, val, CompileType::compile);

		if (!reference) {			
			Expression::rvalue(compiler, val, CompileType::compile);
		}
		else {
			if (!val.lvalue) {
				throw_specific_error(err, "Cannot create reference to rvalue");
			}
		}
		Type* new_t = val.type;
		new_t->compile();

		if (new_t->context() != ILContext::both && compiler.scope_context() != new_t->context()) {
			throw_specific_error(err, "Type was not designed for this context");
		}

		if (reference) {
			new_t = new_t->generate_reference();
		}

		compiler.target()->local_stack_lifetime.resolve_unknown(let_holder,new_t->size());

		compiler.stack()->push_item(name.buffer(), new_t, local_id, StackItemTag::regular);
		/*if (new_t->has_special_constructor()) {
			ILBuilder::build_local(compiler.scope(), local_id);
			new_t->build_construct();
		}*/


		ILBuilder::build_local(compiler.scope(), local_id);
		Expression::copy_from_rvalue(new_t, CompileType::compile, false);
		
		if (tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}
		c.move(tok);
	}

	void Statement::parse_make(Compiler& compiler, Cursor& c, RecognizedToken& tok) {
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

		compiler.push_scope_context(ILContext::compile);

		Expression::parse(compiler, c,tok, val, CompileType::eval);
		Expression::rvalue(compiler, val, CompileType::eval);

		if (val.type != compiler.types()->t_type) {
			throw_specific_error(err, "Exprected type");
		}
		compiler.pop_scope_context();

		Type* new_t = compiler.evaluator()->pop_register_value<Type*>();
		new_t->compile();

		if (new_t->context() != ILContext::both && compiler.scope_context() != new_t->context()) {
			throw_specific_error(err, "Type was not designed for this context");
		}

		ILSize s = new_t->size();
		uint32_t local_id = compiler.target()->local_stack_lifetime.append(s);
		compiler.stack()->push_item(name.buffer(), new_t, local_id, StackItemTag::regular);

		if (tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}
		c.move(tok);
	}
}