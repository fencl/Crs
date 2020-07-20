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

			if (is_generic) {
				Cursor c = annotation;
				compiler->push_workspace(parent);

				while (true) {


					Cursor name = c;
					c.move();
					if (c.tok != RecognizedToken::Colon) {
						throw_wrong_token_error(c, "':'");
					}
					c.move();
					Cursor err = c;
					CompileValue value;
					Expression::parse(*compiler,c, value, CompileType::eval);
					Expression::rvalue(*compiler,value, CompileType::eval);

					if (value.type != compiler->types()->t_type) {
						throw_specific_error(err, "Expected type value");
					}

					Type* t = compiler->evaluator()->pop_register_value<Type*>();

					t->compile();

					generic_ctx.generate_heap_size += t->size().eval(compiler->global_module(), compiler_arch);
					generic_ctx.generic_layout.push_back(std::make_tuple(name, t));


					if (c.tok == RecognizedToken::Comma) {
						c.move();
					}
					else if (c.tok == RecognizedToken::CloseParenthesis) {
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
			throw_specific_error(name, "compile cycle");
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

			if (is_generic) {
				Cursor c = annotation;

				compiler->push_workspace(parent);

				while (true) {

					Cursor name = c;
					c.move();
					if (c.tok != RecognizedToken::Colon) {
						throw_wrong_token_error(c, "':'");
					}
					c.move();
					Cursor err = c;
					CompileValue value;
					Expression::parse(*compiler,c, value, CompileType::eval);
					Expression::rvalue(*compiler,value, CompileType::eval);

					if (value.type != compiler->types()->t_type) {
						throw_specific_error(err, "Expected type value");
					}

					Type* t = compiler->evaluator()->pop_register_value<Type*>();

					/*if (t->type() != TypeInstanceType::type_structure_instance && t->type() != TypeInstanceType::type_template && t->type() != TypeInstanceType::type_function) {
						throw_specific_error(err, "Type does not point to instance");
						return false;
					}*/

					t->compile();

					generic_ctx.generate_heap_size += t->size().eval(compiler->global_module(), compiler_arch);
					generic_ctx.generic_layout.push_back(std::make_tuple(name, t));


					if (c.tok == RecognizedToken::Comma) {
						c.move();
					}
					else if (c.tok == RecognizedToken::CloseParenthesis) {
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
			throw_specific_error(name, "compile cycle");
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



			if (is_generic) {
				Cursor c = annotation;

				compiler->push_workspace(parent);

				while (true) {

					Cursor name = c;
					c.move();
					if (c.tok != RecognizedToken::Colon) {
						throw_wrong_token_error(c, "':'");
					}
					c.move();
					Cursor err = c;
					CompileValue value;
					Expression::parse(*compiler,c, value, CompileType::eval);
					Expression::rvalue(*compiler,value, CompileType::eval);

					if (value.type != compiler->types()->t_type) {
						throw_specific_error(err, "Expected type value");
					}

					Type* t = compiler->evaluator()->pop_register_value<Type*>();

					/*if (t->type() != TypeInstanceType::type_structure_instance && t->type() != TypeInstanceType::type_template && t->type() != TypeInstanceType::type_function) {
						throw_specific_error(err, "Type does not point to instance");
						return false;
					}*/

					t->compile();

					generic_ctx.generate_heap_size += t->size().eval(compiler->global_module(), compiler_arch);
					generic_ctx.generic_layout.push_back(std::make_tuple(name, t));


					if (c.tok == RecognizedToken::Comma) {
						c.move();
					}
					else if (c.tok == RecognizedToken::CloseParenthesis) {
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
			throw_specific_error(name, "compile cycle");
		}
	}



	void StructureTemplate::generate(unsigned char* argdata, StructureInstance*& out) {
		StructureInstance* new_inst = nullptr;
		unsigned char* new_key = nullptr;

		compiler->evaluator()->stack_push();
		compiler->compiler_stack()->push();

		if (!is_generic) {
			if (singe_instance == nullptr) {
				singe_instance = std::make_unique<StructureInstance>();
				new_inst = singe_instance.get();
			}
			out = singe_instance.get();
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
					std::get<1>(*l)->move(new_offset, old_offset);
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
			new_inst->type = std::make_unique<TypeStructureInstance>();
			new_inst->type->owner = new_inst;
			new_inst->parent = parent;
			new_inst->compiler = compiler;
			new_inst->name = name;
			new_inst->namespace_type = NamespaceType::t_struct_instance;
			new_inst->generic_inst.key = new_key;
			new_inst->generic_inst.generator = &generic_ctx;

			new_inst->generic_inst.insert_key_on_stack(*compiler);

			compiler->push_scope_context(ILContext::compile);
			compiler->push_workspace(new_inst);

			for (auto&& m : member_funcs) {
				Cursor c = m.type;
				//TODO we could probably skip the catalogue stage and build the functiontemplate directly in the structure template


				std::unique_ptr<FunctionTemplate> ft = std::make_unique<FunctionTemplate>();
				ft->context = m.context;
				ft->name = m.name;
				ft->compiler = compiler;
				ft->annotation = m.annotation;
				ft->is_generic = m.annotation.tok != RecognizedToken::Eof;
				ft->parent = new_inst;
				ft->generic_ctx.generator = &new_inst->generic_inst;
				ft->decl_type = m.type;
				ft->block = m.block;

				ft->type = std::make_unique<TypeFunctionTemplate>();
				ft->type->owner = ft.get();

				if (new_inst->name_table.find(m.name.buffer) != new_inst->name_table.end()) {
					throw_specific_error(m.name, "Name already exists in the structure");
				}

				new_inst->name_table[m.name.buffer] = std::make_pair((uint8_t)2, (uint32_t)new_inst->subfunctions.size());
				new_inst->subfunctions.push_back(std::move(ft));
			}

			for (auto&& t : member_templates) {
				Cursor tc = t.cursor;
				std::unique_ptr<StructureTemplate> decl;
				StructureTemplate::parse(*compiler,tc, new_inst, &new_inst->generic_inst, decl);

				if (new_inst->name_table.find(decl->name.buffer) != new_inst->name_table.end()) {
					throw_specific_error(decl->name, "Name already exists in the structure");
				}

				new_inst->name_table[decl->name.buffer] = std::make_pair((uint8_t)1, (uint32_t)new_inst->subtemplates.size());
				new_inst->subtemplates.push_back(std::move(decl));
			}



			for (auto&& m : member_implementation) {
				Cursor c = m.type;
				CompileValue value;

				Cursor err = c;
				Expression::parse(*compiler,c, value, CompileType::eval);
				Expression::rvalue(*compiler,value, CompileType::eval);

				if (value.type != compiler->types()->t_type) {
					throw_specific_error(m.type, "Expected type value");
				}

				Type* t = compiler->evaluator()->pop_register_value<Type*>();

				if (t->type() != TypeInstanceType::type_trait) {
					throw_specific_error(m.type, "Expected trait instance type");
				}

				t->compile();

				TypeTraitInstance* tt = (TypeTraitInstance*)t;
				if (new_inst->traitfunctions.find(tt->owner) != new_inst->traitfunctions.end()) {
					throw_specific_error(err, "This trait was already implemented");
				}
				else {
					std::vector<std::unique_ptr<FunctionInstance>> trait(tt->owner->member_funcs.size());

					unsigned int func_count = 0;
					for (auto&& f : m.functions) {

						std::unique_ptr<FunctionInstance> ft = std::make_unique<FunctionInstance>();
						ft->compile_state = 0;
						ft->generic_inst.generator = &generic_ctx;
						ft->generic_inst.key = new_key;
						ft->compiler = compiler;

						Cursor err;
						ft->block = f.block;
						if (f.name.buffer == "<s>") {
							ft->name = tt->owner->member_funcs.back().name;
							err = m.type;
						}
						else {
							ft->name = f.name;
							err = ft->name;
						}
						ft->parent = (Namespace*)this;

						auto ttid = tt->owner->member_table.find(ft->name.buffer);

						if (ttid == tt->owner->member_table.end()) {
							throw_specific_error(err, "Implemented trait has no function with this name");
						}
						else if (trait[ttid->second] != nullptr) {
							throw_specific_error(err, "Funtion with the same name already exists in the implementation");
						}


						func_count += 1;
						auto& fundecl = tt->owner->member_funcs[ttid->second];
						ft->context = fundecl.ctx;

						auto& args = compiler->types()->argument_array_storage.get(fundecl.type->argument_array_id);

						Cursor c = f.type;
						c.move();
						if (c.tok != RecognizedToken::CloseParenthesis) {
							while (true) {
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
								CompileValue res;
								Expression::parse(*compiler,c, res, CompileType::eval);
								Expression::rvalue(*compiler,res, CompileType::eval);
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

								if (c.tok == RecognizedToken::Comma) {
									c.move();
								}
								else if (c.tok == RecognizedToken::CloseParenthesis) {
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

						c.move();

						if (c.tok != RecognizedToken::OpenBrace) {
							Cursor err = c;
							CompileValue res;
							Expression::parse(*compiler,c, res, CompileType::eval);
							Expression::rvalue(*compiler,res, CompileType::eval);

							if (res.type != compiler->types()->t_type) {
								throw_specific_error(err, "Expected type");
							}
							Type* rett = compiler->evaluator()->pop_register_value<Type*>();

							Type* req_type = fundecl.type->return_type;
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

						ft->type = compiler->types()->load_or_register_function_type(std::move(argtypes), ft->returns.second, ft->context);
						ft->compile_state = 1;
						trait[ttid->second] = std::move(ft);

					}

					if (func_count != tt->owner->member_funcs.size()) {
						throw_specific_error(m.type, "Trait implementation is missing some functions");
					}

					if (tt->owner->generic_inst.generator == &compiler->types()->tr_copy->generic_ctx) {
						new_inst->impl_copy = trait.begin()->get();
						new_inst->impl_copy->parent = new_inst;
					}

					if (tt->owner->generic_inst.generator == &compiler->types()->tr_move->generic_ctx) {
						new_inst->impl_move = trait.begin()->get();
						new_inst->impl_move->parent = new_inst;
					}

					if (tt->owner->generic_inst.generator == &compiler->types()->tr_compare->generic_ctx) {
						new_inst->impl_compare = trait.begin()->get();
						new_inst->impl_compare->parent = new_inst;
					}

					if (tt->owner->generic_inst.generator == &compiler->types()->tr_drop->generic_ctx) {
						new_inst->impl_drop = trait.begin()->get();
						new_inst->impl_drop->parent = new_inst;
					}

					if (tt->owner->generic_inst.generator == &compiler->types()->tr_ctor->generic_ctx) {
						new_inst->impl_ctor = trait.begin()->get();
						new_inst->impl_ctor->parent = new_inst;
					}

					new_inst->traitfunctions[tt->owner] = std::move(trait);

				}
			}

			for (auto&& m : member_vars) {
				Cursor c = m.type;

				CompileValue value;

				Cursor err = c;
				Expression::parse(*compiler,c, value, CompileType::eval);
				Expression::rvalue(*compiler,value, CompileType::eval);
				if (value.type != compiler->types()->t_type) {
					throw_specific_error(m.type, "Expected type value");
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

				new_inst->member_table[m.name.buffer] = std::make_pair((uint16_t)new_inst->member_vars.size(), MemberTableEntryType::var);
				if (m.composite) {
					new_inst->member_composites.push_back((uint16_t)new_inst->member_vars.size());
				}
				new_inst->member_vars.push_back(std::make_pair(m_t, 0));
			}

			if (new_inst->context == ILContext::runtime) {
				for (auto&& f : new_inst->subfunctions) {
					f->context = ILContext::runtime;
				}
			}

			if (new_inst->context == ILContext::compile) {
				for (auto&& f : new_inst->subfunctions) {
					f->context = ILContext::compile;
				}
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


		compiler->evaluator()->stack_push();
		compiler->compiler_stack()->push();

		if (!is_generic) {
			if (singe_instance == nullptr) {
				singe_instance = std::make_unique<TraitInstance>();
				new_inst = singe_instance.get();
			}
			out = singe_instance.get();
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
					std::get<1>(*l)->move(new_offset, old_offset);
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
			new_inst->type = std::make_unique<TypeTraitInstance>();
			new_inst->type->owner = new_inst;
			new_inst->compiler = compiler;
			new_inst->parent = parent;
			new_inst->name = name;
			new_inst->generic_inst.key = new_key;
			new_inst->generic_inst.generator = &generic_ctx;

			new_inst->generic_inst.insert_key_on_stack(*compiler);

			compiler->push_workspace(parent);

			for (auto&& m : member_funcs) {



				TraitInstanceMemberRecord member;
				member.name = m.name;
				member.definition = m.type;
				member.ctx = m.ctx;
				Cursor c = member.definition;

				std::vector<Type*> args;
				Type* ret_type;

				if (c.tok != RecognizedToken::OpenParenthesis) {
					throw_wrong_token_error(c, "'('");
				}

				c.move();

				if (c.tok != RecognizedToken::CloseParenthesis) {
					while (true) {
						Cursor err = c;
						CompileValue val;
						Expression::parse(*compiler,c, val, CompileType::eval);
						Expression::rvalue(*compiler,val, CompileType::eval);

						if (val.type != compiler->types()->t_type) {
							throw_specific_error(err, "Expected type");
						}
						Type* t = compiler->evaluator()->pop_register_value<Type*>();
						args.push_back(t);
						if (c.tok == RecognizedToken::Comma) {
							c.move();
						}
						else if (c.tok == RecognizedToken::CloseParenthesis) {
							break;
						}
						else {
							throw_wrong_token_error(c, "',' or ')'");
						}
					}
				}
				c.move();

				if (c.tok == RecognizedToken::Semicolon) {
					ret_type = compiler->types()->t_void;
				}
				else {
					Cursor err = c;
					CompileValue val;
					Expression::parse(*compiler,c, val, CompileType::eval);
					Expression::rvalue(*compiler,val, CompileType::eval);

					if (val.type != compiler->types()->t_type) {
						throw_specific_error(err, "Expected type");
					}
					Type* t = compiler->evaluator()->pop_register_value<Type*>();
					ret_type = t;
				}


				member.type = compiler->types()->load_or_register_function_type(std::move(args), ret_type, ILContext::both);

				new_inst->member_table[m.name.buffer] = (uint16_t)new_inst->member_funcs.size();
				new_inst->member_funcs.push_back(std::move(member));
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

		if (!is_generic) {
			if (singe_instance == nullptr) {
				singe_instance = std::make_unique<FunctionInstance>();
				new_inst = singe_instance.get();
			}
			out = singe_instance.get();
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
					std::get<1>(*l)->move(new_offset, old_offset);
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
			new_inst->block = block;
			new_inst->name = name;
			new_inst->context = context;

			new_inst->generic_inst.insert_key_on_stack(*compiler);

			compiler->push_workspace(parent);

			CompileValue cvres;

			Cursor cc = decl_type;
			cc.move();
			if (cc.tok != RecognizedToken::CloseParenthesis) {
				while (true) {

					Cursor argname = cc;
					if (block.src) {
						cc.move();
						if (cc.tok != RecognizedToken::Colon) {
							throw_wrong_token_error(cc, "':'");
						}
						cc.move();
					}

					Cursor err = cc;
					Expression::parse(*compiler,cc, cvres, CompileType::eval);
					Expression::rvalue(*compiler,cvres, CompileType::eval);

					if (cvres.type != compiler->types()->t_type) {
						throw_cannot_cast_error(err, cvres.type, compiler->types()->t_type);
					}
					Type* t = compiler->evaluator()->pop_register_value<Type*>();
					new_inst->arguments.push_back(std::make_pair(argname, t));

					if (cc.tok == RecognizedToken::Comma) {
						cc.move();
					}
					else if (cc.tok == RecognizedToken::CloseParenthesis) {
						cc.move();
						break;
					}
					else {
						throw_wrong_token_error(cc, "',' or ')'");
					}
				}
			}
			else { cc.move(); }

			if (cc.tok != RecognizedToken::OpenBrace && cc.tok != RecognizedToken::Semicolon) {
				Cursor err = cc;
				Expression::parse(*compiler,cc, cvres, CompileType::eval);
				Expression::rvalue(*compiler,cvres, CompileType::eval);

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

			new_inst->type = compiler->types()->load_or_register_function_type(std::move(argtypes), new_inst->returns.second, new_inst->context);
			new_inst->compile_state = 1;

			compiler->pop_workspace();
		}

		compiler->compiler_stack()->pop();
		compiler->evaluator()->stack_pop();
	}

	void FunctionInstance::compile() {
		if (compile_state == 1) {
			compile_state = 2;
			if (block.src) {
				type->compile();
				auto func = compiler->global_module()->create_function();
				this->func = func;
				func->decl_id = type->il_function_decl;
				func->alias = name.buffer;

				ILBlock* b = func->create_and_append_block();
				b->alias = "entry";


				compiler->push_workspace(parent);
				compiler->push_scope_context(context);
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
					if (a.second->context() == ILContext::compile && context != ILContext::compile) {
						Cursor err = a.first;
						err.move();
						err.move();
						throw_specific_error(err, "Type is marked for compile time use only");
					}

					a.second->compile();
					uint16_t id = func->local_stack_lifetime.append(a.second->size());

					compiler->stack()->push_item(a.first.buffer, a.second, id, StackItemTag::regular);
				}



				uint16_t argid = (uint16_t)(arguments.size() - (ret_rval_stack ? 0 : 1));
				for (auto a = arguments.rbegin(); a != arguments.rend(); a++) {

					if (a->second->has_special_constructor()) {
						ILBuilder::build_local(compiler->scope(), argid);
						a->second->build_construct();
					}

					ILBuilder::build_local(compiler->scope(), argid);

					Expression::copy_from_rvalue(a->second, CompileType::compile, false);

					argid--;
				}

				if (ret_rval_stack) {
					ILBuilder::build_local(compiler->scope(), return_ptr_local_id);
					ILBuilder::build_store(compiler->scope(), ILDataType::word);
				}


				if (returns.second->context() == ILContext::compile && context != ILContext::compile) {
					throw_specific_error(returns.first, "Type is marked for compile time use only");
				}


				compile_state = 3;

				Cursor cb = block;
				BlockTermination term;
				Statement::parse_inner_block(*compiler,cb, term, true, &name);


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

			
			} else {
				auto func = compiler->global_module()->create_ext_function(); 
				this->func = func;
				func->alias = name.buffer;
				compile_state = 3;
			}
		}
		else if (compile_state == 3) {

		}
		else if (compile_state == 2) {
			throw_specific_error(name, "Build cycle");
		}
		else if (compile_state == 0) {
			throw_specific_error(name, "Build cycle");
		}
	}



	void StructureInstance::build_automatic_constructor() {
		auto func = compiler->global_module()->create_function();
		func->alias = "auto_ctor";

		ILBlock* block = func->create_and_append_block();
		block->alias = "entry";
		compiler->push_scope(block);

		ILBuilder::build_accept(block, ILDataType::none);

		uint16_t clones = 0;

		for (size_t i = 0; i < member_vars.size(); ++i) {
			if (member_vars[i].first->has_special_constructor()) {
				clones++;
			}
		}

		if (clones > 1) {
			ILBuilder::build_clone(block, ILDataType::word, clones);
		}

		auto do_lmbda = [&block, this](size_t ind, std::pair<Type*, uint32_t>& c_var) {
			if (c_var.first->has_special_constructor()) {

				if (size.type == ILSizeType::table) {
					ILBuilder::build_tableoffset(block, size.value, (uint16_t)ind);
				}
				else if (size.type == ILSizeType::absolute) {
					ILBuilder::build_aoffset(block, c_var.second);
				}
				else if (size.type == ILSizeType::word) {
					ILBuilder::build_woffset(block, c_var.second);
				}

				c_var.first->build_construct();
			}
		};

		for (size_t ind = 0; ind < member_vars.size(); ++ind) {
			do_lmbda(ind, member_vars[ind]);
		}

		ILBuilder::build_yield(block, ILDataType::none);
		ILBuilder::build_ret(block, ILDataType::none);

		auto_constructor = func;
		compiler->pop_scope();
	}

	void StructureInstance::build_automatic_destructor() {
		auto func = compiler->global_module()->create_function();
		func->alias = "auto_drop";

		ILBlock* block = func->create_and_append_block();
		block->alias = "entry";
		compiler->push_scope(block);

		ILBuilder::build_accept(block, ILDataType::none);

		uint16_t clones = 0;

		for (size_t i = 0; i < member_vars.size(); ++i) {
			if (member_vars[i].first->has_special_destructor()) {
				clones++;
			}
		}

		if (clones > 1) {
			ILBuilder::build_clone(block, ILDataType::word, clones);
		}


		auto do_lmbda = [&block, this](size_t ind, std::pair<Type*, uint32_t>& c_var) {
			if (c_var.first->has_special_destructor()) {

				if (size.type == ILSizeType::table) {
					ILBuilder::build_tableoffset(block, size.value, (uint16_t)ind);
				}
				else if (size.type == ILSizeType::absolute) {
					ILBuilder::build_aoffset(block, c_var.second);
				}
				else if (size.type == ILSizeType::word) {
					ILBuilder::build_woffset(block, c_var.second);
				}

				c_var.first->build_drop();
			}
		};

		for (size_t ind = 0; ind < member_vars.size(); ++ind) {
			do_lmbda(ind, member_vars[ind]);
		}

		ILBuilder::build_yield(block, ILDataType::none);
		ILBuilder::build_ret(block, ILDataType::none);

		auto_destructor = func;
		compiler->pop_scope();
	}

	void StructureInstance::build_automatic_move() {
		auto func = compiler->global_module()->create_function();
		func->alias = "auto_move";

		ILBlock* block = func->create_and_append_block();
		block->alias = "entry";
		compiler->push_scope(block);

		ILBuilder::build_accept(block, ILDataType::none);


		if (member_vars.size() > 1) {
			ILBuilder::build_clone_pair(block, ILDataType::word, (uint16_t)(member_vars.size()));
		}


		auto do_lmbda = [&block, this](size_t ind, std::pair<Type*, uint32_t>& c_var) {
			if (size.type == ILSizeType::table) {
				ILBuilder::build_tableoffset_pair(block, size.value, (uint16_t)ind);
			}
			else if (size.type == ILSizeType::absolute) {
				ILBuilder::build_aoffset_pair(block, c_var.second);
			}
			else if (size.type == ILSizeType::word) {
				ILBuilder::build_woffset_pair(block, c_var.second);
			}

			if (c_var.first->has_special_move()) {
				c_var.first->build_move();
			}
			else {
				ILBuilder::build_memcpy_rev(compiler->scope(), c_var.first->size());
			}
		};

		for (size_t ind = 0; ind < member_vars.size(); ++ind) {
			do_lmbda(ind, member_vars[ind]);
		}


		ILBuilder::build_ret(block, ILDataType::none);

		auto_move = func;
		compiler->pop_scope();
	}

	void StructureInstance::build_automatic_copy() {
		auto func = compiler->global_module()->create_function();
		func->alias = "auto_copy";

		ILBlock* block = func->create_and_append_block();
		block->alias = "entry";
		compiler->push_scope(block);

		ILBuilder::build_accept(block, ILDataType::none);

		if (member_vars.size() > 1) {
			ILBuilder::build_clone_pair(block, ILDataType::word, (uint16_t)(member_vars.size()));
		}

		auto do_lmbda = [&block, this](size_t ind, std::pair<Type*, uint32_t>& c_var) {
			if (size.type == ILSizeType::table) {
				ILBuilder::build_tableoffset_pair(block, size.value, (uint16_t)ind);
			}
			else if (size.type == ILSizeType::absolute) {
				ILBuilder::build_aoffset_pair(block, c_var.second);
			}
			else if (size.type == ILSizeType::word) {
				ILBuilder::build_woffset_pair(block, c_var.second);
			}

			if (c_var.first->has_special_copy()) {
				c_var.first->build_copy();
			}
			else {
				ILBuilder::build_memcpy_rev(compiler->scope(), c_var.first->size());
			}
		};

		for (size_t ind = 0; ind < member_vars.size(); ++ind) {
			do_lmbda(ind, member_vars[ind]);
		}

		ILBuilder::build_ret(block, ILDataType::none);

		auto_copy = func;
		compiler->pop_scope();
	}

	void StructureInstance::build_automatic_compare() {
		auto func = compiler->global_module()->create_function();
		func->alias = "auto_compare";

		ILBlock* block = func->create_and_append_block();
		ILBlock* block_return = func->create_block();
		block_return->alias = "exit";
		ILBuilder::build_accept(block_return, ILDataType::i8);
		ILBuilder::build_ret(block_return, ILDataType::i8);

		block->alias = "entry";
		compiler->push_scope(block);

		ILBuilder::build_accept(block, ILDataType::none);

		if (member_vars.size() > 1) {
			ILBuilder::build_clone_pair(block, ILDataType::word, (uint16_t)(member_vars.size()));
		}

		auto do_lmbda = [&block, &func, &block_return, this](size_t ind, std::pair<Type*, uint32_t>& c_var) {
			if (size.type == ILSizeType::table) {
				ILBuilder::build_tableoffset_pair(compiler->scope(), size.value, (uint16_t)ind);
			}
			else if (size.type == ILSizeType::absolute) {
				ILBuilder::build_aoffset_pair(compiler->scope(), c_var.second);
			}
			else if (size.type == ILSizeType::word) {
				ILBuilder::build_woffset_pair(compiler->scope(), c_var.second);
			}


			if (c_var.first->has_special_compare()) {
				c_var.first->build_compare();
			}
			else {
				ILBuilder::build_memcmp_rev(compiler->scope(), c_var.first->size());
			}

			if (ind < member_vars.size() - 1) {
				ILBlock* continue_block = func->create_and_append_block();

				ILBuilder::build_discard(continue_block, ILDataType::i8);
				ILBuilder::build_duplicate(compiler->scope(), ILDataType::i8);
				ILBuilder::build_yield(compiler->scope(), ILDataType::i8);
				ILBuilder::build_jmpz(compiler->scope(), continue_block, block_return);
				compiler->pop_scope();
				compiler->push_scope(continue_block);
			}
			else {
				ILBuilder::build_yield(compiler->scope(), ILDataType::i8);
				ILBuilder::build_jmp(compiler->scope(), block_return);
			}
		};

		for (size_t ind = 0; ind < member_vars.size(); ++ind) {
			do_lmbda(ind, member_vars[ind]);
		}


		func->append_block(block_return);
		auto_compare = func;
		compiler->pop_scope();
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

			bool has_special_constructor = false;
			bool has_special_destructor = false;
			bool has_special_copy = false;
			bool has_special_move = false;
			bool has_special_compare = false;

			for (auto&& m : member_vars) {
				m.first->compile();

				if (m.first->has_special_destructor())
					has_special_destructor = true;
				if (m.first->has_special_copy())
					has_special_copy = true;
				if (m.first->has_special_move())
					has_special_move = true;
				if (m.first->has_special_compare())
					has_special_compare = true;
				if (m.first->has_special_constructor())
					has_special_constructor = true;

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
				if (!gf->is_generic) {
					gf->compile();
					FunctionInstance* fi;
					gf->generate(nullptr, fi);

					if (fi->arguments.size() > 0 && fi->arguments[0].second == type.get()->generate_reference()) {
						member_table.insert(std::make_pair(fi->name.buffer, std::make_pair<uint16_t, MemberTableEntryType>((uint16_t)i, MemberTableEntryType::func)));
					}
				}
			}

			compile_state = 3;

			if (impl_copy) {
				impl_copy->compile();
				auto_copy = impl_copy->func;
				has_special_copy = true;
			}
			else {
				if (has_special_copy)
					build_automatic_copy();
			}

			if (impl_move) {
				impl_move->compile();
				auto_move = impl_move->func;
				has_special_move = true;
			}
			else {
				if (has_special_move)
					build_automatic_move();
			}

			if (impl_compare) {
				impl_compare->compile();
				auto_compare = impl_compare->func;
				has_special_compare = true;
			}
			else {
				if (has_special_compare)
					build_automatic_compare();
			}

			if (impl_drop) {
				impl_drop->compile();
				auto_destructor = impl_drop->func;
				has_special_destructor = true;
			}
			else {
				if (has_special_destructor)
					build_automatic_destructor();
			}

			if (impl_ctor) {
				impl_ctor->compile();
				auto_constructor = impl_ctor->func;
				has_special_constructor = true;
			}
			else {
				if (has_special_constructor)
					build_automatic_constructor();
			}


			compiler->compiler_stack()->pop();
			compiler->evaluator()->stack_pop();
			compiler->pop_scope_context();
		}
		else if (compile_state == 3) {

		}
		else {
			throw_specific_error(name, "Build cycle");
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
				compiler.compiler_stack()->push_item(std::get<0>(*key_l).buffer, std::get<1>(*key_l), sid, StackItemTag::alias);

				key_ptr += std::get<1>(*key_l)->size().eval(compiler.global_module(), compiler_arch);
			}

			
		}

	}


	void TraitInstance::generate_vtable(StructureInstance* forinst, uint32_t& optid) {
		forinst->compile();

		std::unique_ptr<void* []> vtable = std::make_unique<void* []>(member_funcs.size());

		auto& f_table = forinst->traitfunctions[this];
		size_t id = 0;
		for (auto&& m_func : member_funcs) {
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