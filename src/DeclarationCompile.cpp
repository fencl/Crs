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
					if (!Expression::rvalue(value, CompileType::eval)) return false;
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

					generic_ctx.generate_heap_size += t->size().eval(compiler_arch);
					generic_ctx.generic_layout.push_back(std::make_tuple(name, t));


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
				instances = std::make_unique<std::map<unsigned char*, std::pair<std::unique_ptr<unsigned char[]>, std::unique_ptr<FunctionInstance>>, GenericTemplateCompare>>(gen_template_cmp);
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
					if (!Expression::rvalue(value, CompileType::eval)) return false;

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
					
					generic_ctx.generate_heap_size += t->size().eval(compiler_arch);
					generic_ctx.generic_layout.push_back(std::make_tuple(name,t));


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
					if (!Expression::rvalue(value, CompileType::eval)) return false;

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

					generic_ctx.generate_heap_size += t->size().eval(compiler_arch);
					generic_ctx.generic_layout.push_back(std::make_tuple(name, t));


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

				std::unique_ptr<unsigned char[]> new_key_inst = std::make_unique<unsigned char[]>(generic_ctx.generate_heap_size);
				unsigned char* old_offset = argdata;
				unsigned char* new_offset = new_key_inst.get();
				new_key = new_key_inst.get();

				//memcpy(new_offset, old_offset, generic_ctx.generate_heap_size);

				for (auto l = generic_ctx.generic_layout.rbegin(); l != generic_ctx.generic_layout.rend(); l++) {
					std::get<1>(*l)->move(nctx.eval, new_offset,old_offset);
					size_t c_size = std::get<1>(*l)->size().eval(compiler_arch);
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
			new_inst->parent = parent;
			new_inst->name = name;
			new_inst->namespace_type = NamespaceType::t_struct_instance;
			new_inst->generic_inst.key = new_key;
			new_inst->generic_inst.generator = &generic_ctx;

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
				ft->generic_ctx.generator = &new_inst->generic_inst;
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
				if (!StructureTemplate::parse(tc, new_inst,&new_inst->generic_inst, decl)) return false;
				new_inst->subtemplates[decl->name.buffer] = std::move(decl);
			}



			for (auto&& m : member_implementation) {
				Cursor c = m.type;
				CompileValue value;

				Cursor err = c;
				if (!Expression::parse(c, value, CompileType::eval)) return false;
				if (!Expression::rvalue(value, CompileType::eval)) return false;

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
						ft->generic_inst.generator = nullptr;
						ft->generic_inst.key = nullptr;
						ft->context = f.ctx;

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

						if (fundecl.ctx != f.ctx) {
							throw_specific_error(f.name, "Implemented function cannot use different context than the declaration");
							return false;
						}

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
								if (!Expression::rvalue(res, CompileType::eval)) return false;
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
							if (!Expression::rvalue(res, CompileType::eval)) return false;

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
					
					if (tt->owner->generic_inst.generator == &nctx.default_types->tr_copy->generic_ctx) {
						new_inst->impl_copy = trait.begin()->get();
					}

					if (tt->owner->generic_inst.generator == &nctx.default_types->tr_move->generic_ctx) {
						new_inst->impl_move = trait.begin()->get();
					}

					if (tt->owner->generic_inst.generator == &nctx.default_types->tr_compare->generic_ctx) {
						new_inst->impl_compare = trait.begin()->get();
					}

					new_inst->traitfunctions[tt->owner] = std::move(trait);
					
				}
			}

			for (auto&& m : member_vars) {
				Cursor c = m.type;

				CompileValue value;

				Cursor err = c;
				if (!Expression::parse(c, value, CompileType::eval)) return false;
				if (!Expression::rvalue(value, CompileType::eval)) return false;
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
				
				// TODO BETTER CHECK
				/*if (m_t->size().absolute == 0 && m_t->size().pointers == 0) {
					throw_specific_error(m.name, "Specified type is not an instantiable type");
					std::cerr << " | \tType was '";
					m_t->print(std::cerr);
					std::cerr << "'\n";
					return false;
				}*/

				new_inst->member_vars.push_back(std::make_pair(m_t, ILSize()));
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

				std::unique_ptr<unsigned char[]> new_key_inst = std::make_unique<unsigned char[]>(generic_ctx.generate_heap_size);
				new_key = new_key_inst.get();
				unsigned char* old_offset = argdata;
				unsigned char* new_offset = new_key_inst.get();

				for (auto l = generic_ctx.generic_layout.rbegin(); l != generic_ctx.generic_layout.rend(); l++) {
					std::get<1>(*l)->move(nctx.eval, new_offset, old_offset);
					size_t c_size = std::get<1>(*l)->size().eval(compiler_arch);
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

			CompileContext nctx = CompileContext::get();
			nctx.inside = parent;
			CompileContext::push(nctx);

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
					return false;
				}

				c.move();

				if (c.tok != RecognizedToken::CloseParenthesis) {
					while (true) {
						Cursor err = c;
						CompileValue val;
						if (!Expression::parse(c, val, CompileType::eval)) return false;
						if (!Expression::rvalue(val, CompileType::eval)) return false;

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
					if (!Expression::rvalue(val, CompileType::eval)) return false;

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
			CompileContext& nctx = CompileContext::get();

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
					std::get<1>(*l)->move(nctx.eval, new_offset, old_offset);
					size_t c_size = std::get<1>(*l)->size().eval(compiler_arch);
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
					if (!Expression::parse(cc, cvres, CompileType::eval)) return false;
					if (!Expression::rvalue(cvres, CompileType::eval)) return false;

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
				if (!Expression::rvalue(cvres, CompileType::eval)) return false;

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

			nctx.eval->stack_push();
			nctx.compile_stack->push();
			generic_inst.insert_key_on_stack(nctx.eval);

			nctx.runtime_stack->push();

			bool ret_rval_stack = false;
			uint16_t return_ptr_local_id = 0;


			if (returns.second->rvalue_stacked()) {
				ret_rval_stack = true;
				return_ptr_local_id = func->register_local(returns.second->size());
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
				uint16_t id = func->register_local(argval.t->size());
				func->arguments.push_back(a.second->rvalue());

				nctx.runtime_stack->push_item(a.first.buffer,argval,id,StackItemTag::regular);
			}

			

			uint16_t argid = (uint16_t)(arguments.size()-(ret_rval_stack?0:1));
			for (auto a = arguments.rbegin(); a != arguments.rend(); a++) {

				
				if (a->second->has_special_constructor()) {
					ILBuilder::build_local(nctx.scope, argid);
					a->second->build_construct();
				}
				
				ILBuilder::build_local(nctx.scope, argid);

				if (!Expression::copy_from_rvalue(a->second, CompileType::compile, false)) return false;

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


			StackItem sitm;
			while (nctx.runtime_stack->pop_item(sitm)) {
				if (sitm.value.t->has_special_destructor()) {
					ILBuilder::build_local(nctx.scope, sitm.id);
					sitm.value.t->build_drop();
				}
			}


			ILBuilder::build_ret(nctx.scope,func->returns);



			func->dump();
			std::cout << std::endl;

			if (!func->assert_flow()) return false;

			nctx.runtime_stack->pop();
			nctx.compile_stack->pop();
			nctx.eval->stack_pop();

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



	void StructureInstance::build_automatic_constructor() {
		//TODO
	}

	void StructureInstance::build_automatic_destructor() {
		//TODO
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

	bool StructureInstance::compile() {
		if (compile_state == 0) {
			compile_state = 1;

			CompileContext& nctx = CompileContext::get();

			size = { 0,0 };
			alignment = { 1,0 };


			for (auto&& m : member_vars) {
				if (!m.first->compile()) return false;

				/*if (m.first->has_special_constructor())
					has_special_constructor = true;

				if (m.first->has_special_destructor())
					has_special_destructor = true;*/


				/*uint32_t abss = size.absolute;
				abss = _align_up(abss, malign);
				size.absolute = abss;*/

				m.second = size;
				size = size + m.first->size();

				//absolute_alignment = std::max(absolute_alignment, malign);
				
			}


			for (auto&& m : subfunctions) {
				if (!m.second->compile()) return false;
			}

			//size = _align_up(size, alignment);
			//compile_size = _align_up(compile_size, compile_alignment);
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
				if (!impl_copy->compile()) return false;
				auto_copy = impl_copy->func;
				has_special_copy = true;
			}

			if (impl_move) {
				if (!impl_move->compile()) return false;
				auto_move = impl_move->func;
				has_special_move = true;
			}

			if (impl_compare) {
				if (!impl_compare->compile()) return false;
				auto_compare = impl_compare->func;
				has_special_compare = true;
			}
		}
		else if (compile_state == 2) {
			return true;
		}
		else {
			throw_specific_error(name, "Build cycle");
			return false;
		}

		return true;
	}

	void GenericInstance::insert_key_on_stack(ILEvaluator* eval) {
		

		if (generator != nullptr) {
			
			if (generator->generator != nullptr) {
				generator->generator->insert_key_on_stack(eval);
			}

			CompileContext& nctx = CompileContext::get();

			unsigned char* key_ptr = key;
			for (auto key_l = generator->generic_layout.rbegin(); key_l != generator->generic_layout.rend(); key_l++) {

				CompileValue res;
				res.lvalue = true;
				res.t = std::get<1>(*key_l);

				/*uint16_t sid = nctx.eval->push_local(res.t->size());
				unsigned char* data_place = nctx.eval->stack_ptr(sid);
				nctx.compile_stack->push_item(std::get<0>(*key_l).buffer, res, sid);

				res.t->copy(eval, data_place, key_ptr);*/

				uint16_t sid = nctx.eval->mask_local(key_ptr);
				nctx.compile_stack->push_item(std::get<0>(*key_l).buffer, res, sid, StackItemTag::alias);

				key_ptr += res.t->size().eval(compiler_arch);
			}

			
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