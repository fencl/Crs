#include "Statement.hpp"
#include "Expression.hpp"
#include "Operand.hpp"
#include "Error.hpp"
#include "StackManager.hpp"
#include <iostream>

namespace Corrosive {

	errvoid Statement::parse_inner_block_start(ILBlock* block, ForceCompile force_compile) {
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
		return err::ok;
	}

	errvoid Statement::parse_inner_block(Cursor& c, BlockTermination& termination, bool exit_returns, Cursor* err, ForceCompile force_compile) {
		Compiler* compiler = Compiler::current();

		StackItem sitm;
		termination = BlockTermination::continued;

		int d = 0;
		std::size_t line = 0;

		while (c.tok != RecognizedToken::CloseBrace) {
			if (force_compile == ForceCompile::no || force_compile == ForceCompile::inlineblock)
				compiler->temp_stack()->push();

			
			if (c.y != line) {
				if (Statement::runtime(force_compile)) {
					if (c.src != nullptr) {
						if (!ILBuilder::build_debug(compiler->scope(), c.src->debug_id, (std::uint16_t)c.y)) return err::fail;
					}
					else {
						if (!ILBuilder::build_debug(compiler->scope(), UINT16_MAX, (std::uint16_t)c.y)) return err::fail;
					}
				} else {
					if (c.src != nullptr) {
						if (!ILBuilder::eval_debug(c.src->debug_id, (std::uint16_t)c.y)) return err::fail;
					}
					else {
						if (!ILBuilder::eval_debug(UINT16_MAX, (std::uint16_t)c.y)) return err::fail;
					}
				}
				line = c.y;
			}

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

			if (!Statement::parse(c, termination, new_fc)) return err::fail;

			if (termination != BlockTermination::continued && c.tok != RecognizedToken::CloseBrace) {
				return throw_specific_error(c, "Instruction after the current branch has been terminated");
			}

			if (Statement::runtime(force_compile)) {

				while (compiler->temp_stack()->pop_item(sitm)) { d++; }

				compiler->temp_stack()->pop();
			}

			if (compiler->compile_in_loop()) {
				if (compiler->compile_loop_state() != CompileTimeBlockState::run) break;
			}
		}
		c.move();

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
					if (!ILBuilder::build_call(prev_scope, (*d)->il_function_decl)) return err::fail;
					if ((*d)->return_type->rvalue_stacked()) {
						if (!ILBuilder::build_forget(prev_scope, ILDataType::word)) return err::fail;
					}
					else {
						if (!ILBuilder::build_forget(prev_scope, (*d)->return_type->rvalue())) return err::fail;
					}
				}


				if (!exit_returns) {
					if (!ILBuilder::build_jmp(prev_scope, compiler->scope())) return err::fail;
				}
				else {
					if (compiler->return_type() != compiler->types()->t_void) {
						if (err != nullptr) {
							return throw_specific_error(*err, "Function does not always return value");
						}
						else {
							throw string_exception("Compiler error, Function does not always return value, error cursor not provided");
						}
					}
					else {
						if (!ILBuilder::build_ret(prev_scope, ILDataType::none)) return err::fail;
					}
				}
			}


			compiler->defers.back().pop_back();
			compiler->stack()->pop_block();
		}

		for (auto d = compiler->compile_defer_scope().rbegin(); d != compiler->compile_defer_scope().rend(); d++) {
			if (!ILBuilder::eval_call((*d)->il_function_decl)) return err::fail;
			if ((*d)->return_type->rvalue_stacked()) {
				if (!ILBuilder::eval_forget(ILDataType::word)) return err::fail;
			}
			else {
				if (!ILBuilder::eval_forget((*d)->return_type->rvalue())) return err::fail;
			}
		}
		
		compiler->compile_defers.pop_back();
		compiler->stack_pop_block();
		compiler->compiler_stack()->pop_block();
		return err::ok;
	}

	errvoid Statement::parse(Cursor& c, BlockTermination& termination, ForceCompile force_compile) {
		Compiler* compiler = Compiler::current();

		ForceCompile compile = force_compile;
		if (c.buffer() == "compile") {
			if(Statement::runtime(compile)) compile = ForceCompile::single;
			c.move();
		}
		else if (c.buffer() == "runtime") {
			if (!compiler->target()) {
				return throw_specific_error(c, "Statement is not compiled");
			}

			if (!Statement::runtime(compile)) compile = ForceCompile::inlineblock;
			c.move();
		}

		auto scope = ScopeState();
		if (!Statement::runtime(compile)) {
			scope.context(ILContext::compile);
		}


		switch (c.tok) {
			case RecognizedToken::Symbol:
			{
				auto buf = c.buffer();
				if (buf == "return") {
					if (!compiler->target()) {
						return throw_specific_error(c, "Statement is not compiled");
					}

					termination = BlockTermination::terminated;
					if (!Statement::parse_return(c)) return err::fail;
					return err::ok; 
				}else if (buf == "make") {
					if (!Statement::parse_make(c, compile)) return err::fail;
					return err::ok;
				}
				else if (buf == "let") {
					if (!Statement::parse_let(c, compile)) return err::fail;
					return err::ok;
				}
				else if (buf == "if") {
					if (!Statement::parse_if(c, termination, compile)) return err::fail;
					return err::ok;
				}
				else if (buf == "while") {
					if (!Statement::parse_while(c, termination, compile)) return err::fail;
					return err::ok;
				}
				else if (buf == "for") {
					if (!Statement::parse_for(c, termination, compile)) return err::fail;
					return err::ok;
				}
				else if (buf == "panic") {
					return throw_specific_error(c,"Compilation was manually halted.");
				}
				else if (buf == "break") {

					if (Statement::runtime(compile)) {

						for (auto d = compiler->defer_scope().rbegin(); d != compiler->defer_scope().rend(); d++) {
							if (!ILBuilder::build_call(compiler->scope(), (*d)->il_function_decl)) return err::fail;
							if ((*d)->return_type->rvalue_stacked()) {
								if (!ILBuilder::build_forget(compiler->scope(), ILDataType::word)) return err::fail;
							}
							else {
								if (!ILBuilder::build_forget(compiler->scope(), (*d)->return_type->rvalue())) return err::fail;
							}
						}

						if (!compiler->has_loop()) {
							return throw_specific_error(c, "Break must be called from loop");
						}

						if (!ILBuilder::build_jmp(compiler->scope(), compiler->loop_break())) return err::fail;
						termination = BlockTermination::breaked;
					}
					else {
						if (!compiler->compile_in_loop()) {
							return throw_specific_error(c, "Break must be called from loop");
						}
						compiler->compile_loop_state() = CompileTimeBlockState::jump_over;
					}

					c.move();
					if (c.tok != RecognizedToken::Semicolon) {
						return throw_wrong_token_error(c, "';'");
					}
					c.move();
					return err::ok;
				}
				else if (buf == "continue") {

					if (Statement::runtime(compile)) {

						for (auto d = compiler->defer_scope().rbegin(); d != compiler->defer_scope().rend(); d++) {
							if (!ILBuilder::build_call(compiler->scope(), (*d)->il_function_decl)) return err::fail;
							if ((*d)->return_type->rvalue_stacked()) {
								if (!ILBuilder::build_forget(compiler->scope(), ILDataType::word)) return err::fail;
							}
							else {
								if (!ILBuilder::build_forget(compiler->scope(), (*d)->return_type->rvalue())) return err::fail;
							}
						}

						if (!compiler->has_loop()) {
							return throw_specific_error(c, "Continue must be called from loop");
						}

						if (!ILBuilder::build_jmp(compiler->scope(), compiler->loop_continue())) return err::fail;
						termination = BlockTermination::breaked;
					}
					else {
						if (!compiler->compile_in_loop()) {
							return throw_specific_error(c, "Continue must be called from loop");
						}
						compiler->compile_loop_state() = CompileTimeBlockState::jump_back;
					}

					c.move();
					if (c.tok != RecognizedToken::Semicolon) {
						return throw_wrong_token_error(c, "';'");
					}
					c.move();
					return err::ok;
				}
				else if (buf == "defer") {
					c.move();

					if (Statement::runtime(compile)) {
						CompileValue cval;
						Cursor err = c;
						if (!Operand::parse(c, cval, CompileType::compile, true)) return err::fail;

						if (cval.type != compiler->types()->t_void || !cval.lvalue) {
							return throw_specific_error(err, "Only function call can be defered");
						}
					}
					else {
						CompileValue cval;
						Cursor err = c;
						if (!Operand::parse(c, cval, CompileType::eval, true)) return err::fail;

						if (cval.type != compiler->types()->t_void || !cval.lvalue) {
							return throw_specific_error(err, "Only function call can be defered");
						}
					}

					if (c.tok != RecognizedToken::Semicolon) {
						return throw_wrong_token_error(c, "';'");
					}
					c.move();
					return err::ok;
				}

			}break;
			case RecognizedToken::OpenBrace: {
				c.move();
				if (compile == ForceCompile::no) {
					ILBlock* block = compiler->target()->create_and_append_block();

					if (!ILBuilder::build_jmp(compiler->scope(), block)) return err::fail;

					ILBlock* continue_block = compiler->target()->create_block();

					compiler->switch_scope(continue_block);
					if (!Statement::parse_inner_block_start(block, compile)) return err::fail;
					if (!Statement::parse_inner_block(c, termination, false, nullptr, compile)) return err::fail;
					compiler->target()->append_block(continue_block);
				}
				else {
					if (!Statement::parse_inner_block_start(nullptr, compile)) return err::fail;
					if (!Statement::parse_inner_block(c, termination, false, nullptr, compile)) return err::fail;
				}
				return err::ok;
			}break;
		}

		//fallthrough if every other check fails, statement is plain expression

		CompileValue ret_val;
		if (Statement::runtime(compile)) {
			if (!Expression::parse(c, ret_val, CompileType::compile, false)) return err::fail;
			if (!Operand::deref(ret_val, CompileType::compile)) return err::fail;
		}
		else {
			if (!Expression::parse(c,ret_val, CompileType::eval, false)) return err::fail;
			if (!Operand::deref(ret_val, CompileType::eval)) return err::fail;
		}

		if (c.tok != RecognizedToken::Semicolon) {
			return throw_wrong_token_error(c, "';'");
		}
		c.move();
		return err::ok;
	}

	errvoid Statement::parse_if(Cursor& c, BlockTermination& termination, ForceCompile force_compile, bool do_next) {
		Compiler* compiler = Compiler::current();
		ForceCompile compile = force_compile;
		c.move();

		if (Statement::runtime(compile)) {
			CompileValue test_value;
			Cursor err = c;

			if (!Expression::parse(c, test_value, CompileType::compile)) return err::fail;
			if (!Operand::deref(test_value, CompileType::compile)) return err::fail;
			if (!Operand::cast(err, test_value, compiler->types()->t_bool, CompileType::compile, true)) return err::fail;
			if (!Expression::rvalue(test_value, CompileType::compile)) return err::fail;

			if (c.tok != RecognizedToken::OpenBrace) {
				return throw_wrong_token_error(c, "'{'");
			}
			c.move();

			ILBlock* block_from = compiler->scope();
			if (!ILBuilder::build_yield(block_from, ILDataType::none)) return err::fail;

			ILBlock* continue_block = compiler->target()->create_block();

			compiler->switch_scope(continue_block);

			ILBlock* block = compiler->target()->create_and_append_block();

			BlockTermination term = BlockTermination::continued;
			if (!Statement::parse_inner_block_start(block)) return err::fail;
			if (!Statement::parse_inner_block(c, term)) return err::fail;

			if (c.buffer() == "else") {
				c.move();

				if (c.tok == RecognizedToken::OpenBrace) {
					c.move();

					ILBlock* else_block = compiler->target()->create_and_append_block();

					BlockTermination term2 = BlockTermination::continued;
					if (!Statement::parse_inner_block_start(else_block)) return err::fail;
					if (!Statement::parse_inner_block(c, term2)) return err::fail;

					if (term == BlockTermination::terminated && term2 == BlockTermination::terminated) { termination = BlockTermination::terminated; }
					else { termination = BlockTermination::continued; }

					if (!ILBuilder::build_jmpz(block_from, else_block, block)) return err::fail;
				}
				else if (c.buffer() == "if") {
					BlockTermination term2 = BlockTermination::continued;
					if (!Statement::parse_if(c, term2, force_compile)) return err::fail;

					if (term == BlockTermination::terminated && term2 == BlockTermination::terminated) { termination = BlockTermination::terminated; }
					else { termination = BlockTermination::continued; }

					if (!ILBuilder::build_jmpz(block_from, continue_block, block)) return err::fail;
				}
				else {
					termination = term;
					return throw_specific_error(c, "else can be followed only by { or if");
				}
			}
			else {
				if (!ILBuilder::build_jmpz(block_from, continue_block, block)) return err::fail;
			}

			compiler->target()->append_block(continue_block);
		}
		else {

			CompileValue test_value;
			Cursor err = c;

			if (!Expression::parse(c, test_value, CompileType::eval)) return err::fail;
			if (!Operand::deref(test_value, CompileType::eval)) return err::fail;
			if (!Operand::cast(err, test_value, compiler->types()->t_bool, CompileType::eval, true)) return err::fail;
			if (!Expression::rvalue(test_value, CompileType::eval)) return err::fail;


			bool condition;
			if (!compiler->evaluator()->pop_register_value<bool>(condition)) return err::fail;

			condition &= do_next;

			if (c.tok != RecognizedToken::OpenBrace) {
				return throw_wrong_token_error(c, "'{'");
			}


			if (condition) {
				condition = false;
				
				auto scope = ScopeState();
				if (compile != ForceCompile::force) scope.context(compiler->target()->context);

				if (!Statement::parse(c, termination, compile == ForceCompile::force ? ForceCompile::force : ForceCompile::inlineblock)) return err::fail;
			}
			else {
				condition = do_next;
				c.move_matching();
				c.move();
			}
			

			if (c.buffer() == "else") {
				c.move();
				if (c.buffer() == "if") {
					if (!parse_if(c, termination, compile== ForceCompile::force? ForceCompile::force: ForceCompile::no, condition)) return err::fail;
				}
				else {
					if (c.tok != RecognizedToken::OpenBrace) {
						return throw_wrong_token_error(c, "'{'");
					}

					if (condition) {
						auto scope = ScopeState();
						if (compile != ForceCompile::force) scope.context(compiler->target()->context);

						if (!Statement::parse(c, termination, compile == ForceCompile::force ? ForceCompile::force : ForceCompile::inlineblock)) return err::fail;
					}
					else {
						c.move_matching();
						c.move();
					}
				}
			}

		}
		return err::ok;
	}


	errvoid Statement::parse_while(Cursor& c, BlockTermination& termination, ForceCompile force_compile) {
		Compiler* compiler = Compiler::current();

		ForceCompile compile = force_compile;
		c.move();

		if (Statement::runtime(compile)) {
			ILBlock* test_block = compiler->target()->create_and_append_block();

			if (!ILBuilder::build_jmp(compiler->scope(), test_block)) return err::fail;
			compiler->switch_scope(test_block);

			CompileValue test_value;
			Cursor err = c;

			if (!Expression::parse(c, test_value, CompileType::compile)) return err::fail;
			if (!Operand::deref(test_value, CompileType::compile)) return err::fail;
			if (!Operand::cast(err, test_value, compiler->types()->t_bool, CompileType::compile, true)) return err::fail;
			if (!Expression::rvalue(test_value, CompileType::compile)) return err::fail;

			if (c.tok != RecognizedToken::OpenBrace) {
				return throw_wrong_token_error(c, "'{'");
			}
			c.move();

			ILBlock* continue_block = compiler->target()->create_block();

			compiler->loop_block_stack.push_back(std::make_pair(continue_block, test_block));

			ILBlock* block = compiler->target()->create_and_append_block();
			if (!Statement::parse_inner_block_start(block)) return err::fail;
			bool term = false;
			if (!Statement::parse_inner_block(c, termination)) return err::fail;

			if (!ILBuilder::build_jmpz(test_block, continue_block, block)) return err::fail;

			compiler->loop_block_stack.pop_back();

			compiler->switch_scope(continue_block);

			compiler->target()->append_block(continue_block);
		}
		else {
			Cursor save = c;
			bool jump_over_set = false;
			Cursor jump_over = c;

			CompileTimeBlockState state = CompileTimeBlockState::run;
			compiler->push_compile_loop_state(state);
			while (state == CompileTimeBlockState::run) {
				c = save;
				Cursor err = c;
				CompileValue res;
				if (!Expression::parse(c, res, CompileType::eval, true)) return err::fail;
				if (!Operand::deref(res, CompileType::eval)) return err::fail;
				if (!Operand::cast(err, res, compiler->types()->t_bool, CompileType::eval, true)) return err::fail;
				if (!Expression::rvalue(res, CompileType::eval)) return err::fail;

				if (c.tok != RecognizedToken::OpenBrace) {
					return throw_wrong_token_error(c, "'{'");
				}

				if (!jump_over_set) {
					jump_over_set = true;
					jump_over = c;
					jump_over.move_matching();
					jump_over.move();
				}

				bool condition;
				if (!compiler->evaluator()->pop_register_value<bool>(condition)) return err::fail;
				if (!condition) break;


				{
					auto scope = ScopeState();
					if (compile != ForceCompile::force) scope.context(compiler->target()->context);
					
					if (!Statement::parse(c, termination, compile == ForceCompile::force ? ForceCompile::force : ForceCompile::inlineblock)) return err::fail;
				}

				if (termination != BlockTermination::continued) break;
				if (state == CompileTimeBlockState::jump_over) break;
				if (state == CompileTimeBlockState::jump_back) state= CompileTimeBlockState::run;
			}

			compiler->pop_compile_loop_state();
			c = jump_over;

		}
		return err::ok;
	}


	errvoid Statement::parse_for(Cursor& c, BlockTermination& termination, ForceCompile force_compile) {
		Compiler* compiler = Compiler::current();

		ForceCompile compile = force_compile;
		c.move();

		if (c.tok != RecognizedToken::OpenParenthesis) {
			return throw_wrong_token_error(c, "'('");
		}

		if (Statement::runtime(compile)) {
			compiler->stack()->push_block();


			Cursor cc = c;
			Cursor err = cc;
			cc.move();
			if (!Statement::parse(cc, termination)) return err::fail;


			ILBlock* test_block = compiler->target()->create_and_append_block();
			if (!ILBuilder::build_jmp(compiler->scope(), test_block)) return err::fail;
			compiler->switch_scope(test_block);

			CompileValue test_value;

			err = cc;
			if (!Expression::parse(cc, test_value, CompileType::compile)) return err::fail;
			if (!Operand::deref(test_value, CompileType::compile)) return err::fail;
			if (!Operand::cast(err, test_value, compiler->types()->t_bool, CompileType::compile, true)) return err::fail;
			if (!Expression::rvalue(test_value, CompileType::compile)) return err::fail;

			if (cc.tok != RecognizedToken::Semicolon) {
				return throw_wrong_token_error(c, "';'");
			}
			cc.move();


			c.move_matching();
			c.move();
			if (c.tok != RecognizedToken::OpenBrace) {
				return throw_wrong_token_error(c, "'{'");
			}
			c.move();

			ILBlock* continue_block = compiler->target()->create_block();

			ILBlock* increment_block = compiler->target()->create_and_append_block();
			compiler->push_scope(increment_block);

			if (!Expression::parse(cc, test_value, CompileType::compile, false)) return err::fail;
			if (!Operand::deref(test_value, CompileType::compile)) return err::fail;
			if (!ILBuilder::build_jmp(increment_block, test_block)) return err::fail;

			
			compiler->loop_block_stack.push_back(std::make_pair(continue_block, increment_block));

			ILBlock* block = compiler->target()->create_and_append_block();

			if (!Statement::parse_inner_block_start(block)) return err::fail;
			bool term = false;
			if (!Statement::parse_inner_block(c, termination)) return err::fail;
			if (termination == BlockTermination::breaked) termination = BlockTermination::continued;


			compiler->loop_block_stack.pop_back();

			compiler->pop_scope();

			if (!ILBuilder::build_jmpz(test_block, continue_block, block)) return err::fail;

			compiler->switch_scope(continue_block);

			compiler->target()->append_block(continue_block);
		}
		else {
			Cursor statement = c;
			statement.move_matching();
			statement.move();

			if (statement.tok != RecognizedToken::OpenBrace) {
				return throw_wrong_token_error(statement, "'{'");
			}

			compiler->compiler_stack()->push_block();
			Cursor c_init = c;
			c_init.move();
			BlockTermination unused;
			if (!Statement::parse(c_init, unused, ForceCompile::force)) return err::fail;

			CompileTimeBlockState state = CompileTimeBlockState::run;
			compiler->push_compile_loop_state(state);

			while (state == CompileTimeBlockState::run) {
				Cursor c_condition = c_init;
				Cursor err = c_condition;
				CompileValue res;
				if (!Expression::parse(c_condition, res, CompileType::eval)) return err::fail;
				if (!Operand::deref(res, CompileType::eval)) return err::fail;
				if (!Operand::cast(err, res, compiler->types()->t_bool, CompileType::eval, true)) return err::fail;
				if (!Expression::rvalue(res, CompileType::eval)) return err::fail;

				bool condition;
				if (!compiler->evaluator()->pop_register_value<bool>(condition)) return err::fail;
				if (!condition) break;


				Cursor statement_copy = statement;

				{
					auto scope = ScopeState();
					if (compile != ForceCompile::force) scope.context(compiler->target()->context);
					if (!Statement::parse(statement_copy, termination, compile == ForceCompile::force ? ForceCompile::force : ForceCompile::inlineblock)) return err::fail;
				}

				if (termination == BlockTermination::terminated) break;
				if (state == CompileTimeBlockState::jump_over) break;
				if (state == CompileTimeBlockState::jump_back) state = CompileTimeBlockState::run;

				if (c_condition.tok != RecognizedToken::Semicolon) {
					return throw_wrong_token_error(c_condition, "';'");
				}
				c_condition.move();
				if (!Expression::parse(c_condition, res, CompileType::eval,false)) return err::fail;
			}
			compiler->pop_compile_loop_state();

			compiler->compiler_stack()->pop_block();

			c = statement;
			c.move_matching();
			c.move();
		}
		return err::ok;
		
	}




	errvoid Statement::parse_return(Cursor& c) {
		Compiler* compiler = Compiler::current();

		bool has_defer = false;
		for (auto ds = compiler->defer_function().rbegin(); ds != compiler->defer_function().rend(); ds++) {
			for (auto d = ds->rbegin(); d != ds->rend(); d++) {
				has_defer = true;
				break;
			}
			if (has_defer) break;
		}

		c.move();
		CompileValue ret_val;
		Cursor err = c;
		Type* to = compiler->return_type();

		if (to != compiler->types()->t_void) {
			if (!Expression::parse(c, ret_val, CompileType::compile)) return err::fail;
			if (!Operand::deref(ret_val, CompileType::compile)) return err::fail;
			if (!Operand::cast(err, ret_val, to, CompileType::compile, false)) return err::fail;
			if (!Expression::rvalue(ret_val, CompileType::compile)) return err::fail;
		}

		if (compiler->return_type()->rvalue_stacked()) {
			if (!ILBuilder::build_local(compiler->scope(), 0)) return err::fail;
			if (!ILBuilder::build_load(compiler->scope(), ILDataType::word)) return err::fail;
			if (!Expression::copy_from_rvalue(compiler->return_type(), CompileType::compile)) return err::fail;
		}
		else {
			if (has_defer) {
				if (!ILBuilder::build_yield(compiler->scope(), compiler->return_type()->rvalue())) return err::fail;
			}
		}

		

		for (auto ds = compiler->defer_function().rbegin(); ds != compiler->defer_function().rend(); ds++) {
			for (auto d = ds->rbegin(); d != ds->rend(); d++) {
				if (!ILBuilder::build_call(compiler->scope(), (*d)->il_function_decl)) return err::fail;
				if ((*d)->return_type->rvalue_stacked()) {
					if (!ILBuilder::build_forget(compiler->scope(), ILDataType::word)) return err::fail;
				}
				else {
					if (!ILBuilder::build_forget(compiler->scope(), (*d)->return_type->rvalue())) return err::fail;
				}
			}
		}


		if (compiler->return_type()->rvalue_stacked()) {
			if (!ILBuilder::build_ret(compiler->scope(), ILDataType::none)) return err::fail;
		}
		else {

			if (has_defer) {
				ILBlock* exit_block = compiler->target()->create_and_append_block();
				if (!ILBuilder::build_jmp(compiler->scope(), exit_block)) return err::fail;
				compiler->switch_scope(exit_block);
				if (!ILBuilder::build_accept(exit_block, compiler->return_type()->rvalue())) return err::fail;
			}

			if (!ILBuilder::build_ret(compiler->scope(), compiler->return_type()->rvalue())) return err::fail;
		}


		if (c.tok != RecognizedToken::Semicolon) {
			return throw_wrong_token_error(c, "';'");
		}

		c.move();
		return err::ok;
	}

	errvoid Statement::parse_let(Cursor& c, ForceCompile force_compile) {
		Compiler* compiler = Compiler::current();
		ForceCompile compile = force_compile;
		c.move();

		bool reference = false;
		if (c.tok == RecognizedToken::And) {
			reference = true;
			c.move();
		}

		if (c.tok != RecognizedToken::Symbol) {
			return throw_not_a_name_error(c);
		}
		Cursor name = c;
		c.move();

		if (!reference) {
			if (c.tok != RecognizedToken::Equals) {
				return throw_wrong_token_error(c, "'='");
			}
		}
		else {
			if (c.tok != RecognizedToken::Equals) {
				return throw_wrong_token_error(c, "'='");
			}
		}
		
		c.move();

		if (Statement::runtime(compile)) {
			std::size_t let_holder;
			stackid_t local_id = compiler->target()->local_stack_lifetime.append_unknown(let_holder);

			Cursor err = c;
			CompileValue val;
			if (!Expression::parse(c, val, CompileType::compile)) return err::fail;
			if (!Operand::deref(val, CompileType::compile)) return err::fail;

			if (!reference) {
				if (!Expression::rvalue(val, CompileType::compile)) return err::fail;
			}
			else {
				if (!val.lvalue) {
					return throw_specific_error(err, "Cannot create reference to rvalue");
				}
			}
			Type* new_t = val.type;
			if (!new_t->compile()) return err::fail;

			if (new_t->context() != ILContext::both && compiler->scope_context() != new_t->context()) {
				return throw_specific_error(err, "Type was not designed for this context");
			}

			if (reference) {
				new_t = new_t->generate_reference();
			}

			compiler->target()->local_stack_lifetime.resolve_unknown(let_holder, new_t->size());
			compiler->stack()->push_item(name.buffer(), new_t, local_id);


			if (!ILBuilder::build_local(compiler->scope(), local_id)) return err::fail;
			if (!Expression::copy_from_rvalue(new_t, CompileType::compile)) return err::fail;
		}
		else {

			Cursor err = c;
			CompileValue val;
			if (!Expression::parse(c, val, CompileType::eval)) return err::fail;
			if (!Operand::deref(val, CompileType::eval)) return err::fail;

			if (!reference) {
				if (!Expression::rvalue(val, CompileType::eval)) return err::fail;
			}
			else {
				if (!val.lvalue) {
					return throw_specific_error(err, "Cannot create reference to rvalue");
				}
			}
			Type* new_t = val.type;
			if (!new_t->compile()) return err::fail;

			if (new_t->context() != ILContext::both && compiler->scope_context() != new_t->context()) {
				return throw_specific_error(err, "Type was not designed for this context");
			}

			if (reference) {
				new_t = new_t->generate_reference();
			}

			stackid_t loc_id = compiler->push_local(new_t->size());
			std::uint8_t* loc_ptr = compiler->stack_ptr(loc_id);

			compiler->compiler_stack()->push_item(name.buffer(), new_t, loc_id);

			if (!ILBuilder::eval_const_word(loc_ptr)) return err::fail;
			if (!Expression::copy_from_rvalue(new_t, CompileType::eval)) return err::fail;
		}
		
		if (c.tok != RecognizedToken::Semicolon) {
			return throw_wrong_token_error(c, "';'");
		}
		c.move();
		return err::ok;
	}

	errvoid Statement::parse_make(Cursor& c, ForceCompile force_compile) {
		Compiler* compiler = Compiler::current();

		ForceCompile compile = force_compile;
		c.move();

		if (c.tok != RecognizedToken::Symbol) {
			return throw_not_a_name_error(c);
		}
		Cursor name = c;
		c.move();
		if (c.tok != RecognizedToken::Colon) {
			return throw_wrong_token_error(c, "':'");
		}
		c.move();
		if (Statement::runtime(compile)) {
			Cursor err = c;
			CompileValue val;

			{
				auto scope = ScopeState().context(ILContext::compile);


				if (!Expression::parse(c, val, CompileType::eval)) return err::fail;
				if (!Operand::deref(val, CompileType::eval)) return err::fail;
				if (!Expression::rvalue(val, CompileType::eval)) return err::fail;

				if (val.type != compiler->types()->t_type) {
					return throw_specific_error(err, "Exprected type");
				}

			}

			Type* new_t;
			if (!compiler->evaluator()->pop_register_value<Type*>(new_t)) return err::fail;
			if (!Type::assert(err, new_t)) return err::fail;
			if (!new_t->compile()) return err::fail;

			if (new_t->context() != ILContext::both && compiler->scope_context() != new_t->context()) {
				return throw_specific_error(err, "Type was not designed for this context");
			}

			ILSize s = new_t->size();
			stackid_t local_id = compiler->target()->local_stack_lifetime.append(s);
			compiler->stack()->push_item(name.buffer(), new_t, local_id);
		}
		else {
			Cursor err = c;
			CompileValue val;


			if (!Expression::parse(c, val, CompileType::eval)) return err::fail;
			if (!Operand::deref(val, CompileType::eval)) return err::fail;
			if (!Expression::rvalue(val, CompileType::eval)) return err::fail;

			if (val.type != compiler->types()->t_type) {
				return throw_specific_error(err, "Exprected type");
			}

			Type* new_t;
			if (!compiler->evaluator()->pop_register_value<Type*>(new_t)) return err::fail;
			if (!Type::assert(err, new_t)) return err::fail;
			if (!new_t->compile()) return err::fail;

			if (new_t->context() != ILContext::both && compiler->scope_context() != new_t->context()) {
				return throw_specific_error(err, "Type was not designed for this context");
			}

			ILSize s = new_t->size();
			stackid_t local_id = compiler->push_local(s);
			compiler->compiler_stack()->push_item(name.buffer(), new_t, local_id);
		}

		if (c.tok != RecognizedToken::Semicolon) {
			return throw_wrong_token_error(c, "';'");
		}
		c.move();
		return err::ok;
	}
}