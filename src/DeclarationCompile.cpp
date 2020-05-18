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

namespace Corrosive {

	bool FunctionTemplate::compile() {
		if (compile_state == 0) {
			compile_state = 1;

			if (is_generic) {
				Cursor c = annotation;
				CompileContext nctx = CompileContext::get();
				nctx.inside = parent;
				CompileContext::push(nctx);

				while (true) {
					

					Cursor name = c;
					c.move();
					if (c.tok != RecognizedToken::Colon) {
						throw_wrong_token_error(c, "':'");
						return false;
					}
					c.move();
					Cursor err = c;
					CompileValue value;
					if (!Expression::parse(c, value, CompileType::eval)) return false;
					if (value.t != nctx.default_types->t_type) {
						throw_specific_error(err, "Expected type value");
						return false;
					}

					Type* t = nctx.eval->pop_register_value<Type*>();

					/*if (t->type() != TypeInstanceType::type_structure_instance && t->type()!=TypeInstanceType::type_template && t->type() != TypeInstanceType::type_function) {
						throw_specific_error(err, "Type does not point to instance");
						return false;
					}*/

					if (!t->compile()) return false;

					generate_heap_size += t->compile_size(nctx.eval);
					generic_layout.push_back(std::make_tuple(name, t));


					if (c.tok == RecognizedToken::Comma) {
						c.move();
					}
					else if (c.tok == RecognizedToken::CloseParenthesis) {
						break;
					}
					else {
						throw_wrong_token_error(c, "',' or ')'");
						return false;
					}
				}



				gen_template_cmp.parent = this;
				instances = std::make_unique<std::map<std::pair<unsigned int, unsigned char*>, std::unique_ptr<FunctionInstance>, GenericTemplateCompare>>(gen_template_cmp);
				CompileContext::pop();
			}

			compile_state = 2;
		}
		else if (compile_state == 2) {
			return true;
		}
		else {
			throw_specific_error(name, "compile cycle");
			return false;
		}


		return true;
	}


	bool StructureTemplate::compile() {
		if (compile_state == 0) {
			compile_state = 1;

			type = std::make_unique<TypeStructureTemplate>();
			type->owner = this;

			if (is_generic) {
				Cursor c = annotation;

				CompileContext nctx = CompileContext::get();
				nctx.inside = parent;
				CompileContext::push(nctx);

				while (true) {

					Cursor name = c;
					c.move();
					if (c.tok != RecognizedToken::Colon) {
						throw_wrong_token_error(c, "':'");
						return false;
					}
					c.move();
					Cursor err = c;
					CompileValue value;
					if (!Expression::parse(c, value, CompileType::eval)) return false;
					if (value.t != nctx.default_types->t_type) {
						throw_specific_error(err, "Expected type value");
						return false;
					}

					Type* t = nctx.eval->pop_register_value<Type*>();

					/*if (t->type() != TypeInstanceType::type_structure_instance && t->type() != TypeInstanceType::type_template && t->type() != TypeInstanceType::type_function) {
						throw_specific_error(err, "Type does not point to instance");
						return false;
					}*/

					if (!t->compile()) return false;
					
					generate_heap_size += t->compile_size(nctx.eval);
					generic_layout.push_back(std::make_tuple(name,t));


					if (c.tok == RecognizedToken::Comma) {
						c.move();
					}
					else if (c.tok == RecognizedToken::CloseParenthesis) {
						break;
					}
					else {
						throw_wrong_token_error(c, "',' or ')'");
						return false;
					}
				}


				gen_template_cmp.parent = this;
				instances = std::make_unique<std::map<unsigned char*, std::pair<std::unique_ptr<unsigned char[]>,std::unique_ptr<StructureInstance>>, GenericTemplateCompare>>(gen_template_cmp);

				CompileContext::pop();
			}

			compile_state = 2;
		}
		else if (compile_state == 2) {
			return true;
		}
		else {
			throw_specific_error(name,"compile cycle");
			return false;
		}


		return true;
	}

	bool TraitTemplate::compile() {
		if (compile_state == 0) {
			compile_state = 1;
			
			type = std::make_unique<TypeTraitTemplate>();
			type->owner = this;

			if (is_generic) {
				Cursor c = annotation;

				CompileContext nctx = CompileContext::get();
				nctx.inside = parent;
				CompileContext::push(nctx);

				while (true) {

					Cursor name = c;
					c.move();
					if (c.tok != RecognizedToken::Colon) {
						throw_wrong_token_error(c, "':'");
						return false;
					}
					c.move();
					Cursor err = c;
					CompileValue value;
					if (!Expression::parse(c, value, CompileType::eval)) return false;
					if (value.t != nctx.default_types->t_type) {
						throw_specific_error(err, "Expected type value");
						return false;
					}

					Type* t = nctx.eval->pop_register_value<Type*>();

					/*if (t->type() != TypeInstanceType::type_structure_instance && t->type() != TypeInstanceType::type_template && t->type() != TypeInstanceType::type_function) {
						throw_specific_error(err, "Type does not point to instance");
						return false;
					}*/

					if (!t->compile()) return false;

					generate_heap_size += t->compile_size(nctx.eval);
					generic_layout.push_back(std::make_tuple(name, t));


					if (c.tok == RecognizedToken::Comma) {
						c.move();
					}
					else if (c.tok == RecognizedToken::CloseParenthesis) {
						break;
					}
					else {
						throw_wrong_token_error(c, "',' or ')'");
						return false;
					}
				}


				gen_template_cmp.parent = this;
				instances = std::make_unique<std::map<unsigned char*, std::pair<std::unique_ptr<unsigned char[]>, std::unique_ptr<TraitInstance>>, GenericTemplateCompare>>(gen_template_cmp);
				CompileContext::pop();
			}

			compile_state = 2;
		}
		else if (compile_state == 2) {
			return true;
		}
		else {
			throw_specific_error(name, "compile cycle");
			return false;
		}


		return true;
	}



	bool StructureTemplate::generate(unsigned char* argdata, StructureInstance*& out) {
		CompileContext& nctx = CompileContext::get();

		StructureInstance* new_inst = nullptr;
		unsigned char* new_key = nullptr;

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

				std::unique_ptr<unsigned char[]> new_key_inst = std::make_unique<unsigned char[]>(generate_heap_size);
				unsigned char* old_offset = argdata;
				unsigned char* new_offset = new_key_inst.get();
				new_key = new_key_inst.get();

				for (auto l = generic_layout.rbegin(); l != generic_layout.rend(); l++) {
					std::get<1>(*l)->move(nctx.eval, old_offset, new_offset);
					uint32_t c_size = std::get<1>(*l)->compile_size(nctx.eval);
					old_offset += c_size;
					new_offset += c_size;
				}

				instances->emplace(new_key, std::make_pair(std::move(new_key_inst),std::move(inst)));

			}
			else {
				out = f->second.second.get();
			}
		}


		if (new_inst != nullptr) {
			new_inst->type = std::make_unique<TypeStructureInstance>();
			new_inst->type->owner = new_inst;
			new_inst->generator = this;
			new_inst->parent = parent;
			new_inst->name = name;
			new_inst->namespace_type = NamespaceType::t_struct_instance;
			new_inst->key = new_key;


			CompileContext nctx = CompileContext::get();
			nctx.inside = new_inst;
			nctx.scope_context = ILContext::compile;
			CompileContext::push(nctx);

			for (auto&& m : member_funcs) {
				Cursor c = m.type;
				//TODO we could probably skip the catalogue stage and build the functiontemplate directly in the structure template


				std::unique_ptr<FunctionTemplate> ft = std::make_unique<FunctionTemplate>();
				ft->context = m.context;
				ft->name = m.name;
				ft->annotation = m.annotation;
				ft->is_generic = m.annotation.tok != RecognizedToken::Eof;
				ft->parent = new_inst;
				ft->template_parent = new_inst;
				ft->decl_type = m.type;
				ft->block = m.block;

				ft->type = std::make_unique<TypeFunctionTemplate>();
				ft->type->owner = ft.get();

				if (new_inst->subfunctions.find(m.name.buffer) != new_inst->subfunctions.end()) {
					throw_specific_error(m.name, "Funtion with the same name already exists in the structure");
					return false;
				}

				new_inst->subfunctions[m.name.buffer] = std::move(ft);
			}

			for (auto&& t : member_templates) {
				Cursor tc = t.cursor;
				std::unique_ptr<StructureTemplate> decl;
				if (!StructureTemplate::parse(tc, new_inst, decl)) return false;
				new_inst->subtemplates[decl->name.buffer] = std::move(decl);
			}



			for (auto&& m : member_implementation) {
				Cursor c = m.type;
				CompileValue value;

				Cursor err = c;
				if (!Expression::parse(c, value, CompileType::eval)) return false;
				if (value.t != nctx.default_types->t_type) {
					throw_specific_error(m.type, "Expected type value");
					std::cerr << " | \tType was '";
					value.t->print(std::cerr);
					std::cerr << "'\n";
					return false;
				}

				Type* t = nctx.eval->pop_register_value<Type*>();

				if (t->type() != TypeInstanceType::type_trait) {
					throw_specific_error(m.type, "Expected trait instance type");
					std::cerr << " |\tType was ";
					t->print(std::cerr);
					std::cerr << std::endl;
					return false;
				}

				if (!t->compile()) return false;

				TypeTraitInstance* tt = (TypeTraitInstance*)t;
				if (new_inst->traitfunctions.find(tt->owner) != new_inst->traitfunctions.end()) {
					throw_specific_error(err, "This trait was already implemented");
					return false;
				}
				else {
					std::vector<std::unique_ptr<FunctionInstance>> trait(tt->owner->member_funcs.size());

					unsigned int func_count = 0;
					for (auto&& f : m.functions) {

						std::unique_ptr<FunctionInstance> ft = std::make_unique<FunctionInstance>();
						ft->compile_state = 0;
						ft->key = nullptr;
						ft->block = f.block;
						ft->name = f.name;
						ft->parent = (Namespace*)this;

						auto ttid = tt->owner->member_table.find(f.name.buffer);

						if (ttid == tt->owner->member_table.end()) {
							throw_specific_error(f.name, "Implemented trait has no function with this name");
							return false;
						}
						else if (trait[ttid->second]!=nullptr) {
							throw_specific_error(f.name, "Funtion with the same name already exists in the implementation");
							return false;
						}


						func_count += 1;
						auto& fundecl = tt->owner->member_funcs[ttid->second];

						auto& args = nctx.default_types->argument_array_storage.get(fundecl.type->argument_array_id);

						Cursor c = f.type;
						c.move();
						if (c.tok != RecognizedToken::CloseParenthesis) {
							while (true) {
								if (c.tok != RecognizedToken::Symbol) {
									throw_not_a_name_error(c);
									return false;
								}
								Cursor name = c;
								c.move();
								if (c.tok != RecognizedToken::Colon) {
									throw_wrong_token_error(c, "':'");
									return false;
								}
								c.move();

								Cursor err = c;
								CompileValue res;
								if (!Expression::parse(c, res, CompileType::eval)) return false;
								if (res.t != nctx.default_types->t_type) {
									throw_specific_error(err, "Expected type");
									return false;
								}
								Type* argt = nctx.eval->pop_register_value<Type*>();
								if (ft->arguments.size() == 0) {
									Type* this_type = new_inst->type->generate_reference();
									if (argt != this_type) {
										throw_specific_error(err, "First argument in implementation of trait function must be self reference to the structure");
										return false;
									}

									ft->arguments.push_back(std::make_pair(name,argt));
								}
								else {
									if (ft->arguments.size()-1 >= args.size()) {
										throw_specific_error(err, "There are more arguments than in the original trait function");
										return false;
									}

									Type* req_type = args[ft->arguments.size() - 1];
									if (argt != req_type) {
										throw_specific_error(err, "Argument does not match the type of the original trait function");
										return false;
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
									return false;
								}
							}
						}

						if (ft->arguments.size() != args.size() + 1) {
							throw_specific_error(c, "Trait function declaration lacks arguments from the original");
							return false;
						}

						c.move();

						if (c.tok != RecognizedToken::OpenBrace) {
							Cursor err = c;
							CompileValue res;
							if (!Expression::parse(c, res, CompileType::eval)) return false;
							if (res.t != nctx.default_types->t_type) {
								throw_specific_error(err, "Expected type");
								return false;
							}
							Type* rett = nctx.eval->pop_register_value<Type*>();

							Type* req_type = fundecl.type->return_type;
							if (rett != req_type) {
								throw_specific_error(err, "Return type does not match the type of the original trait function");
								std::cerr << " |\tRequired type was: "; req_type->print(std::cerr);
								std::cerr << "\n";
								return false;
							}

							ft->returns.first = err;
							ft->returns.second = rett;
						}
						else {
							ft->returns.second = nctx.default_types->t_void;
						}
						
						
						std::vector<Type*> argtypes;
						for (auto&& a : ft->arguments) {
							argtypes.push_back(a.second);
						}

						ft->type = nctx.default_types->load_or_register_function_type(std::move(argtypes), ft->returns.second,ft->context);
						ft->compile_state = 1;
						trait[ttid->second] = std::move(ft);

					}

					if (func_count != tt->owner->member_funcs.size()) {
						throw_specific_error(m.type, "Trait implementation is missing some functions");
						return false;
					}

					new_inst->traitfunctions[tt->owner] = std::move(trait);
				}
			}

			for (auto&& m : member_vars) {
				Cursor c = m.type;

				CompileValue value;

				Cursor err = c;
				if (!Expression::parse(c, value, CompileType::eval)) return false;
				if (value.t != nctx.default_types->t_type) {
					throw_specific_error(m.type, "Expected type value");
					std::cerr << " | \tType was '";
					value.t->print(std::cerr);
					std::cerr << "'\n";
					return false;
				}

				Type* m_t = nctx.eval->pop_register_value<Type*>();
				if (m_t->context() == ILContext::compile) {
					if (new_inst->context == ILContext::both || new_inst->context == ILContext::compile) {
						new_inst->context = ILContext::compile;
					}
					else {
						throw_specific_error(err, "Cannot use compile type in runtime-only structure");
						return false;
					}
				}
				else if (m_t->context() == ILContext::runtime || new_inst->context == ILContext::runtime) {
					if (new_inst->context == ILContext::both) {
						new_inst->context = ILContext::runtime;
					}
					else {
						throw_specific_error(err, "Cannot use runtime type in compile-only structure");
						return false;
					}
				}

				new_inst->member_table[m.name.buffer] = new_inst->member_vars.size();
				StructureInstanceMemberRecord rec;
				rec.definition = m.name;
				rec.type = m_t;

				new_inst->member_vars.push_back(rec);
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


			CompileContext::pop();
			
		}

		return true;
	}


	bool TraitTemplate::generate(unsigned char* argdata, TraitInstance*& out) {
		CompileContext& nctx = CompileContext::get();
		TraitInstance* new_inst = nullptr;
		unsigned char* new_key = nullptr;

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

				std::unique_ptr<unsigned char[]> new_key_inst = std::make_unique<unsigned char[]>(generate_heap_size);
				new_key = new_key_inst.get();
				unsigned char* old_offset = argdata;
				unsigned char* new_offset = new_key_inst.get();

				for (auto l = generic_layout.rbegin(); l != generic_layout.rend(); l++) {
					std::get<1>(*l)->move(nctx.eval, old_offset, new_offset);
					uint32_t c_size = std::get<1>(*l)->compile_size(nctx.eval);
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

			new_inst->generator = this;
			new_inst->parent = parent;
			new_inst->name = name;
			new_inst->key = new_key;

			CompileContext nctx = CompileContext::get();
			nctx.inside = parent;
			CompileContext::push(nctx);

			for (auto&& m : member_funcs) {



				TraitInstanceMemberRecord member;
				member.name = m.name;
				member.definition = m.type;
				Cursor c = member.definition;

				std::vector<Type*> args;
				Type* ret_type;

				if (c.tok != RecognizedToken::OpenParenthesis) {
					throw_wrong_token_error(c, "'('");
					return false;
				}

				c.move();

				if (c.tok != RecognizedToken::CloseParenthesis) {
					while (true) {
						Cursor err = c;
						CompileValue val;
						if (!Expression::parse(c, val, CompileType::eval)) return false;

						if (val.t != nctx.default_types->t_type) {
							throw_specific_error(err, "Expected type");
							return false;
						}
						Type* t = nctx.eval->pop_register_value<Type*>();
						args.push_back(t);
						if (c.tok == RecognizedToken::Comma) {
							c.move();
						}
						else if (c.tok == RecognizedToken::CloseParenthesis) {
							break;
						}
						else {
							throw_wrong_token_error(c, "',' or ')'");
							return false;
						}
					}
				}
				c.move();

				if (c.tok == RecognizedToken::Semicolon) {
					ret_type = nctx.default_types->t_void;
				}
				else {
					Cursor err = c;
					CompileValue val;
					if (!Expression::parse(c, val, CompileType::eval)) return false;

					if (val.t != nctx.default_types->t_type) {
						throw_specific_error(err, "Expected type");
						return false;
					}
					Type* t = nctx.eval->pop_register_value<Type*>();
					ret_type = t;
				}


				member.type = nctx.default_types->load_or_register_function_type(std::move(args), ret_type, ILContext::both);
				
				new_inst->member_table[m.name.buffer] = new_inst->member_funcs.size();
				new_inst->member_funcs.push_back(std::move(member));
			}

			CompileContext::pop();
		}

		return true;
	}


	bool FunctionTemplate::generate(unsigned char* argdata, FunctionInstance*& out) {
		FunctionInstance* new_inst = nullptr;
		unsigned char* new_key = nullptr;

		if (!is_generic) {
			if (singe_instance == nullptr) {
				singe_instance = std::make_unique<FunctionInstance>();
				new_inst = singe_instance.get();
			}
			out = singe_instance.get();
		}
		else {
			std::pair<unsigned int, unsigned char*> key = std::make_pair((unsigned int)generate_heap_size, argdata);

			auto f = instances->find(key);

			if (f == instances->end()) {
				std::unique_ptr<FunctionInstance> inst = std::make_unique<FunctionInstance>();

				new_inst = inst.get();
				out = inst.get();
				key.second = (unsigned char*)malloc(generate_heap_size);
				memcpy(key.second, argdata, generate_heap_size);
				new_key = key.second;

				instances->emplace(key, std::move(inst));

			}
			else {
				out = f->second.get();
			}
		}


		if (new_inst != nullptr) {
			new_inst->compile_state = 0;
			new_inst->parent = parent;
			new_inst->key = new_key;
			new_inst->block = block;
			new_inst->name = name;
			new_inst->context = context;

			CompileContext nctx = CompileContext::get();
			nctx.inside = parent;
			CompileContext::push(nctx);

			CompileValue cvres;

			Cursor cc = decl_type;
			cc.move();
			if (cc.tok != RecognizedToken::CloseParenthesis) {
				while (true) {
					Cursor argname = cc;
					cc.move();
					if (cc.tok != RecognizedToken::Colon) {
						throw_wrong_token_error(cc, "':'");
						return false;
					}
					cc.move();
					Cursor err = cc;
					if (!Expression::parse(cc, cvres, CompileType::eval))return false;
					if (cvres.t != nctx.default_types->t_type) {
						throw_cannot_cast_error(err, cvres.t, nctx.default_types->t_type);
						return false;
					}
					Type* t = nctx.eval->pop_register_value<Type*>();
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
						return false;
					}
				}
			}
			else { cc.move(); }

			if (cc.tok != RecognizedToken::OpenBrace) {
				Cursor err = cc;
				if (!Expression::parse(cc, cvres, CompileType::eval))return false;
				if (cvres.t != nctx.default_types->t_type) {
					throw_cannot_cast_error(err, cvres.t, nctx.default_types->t_type);
					return false;
				}
				Type* t = nctx.eval->pop_register_value<Type*>();
				new_inst->returns.first = err;
				new_inst->returns.second = t;
			}
			else {
				new_inst->returns.second = nctx.default_types->t_void;
			}

			std::vector<Type*> argtypes;
			for (auto&& a : new_inst->arguments) {
				argtypes.push_back(a.second);
			}

			new_inst->type = nctx.default_types->load_or_register_function_type(std::move(argtypes), new_inst->returns.second,new_inst->context);
			new_inst->compile_state = 1;

			CompileContext::pop();
		}

		return true;
	}

	bool FunctionInstance::compile() {
		if (compile_state == 1) {
			compile_state = 2;


			func = CompileContext::get().module->create_function();
			func->alias = name.buffer;

			ILBlock* b = func->create_and_append_block(ILDataType::none);
			b->alias = "entry";


			CompileContext cctx = CompileContext::get();
			cctx.inside = parent;
			cctx.scope = b;
			cctx.function = func;
			cctx.function_returns = returns.second;
			cctx.scope_context = context;

			CompileContext::push(cctx);
			CompileContext& nctx = CompileContext::get();

			auto ss = std::move(StackManager::move_stack_out<0>());
			bool ret_rval_stack = false;
			uint16_t return_ptr_local_id = 0;


			if (returns.second->rvalue_stacked()) {
				ret_rval_stack = true;
				return_ptr_local_id = func->register_local(returns.second->compile_size(nctx.eval), returns.second->size(nctx.eval));
				func->returns = ILDataType::none;
			}
			else {
				func->returns = returns.second->rvalue();
			}

			for (auto&& a : arguments) {

				if (a.second->context()==ILContext::compile && context != ILContext::compile) {
					Cursor err = a.first;
					err.move();
					err.move();
					throw_specific_error(err, "Type is marked for compile time use only");
					return false;
				}

				if (!a.second->compile()) return false;

				CompileValue argval;
				argval.t = a.second;
				argval.lvalue = true;
				uint16_t id = func->register_local(argval.t->compile_size(nctx.eval), argval.t->size(nctx.eval));
				func->arguments.push_back(a.second->rvalue());
				StackManager::stack_push<0>(nctx.eval,a.first.buffer, argval, id);
			}

			

			uint16_t argid = (uint16_t)(arguments.size()-(ret_rval_stack?0:1));
			for (auto a = arguments.rbegin(); a != arguments.rend(); a++) {

				if (a->second->has_special_constructor()) {
					ILBuilder::build_local(nctx.scope, argid);
					a->second->build_construct();
				}

				ILBuilder::build_local(nctx.scope, argid);
				a->second->build_copy();
				argid--;
			}

			if (returns.second->rvalue_stacked()) {
				ILBuilder::build_local(nctx.scope, return_ptr_local_id);
				ILBuilder::build_store(nctx.scope, ILDataType::ptr);
			}


			if (returns.second->context() == ILContext::compile && context != ILContext::compile) {
				throw_specific_error(returns.first, "Type is marked for compile time use only");
				return false;
			}

			

			Cursor cb = block;
			CompileValue cvres;
			bool terminated;
			if (!Statement::parse_inner_block(cb, cvres, terminated)) return false;


			ILBlock* b_exit = func->create_and_append_block(func->returns);

			ILBuilder::build_yield(nctx.scope, func->returns);
			ILBuilder::build_jmp(nctx.scope, b_exit);

			nctx.scope = b_exit;
			b_exit->alias = "entry_exit";

			ILBuilder::build_accept(nctx.scope,func->returns);


			while (StackManager::stack_state<0>() > 0) {
				StackItem sitm = StackManager::stack_pop<0>(nctx.eval);

				if (sitm.value.t->has_special_destructor()) {
					ILBuilder::build_local(nctx.scope, sitm.id);
					sitm.value.t->build_drop();
				}
			}


			ILBuilder::build_ret(nctx.scope,func->returns);



			//func->dump();
			//std::cout << std::endl;

			if (!func->assert_flow()) return false;

			StackManager::move_stack_in<0>(std::move(ss));


			CompileContext::pop();
			compile_state = 3;
		}
		else if (compile_state == 3) {
			return true;
		}
		else if (compile_state == 2) {
			throw_specific_error(name, "Build cycle");
			return false;
		}
		else if (compile_state == 0) {
			throw_specific_error(name, "Build cycle");
			return false;
		}


		return true;
	}



	unsigned int _align_up(unsigned int value, unsigned int alignment) {
		return alignment == 0 ? value : ((value % alignment == 0) ? value : value + (alignment - (value % alignment)));
	}


	void StructureInstance::build_automatic_constructor() {
		CompileContext& nctx_old = CompileContext::get();
		auto_constructor = nctx_old.module->create_function();
		CompileContext cctx = nctx_old;

		ILBlock* block = auto_constructor->create_and_append_block(ILDataType::none);

		cctx.inside = this;
		cctx.scope = block;
		cctx.function = auto_constructor;
		cctx.function_returns = nctx_old.default_types->t_void;
		cctx.scope_context = context;
		CompileContext::push(cctx);

		auto ss = std::move(StackManager::move_stack_out<0>());

		CompileContext& nctx = CompileContext::get();


		uint16_t self_id = auto_constructor->register_local(nctx.eval->get_compile_pointer_size(), nctx.eval->get_pointer_size());
		ILBuilder::build_store(block,ILDataType::ptr);

		for (auto&& m : member_vars) {
			if (m.type->has_special_constructor())
			{
				ILBuilder::build_local(block, self_id);
				ILBuilder::build_member(block, m.compile_offset, m.offset);

				m.type->build_construct();
			}
		}

		StackManager::move_stack_in<0>(ss);
		CompileContext::pop();
	}

	void StructureInstance::build_automatic_destructor() {

	}

	bool StructureInstance::compile() {
		if (compile_state == 0) {
			compile_state = 1;

			CompileContext& nctx = CompileContext::get();

			alignment = 0;
			size = 0;

			compile_alignment = 0;
			compile_size = 0;

			for (auto&& m : member_vars) {
				if (!m.type->compile()) return false;
				
				if (m.type->has_special_constructor())
					has_special_constructor = true;

				if (m.type->has_special_destructor())
					has_special_destructor = true;

				if (m.type->size(nctx.eval) > 0) {
					size = _align_up(size, m.type->alignment(nctx.eval));
					compile_size = _align_up(compile_size, m.type->compile_alignment(nctx.eval));
					m.offset = size;
					m.compile_offset = compile_size;
					size += m.type->size(nctx.eval);
					compile_size += m.type->compile_size(nctx.eval);

					alignment = std::max(alignment, m.type->alignment(nctx.eval));
					compile_alignment = std::max(compile_alignment, m.type->compile_alignment(nctx.eval));
				}
				else {
					throw_specific_error(m.definition, "Specified type is not an instantiable type");
					std::cerr << " | \tType was '";
					m.type->print(std::cerr);
					std::cerr << "'\n";
					return false;
				}
			}

			for (auto&& m : subfunctions) {
				if (!m.second->compile()) return false;
			}

			size = _align_up(size, alignment);
			compile_size = _align_up(compile_size, compile_alignment);
			compile_state = 2;

			if (size <= 8) {
				structure_type = StructureInstanceType::compact_structure;

				if (size == 1)
					rvalue = ILDataType::u8;
				else if (size == 2)
					rvalue = ILDataType::u16;
				else if (size <= 4)
					rvalue = ILDataType::u32;
				else if (size <= 8)
					rvalue = ILDataType::u64;
			}
			else {
				structure_type = StructureInstanceType::normal_structure;
			}

			if (has_special_constructor)
				build_automatic_constructor();
			if (has_special_destructor)
				build_automatic_destructor();
		}
		else if (compile_state == 2) {
			return true;
		}
		else {
			throw_specific_error(generator->name, "Build cycle");
			return false;
		}

		return true;
	}

	void TraitInstance::insert_key_on_stack(ILEvaluator* eval) {
		if (generator->template_parent != nullptr) {
			generator->template_parent->insert_key_on_stack(eval);
		}

		unsigned char* key_ptr = key;
		for (auto key_l = generator->generic_layout.rbegin(); key_l != generator->generic_layout.rend(); key_l++) {

			CompileValue res;
			res.lvalue = true;
			res.t = std::get<1>(*key_l);

			unsigned char* data_place = eval->stack_reserve(res.t->compile_size(eval));

			StackManager::stack_push<1>(eval, std::get<0>(*key_l).buffer, res,0);

			res.t->copy(eval, key_ptr, data_place);

			key_ptr += res.t->compile_size(eval);
		}

	}



	void StructureInstance::insert_key_on_stack(ILEvaluator* eval) {
		if (generator->template_parent != nullptr) {
			generator->template_parent->insert_key_on_stack(eval);
		}

		unsigned char* key_ptr = key;
		for (auto key_l = generator->generic_layout.rbegin(); key_l != generator->generic_layout.rend(); key_l++) {
			
		
			CompileValue res;
			res.lvalue = true;
			res.t = std::get<1>(*key_l);

			unsigned char* data_place = eval->stack_reserve(res.t->compile_size(eval));
			
			StackManager::stack_push<1>(eval,std::get<0>(*key_l).buffer, res,0);

			res.t->copy(eval, key_ptr, data_place);

			key_ptr += res.t->compile_size(eval);
		}

	}


	bool TraitInstance::generate_vtable(StructureInstance* forinst, uint32_t& optid) {
		if (!forinst->compile()) return false;

		std::unique_ptr<void* []> vtable = std::make_unique<void* []>(member_funcs.size());

		auto& f_table = forinst->traitfunctions[this];
		size_t id = 0;
		for (auto&& m_func : member_funcs) {
			FunctionInstance* finst = f_table[id].get();
			if (!finst->compile()) return false;
			vtable[id] = finst->func;
		}

		void** vt = vtable.get();
		CompileContext& nctx = CompileContext::get();
		uint32_t vtid = nctx.module->register_vtable(std::move(vtable));
		vtable_instances[forinst] = vtid;
		optid = vtid;
		return true;
	}
}