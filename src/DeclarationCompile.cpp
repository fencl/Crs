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
			type->rvalue = ILDataType::ptr;

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
			type->rvalue = ILDataType::ptr;
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
				new_key = new_key_inst.get();
				memcpy(new_key, argdata, generate_heap_size);

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
			new_inst->type->rvalue = ILDataType::ptr;
			new_inst->namespace_type = NamespaceType::t_struct_instance;
			new_inst->key = new_key;


			CompileContext nctx = CompileContext::get();
			nctx.inside = new_inst;
			CompileContext::push(nctx);

			for (auto&& m : member_funcs) {
				Cursor c = m.type;
				//TODO we could probably skip the catalogue stage and build the functiontemplate directly in the structure template


				std::unique_ptr<FunctionTemplate> ft = std::make_unique<FunctionTemplate>();
				ft->name = m.name;
				ft->annotation = m.annotation;
				ft->is_generic = m.annotation.tok != RecognizedToken::Eof;
				ft->parent = new_inst;
				ft->template_parent = new_inst;
				ft->decl_type = m.type;
				ft->block = m.block;

				ft->type = std::make_unique<TypeFunctionTemplate>();
				ft->type->owner = ft.get();
				ft->type->rvalue = ILDataType::type;

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
					std::map<std::string_view, std::unique_ptr<FunctionInstance>> trait;
					unsigned int func_count = 0;
					for (auto&& f : m.functions) {

						std::unique_ptr<FunctionInstance> ft = std::make_unique<FunctionInstance>();
						ft->key = nullptr;
						ft->block = f.block;

						if (trait.find(f.name.buffer) != trait.end()) {
							throw_specific_error(f.name, "Funtion with the same name already exists in the implementation");
							return false;
						}

						auto fundecl_id = tt->owner->member_table.find(f.name.buffer);
						if (fundecl_id == tt->owner->member_table.end()) {
							throw_specific_error(f.name, "Implemented trait has no function with this name");
							return false;
						}

						func_count += 1;

						auto& fundecl = tt->owner->member_funcs[fundecl_id->second];

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
									if (ft->arguments.size()-1 >= fundecl.arg_types.size()) {
										throw_specific_error(err, "There are more arguments than in the original trait function");
										return false;
									}

									Type* req_type = fundecl.arg_types[ft->arguments.size() - 1];
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

						if (ft->arguments.size() != fundecl.arg_types.size() + 1) {
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

							Type* req_type = fundecl.return_type;
							if (rett != req_type) {
								throw_specific_error(err, "Return type does not match the type of the original trait function");
								std::cerr << " |\tRequired type was: "; req_type->print(std::cerr);
								std::cerr << "\n";
								return false;
							}

							ft->returns = rett;
						}
						else {
							ft->returns = nctx.default_types->t_void;
						}
						
						
						std::vector<Type*> argtypes;
						for (auto&& a : ft->arguments) {
							argtypes.push_back(a.second);
						}

						ft->type = nctx.default_types->load_or_register_function_type(std::move(argtypes), ft->returns);

						trait[f.name.buffer] = std::move(ft);
					}

					if (func_count != tt->owner->member_funcs.size()) {
						throw_specific_error(m.type,"Trait implementation is missing some functions");
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
				if (m_t == nullptr) {
					std::cout << "error";
				}

				new_inst->member_table[m.name.buffer] = new_inst->member_vars.size();
				StructureInstanceMemberRecord rec;
				rec.definition = m.name;
				rec.type = m_t;

				new_inst->member_vars.push_back(rec);
			}

			CompileContext::pop();
			
		}

		return true;
	}


	bool TraitTemplate::generate(unsigned char* argdata, TraitInstance*& out) {
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
				memcpy(new_key, argdata, generate_heap_size);

				instances->emplace(new_key, std::make_pair(std::move(new_key_inst), std::move(inst)));

			}
			else {
				out = f->second.second.get();
			}
		}


		if (new_inst != nullptr) {
			new_inst->type = std::make_unique<TypeTraitInstance>();
			new_inst->type->owner = new_inst;
			new_inst->type->rvalue = ILDataType::ptr;

			new_inst->generator = this;
			new_inst->parent = parent;
			new_inst->name = name;
			new_inst->key = new_key;

			CompileContext nctx = CompileContext::get();
			nctx.inside = parent;
			CompileContext::push(nctx);

			for (auto&& m : member_funcs) {



				TraitInstanceMemberRecord member;

				member.definition = m.type;
				Cursor c = member.definition;

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
						member.arg_types.push_back(t);
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
					member.return_type = nctx.default_types->t_void;
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
					member.return_type = t;
				}



				
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
			new_inst->generator = this;
			new_inst->key = new_key;
			new_inst->block = block;

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
				new_inst->returns = t;
			}
			else {
				new_inst->returns = nctx.default_types->t_void;
			}

			std::vector<Type*> argtypes;
			for (auto&& a : new_inst->arguments) {
				argtypes.push_back(a.second);
			}

			new_inst->type = nctx.default_types->load_or_register_function_type(std::move(argtypes), new_inst->returns);
			new_inst->compile_state = 1;

			CompileContext::pop();
		}

		return true;
	}

	bool FunctionInstance::compile(Cursor err) {
		if (compile_state == 1) {
			compile_state = 2;


			func = CompileContext::get().module->create_function();
			ILBlock* b = func->create_and_append_block(ILDataType::none);
			b->alias = "entry";


			CompileContext nctx = CompileContext::get();
			nctx.inside = generator->parent;
			nctx.scope = b;
			nctx.function = this;
			CompileContext::push(nctx);

			auto ss = std::move(StackManager::move_stack_out<0>());
			for (auto&& a : arguments) {
				CompileValue argval;
				argval.t = a.second;
				argval.lvalue = true;
				uint16_t id = func->register_local(argval.t->compile_size(nctx.eval), argval.t->size(nctx.eval));
				func->arguments.push_back(a.second->rvalue);
				StackManager::stack_push<0>(nctx.eval,a.first.buffer, argval, id);
			}

			uint16_t argid = (uint16_t)(arguments.size()-1);
			for (auto a = arguments.rbegin(); a != arguments.rend(); a++) {
				ILBuilder::build_local(nctx.scope, argid);
				ILBuilder::build_store(nctx.scope, a->second->rvalue);
				argid--;
			}

			func->returns = returns->rvalue;

			

			Cursor cb = block;
			CompileValue cvres;
			bool terminated;
			if (!Statement::parse_inner_block(cb, cvres, CompileType::compile,terminated)) return false;


			ILBlock* b_exit = func->create_and_append_block(returns->rvalue);

			ILBuilder::build_yield(nctx.scope, cvres.t->rvalue);
			ILBuilder::build_jmp(nctx.scope, b_exit);


			b_exit->alias = "entry_exit";
			ILBuilder::build_accept(b_exit,returns->rvalue);
			ILBuilder::build_ret(b_exit,returns->rvalue);




			func->dump();
			std::cout << std::endl;

			if (!func->assert_flow()) return false;

			StackManager::move_stack_in<0>(std::move(ss));


			CompileContext::pop();
			compile_state = 3;
		}
		else if (compile_state == 3) {
			return true;
		}
		else if (compile_state == 2) {
			throw_specific_error(generator->name, "Build cycle");
			return false;
		}
		else if (compile_state == 0) {
			throw_specific_error(err, "Build cycle");
			return false;
		}


		return true;
	}



	unsigned int _align_up(unsigned int value, unsigned int alignment) {
		return alignment == 0 ? value : ((value % alignment == 0) ? value : value + (alignment - (value % alignment)));
	}

	bool TraitInstance::compile() {
		if (compile_state == 0) {
			compile_state = 1;

			/*auto ss = std::move(StackManager::move_stack_out<1>());
			auto sp = ctx.eval->stack_push();

			insert_key_on_stack(ctx);*/

			alignment = 0;
			size = 0;

			compile_alignment = 0;
			compile_size = 0;

			CompileContext& nctx = CompileContext::get();


			for (auto&& m : member_funcs) {
				size = _align_up(size, nctx.default_types->t_ptr->alignment(nctx.eval));
				compile_size = _align_up(compile_size, nctx.default_types->t_ptr->compile_alignment(nctx.eval));

				m.offset = size;
				m.compile_offset = compile_size;


				size += nctx.default_types->t_ptr->size(nctx.eval);
				compile_size += nctx.default_types->t_ptr->compile_size(nctx.eval);

				alignment = std::max(alignment, nctx.default_types->t_ptr->alignment(nctx.eval));
				compile_alignment = std::max(compile_alignment, nctx.default_types->t_ptr->compile_alignment(nctx.eval));

			}

			size = _align_up(size, alignment);
			compile_size = _align_up(compile_size, compile_alignment);

			//ctx.eval->stack_pop(sp);
			//StackManager::move_stack_in<1>(std::move(ss));

			compile_state = 2;
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

			res.t->move(eval, key_ptr, data_place); // TODO! there will be copy

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

			res.t->move(eval, key_ptr, data_place); // TODO! there will be copy

			key_ptr += res.t->compile_size(eval);
		}

	}
}