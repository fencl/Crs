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

namespace Corrosive {

	void FunctionTemplate::compile() {
		if (compile_state == 0) {
			compile_state = 1;

			if (is_generic) {
				Cursor c = annotation;
				Ctx::push_workspace(parent);

				while (true) {
					

					Cursor name = c;
					c.move();
					if (c.tok != RecognizedToken::Colon) {
						throw_wrong_token_error(c, "':'");
					}
					c.move();
					Cursor err = c;
					CompileValue value;
					Expression::parse(c, value, CompileType::eval);
					Expression::rvalue(value, CompileType::eval);

					if (value.t != Ctx::types()->t_type) {
						throw_specific_error(err, "Expected type value");
					}

					Type* t = Ctx::eval()->pop_register_value<Type*>();

					/*if (t->type() != TypeInstanceType::type_structure_instance && t->type()!=TypeInstanceType::type_template && t->type() != TypeInstanceType::type_function) {
						throw_specific_error(err, "Type does not point to instance");
						return false;
					}*/

					t->compile();

					generic_ctx.generate_heap_size += t->size().eval(Ctx::global_module(), compiler_arch);
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
				
				Ctx::pop_workspace();
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

			Ctx::eval()->stack_push();
			Ctx::eval_stack()->push();
			if (generic_ctx.generator != nullptr) {
				generic_ctx.generator->insert_key_on_stack(Ctx::eval());
			}

			type = std::make_unique<TypeStructureTemplate>();
			type->owner = this;

			if (is_generic) {
				Cursor c = annotation;

				Ctx::push_workspace(parent);

				while (true) {

					Cursor name = c;
					c.move();
					if (c.tok != RecognizedToken::Colon) {
						throw_wrong_token_error(c, "':'");
					}
					c.move();
					Cursor err = c;
					CompileValue value;
					Expression::parse(c, value, CompileType::eval);
					Expression::rvalue(value, CompileType::eval);

					if (value.t != Ctx::types()->t_type) {
						throw_specific_error(err, "Expected type value");
					}

					Type* t = Ctx::eval()->pop_register_value<Type*>();

					/*if (t->type() != TypeInstanceType::type_structure_instance && t->type() != TypeInstanceType::type_template && t->type() != TypeInstanceType::type_function) {
						throw_specific_error(err, "Type does not point to instance");
						return false;
					}*/

					t->compile();
					
					generic_ctx.generate_heap_size += t->size().eval(Ctx::global_module(), compiler_arch);
					generic_ctx.generic_layout.push_back(std::make_tuple(name,t));


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
				instances = std::make_unique<std::map<unsigned char*, std::pair<std::unique_ptr<unsigned char[]>,std::unique_ptr<StructureInstance>>, GenericTemplateCompare>>(gen_template_cmp);

				Ctx::pop_workspace();
			}

			Ctx::eval()->stack_pop();
			Ctx::eval_stack()->pop();

			compile_state = 2;
		}
		else if (compile_state == 2) {

		}
		else {
			throw_specific_error(name,"compile cycle");
		}
	}

	void TraitTemplate::compile() {
		if (compile_state == 0) {
			compile_state = 1;
			
			Ctx::eval()->stack_push();
			Ctx::eval_stack()->push();
			if (generic_ctx.generator != nullptr) {
				generic_ctx.generator->insert_key_on_stack(Ctx::eval());
			}

			type = std::make_unique<TypeTraitTemplate>();
			type->owner = this;



			if (is_generic) {
				Cursor c = annotation;

				Ctx::push_workspace(parent);

				while (true) {

					Cursor name = c;
					c.move();
					if (c.tok != RecognizedToken::Colon) {
						throw_wrong_token_error(c, "':'");
					}
					c.move();
					Cursor err = c;
					CompileValue value;
					Expression::parse(c, value, CompileType::eval);
					Expression::rvalue(value, CompileType::eval);

					if (value.t != Ctx::types()->t_type) {
						throw_specific_error(err, "Expected type value");
					}

					Type* t = Ctx::eval()->pop_register_value<Type*>();

					/*if (t->type() != TypeInstanceType::type_structure_instance && t->type() != TypeInstanceType::type_template && t->type() != TypeInstanceType::type_function) {
						throw_specific_error(err, "Type does not point to instance");
						return false;
					}*/

					t->compile();
					
					generic_ctx.generate_heap_size += t->size().eval(Ctx::global_module(), compiler_arch);
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
				Ctx::pop_workspace();
			}

			Ctx::eval()->stack_pop();
			Ctx::eval_stack()->pop();
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

		Ctx::eval()->stack_push();
		Ctx::eval_stack()->push();

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
					std::get<1>(*l)->move(new_offset,old_offset);
					size_t c_size = std::get<1>(*l)->size().eval(Ctx::global_module(), compiler_arch);
					old_offset += c_size;
					new_offset += c_size;
				}

				instances->emplace(new_key, std::make_pair(std::move(new_key_inst),std::move(inst)));
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
			new_inst->name = name;
			new_inst->namespace_type = NamespaceType::t_struct_instance;
			new_inst->generic_inst.key = new_key;
			new_inst->generic_inst.generator = &generic_ctx;

			new_inst->generic_inst.insert_key_on_stack(Ctx::eval());

			Ctx::push_scope_context(ILContext::compile);
			Ctx::push_workspace(new_inst);


			for (auto&& m : member_funcs) {
				Cursor c = m.type;
				//TODO we could probably skip the catalogue stage and build the functiontemplate directly in the structure template


				std::unique_ptr<FunctionTemplate> ft = std::make_unique<FunctionTemplate>();
				ft->context = m.context;
				ft->name = m.name;
				ft->annotation = m.annotation;
				ft->is_generic = m.annotation.tok != RecognizedToken::Eof;
				ft->parent = new_inst;
				ft->generic_ctx.generator = &new_inst->generic_inst;
				ft->decl_type = m.type;
				ft->block = m.block;

				ft->type = std::make_unique<TypeFunctionTemplate>();
				ft->type->owner = ft.get();

				if (new_inst->subfunctions.find(m.name.buffer) != new_inst->subfunctions.end()) {
					throw_specific_error(m.name, "Funtion with the same name already exists in the structure");
				}

				new_inst->subfunctions[m.name.buffer] = std::move(ft);
			}

			for (auto&& t : member_templates) {
				Cursor tc = t.cursor;
				std::unique_ptr<StructureTemplate> decl;
				StructureTemplate::parse(tc, new_inst, &new_inst->generic_inst, decl);
				new_inst->subtemplates[decl->name.buffer] = std::move(decl);
			}



			for (auto&& m : member_implementation) {
				Cursor c = m.type;
				CompileValue value;

				Cursor err = c;
				Expression::parse(c, value, CompileType::eval);
				Expression::rvalue(value, CompileType::eval);

				if (value.t != Ctx::types()->t_type) {
					throw_specific_error(m.type, "Expected type value");
				}

				Type* t = Ctx::eval()->pop_register_value<Type*>();

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
						else if (trait[ttid->second]!=nullptr) {
							throw_specific_error(err, "Funtion with the same name already exists in the implementation");
						}


						func_count += 1;
						auto& fundecl = tt->owner->member_funcs[ttid->second];
						ft->context = fundecl.ctx;

						auto& args = Ctx::types()->argument_array_storage.get(fundecl.type->argument_array_id);

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
								Expression::parse(c, res, CompileType::eval);
								Expression::rvalue(res, CompileType::eval);
								if (res.t != Ctx::types()->t_type) {
									throw_specific_error(err, "Expected type");
								}
								Type* argt = Ctx::eval()->pop_register_value<Type*>();
								if (ft->arguments.size() == 0) {
									Type* this_type = new_inst->type->generate_reference();
									if (argt != this_type) {
										throw_specific_error(err, "First argument in implementation of trait function must be self reference to the structure");
									}

									ft->arguments.push_back(std::make_pair(name,argt));
								}
								else {
									if (ft->arguments.size()-1 >= args.size()) {
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
							Expression::parse(c, res, CompileType::eval);
							Expression::rvalue(res, CompileType::eval);

							if (res.t != Ctx::types()->t_type) {
								throw_specific_error(err, "Expected type");
							}
							Type* rett = Ctx::eval()->pop_register_value<Type*>();

							Type* req_type = fundecl.type->return_type;
							if (rett != req_type) {
								throw_specific_error(err, "Return type does not match the type of the original trait function");
							}

							ft->returns.first = err;
							ft->returns.second = rett;
						}
						else {
							ft->returns.second = Ctx::types()->t_void;
						}
						
						
						std::vector<Type*> argtypes;
						for (auto&& a : ft->arguments) {
							argtypes.push_back(a.second);
						}

						ft->type = Ctx::types()->load_or_register_function_type(std::move(argtypes), ft->returns.second,ft->context);
						ft->compile_state = 1;
						trait[ttid->second] = std::move(ft);

					}

					if (func_count != tt->owner->member_funcs.size()) {
						throw_specific_error(m.type, "Trait implementation is missing some functions");
					}
					
					if (tt->owner->generic_inst.generator == &Ctx::types()->tr_copy->generic_ctx) {
						new_inst->impl_copy = trait.begin()->get();
						new_inst->impl_copy->parent = new_inst;
					}

					if (tt->owner->generic_inst.generator == &Ctx::types()->tr_move->generic_ctx) {
						new_inst->impl_move = trait.begin()->get();
						new_inst->impl_move->parent = new_inst;
					}

					if (tt->owner->generic_inst.generator == &Ctx::types()->tr_compare->generic_ctx) {
						new_inst->impl_compare = trait.begin()->get();
						new_inst->impl_compare->parent = new_inst;
					}

					if (tt->owner->generic_inst.generator == &Ctx::types()->tr_drop->generic_ctx) {
						new_inst->impl_drop = trait.begin()->get();
						new_inst->impl_drop->parent = new_inst;
					}

					new_inst->traitfunctions[tt->owner] = std::move(trait);
					
				}
			}

			for (auto&& m : member_vars) {
				Cursor c = m.type;

				CompileValue value;

				Cursor err = c;
				Expression::parse(c, value, CompileType::eval);
				Expression::rvalue(value, CompileType::eval);
				if (value.t != Ctx::types()->t_type) {
					throw_specific_error(m.type, "Expected type value");
				}

				Type* m_t = Ctx::eval()->pop_register_value<Type*>();
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

				new_inst->member_table[m.name.buffer] = (uint16_t)new_inst->member_vars.size();
				
				// TODO BETTER CHECK
				/*if (m_t->size().absolute == 0 && m_t->size().pointers == 0) {
					throw_specific_error(m.name, "Specified type is not an instantiable type");
					std::cerr << " | \tType was '";
					m_t->print(std::cerr);
					std::cerr << "'\n";
					return false;
				}*/

				new_inst->member_vars.push_back(std::make_pair(m_t,0));
			}

			if (new_inst->context == ILContext::runtime) {
				for (auto&& f : new_inst->subfunctions) {
					f.second.get()->context = ILContext::runtime;
				}
			}

			if (new_inst->context == ILContext::compile) {
				for (auto&& f : new_inst->subfunctions) {
					f.second.get()->context = ILContext::compile;
				}
			}


			Ctx::pop_workspace();
			Ctx::pop_scope_context();
			
		}


		Ctx::eval_stack()->pop();
		Ctx::eval()->stack_pop();
	}


	void TraitTemplate::generate(unsigned char* argdata, TraitInstance*& out) {
		TraitInstance* new_inst = nullptr;
		unsigned char* new_key = nullptr;


		Ctx::eval()->stack_push();
		Ctx::eval_stack()->push();

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
					size_t c_size = std::get<1>(*l)->size().eval(Ctx::global_module(), compiler_arch);
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

			new_inst->parent = parent;
			new_inst->name = name;
			new_inst->generic_inst.key = new_key;
			new_inst->generic_inst.generator = &generic_ctx;

			new_inst->generic_inst.insert_key_on_stack(Ctx::eval());

			Ctx::push_workspace(parent);

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
						Expression::parse(c, val, CompileType::eval);
						Expression::rvalue(val, CompileType::eval);

						if (val.t != Ctx::types()->t_type) {
							throw_specific_error(err, "Expected type");
						}
						Type* t = Ctx::eval()->pop_register_value<Type*>();
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
					ret_type = Ctx::types()->t_void;
				}
				else {
					Cursor err = c;
					CompileValue val;
					Expression::parse(c, val, CompileType::eval);
					Expression::rvalue(val, CompileType::eval);

					if (val.t != Ctx::types()->t_type) {
						throw_specific_error(err, "Expected type");
					}
					Type* t = Ctx::eval()->pop_register_value<Type*>();
					ret_type = t;
				}


				member.type = Ctx::types()->load_or_register_function_type(std::move(args), ret_type, ILContext::both);
				
				new_inst->member_table[m.name.buffer] = new_inst->member_funcs.size();
				new_inst->member_funcs.push_back(std::move(member));
			}

			Ctx::pop_workspace();
		}

		Ctx::eval_stack()->pop();
		Ctx::eval()->stack_pop();

	}


	void FunctionTemplate::generate(unsigned char* argdata, FunctionInstance*& out) {
		FunctionInstance* new_inst = nullptr;
		unsigned char* new_key = nullptr;

		Ctx::eval()->stack_push();
		Ctx::eval_stack()->push();

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
					size_t c_size = std::get<1>(*l)->size().eval(Ctx::global_module(), compiler_arch);
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
			new_inst->generic_inst.key = new_key;
			new_inst->generic_inst.generator = &generic_ctx;
			new_inst->block = block;
			new_inst->name = name;
			new_inst->context = context;

			new_inst->generic_inst.insert_key_on_stack(Ctx::eval());

			Ctx::push_workspace(parent);

			CompileValue cvres;

			Cursor cc = decl_type;
			cc.move();
			if (cc.tok != RecognizedToken::CloseParenthesis) {
				while (true) {
					Cursor argname = cc;
					cc.move();
					if (cc.tok != RecognizedToken::Colon) {
						throw_wrong_token_error(cc, "':'");
					}
					cc.move();
					Cursor err = cc;
					Expression::parse(cc, cvres, CompileType::eval);
					Expression::rvalue(cvres, CompileType::eval);

					if (cvres.t != Ctx::types()->t_type) {
						throw_cannot_cast_error(err, cvres.t, Ctx::types()->t_type);
					}
					Type* t = Ctx::eval()->pop_register_value<Type*>();
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

			if (cc.tok != RecognizedToken::OpenBrace) {
				Cursor err = cc;
				Expression::parse(cc, cvres, CompileType::eval);
				Expression::rvalue(cvres, CompileType::eval);

				if (cvres.t != Ctx::types()->t_type) {
					throw_cannot_cast_error(err, cvres.t, Ctx::types()->t_type);
				}
				Type* t = Ctx::eval()->pop_register_value<Type*>();
				new_inst->returns.first = err;
				new_inst->returns.second = t;
			}
			else {
				new_inst->returns.second = Ctx::types()->t_void;
			}

			std::vector<Type*> argtypes;
			for (auto&& a : new_inst->arguments) {
				argtypes.push_back(a.second);
			}

			new_inst->type = Ctx::types()->load_or_register_function_type(std::move(argtypes), new_inst->returns.second,new_inst->context);
			new_inst->compile_state = 1;

			Ctx::pop_workspace();
		}

		Ctx::eval_stack()->pop();
		Ctx::eval()->stack_pop();
	}

	void FunctionInstance::compile() {
		if (compile_state == 1) {
			compile_state = 2;
			
			func = Ctx::global_module()->create_function();
			func->alias = name.buffer;

			ILBlock* b = func->create_and_append_block();
			b->alias = "entry";


			Ctx::push_workspace(parent);
			Ctx::push_scope_context(context);
			Ctx::push_function(func, returns.second);
			Ctx::push_scope(b);


			Ctx::eval()->stack_push();
			Ctx::eval_stack()->push();
			generic_inst.insert_key_on_stack(Ctx::eval());

			Ctx::stack()->push();
			Ctx::temp_stack()->push();

			Ctx::workspace_function()->local_stack_lifetime.push();
			Ctx::stack()->push_block();
			Ctx::temp_stack()->push_block();
			ILBuilder::build_accept(Ctx::scope(), ILDataType::none);

			bool ret_rval_stack = false;
			uint16_t return_ptr_local_id = 0;

			returns.second->compile();
			func->local_stack_lifetime.push();

			if (returns.second->rvalue_stacked()) {
				ret_rval_stack = true;
				return_ptr_local_id = func->local_stack_lifetime.append(Ctx::types()->t_ptr->size());
				func->returns = ILDataType::none;
			}
			else {
				func->returns = returns.second->rvalue();
			}

			for (auto&& a : arguments) {

				a.second->compile();
				if (a.second->context()==ILContext::compile && context != ILContext::compile) {
					Cursor err = a.first;
					err.move();
					err.move();
					throw_specific_error(err, "Type is marked for compile time use only");
				}

				a.second->compile();
				uint16_t id = func->local_stack_lifetime.append(a.second->size());
				func->arguments.push_back(a.second->rvalue());

				Ctx::stack()->push_item(a.first.buffer, a.second,id,StackItemTag::regular);
			}

			

			uint16_t argid = (uint16_t)(arguments.size()-(ret_rval_stack?0:1));
			for (auto a = arguments.rbegin(); a != arguments.rend(); a++) {
				
				if (a->second->has_special_constructor()) {
					ILBuilder::build_local(Ctx::scope(), argid);
					a->second->build_construct();
				}
				
				ILBuilder::build_local(Ctx::scope(), argid);

				Expression::copy_from_rvalue(a->second, CompileType::compile, false);

				argid--;
			}

			if (ret_rval_stack) {
				ILBuilder::build_local(Ctx::scope(), return_ptr_local_id);
				ILBuilder::build_store(Ctx::scope(), ILDataType::ptr);
			}


			if (returns.second->context() == ILContext::compile && context != ILContext::compile) {
				throw_specific_error(returns.first, "Type is marked for compile time use only");
			}


			compile_state = 3;

			Cursor cb = block;
			BlockTermination term;
			Statement::parse_inner_block(cb, term, true, &name);


			func->dump();
			std::cout << std::endl;

			func->assert_flow();

			Ctx::temp_stack()->pop_block();

			Ctx::stack()->pop();
			Ctx::temp_stack()->pop();
			Ctx::eval_stack()->pop();
			Ctx::eval()->stack_pop();

			Ctx::pop_workspace();
			Ctx::pop_scope_context();
			Ctx::pop_function();
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
		//TODO
	}

	void StructureInstance::build_automatic_destructor() {
		ILFunction* func = Ctx::global_module()->create_function();
		func->alias = "auto_drop";

		ILBlock* block = func->create_and_append_block();
		block->alias = "entry";
		Ctx::push_scope(block);

		ILBuilder::build_accept(block, ILDataType::none);

		uint16_t clones = 0;

		for (size_t i = 0; i < member_vars.size(); ++i) {
			if (member_vars[i].first->has_special_destructor()) {
				clones++;
			}
		}

		if (clones > 1) {
			ILBuilder::build_clone(block, ILDataType::ptr, clones);
		}

		size_t ind = 0;
		for (ind = 0; ind < member_vars.size(); ++ind) {
			auto& c_var = member_vars[ind];

			if (c_var.first->has_special_destructor()) {
				
				if (size.type == ILSizeType::table) {
					ILBuilder::build_tableoffset(block, size.value, (uint16_t)ind);
				}
				else if (size.type == ILSizeType::absolute){
					ILBuilder::build_aoffset(block, c_var.second);
				}
				else if (size.type == ILSizeType::word){
					ILBuilder::build_woffset(block, c_var.second);
				}
				
				c_var.first->build_drop();
			}
		}

		ILBuilder::build_yield(block, ILDataType::none);
		ILBuilder::build_ret(block, ILDataType::none);

		auto_destructor = func;
		Ctx::pop_scope();

		func->dump();
		std::cout << std::endl;
	}

	void StructureInstance::build_automatic_move() {
		//TODO
	}

	void StructureInstance::build_automatic_copy() {
		//TODO
	}

	void StructureInstance::build_automatic_compare() {
		//TODO
	}

	void StructureInstance::compile() {
		if (compile_state == 0) {
			compile_state = 1;

			Ctx::eval()->stack_push();
			Ctx::eval_stack()->push();
			generic_inst.insert_key_on_stack(Ctx::eval());
			
			ILStructTable table;

			size = ILSize(ILSizeType::absolute, 0);

			uint32_t max_align = 0;

			for (auto&& m : member_vars) {
				m.first->compile();

				/*if (m.first->has_special_constructor())
					has_special_constructor = true;*/

				if (m.first->has_special_destructor())
					has_special_destructor = true;

				ILSize m_s = m.first->size();

				if (m_s.type == ILSizeType::absolute && m_s.value<=4) { // up to 4 bytes always aligned to 4 bytes or less
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
					size.value = Ctx::global_module()->register_structure_table();
					Ctx::global_module()->structure_tables[size.value] = std::move(table);
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
				m.second->compile();
			}

			compile_state = 2;

			
			/*size_t abssize = size.eval(nctx.module->architecture);
			if (abssize <= 8) {
				structure_type = StructureInstanceType::compact_structure;

				if (abssize == 1)
					rvalue = ILDataType::u8;
				else if (abssize == 2)
					rvalue = ILDataType::u16;
				else if (abssize <= 4)
					rvalue = ILDataType::u32;
				else if (abssize <= 8)
					rvalue = ILDataType::u64;
			}
			else {*/

				structure_type = StructureInstanceType::normal_structure;
			
			//}

			/*if (has_special_constructor)
				build_automatic_constructor();
			if (has_special_destructor)
				build_automatic_destructor();*/

			if (impl_copy) {
				impl_copy->compile();
				auto_copy = impl_copy->func;
				has_special_copy = true;
			}

			if (impl_move) {
				impl_move->compile();
				auto_move = impl_move->func;
				has_special_move = true;
			}

			if (impl_compare) {
				impl_compare->compile();
				auto_compare = impl_compare->func;
				has_special_compare = true;
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


			Ctx::eval_stack()->pop();
			Ctx::eval()->stack_pop();
		}
		else if (compile_state == 2) {

		}
		else {
			throw_specific_error(name, "Build cycle");
		}
	}

	void GenericInstance::insert_key_on_stack(ILEvaluator* eval) {
		

		if (generator != nullptr) {
			
			if (generator->generator != nullptr) {
				generator->generator->insert_key_on_stack(eval);
			}


			unsigned char* key_ptr = key;
			for (auto key_l = generator->generic_layout.rbegin(); key_l != generator->generic_layout.rend(); key_l++) {

				uint16_t sid = Ctx::eval()->mask_local(key_ptr);
				Ctx::eval_stack()->push_item(std::get<0>(*key_l).buffer, std::get<1>(*key_l), sid, StackItemTag::alias);

				key_ptr += std::get<1>(*key_l)->size().eval(Ctx::global_module(),compiler_arch);
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
		}

		void** vt = vtable.get();
		uint32_t vtid = Ctx::global_module()->register_vtable(std::move(vtable));
		vtable_instances[forinst] = vtid;
		optid = vtid;
	}
}