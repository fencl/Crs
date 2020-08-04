#include "Statement.h"
#include "Expression.h"
#include "Operand.h"
#include "Error.h"
#include "StackManager.h"
#include <iostream>

namespace Corrosive {

	void Statement::parse_inner_block_start(ILBlock* block) {
		Compiler::current()->push_scope(block);
		Compiler::current()->stack()->push_block();
		Compiler::current()->target()->local_stack_lifetime.push(); // scope push
		Compiler::current()->push_defer_scope();

		Compiler::current()->compiler_stack()->push_block();
		Compiler::current()->stack_push_block();
	}

	void Statement::parse_inner_block(Cursor& c,RecognizedToken& tok, BlockTermination& termination, bool exit_returns, Cursor* err, bool force_compile) {
		StackItem sitm;
		termination = BlockTermination::continued;

		int d = 0;

		while (tok != RecognizedToken::CloseBrace) {

			Compiler::current()->temp_stack()->push();

			//TODO
			/*if (c.src != nullptr) {
				Source* src = (Source*)c.src;
				ILBuilder::build_debug(Compiler::current()->scope(),src->debug_id, top);
			}
			else {

				ILBuilder::build_debug(Compiler::current()->scope(), UINT16_MAX, top);
			}*/

			Statement::parse(c, tok, termination, force_compile);

			if (termination != BlockTermination::continued && tok != RecognizedToken::CloseBrace) {
				throw_specific_error(c, "Instruction after the current branch has been terminated");
			}

			while (Compiler::current()->temp_stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) { d++; }
			
			Compiler::current()->temp_stack()->pop();

		}
		c.move(tok);

		auto ret = Compiler::current()->return_type();


		// scope pop
		while (Compiler::current()->stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) { d++; }
		if (d>0)
			Compiler::current()->target()->local_stack_lifetime.pop();
		else
			Compiler::current()->target()->local_stack_lifetime.discard_push();

		auto prev_scope = Compiler::current()->scope();
		Compiler::current()->pop_scope();

		if (termination == BlockTermination::continued) {

			for (auto d = Compiler::current()->defer_scope().rbegin(); d != Compiler::current()->defer_scope().rend(); d++) {
				ILBuilder::build_call(prev_scope, (*d)->il_function_decl);
				if ((*d)->return_type->rvalue_stacked()) {
					ILBuilder::build_forget(prev_scope, ILDataType::word);
				}
				else {
					ILBuilder::build_forget(prev_scope, (*d)->return_type->rvalue());
				}
			}


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

		Compiler::current()->pop_defer_scope();
		Compiler::current()->stack()->pop_block();

		Compiler::current()->stack_pop_block();
		Compiler::current()->compiler_stack()->pop_block();
	}

	void Statement::parse(Cursor& c,RecognizedToken& tok, BlockTermination& termination, bool force_compile) {

		switch (tok) {
			case RecognizedToken::Symbol:
			{
				auto buf = c.buffer();
				if (buf == "return") {
					termination = BlockTermination::terminated;
					parse_return(c,tok);
					return; 
				}else if (buf == "make") {
					parse_make(c,tok, force_compile);
					return;
				}
				else if (buf == "let") {
					parse_let(c,tok, force_compile);
					return;
				}
				else if (buf == "if") {
					parse_if(c,tok, termination,force_compile);
					return;
				}
				else if (buf == "while") {
					parse_while(c,tok, termination, force_compile);
					return;
				}
				else if (buf == "for") {
					parse_for(c,tok, termination,force_compile);
					return;
				}
				
				else if (buf == "break") {

					
					for (auto d = Compiler::current()->defer_scope().rbegin(); d != Compiler::current()->defer_scope().rend(); d++) {
						ILBuilder::build_call(Compiler::current()->scope(), (*d)->il_function_decl);
						if ((*d)->return_type->rvalue_stacked()) {
							ILBuilder::build_forget(Compiler::current()->scope(), ILDataType::word);
						}
						else {
							ILBuilder::build_forget(Compiler::current()->scope(), (*d)->return_type->rvalue());
						}
					}

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

					for (auto d = Compiler::current()->defer_scope().rbegin(); d != Compiler::current()->defer_scope().rend(); d++) {
						ILBuilder::build_call(Compiler::current()->scope(), (*d)->il_function_decl);
						if ((*d)->return_type->rvalue_stacked()) {
							ILBuilder::build_forget(Compiler::current()->scope(), ILDataType::word);
						}
						else {
							ILBuilder::build_forget(Compiler::current()->scope(), (*d)->return_type->rvalue());
						}
					}

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
				else if (buf == "defer") {
					c.move(tok);
					CompileValue cval;
					Cursor err = c;
					Compiler::current()->targets_defer(true);
					Operand::parse(c, tok, cval, CompileType::compile);
					Compiler::current()->targets_defer(false);

					if (cval.type != Compiler::current()->types()->t_void || !cval.lvalue) {
						throw_specific_error(err, "Only function call can be defered");
					}
					if (tok != RecognizedToken::Semicolon) {
						throw_wrong_token_error(c, "';'");
					}
					c.move(tok);
					return;
				}

			}break;
			case RecognizedToken::OpenBrace: {
				c.move(tok);
				ILBlock* block = Compiler::current()->target()->create_and_append_block();

				ILBuilder::build_jmp(Compiler::current()->scope(), block);

				ILBlock* continue_block = Compiler::current()->target()->create_block();

				Compiler::current()->switch_scope(continue_block);
				parse_inner_block_start(block);
				parse_inner_block(c,tok, termination,false,nullptr,force_compile);
				Compiler::current()->target()->append_block(continue_block);
				return;
			}break;
		}

		//fallthrough if every other check fails, statement is plain expression

		bool compile = force_compile;
		if (c.buffer() == "compile") {
			compile = true;
			c.move(tok);
		}

		CompileValue ret_val;
		if (!compile) {
			Expression::parse(c, tok, ret_val, CompileType::compile, false);
			Operand::deref(ret_val, CompileType::compile);
		}
		else {
			auto scope = ScopeState().context(ILContext::compile);
			Expression::parse(c, tok, ret_val, CompileType::eval, false);
			Operand::deref(ret_val, CompileType::eval);
		}

		if (tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}
		c.move(tok);
	}

	void Statement::parse_if(Cursor& c,RecognizedToken& tok, BlockTermination& termination, bool force_compile, bool do_next) {
		c.move(tok);
		bool compile = force_compile;
		if (c.buffer() == "compile") {
			compile = true;
			c.move(tok);
		}

		if (!compile) {
			CompileValue test_value;
			Cursor err = c;

			Expression::parse(c, tok, test_value, CompileType::compile);
			Operand::deref(test_value, CompileType::compile);
			Operand::cast(err, test_value, Compiler::current()->types()->t_bool, CompileType::compile, false);
			Expression::rvalue(test_value, CompileType::compile);


			if (tok != RecognizedToken::OpenBrace) {
				throw_wrong_token_error(c, "'{'");
			}
			c.move(tok);

			ILBlock* block_from = Compiler::current()->scope();
			ILBuilder::build_yield(block_from, ILDataType::none);



			ILBlock* continue_block = Compiler::current()->target()->create_block();

			Compiler::current()->switch_scope(continue_block);


			ILBlock* block = Compiler::current()->target()->create_and_append_block();
			block->alias = "true";

			BlockTermination term = BlockTermination::continued;
			Statement::parse_inner_block_start(block);
			Statement::parse_inner_block(c, tok, term);



			if (c.buffer() == "else") {
				c.move(tok);

				if (tok == RecognizedToken::OpenBrace) {
					c.move(tok);

					ILBlock* else_block = Compiler::current()->target()->create_and_append_block();
					else_block->alias = "false";

					BlockTermination term2 = BlockTermination::continued;
					Statement::parse_inner_block_start(else_block);
					Statement::parse_inner_block(c, tok, term2);

					if (term == BlockTermination::terminated && term2 == BlockTermination::terminated) { termination = BlockTermination::terminated; }
					else { termination = BlockTermination::continued; }

					ILBuilder::build_jmpz(block_from, else_block, block);
				}
				else if (c.buffer() == "if") {
					BlockTermination term2 = BlockTermination::continued;
					parse_if(c, tok, term2, force_compile);

					if (term == BlockTermination::terminated && term2 == BlockTermination::terminated) { termination = BlockTermination::terminated; }
					else { termination = BlockTermination::continued; }

					ILBuilder::build_jmpz(block_from, continue_block, block);
				}
				else {
					termination = term;
					throw_specific_error(c, "else can be followed only by { or if");
				}
			}
			else {
				ILBuilder::build_jmpz(block_from, continue_block, block);
			}

			Compiler::current()->target()->append_block(continue_block);
		}
		else {

			Compiler::current()->push_scope_context(ILContext::compile);
			CompileValue test_value;
			Cursor err = c;

			Expression::parse(c, tok, test_value, CompileType::eval);
			Operand::deref(test_value, CompileType::eval);
			Operand::cast(err, test_value, Compiler::current()->types()->t_bool, CompileType::eval, false);
			Expression::rvalue(test_value, CompileType::eval);
			bool condition = Compiler::current()->evaluator()->pop_register_value<bool>() & do_next;

			if (tok != RecognizedToken::OpenBrace) {
				throw_wrong_token_error(c, "'{'");
			}


			if (condition) {
				condition = false;
				Compiler::current()->pop_scope_context();
				Statement::parse(c, tok, termination);
				Compiler::current()->push_scope_context(ILContext::compile);
			}
			else {
				condition = do_next;
				c.move_matching(tok);
				c.move(tok);
			}
			

			if (c.buffer() == "else") {
				c.move(tok);
				if (c.buffer() == "if") {
					parse_if(c, tok, termination, true, condition);
				}
				else {
					if (tok != RecognizedToken::OpenBrace) {
						throw_wrong_token_error(c, "'{'");
					}

					if (condition) {
						Compiler::current()->pop_scope_context();
						Statement::parse(c, tok, termination);
						Compiler::current()->push_scope_context(ILContext::compile);
					}
					else {
						c.move_matching(tok);
						c.move(tok);
					}
				}
			}


			Compiler::current()->pop_scope_context();
		}
	}


	void Statement::parse_while(Cursor& c, RecognizedToken& tok, BlockTermination& termination, bool force_compile) {
		bool compile = force_compile;
		c.move(tok);
		if (c.buffer() == "compile") {
			compile = true;
			c.move(tok);
		}

		if (!compile) {
			ILBlock* test_block = Compiler::current()->target()->create_and_append_block();
			test_block->alias = "while_test_block";

			ILBuilder::build_jmp(Compiler::current()->scope(), test_block);
			Compiler::current()->switch_scope(test_block);

			CompileValue test_value;
			Cursor err = c;

			Expression::parse(c, tok, test_value, CompileType::compile);
			Operand::deref(test_value, CompileType::compile);
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
			Statement::parse_inner_block_start(block);
			bool term = false;
			Statement::parse_inner_block(c, tok, termination);
			if (termination == BlockTermination::breaked) termination = BlockTermination::continued;

			ILBuilder::build_jmpz(test_block, continue_block, block);

			Compiler::current()->pop_loop_blocks();
			Compiler::current()->switch_scope(continue_block);

			Compiler::current()->target()->append_block(continue_block);
		}
		else {
			Cursor save = c;
			RecognizedToken save_tok = tok;
			while (true) {
				Compiler::current()->push_scope_context(ILContext::compile);
				c = save;
				tok = save_tok;
				
				CompileValue res;
				Expression::parse(c, tok, res, CompileType::eval, true);
				Operand::deref(res, CompileType::eval);
				Operand::cast(save, res, Compiler::current()->types()->t_bool, CompileType::eval, true);
				Expression::rvalue(res, CompileType::eval);

				Compiler::current()->pop_scope_context(); 
				if (tok != RecognizedToken::OpenBrace) {
					throw_wrong_token_error(c, "'{'");
				}

				bool condition = Compiler::current()->evaluator()->pop_register_value<bool>();
				if (!condition) break; // we should be at {

				

				Statement::parse(c, tok, termination);
				if (termination == BlockTermination::terminated) return; // return to skip matching pair
			}

			c.move_matching(tok);
			c.move(tok);

		}
	}


	void Statement::parse_for(Cursor& c, RecognizedToken& tok, BlockTermination& termination, bool force_compile) {
		bool compile = force_compile;
		c.move(tok);
		if (c.buffer() == "compile") {
			compile = true;
			c.move(tok);
		}

		if (tok != RecognizedToken::OpenParenthesis) {
			throw_wrong_token_error(c, "'('");
		}

		if (!compile) {
			Compiler::current()->stack()->push_block();


			Cursor cc = c;
			Cursor err = cc;
			RecognizedToken tok2;
			cc.move(tok2);
			Statement::parse(cc, tok2, termination);


			ILBlock* test_block = Compiler::current()->target()->create_and_append_block();
			ILBuilder::build_jmp(Compiler::current()->scope(), test_block);
			Compiler::current()->switch_scope(test_block);

			CompileValue test_value;

			err = cc;
			Expression::parse(cc, tok2, test_value, CompileType::compile);
			Operand::deref(test_value, CompileType::compile);
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
			Operand::deref(test_value, CompileType::compile);
			ILBuilder::build_jmp(increment_block, test_block);

			Compiler::current()->push_loop_blocks(continue_block, increment_block);

			ILBlock* block = Compiler::current()->target()->create_and_append_block();
			block->alias = "for";

			Statement::parse_inner_block_start(block);
			bool term = false;
			Statement::parse_inner_block(c, tok, termination);
			if (termination == BlockTermination::breaked) termination = BlockTermination::continued;


			Compiler::current()->pop_loop_blocks();
			Compiler::current()->pop_scope();

			ILBuilder::build_jmpz(test_block, continue_block, block);

			Compiler::current()->switch_scope(continue_block);

			Compiler::current()->target()->append_block(continue_block);
		}
		else {
			Compiler::current()->push_scope_context(ILContext::compile);
			RecognizedToken statement_tok = tok;
			Cursor statement = c;
			statement.move_matching(statement_tok);
			statement.move(statement_tok);

			if (statement_tok != RecognizedToken::OpenBrace) {
				throw_wrong_token_error(statement, "'{'");
			}

			Compiler::current()->compiler_stack()->push_block();
			Cursor c_init = c;
			RecognizedToken c_init_tok;
			c_init.move(c_init_tok);
			BlockTermination unused;
			Statement::parse(c_init, c_init_tok, unused, true);
			while (true) {
				RecognizedToken c_condition_tok = c_init_tok;
				Cursor c_condition = c_init;
				Cursor err = c_condition;
				CompileValue res;
				Expression::parse(c_condition, c_condition_tok, res, CompileType::eval);
				Operand::deref(res, CompileType::eval);
				Operand::cast(err, res, Compiler::current()->types()->t_bool, CompileType::eval,true);
				Expression::rvalue(res, CompileType::eval);
				bool condition = Compiler::current()->evaluator()->pop_register_value<bool>();
				if (!condition) break;

				Compiler::current()->compiler_stack()->push_block();

				Compiler::current()->compiler_stack()->pop_block();
				Cursor statement_copy = statement;
				RecognizedToken statement_tok_copy = statement_tok;
				Statement::parse(statement_copy, statement_tok_copy, termination);
				if (termination == BlockTermination::terminated) break;
				Compiler::current()->pop_scope_context();
				Compiler::current()->push_scope_context(ILContext::compile);

				if (c_condition_tok != RecognizedToken::Semicolon) {
					throw_wrong_token_error(c_condition, "';'");
				}
				c_condition.move(c_condition_tok);
				Expression::parse(c_condition, c_condition_tok, res, CompileType::eval,false);
			}

			Compiler::current()->compiler_stack()->pop_block();
			Compiler::current()->pop_scope_context();

			c = statement;
			tok = statement_tok;
			c.move_matching(tok);
			c.move(tok);
		}
		
	}




	void Statement::parse_return(Cursor& c, RecognizedToken& tok) {

		bool has_defer = false;
		for (auto ds = Compiler::current()->defer_function().rbegin(); ds != Compiler::current()->defer_function().rend(); ds++) {
			for (auto d = ds->rbegin(); d != ds->rend(); d++) {
				has_defer = true;
				break;
			}
			if (has_defer) break;
		}

		c.move(tok);
		CompileValue ret_val;
		Cursor err = c;
		Expression::parse(c,tok, ret_val, CompileType::compile);
		Operand::deref(ret_val, CompileType::compile);

		Type* to = Compiler::current()->return_type();
		Operand::cast(err, ret_val, to, CompileType::compile, false);
		Expression::rvalue(ret_val, CompileType::compile);

		if (Compiler::current()->return_type()->rvalue_stacked()) {
			ILBuilder::build_local(Compiler::current()->scope(), 0);
			ILBuilder::build_load(Compiler::current()->scope(), ILDataType::word);
			Expression::copy_from_rvalue(Compiler::current()->return_type(), CompileType::compile, false);
		}
		else {
			if (has_defer) {
				ILBuilder::build_yield(Compiler::current()->scope(), Compiler::current()->return_type()->rvalue());
			}
		}

		

		for (auto ds = Compiler::current()->defer_function().rbegin(); ds != Compiler::current()->defer_function().rend(); ds++) {
			for (auto d = ds->rbegin(); d != ds->rend(); d++) {
				ILBuilder::build_call(Compiler::current()->scope(), (*d)->il_function_decl);
				if ((*d)->return_type->rvalue_stacked()) {
					ILBuilder::build_forget(Compiler::current()->scope(), ILDataType::word);
				}
				else {
					ILBuilder::build_forget(Compiler::current()->scope(), (*d)->return_type->rvalue());
				}
			}
		}


		if (Compiler::current()->return_type()->rvalue_stacked()) {
			ILBuilder::build_ret(Compiler::current()->scope(), ILDataType::none);
		}
		else {

			if (has_defer) {
				ILBlock* exit_block = Compiler::current()->target()->create_and_append_block();
				exit_block->alias = "exit_block";
				ILBuilder::build_jmp(Compiler::current()->scope(), exit_block);
				Compiler::current()->switch_scope(exit_block);
				ILBuilder::build_accept(exit_block, Compiler::current()->return_type()->rvalue());
			}

			ILBuilder::build_ret(Compiler::current()->scope(), Compiler::current()->return_type()->rvalue());
		}


		if (tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}

		c.move(tok);
	}

	void Statement::parse_let(Cursor& c, RecognizedToken& tok, bool force_compile) {
		//Compiler::current()->target()->local_stack_lifetime.discard_push(); // temp push
		bool compile = force_compile;

		c.move(tok);
		if (c.buffer() == "compile") {
			compile = true;
			c.move(tok);
		}

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

		if (!compile) {
			size_t let_holder;
			uint32_t local_id = Compiler::current()->target()->local_stack_lifetime.append_unknown(let_holder);

			Cursor err = c;
			CompileValue val;
			Expression::parse(c, tok, val, CompileType::compile);
			Operand::deref(val, CompileType::compile);

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

			Compiler::current()->target()->local_stack_lifetime.resolve_unknown(let_holder, new_t->size());
			Compiler::current()->stack()->push_item(name.buffer(), new_t, local_id, StackItemTag::regular);


			ILBuilder::build_local(Compiler::current()->scope(), local_id);
			Expression::copy_from_rvalue(new_t, CompileType::compile, false);
		}
		else {
			auto scope = ScopeState().context(ILContext::compile);

			Cursor err = c;
			CompileValue val;
			Expression::parse(c, tok, val, CompileType::eval);
			Operand::deref(val, CompileType::eval);

			if (!reference) {
				Expression::rvalue(val, CompileType::eval);
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

			uint16_t loc_id = Compiler::current()->push_local(new_t->size());
			uint8_t* loc_ptr = Compiler::current()->stack_ptr(loc_id);

			Compiler::current()->compiler_stack()->push_item(name.buffer(), new_t, loc_id, StackItemTag::regular);

			ILBuilder::eval_const_ptr(Compiler::current()->evaluator(), loc_ptr);
			Expression::copy_from_rvalue(new_t, CompileType::eval, false);
		}
		
		if (tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}
		c.move(tok);
	}

	void Statement::parse_make(Cursor& c, RecognizedToken& tok, bool force_compile) {

		bool compile = force_compile;
		c.move(tok);
		if (c.buffer() == "compile") {
			compile = true;
			c.move(tok);
		}

		if (tok != RecognizedToken::Symbol) {
			throw_not_a_name_error(c);
		}
		Cursor name = c;
		c.move(tok);
		if (tok != RecognizedToken::Colon) {
			throw_wrong_token_error(c, "':'");
		}
		c.move(tok);
		if (!compile) {
			Cursor err = c;
			CompileValue val;

			Compiler::current()->push_scope_context(ILContext::compile);

			Expression::parse(c, tok, val, CompileType::eval);
			Operand::deref(val, CompileType::eval);
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
		}
		else {
			auto scope = ScopeState().context(ILContext::compile);

			Cursor err = c;
			CompileValue val;


			Expression::parse(c, tok, val, CompileType::eval);
			Operand::deref(val, CompileType::eval);
			Expression::rvalue(val, CompileType::eval);

			if (val.type != Compiler::current()->types()->t_type) {
				throw_specific_error(err, "Exprected type");
			}

			Type* new_t = Compiler::current()->evaluator()->pop_register_value<Type*>();
			new_t->compile();

			if (new_t->context() != ILContext::both && Compiler::current()->scope_context() != new_t->context()) {
				throw_specific_error(err, "Type was not designed for this context");
			}

			ILSize s = new_t->size();
			uint16_t local_id = Compiler::current()->push_local(s);
			Compiler::current()->compiler_stack()->push_item(name.buffer(), new_t, local_id, StackItemTag::regular);
		}

		if (tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}
		c.move(tok);

		//Compiler::current()->target()->local_stack_lifetime.push(); // temp push
	}
}