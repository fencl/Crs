#include "Declaration.h"
#include "Error.h"
#include <iostream>
#include <string>
#include "PredefinedTypes.h"
#include "Expression.h"
#include "StackManager.h"
#include "Type.h"
#include <algorithm>
#include "Statement.h"
#include "Utilities.h"
#include "Operand.h"
#include "Compiler.h"

namespace Corrosive {

	void FunctionTemplate::compile() {
		if (compile_state == 0) {
			compile_state = 1;

			type = std::make_unique<TypeFunctionTemplate>();
			type->owner = this;

			if (ast_node->has_body() && ((AstFunctionNode*)ast_node)->is_generic) {
				compiler->push_workspace(parent);
				RecognizedToken tok;
				Cursor c = load_cursor(((AstFunctionNode*)ast_node)->annotation, ast_node->get_source(), tok);
				c.move(tok);
				while (true) {
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
					CompileValue value;
					Expression::parse(*compiler, c, tok, value, CompileType::eval);
					Expression::rvalue(*compiler, value, CompileType::eval);

					if (value.type != compiler->types()->t_type) {
						throw_specific_error(err, "Expected type value");
					}

					Type* t = compiler->evaluator()->pop_register_value<Type*>();

					t->compile();

					generic_ctx.generate_heap_size += t->size().eval(compiler->global_module(), compiler_arch);
					generic_ctx.generic_layout.push_back(std::make_tuple(name, t));

					if (tok == RecognizedToken::Comma) {
						c.move(tok);
					}
					else if (tok == RecognizedToken::CloseParenthesis) {
						break;
					}
					else {
						throw_wrong_token_error(c, "',' or ')'");
					}
				}



				gen_template_cmp.parent = this;
				instances = std::make_unique<std::map<unsigned char*, std::pair<std::unique_ptr<unsigned char[]>, std::unique_ptr<FunctionInstance>>, GenericTemplateCompare>>(gen_template_cmp);

				compiler->pop_workspace();
			}

			compile_state = 2;
		}
		else if (compile_state == 2) {

		}
		else {
			RecognizedToken tok;
			Cursor c = load_cursor(ast_node->name, ast_node->get_source(), tok);
			throw_specific_error(c, "compile cycle");
		}
	}


	void StructureTemplate::compile() {
		if (compile_state == 0) {
			compile_state = 1;

			compiler->evaluator()->stack_push();
			compiler->compiler_stack()->push();
			if (generic_ctx.generator != nullptr) {
				generic_ctx.generator->insert_key_on_stack(*compiler);
			}

			type = std::make_unique<TypeStructureTemplate>();
			type->owner = this;


			RecognizedToken tok;

			if (ast_node->is_generic) {
				Cursor c = load_cursor(ast_node->annotation, ast_node->get_source(), tok);
				c.move(tok);
				compiler->push_workspace(parent);

				while (true) {
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
					CompileValue value;
					Expression::parse(*compiler, c, tok, value, CompileType::eval);
					Expression::rvalue(*compiler, value, CompileType::eval);

					if (value.type != compiler->types()->t_type) {
						throw_specific_error(err, "Expected type value");
					}

					Type* t = compiler->evaluator()->pop_register_value<Type*>();

					t->compile();

					generic_ctx.generate_heap_size += t->size().eval(compiler->global_module(), compiler_arch);
					generic_ctx.generic_layout.push_back(std::make_tuple(name, t));


					if (tok == RecognizedToken::Comma) {
						c.move(tok);
					}
					else if (tok == RecognizedToken::CloseParenthesis) {
						break;
					}
					else {
						throw_wrong_token_error(c, "',' or ')'");
					}
				}


				gen_template_cmp.parent = this;
				instances = std::make_unique<std::map<unsigned char*, std::pair<std::unique_ptr<unsigned char[]>, std::unique_ptr<StructureInstance>>, GenericTemplateCompare>>(gen_template_cmp);

				compiler->pop_workspace();
			}

			compiler->evaluator()->stack_pop();
			compiler->compiler_stack()->pop();

			compile_state = 2;
		}
		else if (compile_state == 2) {

		}
		else {
			RecognizedToken tok;
			Cursor c = load_cursor(ast_node->name, ast_node->get_source(), tok);
			throw_specific_error(c, "compile cycle");
		}
	}

	void TraitTemplate::compile() {
		if (compile_state == 0) {
			compile_state = 1;

			compiler->evaluator()->stack_push();
			compiler->compiler_stack()->push();
			if (generic_ctx.generator != nullptr) {
				generic_ctx.generator->insert_key_on_stack(*compiler);
			}

			type = std::make_unique<TypeTraitTemplate>();
			type->owner = this;


			if (ast_node->is_generic) {
				compiler->push_workspace(parent);
				RecognizedToken tok;
				Cursor c = load_cursor(ast_node->annotation, ast_node->get_source(), tok);
				c.move(tok);

				while (true) {
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
					CompileValue value;
					Expression::parse(*compiler, c, tok, value, CompileType::eval);
					Expression::rvalue(*compiler, value, CompileType::eval);

					if (value.type != compiler->types()->t_type) {
						throw_specific_error(err, "Expected type value");
					}

					Type* t = compiler->evaluator()->pop_register_value<Type*>();

					t->compile();

					generic_ctx.generate_heap_size += t->size().eval(compiler->global_module(), compiler_arch);
					generic_ctx.generic_layout.push_back(std::make_tuple(name, t));


					if (tok == RecognizedToken::Comma) {
						c.move(tok);
					}
					else if (tok == RecognizedToken::CloseParenthesis) {
						break;
					}
					else {
						throw_wrong_token_error(c, "',' or ')'");
					}
				}


				gen_template_cmp.parent = this;
				instances = std::make_unique<std::map<unsigned char*, std::pair<std::unique_ptr<unsigned char[]>, std::unique_ptr<TraitInstance>>, GenericTemplateCompare>>(gen_template_cmp);
				compiler->pop_workspace();
			}

			compiler->evaluator()->stack_pop();
			compiler->compiler_stack()->pop();
			compile_state = 2;
		}
		else if (compile_state == 2) {

		}
		else {
			RecognizedToken tok;
			Cursor c = load_cursor(ast_node->name, ast_node->get_source(), tok);
			throw_specific_error(c, "compile cycle");
		}
	}



	void StructureTemplate::generate(unsigned char* argdata, StructureInstance*& out) {
		StructureInstance* new_inst = nullptr;
		unsigned char* new_key = nullptr;

		Source* src = ast_node->get_source();

		compiler->evaluator()->stack_push();
		compiler->compiler_stack()->push();

		if (!ast_node->is_generic) {
			if (single_instance == nullptr) {
				single_instance = std::make_unique<StructureInstance>();
				new_inst = single_instance.get();
			}
			out = single_instance.get();
		}
		else {
			unsigned char* key = argdata;

			auto f = instances->find(key);
			if (f == instances->end()) {
				std::unique_ptr<StructureInstance> inst = std::make_unique<StructureInstance>();

				new_inst = inst.get();
				out = inst.get();

				std::unique_ptr<unsigned char[]> new_key_inst = std::make_unique<unsigned char[]>(generic_ctx.generate_heap_size);
				unsigned char* old_offset = argdata;
				unsigned char* new_offset = new_key_inst.get();
				new_key = new_key_inst.get();

				for (auto l = generic_ctx.generic_layout.rbegin(); l != generic_ctx.generic_layout.rend(); l++) {
					std::get<1>(*l)->copy(new_offset, old_offset);
					size_t c_size = std::get<1>(*l)->size().eval(compiler->global_module(), compiler_arch);
					old_offset += c_size;
					new_offset += c_size;
				}

				instances->emplace(new_key, std::make_pair(std::move(new_key_inst), std::move(inst)));
				if (new_key == nullptr) {
					std::cout << "error";
				}
			}
			else {
				out = f->second.second.get();
			}
		}


		if (new_inst != nullptr) {
			new_inst->ast_node = ast_node;
			new_inst->type = std::make_unique<TypeStructureInstance>();
			new_inst->type->owner = new_inst;
			new_inst->parent = parent;
			new_inst->compiler = compiler;
			new_inst->generic_inst.key = new_key;
			new_inst->generic_inst.generator = &generic_ctx;
			new_inst->context = ast_node->context;

			RecognizedToken tok;

			new_inst->generic_inst.insert_key_on_stack(*compiler);

			compiler->push_scope_context(ILContext::compile);
			compiler->push_workspace(new_inst);

			for (auto&& m : ast_node->functions) {

				std::unique_ptr<FunctionTemplate> ft = std::make_unique<FunctionTemplate>();

				ft->ast_node = m.get();
				ft->compiler = compiler;
				ft->parent = new_inst;
				ft->generic_ctx.generator = &new_inst->generic_inst;

				if (new_inst->name_table.find(m->name_string) != new_inst->name_table.end()) {
					Cursor c = load_cursor(m->name, src, tok);
					throw_specific_error(c, "Name already exists in the structure");
				}

				new_inst->name_table[m->name_string] = std::make_pair((uint8_t)2, (uint32_t)new_inst->subfunctions.size());
				new_inst->subfunctions.push_back(std::move(ft));
			}

			for (auto&& t : ast_node->structures) {
				std::unique_ptr<StructureTemplate> decl = std::make_unique<StructureTemplate>();
				decl->ast_node = t.get();
				decl->compiler = compiler;
				decl->parent = new_inst;
				decl->generic_ctx.generator = &new_inst->generic_inst;

				if (new_inst->name_table.find(t->name_string) != new_inst->name_table.end()) {
					Cursor c = load_cursor(t->name, src, tok);
					throw_specific_error(c, "Name already exists in the structure");
				}

				new_inst->name_table[t->name_string] = std::make_pair((uint8_t)1, (uint32_t)new_inst->subtemplates.size());
				new_inst->subtemplates.push_back(std::move(decl));
			}



			for (auto&& m : ast_node->implementations) {

				CompileValue value;
				Cursor c = load_cursor(m->trait, src, tok);
				Cursor err = c;
				Expression::parse(*compiler, c, tok, value, CompileType::eval);
				Expression::rvalue(*compiler, value, CompileType::eval);

				if (value.type != compiler->types()->t_type) {
					throw_specific_error(err, "Expected type value");
				}

				Type* t = compiler->evaluator()->pop_register_value<Type*>();

				if (t->type() != TypeInstanceType::type_trait) {
					throw_specific_error(err, "Expected trait instance type");
				}

				t->compile();

				TypeTraitInstance* tt = (TypeTraitInstance*)t;
				if (new_inst->traitfunctions.find(tt->owner) != new_inst->traitfunctions.end()) {
					throw_specific_error(err, "This trait was already implemented");
				}
				else {
					std::vector<std::unique_ptr<FunctionInstance>> trait(tt->owner->member_declarations.size());

					unsigned int func_count = 0;
					for (auto&& f : m->functions) {

						std::unique_ptr<FunctionInstance> ft = std::make_unique<FunctionInstance>();
						ft->compile_state = 0;
						ft->generic_inst.generator = &generic_ctx;
						ft->generic_inst.key = new_key;
						ft->compiler = compiler;
						ft->ast_node = f.get();
						ft->parent = (Namespace*)this;

						std::string_view name_str;
						Cursor name_c;
						if (!m->fast) {
							name_str = f->name_string;
							name_c = load_cursor(f->name, src, tok);
						}
						else {
							name_str = tt->owner->ast_node->declarations[0]->name_string;
							name_c = load_cursor(tt->owner->ast_node->declarations[0]->name, src, tok);
						}

						auto ttid = tt->owner->member_table.find(name_str);

						if (ttid == tt->owner->member_table.end()) {
							throw_specific_error(name_c, "Implemented trait has no function with this name");
						}
						else if (trait[ttid->second] != nullptr) {
							throw_specific_error(name_c, "Funtion with the same name already exists in the implementation");
						}


						func_count += 1;
						auto& fundecl = tt->owner->member_declarations[ttid->second];

						if (f->context != fundecl->ptr_context) {
							throw_specific_error(name_c, "Funtion has different context");
						}

						auto& args = compiler->types()->argument_array_storage.get(fundecl->argument_array_id);

						c = load_cursor(f->type, src, tok);
						c.move(tok);

						if (tok != RecognizedToken::CloseParenthesis) {
							while (true) {
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
								CompileValue res;
								Expression::parse(*compiler, c, tok, res, CompileType::eval);
								Expression::rvalue(*compiler, res, CompileType::eval);
								if (res.type != compiler->types()->t_type) {
									throw_specific_error(err, "Expected type");
								}
								Type* argt = compiler->evaluator()->pop_register_value<Type*>();
								if (ft->arguments.size() == 0) {
									Type* this_type = new_inst->type->generate_reference();
									if (argt != this_type) {
										throw_specific_error(err, "First argument in implementation of trait function must be self reference to the structure");
									}

									ft->arguments.push_back(std::make_pair(name, argt));
								}
								else {
									if (ft->arguments.size() - 1 >= args.size()) {
										throw_specific_error(err, "There are more arguments than in the original trait function");
									}

									Type* req_type = args[ft->arguments.size() - 1];
									if (argt != req_type) {
										throw_specific_error(err, "Argument does not match the type of the original trait function");
									}

									ft->arguments.push_back(std::make_pair(name, argt));
								}

								if (tok == RecognizedToken::Comma) {
									c.move(tok);
								}
								else if (tok == RecognizedToken::CloseParenthesis) {
									break;
								}
								else {
									throw_wrong_token_error(c, "',' or ')'");
								}
							}
						}

						if (ft->arguments.size() != args.size() + 1) {
							throw_specific_error(c, "Trait function declaration lacks arguments from the original");
						}

						c.move(tok);

						if (tok != RecognizedToken::OpenBrace) {
							Cursor err = c;
							CompileValue res;
							Expression::parse(*compiler, c, tok, res, CompileType::eval);
							Expression::rvalue(*compiler, res, CompileType::eval);

							if (res.type != compiler->types()->t_type) {
								throw_specific_error(err, "Expected type");
							}
							Type* rett = compiler->evaluator()->pop_register_value<Type*>();

							Type* req_type = fundecl->return_type;
							if (rett != req_type) {
								throw_specific_error(err, "Return type does not match the type of the original trait function");
							}

							ft->returns.first = err;
							ft->returns.second = rett;
						}
						else {
							ft->returns.second = compiler->types()->t_void;
						}

						std::vector<Type*> argtypes;
						for (auto&& a : ft->arguments) {
							argtypes.push_back(a.second);
						}

						ft->type = compiler->types()->load_or_register_function_type(ILCallingConvention::bytecode, std::move(argtypes), ft->returns.second, ft->ast_node->context);
						ft->compile_state = 1;
						trait[ttid->second] = std::move(ft);

					}

					if (func_count != tt->owner->member_declarations.size()) {
						Cursor c = load_cursor(m->trait, src, tok);
						throw_specific_error(c, "Trait implementation is missing some functions");
					}
					

					new_inst->traitfunctions[tt->owner] = std::move(trait);

				}
			}

			for (auto&& m : ast_node->variables) {

				bool composite = m->alias;


				CompileValue value;

				Cursor c = load_cursor(m->type, src, tok);
				Cursor err = c;
				Expression::parse(*compiler, c, tok, value, CompileType::eval);
				Expression::rvalue(*compiler, value, CompileType::eval);
				if (value.type != compiler->types()->t_type) {
					throw_specific_error(err, "Expected type value");
				}

				Type* m_t = compiler->evaluator()->pop_register_value<Type*>();
				if (m_t->context() == ILContext::compile) {
					if (new_inst->context == ILContext::both || new_inst->context == ILContext::compile) {
						new_inst->context = ILContext::compile;
					}
					else {
						throw_specific_error(err, "Cannot use compile type in runtime-only structure");
					}
				}
				else if (m_t->context() == ILContext::runtime || new_inst->context == ILContext::runtime) {
					if (new_inst->context == ILContext::both) {
						new_inst->context = ILContext::runtime;
					}
					else {
						throw_specific_error(err, "Cannot use runtime type in compile-only structure");
					}
				}

				new_inst->member_table[m->name_string] = std::make_pair((uint16_t)new_inst->member_vars.size(), MemberTableEntryType::var);
				if (composite) {
					new_inst->member_composites.push_back((uint16_t)new_inst->member_vars.size());
				}
				new_inst->member_vars.push_back(std::make_pair(m_t, 0));
			}

			compiler->pop_workspace();
			compiler->pop_scope_context();

		}


		compiler->compiler_stack()->pop();
		compiler->evaluator()->stack_pop();
	}


	void TraitTemplate::generate(unsigned char* argdata, TraitInstance*& out) {
		TraitInstance* new_inst = nullptr;
		unsigned char* new_key = nullptr;
		Source* src = ast_node->get_source();


		compiler->evaluator()->stack_push();
		compiler->compiler_stack()->push();

		if (!ast_node->is_generic) {
			if (single_instance == nullptr) {
				single_instance = std::make_unique<TraitInstance>();
				new_inst = single_instance.get();
			}
			out = single_instance.get();
		}
		else {
			unsigned char* key = argdata;

			auto f = instances->find(key);
			if (f == instances->end()) {
				std::unique_ptr<TraitInstance> inst = std::make_unique<TraitInstance>();

				new_inst = inst.get();
				out = inst.get();

				std::unique_ptr<unsigned char[]> new_key_inst = std::make_unique<unsigned char[]>(generic_ctx.generate_heap_size);
				new_key = new_key_inst.get();
				unsigned char* old_offset = argdata;
				unsigned char* new_offset = new_key_inst.get();

				for (auto l = generic_ctx.generic_layout.rbegin(); l != generic_ctx.generic_layout.rend(); l++) {
					std::get<1>(*l)->copy(new_offset, old_offset);
					size_t c_size = std::get<1>(*l)->size().eval(compiler->global_module(), compiler_arch);
					old_offset += c_size;
					new_offset += c_size;
				}

				instances->emplace(new_key, std::make_pair(std::move(new_key_inst), std::move(inst)));

			}
			else {
				out = f->second.second.get();
			}
		}


		if (new_inst != nullptr) {
			new_inst->type = std::make_unique<TypeTraitInstance>();
			new_inst->type->owner = new_inst;
			new_inst->compiler = compiler;
			new_inst->parent = parent;
			new_inst->generic_inst.key = new_key;
			new_inst->generic_inst.generator = &generic_ctx;
			new_inst->generic_inst.insert_key_on_stack(*compiler);
			new_inst->ast_node = ast_node;

			compiler->push_workspace(parent);


			for (auto&& m : ast_node->declarations) {

				std::vector<Type*> args;
				Type* ret_type;

				RecognizedToken tok;
				Cursor c = load_cursor(m->type, src, tok);
				c.move(tok);

				if (tok != RecognizedToken::CloseParenthesis) {
					while (true) {
						Cursor err = c;
						CompileValue val;
						Expression::parse(*compiler, c, tok, val, CompileType::eval);
						Expression::rvalue(*compiler, val, CompileType::eval);

						if (val.type != compiler->types()->t_type) {
							throw_specific_error(err, "Expected type");
						}
						Type* t = compiler->evaluator()->pop_register_value<Type*>();
						args.push_back(t);
						if (tok == RecognizedToken::Comma) {
							c.move(tok);
						}
						else if (tok == RecognizedToken::CloseParenthesis) {
							break;
						}
						else {
							throw_wrong_token_error(c, "',' or ')'");
						}
					}
				}
				c.move(tok);

				if (tok == RecognizedToken::Semicolon) {
					ret_type = compiler->types()->t_void;
				}
				else {
					Cursor err = c;
					CompileValue val;
					Expression::parse(*compiler, c, tok, val, CompileType::eval);
					Expression::rvalue(*compiler, val, CompileType::eval);

					if (val.type != compiler->types()->t_type) {
						throw_specific_error(err, "Expected type");
					}
					Type* t = compiler->evaluator()->pop_register_value<Type*>();
					ret_type = t;
				}


				TypeFunction* type = compiler->types()->load_or_register_function_type(ILCallingConvention::bytecode, std::move(args), ret_type, ILContext::both);

				new_inst->member_table[m->name_string] = (uint16_t)new_inst->member_declarations.size();
				new_inst->member_declarations.push_back(type);
			}

			compiler->pop_workspace();
		}

		compiler->compiler_stack()->pop();
		compiler->evaluator()->stack_pop();

	}


	void FunctionTemplate::generate(unsigned char* argdata, FunctionInstance*& out) {
		FunctionInstance* new_inst = nullptr;
		unsigned char* new_key = nullptr;

		compiler->evaluator()->stack_push();
		compiler->compiler_stack()->push();

		if (!(ast_node->has_body() && ((AstFunctionNode*)ast_node)->is_generic)) {
			if (single_instance == nullptr) {
				single_instance = std::make_unique<FunctionInstance>();
				new_inst = single_instance.get();
			}
			out = single_instance.get();
		}
		else {

			unsigned char* key = argdata;

			auto f = instances->find(key);

			if (f == instances->end()) {
				std::unique_ptr<FunctionInstance> inst = std::make_unique<FunctionInstance>();

				new_inst = inst.get();
				out = inst.get();

				std::unique_ptr<unsigned char[]> new_key_inst = std::make_unique<unsigned char[]>(generic_ctx.generate_heap_size);
				new_key = new_key_inst.get();
				unsigned char* old_offset = argdata;
				unsigned char* new_offset = new_key_inst.get();

				for (auto l = generic_ctx.generic_layout.rbegin(); l != generic_ctx.generic_layout.rend(); l++) {
					std::get<1>(*l)->copy(new_offset, old_offset);
					size_t c_size = std::get<1>(*l)->size().eval(compiler->global_module(), compiler_arch);
					old_offset += c_size;
					new_offset += c_size;
				}

				//memcpy(new_offset, old_offset, generic_ctx.generate_heap_size);

				instances->emplace(new_key, std::make_pair(std::move(new_key_inst), std::move(inst)));

			}
			else {
				out = f->second.second.get();
			}
		}


		if (new_inst != nullptr) {


			new_inst->compile_state = 0;
			new_inst->parent = parent;
			new_inst->compiler = compiler;
			new_inst->generic_inst.key = new_key;
			new_inst->generic_inst.generator = &generic_ctx;
			new_inst->ast_node = ast_node;

			new_inst->generic_inst.insert_key_on_stack(*compiler);

			compiler->push_workspace(parent);

			CompileValue cvres;

			RecognizedToken tok;
			Cursor c = load_cursor(ast_node->type, ast_node->get_source(), tok);
			c.move(tok);
			if (tok != RecognizedToken::CloseParenthesis) {
				while (true) {

					Cursor argname = c;
					if (ast_node->has_body()) {
						if (tok != RecognizedToken::Symbol) {
							throw_not_a_name_error(c);
						}
						c.move(tok);
						if (tok != RecognizedToken::Colon) {
							throw_wrong_token_error(c, "':'");
						}
						c.move(tok);
					}

					Cursor err = c;
					Expression::parse(*compiler, c, tok, cvres, CompileType::eval);
					Expression::rvalue(*compiler, cvres, CompileType::eval);

					if (cvres.type != compiler->types()->t_type) {
						throw_cannot_cast_error(err, cvres.type, compiler->types()->t_type);
					}
					Type* t = compiler->evaluator()->pop_register_value<Type*>();
					new_inst->arguments.push_back(std::make_pair(argname, t));

					if (tok == RecognizedToken::Comma) {
						c.move(tok);
					}
					else if (tok == RecognizedToken::CloseParenthesis) {
						c.move(tok);
						break;
					}
					else {
						throw_wrong_token_error(c, "',' or ')'");
					}
				}
			}
			else { c.move(tok); }

			if (tok != RecognizedToken::OpenBrace && tok != RecognizedToken::Semicolon) {
				Cursor err = c;
				Expression::parse(*compiler, c, tok, cvres, CompileType::eval);
				Expression::rvalue(*compiler, cvres, CompileType::eval);

				if (cvres.type != compiler->types()->t_type) {
					throw_cannot_cast_error(err, cvres.type, compiler->types()->t_type);
				}
				Type* t = compiler->evaluator()->pop_register_value<Type*>();
				new_inst->returns.first = err;
				new_inst->returns.second = t;
			}
			else {
				new_inst->returns.second = compiler->types()->t_void;
			}

			std::vector<Type*> argtypes;
			for (auto&& a : new_inst->arguments) {
				argtypes.push_back(a.second);
			}

			new_inst->type = compiler->types()->load_or_register_function_type(ILCallingConvention::bytecode, std::move(argtypes), new_inst->returns.second, new_inst->ast_node->context);
			new_inst->compile_state = 1;

			compiler->pop_workspace();
		}

		compiler->compiler_stack()->pop();
		compiler->evaluator()->stack_pop();
	}

	void FunctionInstance::compile() {
		if (compile_state == 1) {
			compile_state = 2;

			if (ast_node->has_body()) {
				type->compile();
				auto func = compiler->global_module()->create_function();
				this->func = func;
				func->decl_id = type->il_function_decl;
				func->alias = ast_node->name_string;

				ILBlock* b = func->create_and_append_block();
				b->alias = "entry";

				compiler->push_workspace(parent);
				compiler->push_scope_context(ast_node->context);
				compiler->push_function(func, returns.second);
				compiler->push_scope(b);

				compiler->evaluator()->stack_push();
				compiler->compiler_stack()->push();
				generic_inst.insert_key_on_stack(*compiler);

				compiler->stack()->push();
				compiler->temp_stack()->push();

				compiler->target()->local_stack_lifetime.push();
				compiler->stack()->push_block();
				compiler->temp_stack()->push_block();
				ILBuilder::build_accept(compiler->scope(), ILDataType::none);

				bool ret_rval_stack = false;
				uint16_t return_ptr_local_id = 0;

				returns.second->compile();
				func->local_stack_lifetime.push();

				if (returns.second->rvalue_stacked()) {
					ret_rval_stack = true;
					return_ptr_local_id = func->local_stack_lifetime.append(compiler->types()->t_ptr->size());
				}

				for (auto&& a : arguments) {

					a.second->compile();
					if (a.second->context() == ILContext::compile && ast_node->context != ILContext::compile) {
						Cursor err = a.first;
						RecognizedToken tmp;
						err.move(tmp);
						err.move(tmp);
						throw_specific_error(err, "Type is marked for compile time use only");
					}

					a.second->compile();
					uint16_t id = func->local_stack_lifetime.append(a.second->size());

					compiler->stack()->push_item(a.first.buffer(), a.second, id, StackItemTag::regular);
				}



				uint16_t argid = (uint16_t)(arguments.size() - (ret_rval_stack ? 0 : 1));
				for (auto a = arguments.rbegin(); a != arguments.rend(); a++) {
					ILBuilder::build_local(compiler->scope(), argid);
					Expression::copy_from_rvalue(a->second, CompileType::compile, false);

					argid--;
				}

				if (ret_rval_stack) {
					ILBuilder::build_local(compiler->scope(), return_ptr_local_id);
					ILBuilder::build_store(compiler->scope(), ILDataType::word);
				}


				if (returns.second->context() == ILContext::compile && ast_node->context != ILContext::compile) {
					throw_specific_error(returns.first, "Type is marked for compile time use only");
				}


				compile_state = 3;
				Source* src = ast_node->get_source();
				RecognizedToken tok;
				Cursor name = load_cursor(ast_node->name, src, tok);
				Cursor cb = load_cursor(((AstFunctionNode*)ast_node)->block, src, tok);
				BlockTermination term;
				cb.move(tok);
				Statement::parse_inner_block(*compiler, cb, tok, term, true, &name);


				//func->dump();
				//std::cout << std::endl;

				func->assert_flow();

				compiler->temp_stack()->pop_block();

				compiler->stack()->pop();
				compiler->temp_stack()->pop();
				compiler->compiler_stack()->pop();
				compiler->evaluator()->stack_pop();

				compiler->pop_workspace();
				compiler->pop_scope_context();
				compiler->pop_function();


			}
			else {
				auto func = compiler->global_module()->create_ext_function();
				this->func = func;
				func->alias = ast_node->name_string;
				compile_state = 3;
			}
		}
		else if (compile_state == 3) {

		}
		else if (compile_state == 2) {
			RecognizedToken tok;
			Cursor c = load_cursor(ast_node->name, ast_node->get_source(), tok);
			throw_specific_error(c, "Build cycle");
		}
		else if (compile_state == 0) {
			RecognizedToken tok;
			Cursor c = load_cursor(ast_node->name, ast_node->get_source(), tok);
			throw_specific_error(c, "Build cycle");
		}
	}

	void StructureInstance::compile() {
		if (compile_state == 0) {
			compile_state = 1;
			compiler->push_scope_context(ILContext::compile);
			compiler->evaluator()->stack_push();
			compiler->compiler_stack()->push();
			generic_inst.insert_key_on_stack(*compiler);

			ILStructTable table;
			uint32_t max_align = 0;
			size = ILSize(ILSizeType::absolute, 0);

			for (auto&& m : member_vars) {
				m.first->compile();

				ILSize m_s = m.first->size();

				if (m_s.type == ILSizeType::absolute && m_s.value <= 4) { // up to 4 bytes always aligned to 4 bytes or less
					if (size.type == ILSizeType::absolute) {
						max_align = std::max(m_s.value, max_align);
						size.value = (uint32_t)align_up(size.value, upper_power_of_two(m_s.value));
						m.second = size.value;
						size.value += m_s.value;
					}
					else {
						size.type = ILSizeType::table;
					}
				}
				else if (m_s.type == ILSizeType::word) { // words are automatically aligned
					if (size.type == ILSizeType::absolute && size.value == 0) size.type = ILSizeType::word;

					if (size.type == ILSizeType::word) {
						m.second = size.value; // aligned to single word
						size.value += m_s.value;
					}
					else {

						size.type = ILSizeType::table;
					}
				}
				else {
					size.type = ILSizeType::table;
				}


				table.elements.push_back(m_s);
			}


			if (size.type == ILSizeType::table) {
				if (table.elements.size() > 0) {
					size.value = compiler->global_module()->register_structure_table();
					compiler->global_module()->structure_tables[size.value] = std::move(table);
				}
				else {
					size = table.elements.back();
					wrapper = true;
				}
			}
			else if (size.type == ILSizeType::absolute) {
				size.value = (uint32_t)align_up(size.value, max_align);
			}


			for (auto&& m : subfunctions) {
				m->compile();
			}

			structure_type = StructureInstanceType::normal_structure;
			compile_state = 2;

			for (size_t i = 0; i < member_composites.size(); i++) {
				size_t comp = member_composites[i];
				auto& m = member_vars[comp];
				Type* t = m.first;

				if (t->type() == TypeInstanceType::type_reference && ((TypeReference*)t)->owner->type() == TypeInstanceType::type_structure_instance) {
					t = ((TypeReference*)t)->owner;
				}

				if (t->type() == TypeInstanceType::type_structure_instance) {
					TypeStructureInstance* ts = (TypeStructureInstance*)t;
					ts->compile();
					for (auto&& v : ts->owner->member_table) {
						member_table.insert(std::make_pair(v.first, std::make_pair<uint16_t, MemberTableEntryType>((uint16_t)comp, MemberTableEntryType::alias)));
					}
				}
				else if (t->type() == TypeInstanceType::type_slice) {
					TypeSlice* ts = (TypeSlice*)t;
					ts->compile();
					pass_array_operator = true;
					pass_array_id = (uint16_t)comp;
				}
			}


			for (uint32_t i = 0; i < subfunctions.size(); ++i) {
				auto gf = subfunctions[i].get();
				if (!(gf->ast_node->has_body() && ((AstFunctionNode*)gf->ast_node)->is_generic)) {
					gf->compile();
					FunctionInstance* fi;
					gf->generate(nullptr, fi);

					if (fi->arguments.size() > 0 && fi->arguments[0].second == type.get()->generate_reference()) {
						member_table.insert(std::make_pair(fi->ast_node->name_string, std::make_pair<uint16_t, MemberTableEntryType>((uint16_t)i, MemberTableEntryType::func)));
					}
				}
			}

			compile_state = 3;

			compiler->compiler_stack()->pop();
			compiler->evaluator()->stack_pop();
			compiler->pop_scope_context();
		}
		else if (compile_state == 3) {

		}
		else {
			RecognizedToken tok;
			Cursor c = load_cursor(((AstStructureNode*)ast_node)->name, ast_node->get_source(), tok);
			throw_specific_error(c, "Build cycle");
		}
	}

	void GenericInstance::insert_key_on_stack(Compiler& compiler) {
		if (generator != nullptr) {

			if (generator->generator != nullptr) {
				generator->generator->insert_key_on_stack(compiler);
			}

			unsigned char* key_ptr = key;
			for (auto key_l = generator->generic_layout.rbegin(); key_l != generator->generic_layout.rend(); key_l++) {

				uint16_t sid = compiler.evaluator()->mask_local(key_ptr);
				compiler.compiler_stack()->push_item(std::get<0>(*key_l).buffer(), std::get<1>(*key_l), sid, StackItemTag::alias);

				key_ptr += std::get<1>(*key_l)->size().eval(compiler.global_module(), compiler_arch);
			}
		}
	}


	void TraitInstance::generate_vtable(StructureInstance* forinst, uint32_t& optid) {
		forinst->compile();

		std::unique_ptr<void* []> vtable = std::make_unique<void* []>(member_declarations.size());

		auto& f_table = forinst->traitfunctions[this];
		size_t id = 0;
		for (auto&& m_func : member_declarations) {
			FunctionInstance* finst = f_table[id].get();
			finst->compile();
			vtable[id] = finst->func;
			++id;
		}

		void** vt = vtable.get();
		uint32_t vtid = compiler->global_module()->register_vtable(std::move(vtable));
		vtable_instances[forinst] = vtid;
		optid = vtid;
	}
}