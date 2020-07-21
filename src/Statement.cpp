#include "Statement.h"
#include "Expression.h"
#include "Operand.h"
#include "Error.h"
#include "StackManager.h"
#include <iostream>

namespace Corrosive {


	void Statement::parse_inner_block(Compiler& compiler, Cursor& c, BlockTermination& termination, bool exit_returns, Cursor* err) {

		StackItem sitm;
		ILBlock* b_exit = compiler.target()->create_block();
		b_exit->alias = "exit";
		compiler.push_scope_exit(b_exit);


		termination = BlockTermination::no_exit;


		while (c.tok != RecognizedToken::CloseBrace) {

			if (c.src != nullptr) {
				Source* src = (Source*)c.src;
				ILBuilder::build_debug(compiler.scope(),src->debug_id, c.top);
			}
			else {

				ILBuilder::build_debug(compiler.scope(), UINT16_MAX, c.top);
			}

			Statement::parse(compiler,c, CompileType::compile, termination);

			if (termination == BlockTermination::terminated && c.tok != RecognizedToken::CloseBrace) {
				throw_specific_error(c, "Instruction after the current branch has been terminated");
			}

			int d = 0;
			while (compiler.temp_stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) {
				if (sitm.type->has_special_destructor()) {
					ILBuilder::build_local(compiler.scope(), sitm.id);
					sitm.type->build_drop();
				}
				d++;
			}

			if (d>0)
				compiler.target()->local_stack_lifetime.pop();
			else
				compiler.target()->local_stack_lifetime.discard_push();
			
		}
		c.move();

		auto ret = compiler.return_type();
		ILBuilder::build_accept(b_exit, ret->rvalue_stacked()?ILDataType::none:ret->rvalue());



		while (compiler.stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) {
			if (sitm.type->has_special_destructor()) {

				if (termination != BlockTermination::terminated) {
					ILBuilder::build_local(compiler.scope(), sitm.id);
					sitm.type->build_drop();
				}

				if (termination != BlockTermination::no_exit) {
					compiler.push_scope(compiler.scope_exit());
					ILBuilder::build_local(compiler.scope(), sitm.id);
					sitm.type->build_drop();
					compiler.pop_scope();
				}
			}
		}

		compiler.target()->local_stack_lifetime.pop();


		auto prev_scope = compiler.scope();
		compiler.pop_scope();
		compiler.pop_scope_exit();

		if (termination == BlockTermination::terminated) {

			compiler.target()->append_block(b_exit);


			auto ret = compiler.return_type();
			ILBuilder::build_yield(prev_scope, ret->rvalue_stacked() ? ILDataType::none : ret->rvalue());
			ILBuilder::build_jmp(prev_scope, b_exit);

			if (!exit_returns) {
				auto ret = compiler.return_type();
				ILBuilder::build_yield(b_exit, ret->rvalue_stacked() ? ILDataType::none : ret->rvalue());
				ILBuilder::build_jmp(b_exit, compiler.scope_exit());
			}
			else {
				auto ret = compiler.return_type();
				ILBuilder::build_ret(b_exit, ret->rvalue_stacked() ? ILDataType::none : ret->rvalue());
			}
		}
		else if (termination == BlockTermination::needs_exit) {

			compiler.target()->append_block(b_exit);

			ILBuilder::build_yield(compiler.scope(), ILDataType::none);

			if (!exit_returns) {
				ILBuilder::build_jmp(prev_scope, compiler.scope());


				auto ret = compiler.return_type();
				ILBuilder::build_yield(b_exit, ret->rvalue_stacked() ? ILDataType::none : ret->rvalue());
				ILBuilder::build_jmp(b_exit, compiler.scope_exit());
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
					ILBuilder::build_ret(b_exit, ILDataType::none);
				}
			}
		}
		else if (termination == BlockTermination::no_exit) {
			ILBuilder::build_yield(compiler.scope(), ILDataType::none);

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

	void Statement::parse(Compiler& compiler, Cursor& c, CompileType cmp, BlockTermination& termination) {

		if (cmp == CompileType::eval) {
			throw_specific_error(c, "Statement is used in compiletime context. Compiler should not allow it.");
		}

		switch (c.tok) {
			case RecognizedToken::Symbol:
			{
				if (c.buffer == "return") {
					compiler.target()->local_stack_lifetime.push();// temp lifetime push
					termination = BlockTermination::terminated;
					parse_return(compiler, c);
					return; 
				}else if (c.buffer == "make") {
					parse_make(compiler, c);

					compiler.target()->local_stack_lifetime.push();// temp lifetime push
					return;
				}
				else if (c.buffer == "let") {
					parse_let(compiler, c);
					return;
				}
				else if (c.buffer == "if") {
					compiler.target()->local_stack_lifetime.push();// temp lifetime push
					parse_if(compiler, c, termination);
					return;
				}
				else if (c.buffer == "while") {
					compiler.target()->local_stack_lifetime.push();// temp lifetime push
					parse_while(compiler, c, termination);
					return;
				}
			}break;
			case RecognizedToken::OpenBrace: {
				compiler.target()->local_stack_lifetime.push(); // temp lifetime push
				c.move();
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

				parse_inner_block(compiler, c, termination);
				compiler.target()->append_block(continue_block);
				return;
			}break;
		}

		//fallthrough if every other check fails, statement is plain expression

		compiler.target()->local_stack_lifetime.push();// temp lifetime push
		CompileValue ret_val;
		Expression::parse(compiler, c, ret_val, cmp, false);

		if (ret_val.type->rvalue() != ILDataType::none) {
			if (cmp == CompileType::compile) {

				if (ret_val.lvalue || ret_val.type->rvalue_stacked()) {
					if (ret_val.type->has_special_destructor()) {
						ret_val.type->build_drop();
					}
					else {
						ILBuilder::build_forget(compiler.scope(), ILDataType::word);
					}
				}
				else {
					ILBuilder::build_forget(compiler.scope(), ret_val.type->rvalue());
				}
			}
			else {
				if (ret_val.lvalue || ret_val.type->rvalue_stacked()) {
					if (ret_val.type->has_special_destructor()) {
						unsigned char* me = compiler.evaluator()->pop_register_value<unsigned char*>();
						ret_val.type->drop(me);
					}
					else {
						ILBuilder::eval_forget(compiler.evaluator(), ILDataType::word);
					}
				} else {
					ILBuilder::eval_forget(compiler.evaluator(), ret_val.type->rvalue());
				}
			}
		}

		if (c.tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}
		c.move();
	}

	void Statement::parse_if(Compiler& compiler, Cursor& c, BlockTermination& termination) {
		c.move();

		CompileValue test_value;
		Cursor err = c;

		Expression::parse(compiler, c, test_value, CompileType::compile);
		Operand::cast(compiler, err, test_value, compiler.types()->t_bool, CompileType::compile, false);
		Expression::rvalue(compiler, test_value, CompileType::compile);


		if (c.tok != RecognizedToken::OpenBrace) {
			throw_wrong_token_error(c, "'{'");
		}
		c.move();

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
		BlockTermination term = BlockTermination::no_exit;
		Statement::parse_inner_block(compiler, c, term);



		if (c.buffer == "else") {
			c.move();
			
			if (c.tok == RecognizedToken::OpenBrace) {
				c.move();

				ILBlock* else_block = compiler.target()->create_and_append_block();
				else_block->alias = "false";
				compiler.push_scope(else_block);
				compiler.target()->local_stack_lifetime.push();// block lifetime push
				compiler.stack()->push_block();
				ILBuilder::build_accept(compiler.scope(), ILDataType::none);
				BlockTermination term2 = BlockTermination::no_exit;
				Statement::parse_inner_block(compiler, c, term2);
				if (term== BlockTermination::no_exit && term2== BlockTermination::no_exit) { termination = BlockTermination::no_exit; }
				else if (term == BlockTermination::terminated && term2 == BlockTermination::terminated) { termination = BlockTermination::terminated; }
				else { termination = BlockTermination::needs_exit; }

				ILBuilder::build_jmpz(block_from, else_block, block);
			}
			else if (c.buffer == "if") {
				BlockTermination term2 = BlockTermination::no_exit;
				parse_if(compiler, c, term2);

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

		compiler.target()->append_block(continue_block);
	}


	void Statement::parse_while(Compiler& compiler, Cursor& c, BlockTermination& termination) {
		c.move();


		ILBlock* test_block = compiler.target()->create_and_append_block();
		ILBuilder::build_yield(compiler.scope(), ILDataType::none);
		ILBuilder::build_jmp(compiler.scope(), test_block);
		compiler.pop_scope();
		compiler.push_scope(test_block);
		ILBuilder::build_accept(compiler.scope(), ILDataType::none);

		CompileValue test_value;
		Cursor err = c;

		Expression::parse(compiler, c, test_value, CompileType::compile);
		Operand::cast(compiler, err, test_value, compiler.types()->t_bool, CompileType::compile, false);
		Expression::rvalue(compiler, test_value, CompileType::compile);


		if (c.tok != RecognizedToken::OpenBrace) {
			throw_wrong_token_error(c, "'{'");
		}
		c.move();

		ILBuilder::build_yield(test_block, ILDataType::none);
		ILBlock* continue_block = compiler.target()->create_block();



		ILBlock* block = compiler.target()->create_and_append_block();
		block->alias = "while";
		compiler.push_scope(block);
		compiler.target()->local_stack_lifetime.push();// block lifetime push
		compiler.stack()->push_block();
		ILBuilder::build_accept(block, ILDataType::none);
		bool term = false;
		Statement::parse_inner_block(compiler, c, termination);



		ILBuilder::build_jmpz(test_block, continue_block, block);

		ILBuilder::build_accept(continue_block, ILDataType::none);
		

		compiler.pop_scope();
		compiler.push_scope(continue_block);

		compiler.target()->append_block(continue_block);
	}




	void Statement::parse_return(Compiler& compiler, Cursor& c) {
		
		c.move();
		CompileValue ret_val;
		Cursor err = c;
		Expression::parse(compiler, c, ret_val, CompileType::compile);
		Type* to = compiler.return_type();
		Operand::cast(compiler, err, ret_val, to, CompileType::compile, false);
		Expression::rvalue(compiler, ret_val, CompileType::compile);



		if (compiler.return_type()->rvalue_stacked()) {
			ILBuilder::build_local(compiler.scope(), 0);
			ILBuilder::build_load(compiler.scope(), ILDataType::word);
			Expression::move_from_rvalue(compiler.return_type(), CompileType::compile, false);
		}


		if (c.tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}

		c.move();
	}

	void Statement::parse_let(Compiler& compiler, Cursor& c) {

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
		uint32_t local_id = compiler.target()->local_stack_lifetime.append_unknown(let_holder);

		compiler.target()->local_stack_lifetime.push(); // temp lifetime push

		Cursor err = c;
		CompileValue val;
		Expression::parse(compiler, c, val, CompileType::compile);

		if (!reference) {
			if (!val.lvalue) 
				do_copy = false;
			
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

		compiler.stack()->push_item(name.buffer, new_t, local_id, StackItemTag::regular);
		if (new_t->has_special_constructor()) {
			ILBuilder::build_local(compiler.scope(), local_id);
			new_t->build_construct();
		}


		ILBuilder::build_local(compiler.scope(), local_id);

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

	void Statement::parse_make(Compiler& compiler, Cursor& c) {
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

		compiler.push_scope_context(ILContext::compile);

		Expression::parse(compiler, c, val, CompileType::eval);
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
		compiler.stack()->push_item(name.buffer, new_t, local_id, StackItemTag::regular);

		if (construct && new_t->has_special_constructor()) {
			ILBuilder::build_local(compiler.scope(), local_id);
			new_t->build_construct();
		}

		if (c.tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}
		c.move();
	}
}