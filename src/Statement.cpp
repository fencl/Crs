#include "Statement.hpp"
#include "Expression.hpp"
#include "Operand.hpp"
#include "Error.hpp"
#include "StackManager.hpp"
#include <iostream>

namespace Corrosive {

	void Statement::parse_inner_block_start(ILBlock* block, ForceCompile force_compile) {
		Compiler* compiler = Compiler::current();

		if (force_compile == ForceCompile::no) {
			compiler->push_scope(block);
			compiler->stack()->push_block();
			compiler->target()->local_stack_lifetime.push(); // scope push
			compiler->defers.back().push_back(std::vector<TypeFunction*>());
		}

		compiler->compile_defers.push_back(std::vector<TypeFunction*>());
		compiler->compiler_stack()->push_block();
		compiler->stack_push_block();
	}

	void Statement::parse_inner_block(Cursor& c,RecognizedToken& tok, BlockTermination& termination, bool exit_returns, Cursor* err, ForceCompile force_compile) {
		Compiler* compiler = Compiler::current();

		StackItem sitm;
		termination = BlockTermination::continued;

		int d = 0;

		while (tok != RecognizedToken::CloseBrace) {
			if (force_compile == ForceCompile::no || force_compile == ForceCompile::inlineblock)
				compiler->temp_stack()->push();

			//TODO
			/*if (c.src != nullptr) {
				Source* src = (Source*)c.src;
				ILBuilder::build_debug(compiler->scope(),src->debug_id, top);
			}
			else {

				ILBuilder::build_debug(compiler->scope(), UINT16_MAX, top);
			}*/
			ForceCompile new_fc;
			switch (force_compile)
			{
				case ForceCompile::force: new_fc = ForceCompile::force; break;
				case ForceCompile::single: new_fc = ForceCompile::force; break;
				case ForceCompile::no: new_fc = ForceCompile::no; break;
				case ForceCompile::inlineblock: new_fc = ForceCompile::no; break;
				default:
					break;
			}

			Statement::parse(c, tok, termination, new_fc);

			if (termination != BlockTermination::continued && tok != RecognizedToken::CloseBrace) {
				throw_specific_error(c, "Instruction after the current branch has been terminated");
			}

			if (Statement::runtime(force_compile)) {

				while (compiler->temp_stack()->pop_item(sitm)) { d++; }

				compiler->temp_stack()->pop();
			}

			if (compiler->compile_in_loop()) {
				if (compiler->compile_loop_state() != CompileTimeBlockState::run) break;
			}
		}
		c.move(tok);

		if (force_compile == ForceCompile::no) {
			auto ret = compiler->return_type();


			// scope pop
			while (compiler->stack()->pop_item(sitm)) { d++; }
			if (d > 0)
				compiler->target()->local_stack_lifetime.pop();
			else
				compiler->target()->local_stack_lifetime.discard_push();

			auto prev_scope = compiler->scope();
			compiler->pop_scope();

			if (termination == BlockTermination::continued) {

				for (auto d = compiler->defer_scope().rbegin(); d != compiler->defer_scope().rend(); d++) {
					ILBuilder::build_call(prev_scope, (*d)->il_function_decl);
					if ((*d)->return_type->rvalue_stacked()) {
						ILBuilder::build_forget(prev_scope, ILDataType::word);
					}
					else {
						ILBuilder::build_forget(prev_scope, (*d)->return_type->rvalue());
					}
				}


				if (!exit_returns) {
					ILBuilder::build_jmp(prev_scope, compiler->scope());
				}
				else {
					if (compiler->return_type() != compiler->types()->t_void) {
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


			compiler->defers.back().pop_back();
			compiler->stack()->pop_block();
		}

		for (auto d = compiler->compile_defer_scope().rbegin(); d != compiler->compile_defer_scope().rend(); d++) {
			ILBuilder::eval_call(compiler->evaluator(), (*d)->il_function_decl);
			if ((*d)->return_type->rvalue_stacked()) {
				ILBuilder::eval_forget(compiler->evaluator(), ILDataType::word);
			}
			else {
				ILBuilder::eval_forget(compiler->evaluator(), (*d)->return_type->rvalue());
			}
		}
		
		compiler->compile_defers.pop_back();
		compiler->stack_pop_block();
		compiler->compiler_stack()->pop_block();
	}

	void Statement::parse(Cursor& c,RecognizedToken& tok, BlockTermination& termination, ForceCompile force_compile) {
		Compiler* compiler = Compiler::current();

		ForceCompile compile = force_compile;
		if (c.buffer() == "compile") {
			if(Statement::runtime(compile)) compile = ForceCompile::single;
			c.move(tok);
		}
		else if (c.buffer() == "runtime") {
			if (!compiler->target()) {
				throw_specific_error(c, "Statement is not compiled");
			}

			if (!Statement::runtime(compile)) compile = ForceCompile::inlineblock;
			c.move(tok);
		}

		auto scope = ScopeState();
		if (!Statement::runtime(compile)) {
			scope.context(ILContext::compile);
		}


		switch (tok) {
			case RecognizedToken::Symbol:
			{
				auto buf = c.buffer();
				if (buf == "return") {
					if (!compiler->target()) {
						throw_specific_error(c, "Statement is not compiled");
					}

					termination = BlockTermination::terminated;
					parse_return(c,tok);
					return; 
				}else if (buf == "make") {
					parse_make(c,tok, compile);
					return;
				}
				else if (buf == "let") {
					parse_let(c,tok, compile);
					return;
				}
				else if (buf == "if") {
					parse_if(c,tok, termination, compile);
					return;
				}
				else if (buf == "while") {
					parse_while(c,tok, termination, compile);
					return;
				}
				else if (buf == "for") {
					parse_for(c,tok, termination, compile);
					return;
				}
				
				else if (buf == "break") {

					if (Statement::runtime(compile)) {

						for (auto d = compiler->defer_scope().rbegin(); d != compiler->defer_scope().rend(); d++) {
							ILBuilder::build_call(compiler->scope(), (*d)->il_function_decl);
							if ((*d)->return_type->rvalue_stacked()) {
								ILBuilder::build_forget(compiler->scope(), ILDataType::word);
							}
							else {
								ILBuilder::build_forget(compiler->scope(), (*d)->return_type->rvalue());
							}
						}

						if (!compiler->has_loop()) {
							throw_specific_error(c, "Break must be called from loop");
						}

						ILBuilder::build_jmp(compiler->scope(), compiler->loop_break());
						termination = BlockTermination::breaked;
					}
					else {
						if (!compiler->compile_in_loop()) {
							throw_specific_error(c, "Break must be called from loop");
						}
						compiler->compile_loop_state() = CompileTimeBlockState::jump_over;
					}

					c.move(tok);
					if (tok != RecognizedToken::Semicolon) {
						throw_wrong_token_error(c, "';'");
					}
					c.move(tok);
					return;
				}
				else if (buf == "continue") {

					if (Statement::runtime(compile)) {

						for (auto d = compiler->defer_scope().rbegin(); d != compiler->defer_scope().rend(); d++) {
							ILBuilder::build_call(compiler->scope(), (*d)->il_function_decl);
							if ((*d)->return_type->rvalue_stacked()) {
								ILBuilder::build_forget(compiler->scope(), ILDataType::word);
							}
							else {
								ILBuilder::build_forget(compiler->scope(), (*d)->return_type->rvalue());
							}
						}

						if (!compiler->has_loop()) {
							throw_specific_error(c, "Continue must be called from loop");
						}

						ILBuilder::build_jmp(compiler->scope(), compiler->loop_continue());
						termination = BlockTermination::breaked;
					}
					else {
						if (!compiler->compile_in_loop()) {
							throw_specific_error(c, "Continue must be called from loop");
						}
						compiler->compile_loop_state() = CompileTimeBlockState::jump_back;
					}

					c.move(tok);
					if (tok != RecognizedToken::Semicolon) {
						throw_wrong_token_error(c, "';'");
					}
					c.move(tok);
					return;
				}
				else if (buf == "defer") {

					c.move(tok);

					if (Statement::runtime(compile)) {
						CompileValue cval;
						Cursor err = c;
						Operand::parse(c, tok, cval, CompileType::compile, true);

						if (cval.type != compiler->types()->t_void || !cval.lvalue) {
							throw_specific_error(err, "Only function call can be defered");
						}
					}
					else {
						CompileValue cval;
						Cursor err = c;
						Operand::parse(c, tok, cval, CompileType::eval, true);

						if (cval.type != compiler->types()->t_void || !cval.lvalue) {
							throw_specific_error(err, "Only function call can be defered");
						}
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
				if (compile == ForceCompile::no) {
					ILBlock* block = compiler->target()->create_and_append_block();

					ILBuilder::build_jmp(compiler->scope(), block);

					ILBlock* continue_block = compiler->target()->create_block();

					compiler->switch_scope(continue_block);
					parse_inner_block_start(block, compile);
					parse_inner_block(c, tok, termination, false, nullptr, compile);
					compiler->target()->append_block(continue_block);
				}
				else {
					parse_inner_block_start(nullptr, compile);
					parse_inner_block(c, tok, termination, false, nullptr, compile);
				}
				return;
			}break;
		}

		//fallthrough if every other check fails, statement is plain expression

		CompileValue ret_val;
		if (Statement::runtime(compile)) {
			Expression::parse(c, tok, ret_val, CompileType::compile, false);
			Operand::deref(ret_val, CompileType::compile);
		}
		else {
			Expression::parse(c, tok, ret_val, CompileType::eval, false);
			Operand::deref(ret_val, CompileType::eval);
		}

		if (tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}
		c.move(tok);
	}

	void Statement::parse_if(Cursor& c,RecognizedToken& tok, BlockTermination& termination, ForceCompile force_compile, bool do_next) {
		Compiler* compiler = Compiler::current();
		ForceCompile compile = force_compile;
		c.move(tok);

		if (Statement::runtime(compile)) {
			CompileValue test_value;
			Cursor err = c;

			Expression::parse(c, tok, test_value, CompileType::compile);
			Operand::deref(test_value, CompileType::compile);
			Operand::cast(err, test_value, compiler->types()->t_bool, CompileType::compile, true);
			Expression::rvalue(test_value, CompileType::compile);

			if (tok != RecognizedToken::OpenBrace) {
				throw_wrong_token_error(c, "'{'");
			}
			c.move(tok);

			ILBlock* block_from = compiler->scope();
			ILBuilder::build_yield(block_from, ILDataType::none);

			ILBlock* continue_block = compiler->target()->create_block();

			compiler->switch_scope(continue_block);

			ILBlock* block = compiler->target()->create_and_append_block();

			BlockTermination term = BlockTermination::continued;
			Statement::parse_inner_block_start(block);
			Statement::parse_inner_block(c, tok, term);

			if (c.buffer() == "else") {
				c.move(tok);

				if (tok == RecognizedToken::OpenBrace) {
					c.move(tok);

					ILBlock* else_block = compiler->target()->create_and_append_block();

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

			compiler->target()->append_block(continue_block);
		}
		else {

			CompileValue test_value;
			Cursor err = c;

			Expression::parse(c, tok, test_value, CompileType::eval);
			Operand::deref(test_value, CompileType::eval);
			Operand::cast(err, test_value, compiler->types()->t_bool, CompileType::eval, true);
			Expression::rvalue(test_value, CompileType::eval);


			bool condition = compiler->evaluator()->pop_register_value<bool>() & do_next;

			if (tok != RecognizedToken::OpenBrace) {
				throw_wrong_token_error(c, "'{'");
			}


			if (condition) {
				condition = false;
				
				auto scope = ScopeState();
				if (compile != ForceCompile::force) scope.context(compiler->target()->context);

				Statement::parse(c, tok, termination, compile == ForceCompile::force ? ForceCompile::force : ForceCompile::inlineblock);
			}
			else {
				condition = do_next;
				c.move_matching(tok);
				c.move(tok);
			}
			

			if (c.buffer() == "else") {
				c.move(tok);
				if (c.buffer() == "if") {
					parse_if(c, tok, termination, compile== ForceCompile::force? ForceCompile::force: ForceCompile::no, condition);
				}
				else {
					if (tok != RecognizedToken::OpenBrace) {
						throw_wrong_token_error(c, "'{'");
					}

					if (condition) {
						
						auto scope = ScopeState();
						if (compile != ForceCompile::force) scope.context(compiler->target()->context);
						Statement::parse(c, tok, termination, compile == ForceCompile::force ? ForceCompile::force : ForceCompile::inlineblock);
						
					}
					else {
						c.move_matching(tok);
						c.move(tok);
					}
				}
			}

		}
	}


	void Statement::parse_while(Cursor& c, RecognizedToken& tok, BlockTermination& termination, ForceCompile force_compile) {
		Compiler* compiler = Compiler::current();

		ForceCompile compile = force_compile;
		c.move(tok);

		if (Statement::runtime(compile)) {
			ILBlock* test_block = compiler->target()->create_and_append_block();

			ILBuilder::build_jmp(compiler->scope(), test_block);
			compiler->switch_scope(test_block);

			CompileValue test_value;
			Cursor err = c;

			Expression::parse(c, tok, test_value, CompileType::compile);
			Operand::deref(test_value, CompileType::compile);
			Operand::cast(err, test_value, compiler->types()->t_bool, CompileType::compile, true);
			Expression::rvalue(test_value, CompileType::compile);

			if (tok != RecognizedToken::OpenBrace) {
				throw_wrong_token_error(c, "'{'");
			}
			c.move(tok);

			ILBlock* continue_block = compiler->target()->create_block();

			compiler->loop_block_stack.push_back(std::make_pair(continue_block, test_block));

			ILBlock* block = compiler->target()->create_and_append_block();
			Statement::parse_inner_block_start(block);
			bool term = false;
			Statement::parse_inner_block(c, tok, termination);

			ILBuilder::build_jmpz(test_block, continue_block, block);

			compiler->loop_block_stack.pop_back();

			compiler->switch_scope(continue_block);

			compiler->target()->append_block(continue_block);
		}
		else {
			Cursor save = c;
			RecognizedToken save_tok = tok;

			bool jump_over_set = false;
			RecognizedToken jump_over_tok;
			Cursor jump_over = c;

			CompileTimeBlockState state = CompileTimeBlockState::run;
			compiler->push_compile_loop_state(state);
			while (state == CompileTimeBlockState::run) {
				c = save;
				tok = save_tok;
				Cursor err = c;
				CompileValue res;
				Expression::parse(c, tok, res, CompileType::eval, true);
				Operand::deref(res, CompileType::eval);
				Operand::cast(err, res, compiler->types()->t_bool, CompileType::eval, true);
				Expression::rvalue(res, CompileType::eval);

				if (tok != RecognizedToken::OpenBrace) {
					throw_wrong_token_error(c, "'{'");
				}

				if (!jump_over_set) {
					jump_over_set = true;
					jump_over_tok = tok;
					jump_over = c;
					jump_over.move_matching(jump_over_tok);
					jump_over.move(jump_over_tok);
				}

				bool condition = compiler->evaluator()->pop_register_value<bool>();
				if (!condition) break;


				{
					auto scope = ScopeState();
					if (compile != ForceCompile::force) scope.context(compiler->target()->context);
					
					Statement::parse(c, tok, termination, compile == ForceCompile::force ? ForceCompile::force : ForceCompile::inlineblock);
				}

				if (termination != BlockTermination::continued) break;
				if (state == CompileTimeBlockState::jump_over) break;
				if (state == CompileTimeBlockState::jump_back) state= CompileTimeBlockState::run;
			}

			compiler->pop_compile_loop_state();
			c = jump_over;
			tok = jump_over_tok;

		}
	}


	void Statement::parse_for(Cursor& c, RecognizedToken& tok, BlockTermination& termination, ForceCompile force_compile) {
		Compiler* compiler = Compiler::current();

		ForceCompile compile = force_compile;
		c.move(tok);

		if (tok != RecognizedToken::OpenParenthesis) {
			throw_wrong_token_error(c, "'('");
		}

		if (Statement::runtime(compile)) {
			compiler->stack()->push_block();


			Cursor cc = c;
			Cursor err = cc;
			RecognizedToken tok2;
			cc.move(tok2);
			Statement::parse(cc, tok2, termination);


			ILBlock* test_block = compiler->target()->create_and_append_block();
			ILBuilder::build_jmp(compiler->scope(), test_block);
			compiler->switch_scope(test_block);

			CompileValue test_value;

			err = cc;
			Expression::parse(cc, tok2, test_value, CompileType::compile);
			Operand::deref(test_value, CompileType::compile);
			Operand::cast(err, test_value, compiler->types()->t_bool, CompileType::compile, true);
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

			ILBlock* continue_block = compiler->target()->create_block();

			ILBlock* increment_block = compiler->target()->create_and_append_block();
			compiler->push_scope(increment_block);

			Expression::parse(cc, tok2, test_value, CompileType::compile, false);
			Operand::deref(test_value, CompileType::compile);
			ILBuilder::build_jmp(increment_block, test_block);

			
			compiler->loop_block_stack.push_back(std::make_pair(continue_block, increment_block));

			ILBlock* block = compiler->target()->create_and_append_block();

			Statement::parse_inner_block_start(block);
			bool term = false;
			Statement::parse_inner_block(c, tok, termination);
			if (termination == BlockTermination::breaked) termination = BlockTermination::continued;


			compiler->loop_block_stack.pop_back();

			compiler->pop_scope();

			ILBuilder::build_jmpz(test_block, continue_block, block);

			compiler->switch_scope(continue_block);

			compiler->target()->append_block(continue_block);
		}
		else {
			RecognizedToken statement_tok = tok;
			Cursor statement = c;
			statement.move_matching(statement_tok);
			statement.move(statement_tok);

			if (statement_tok != RecognizedToken::OpenBrace) {
				throw_wrong_token_error(statement, "'{'");
			}

			compiler->compiler_stack()->push_block();
			Cursor c_init = c;
			RecognizedToken c_init_tok;
			c_init.move(c_init_tok);
			BlockTermination unused;
			Statement::parse(c_init, c_init_tok, unused, ForceCompile::force);

			CompileTimeBlockState state = CompileTimeBlockState::run;
			compiler->push_compile_loop_state(state);

			while (state == CompileTimeBlockState::run) {
				RecognizedToken c_condition_tok = c_init_tok;
				Cursor c_condition = c_init;
				Cursor err = c_condition;
				CompileValue res;
				Expression::parse(c_condition, c_condition_tok, res, CompileType::eval);
				Operand::deref(res, CompileType::eval);
				Operand::cast(err, res, compiler->types()->t_bool, CompileType::eval, true);
				Expression::rvalue(res, CompileType::eval);

				bool condition = compiler->evaluator()->pop_register_value<bool>();
				if (!condition) break;


				Cursor statement_copy = statement;
				RecognizedToken statement_tok_copy = statement_tok;

				{
					auto scope = ScopeState();
					if (compile != ForceCompile::force) scope.context(compiler->target()->context);
					Statement::parse(statement_copy, statement_tok_copy, termination, compile == ForceCompile::force ? ForceCompile::force : ForceCompile::inlineblock);
				}

				if (termination == BlockTermination::terminated) break;
				if (state == CompileTimeBlockState::jump_over) break;
				if (state == CompileTimeBlockState::jump_back) state = CompileTimeBlockState::run;

				if (c_condition_tok != RecognizedToken::Semicolon) {
					throw_wrong_token_error(c_condition, "';'");
				}
				c_condition.move(c_condition_tok);
				Expression::parse(c_condition, c_condition_tok, res, CompileType::eval,false);
			}
			compiler->pop_compile_loop_state();

			compiler->compiler_stack()->pop_block();

			c = statement;
			tok = statement_tok;
			c.move_matching(tok);
			c.move(tok);
		}
		
	}




	void Statement::parse_return(Cursor& c, RecognizedToken& tok) {
		Compiler* compiler = Compiler::current();

		bool has_defer = false;
		for (auto ds = compiler->defer_function().rbegin(); ds != compiler->defer_function().rend(); ds++) {
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

		Type* to = compiler->return_type();
		Operand::cast(err, ret_val, to, CompileType::compile, false);
		Expression::rvalue(ret_val, CompileType::compile);

		if (compiler->return_type()->rvalue_stacked()) {
			ILBuilder::build_local(compiler->scope(), 0);
			ILBuilder::build_load(compiler->scope(), ILDataType::word);
			Expression::copy_from_rvalue(compiler->return_type(), CompileType::compile);
		}
		else {
			if (has_defer) {
				ILBuilder::build_yield(compiler->scope(), compiler->return_type()->rvalue());
			}
		}

		

		for (auto ds = compiler->defer_function().rbegin(); ds != compiler->defer_function().rend(); ds++) {
			for (auto d = ds->rbegin(); d != ds->rend(); d++) {
				ILBuilder::build_call(compiler->scope(), (*d)->il_function_decl);
				if ((*d)->return_type->rvalue_stacked()) {
					ILBuilder::build_forget(compiler->scope(), ILDataType::word);
				}
				else {
					ILBuilder::build_forget(compiler->scope(), (*d)->return_type->rvalue());
				}
			}
		}


		if (compiler->return_type()->rvalue_stacked()) {
			ILBuilder::build_ret(compiler->scope(), ILDataType::none);
		}
		else {

			if (has_defer) {
				ILBlock* exit_block = compiler->target()->create_and_append_block();
				ILBuilder::build_jmp(compiler->scope(), exit_block);
				compiler->switch_scope(exit_block);
				ILBuilder::build_accept(exit_block, compiler->return_type()->rvalue());
			}

			ILBuilder::build_ret(compiler->scope(), compiler->return_type()->rvalue());
		}


		if (tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}

		c.move(tok);
	}

	void Statement::parse_let(Cursor& c, RecognizedToken& tok, ForceCompile force_compile) {
		Compiler* compiler = Compiler::current();
		ForceCompile compile = force_compile;
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

		if (Statement::runtime(compile)) {
			size_t let_holder;
			stackid_t local_id = compiler->target()->local_stack_lifetime.append_unknown(let_holder);

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

			if (new_t->context() != ILContext::both && compiler->scope_context() != new_t->context()) {
				throw_specific_error(err, "Type was not designed for this context");
			}

			if (reference) {
				new_t = new_t->generate_reference();
			}

			compiler->target()->local_stack_lifetime.resolve_unknown(let_holder, new_t->size());
			compiler->stack()->push_item(name.buffer(), new_t, local_id);


			ILBuilder::build_local(compiler->scope(), local_id);
			Expression::copy_from_rvalue(new_t, CompileType::compile);
		}
		else {

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

			if (new_t->context() != ILContext::both && compiler->scope_context() != new_t->context()) {
				throw_specific_error(err, "Type was not designed for this context");
			}

			if (reference) {
				new_t = new_t->generate_reference();
			}

			stackid_t loc_id = compiler->push_local(new_t->size());
			uint8_t* loc_ptr = compiler->stack_ptr(loc_id);

			compiler->compiler_stack()->push_item(name.buffer(), new_t, loc_id);

			ILBuilder::eval_const_word(compiler->evaluator(), loc_ptr);
			Expression::copy_from_rvalue(new_t, CompileType::eval);
		}
		
		if (tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}
		c.move(tok);
	}

	void Statement::parse_make(Cursor& c, RecognizedToken& tok, ForceCompile force_compile) {
		Compiler* compiler = Compiler::current();

		ForceCompile compile = force_compile;
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
		if (Statement::runtime(compile)) {
			Cursor err = c;
			CompileValue val;

			{
				auto scope = ScopeState().context(ILContext::compile);


				Expression::parse(c, tok, val, CompileType::eval);
				Operand::deref(val, CompileType::eval);
				Expression::rvalue(val, CompileType::eval);

				if (val.type != compiler->types()->t_type) {
					throw_specific_error(err, "Exprected type");
				}

			}

			Type* new_t = compiler->evaluator()->pop_register_value<Type*>();
			new_t->compile();

			if (new_t->context() != ILContext::both && compiler->scope_context() != new_t->context()) {
				throw_specific_error(err, "Type was not designed for this context");
			}

			ILSize s = new_t->size();
			stackid_t local_id = compiler->target()->local_stack_lifetime.append(s);
			compiler->stack()->push_item(name.buffer(), new_t, local_id);
		}
		else {
			Cursor err = c;
			CompileValue val;


			Expression::parse(c, tok, val, CompileType::eval);
			Operand::deref(val, CompileType::eval);
			Expression::rvalue(val, CompileType::eval);

			if (val.type != compiler->types()->t_type) {
				throw_specific_error(err, "Exprected type");
			}

			Type* new_t = compiler->evaluator()->pop_register_value<Type*>();
			new_t->compile();

			if (new_t->context() != ILContext::both && compiler->scope_context() != new_t->context()) {
				throw_specific_error(err, "Type was not designed for this context");
			}

			ILSize s = new_t->size();
			stackid_t local_id = compiler->push_local(s);
			compiler->compiler_stack()->push_item(name.buffer(), new_t, local_id);
		}

		if (tok != RecognizedToken::Semicolon) {
			throw_wrong_token_error(c, "';'");
		}
		c.move(tok);
	}
}