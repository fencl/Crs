#include "Declaration.hpp"
#include "Error.hpp"
#include <iostream>
#include <string>
#include "BuiltIn.hpp"
#include "Expression.hpp"
#include "StackManager.hpp"
#include "Type.hpp"
#include <algorithm>
#include "Statement.hpp"
#include "Operand.hpp"
#include "Compiler.hpp"

namespace Crs {

	bool GenericContext::valid_generic_argument(Type* type) {
		if (type->type() == TypeInstanceType::type_structure_instance) {
			TypeStructureInstance* tsi = (TypeStructureInstance*)type;
			if (tsi->owner->structure_type == StructureInstanceType::primitive_structure) {
				return true;
			}
			else return false;
		}
		else if (type->type() == TypeInstanceType::type_array) {
			TypeArray* tr = (TypeArray*)type;
			return valid_generic_argument(tr->owner);
		}
		else if (type->type() == TypeInstanceType::type_slice) {
			TypeSlice* tr = (TypeSlice*)type;
			return valid_generic_argument(tr->owner);
		}
		else return false;
	}

	errvoid FunctionTemplate::compile() {
		if (compile_state == 0) {
			compile_state = 1;

			auto state = ScopeState().workspace(parent).compiler_stack();
			if (generic_ctx.generator != nullptr) {
				generic_ctx.generator->insert_key_on_stack();
			}

			type = std::make_unique<TypeFunctionTemplate>();
			type->owner = this;

			if (ast_node->has_body() && ((AstFunctionNode*)ast_node)->is_generic) {
				Compiler* compiler = Compiler::current();
				Cursor c = load_cursor(((AstFunctionNode*)ast_node)->annotation, ast_node->get_source());
				c.move();
				while (true) {
					if (c.tok != RecognizedToken::Symbol) {
						return throw_not_a_name_error(c);
					}
					Cursor name = c;
					c.move();
					if (c.tok != RecognizedToken::Colon) {
						return throw_wrong_token_error(c, "':'");
					}
					c.move();
					Cursor err = c;
					CompileValue value;
					if (!Expression::parse(c, value, CompileType::eval)) return err::fail;
					if (!Operand::deref(value, CompileType::eval)) return err::fail;
					if (!Expression::rvalue(value, CompileType::eval)) return err::fail;

					if (value.type != compiler->types()->t_type) {
						return throw_specific_error(err, "Expected type value");
					}

					Type* t;
					if (!compiler->evaluator()->pop_register_value<Type*>(t)) return err::fail;

					if (!Type::assert(err,t)) return err::fail;
					if(!t->compile()) return err::fail;
					if (t->context() == ILContext::runtime) {
						return throw_specific_error(err, "Runtime type cannot be used as generic argument");
					}

					if (!GenericContext::valid_generic_argument(t)) {
						return throw_specific_error(err, "Only primitive types can be used as generic arguments");
					}

					generic_ctx.generate_heap_size += t->size().eval(compiler->global_module());
					generic_ctx.generic_layout.push_back(std::make_tuple(name, t));

					if (c.tok == RecognizedToken::Comma) {
						c.move();
					}
					else if (c.tok == RecognizedToken::CloseParenthesis) {
						break;
					}
					else {
						return throw_wrong_token_error(c, "',' or ')'");
					}
				}



				gen_template_cmp.parent = this;
				instances = std::make_unique<std::map<unsigned char*, std::pair<std::unique_ptr<unsigned char[]>, std::unique_ptr<FunctionInstance>>, GenericTemplateCompare>>(gen_template_cmp);

			}

			compile_state = 2;
		}
		else if (compile_state == 2) {

		}
		else {
			Cursor c = load_cursor(ast_node->name, ast_node->get_source());
			return throw_specific_error(c, "compile cycle");
		}

		return err::ok;
	}


	errvoid StructureTemplate::compile() {
		if (compile_state == 0) {
			compile_state = 1;

			auto state = ScopeState().workspace(parent).compiler_stack();

			if (generic_ctx.generator != nullptr) {
				generic_ctx.generator->insert_key_on_stack();
			}

			type = std::make_unique<TypeStructureTemplate>();
			type->owner = this;

			if (ast_node->is_generic) {
				Cursor c = load_cursor(ast_node->annotation, ast_node->get_source());
				c.move();
				Compiler* compiler = Compiler::current();

				while (true) {
					if (c.tok != RecognizedToken::Symbol) {
						return throw_not_a_name_error(c);
					}
					Cursor name = c;
					c.move();
					if (c.tok != RecognizedToken::Colon) {
						return throw_wrong_token_error(c, "':'");
					}
					c.move();
					Cursor err = c;
					CompileValue value;
					if (!Expression::parse(c, value, CompileType::eval)) return err::fail;
					if (!Operand::deref(value, CompileType::eval)) return err::fail;
					if (!Expression::rvalue(value, CompileType::eval)) return err::fail;

					if (value.type != compiler->types()->t_type) {
						return throw_specific_error(err, "Expected type value");
					}

					Type* t;
					if (!compiler->evaluator()->pop_register_value<Type*>(t)) return err::fail;

					if (!Type::assert(err,t)) return err::fail;
					if(!t->compile()) return err::fail;
					if (t->context() == ILContext::runtime) {
						return throw_specific_error(err, "Runtime type cannot be used as generic argument");
					}

					if (!GenericContext::valid_generic_argument(t)) {
						return throw_specific_error(err, "Only primitive types can be used as generic arguments");
					}

					generic_ctx.generate_heap_size += t->size().eval(compiler->global_module());
					generic_ctx.generic_layout.push_back(std::make_tuple(name, t));


					if (c.tok == RecognizedToken::Comma) {
						c.move();
					}
					else if (c.tok == RecognizedToken::CloseParenthesis) {
						break;
					}
					else {
						return throw_wrong_token_error(c, "',' or ')'");
					}
				}


				gen_template_cmp.parent = this;
				instances = std::make_unique<std::map<unsigned char*, std::pair<std::unique_ptr<unsigned char[]>, std::unique_ptr<StructureInstance>>, GenericTemplateCompare>>(gen_template_cmp);

			}


			compile_state = 2;
		}
		else if (compile_state == 2) {

		}
		else {
			Cursor c = load_cursor(ast_node->name, ast_node->get_source());
			return throw_specific_error(c, "compile cycle");
		}

		return err::ok;
	}

	errvoid TraitTemplate::compile() {
		if (compile_state == 0) {
			compile_state = 1;

			auto state = ScopeState().workspace(parent).compiler_stack();

			if (generic_ctx.generator != nullptr) {
				generic_ctx.generator->insert_key_on_stack();
			}

			type = std::make_unique<TypeTraitTemplate>();
			type->owner = this;


			if (ast_node->is_generic) {
				Compiler* compiler = Compiler::current();
				Cursor c = load_cursor(ast_node->annotation, ast_node->get_source());
				c.move();

				while (true) {
					if (c.tok != RecognizedToken::Symbol) {
						return throw_not_a_name_error(c);
					}
					Cursor name = c;
					c.move();
					if (c.tok != RecognizedToken::Colon) {
						return throw_wrong_token_error(c, "':'");
					}
					c.move();
					Cursor err = c;
					CompileValue value;
					if (!Expression::parse(c, value, CompileType::eval)) return err::fail;
					if (!Operand::deref(value, CompileType::eval)) return err::fail;
					if (!Expression::rvalue(value, CompileType::eval)) return err::fail;

					if (value.type != compiler->types()->t_type) {
						return throw_specific_error(err, "Expected type value");
					}

					Type* t;
					if (!compiler->evaluator()->pop_register_value<Type*>(t)) return err::fail;
					if (!Type::assert(err,t)) return err::fail;
					if(!t->compile()) return err::fail;
					if (t->context() == ILContext::runtime) {
						return throw_specific_error(err, "Runtime type cannot be used as generic argument");
					}

					if (!GenericContext::valid_generic_argument(t)) {
						return throw_specific_error(err, "Only primitive types can be used as generic arguments");
					}

					generic_ctx.generate_heap_size += t->size().eval(compiler->global_module());
					generic_ctx.generic_layout.push_back(std::make_tuple(name, t));


					if (c.tok == RecognizedToken::Comma) {
						c.move();
					}
					else if (c.tok == RecognizedToken::CloseParenthesis) {
						break;
					}
					else {
						return throw_wrong_token_error(c, "',' or ')'");
					}
				}


				gen_template_cmp.parent = this;
				instances = std::make_unique<std::map<unsigned char*, std::pair<std::unique_ptr<unsigned char[]>, std::unique_ptr<TraitInstance>>, GenericTemplateCompare>>(gen_template_cmp);
				
			}

			compile_state = 2;
		}
		else if (compile_state == 2) {

		}
		else {
			Cursor c = load_cursor(ast_node->name, ast_node->get_source());
			return throw_specific_error(c, "compile cycle");
		}

		return err::ok;
	}



	errvoid StructureTemplate::generate(unsigned char* argdata, StructureInstance*& out) {
		StructureInstance* new_inst = nullptr;
		unsigned char* new_key = nullptr;

		Source* src = ast_node->get_source();

		auto state = ScopeState().workspace(parent).compiler_stack().context(ILContext::compile);


		if (!ast_node->is_generic) {
			if (single_instance == nullptr) {
				single_instance = std::make_unique<StructureInstance>();
				new_inst = single_instance.get();
			}
			out = single_instance.get();
		}
		else {
			Compiler* compiler = Compiler::current();
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
					Type* t = std::get<1>(*l);
					std::size_t c_size = t->size().eval(compiler->global_module());
					if (!t->copy_to_generic_storage(old_offset, new_offset)) return err::fail;
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
			new_inst->ast_node = ast_node;
			new_inst->type = std::make_unique<TypeStructureInstance>();
			new_inst->type->owner = new_inst;
			new_inst->parent = parent;
			new_inst->generic_inst.key = new_key;
			new_inst->generic_inst.generator = &generic_ctx;
			new_inst->context = ast_node->context;
			Compiler* compiler = Compiler::current();

			auto state = ScopeState().workspace(new_inst);

			new_inst->generic_inst.insert_key_on_stack();

			for (auto&& m : ast_node->functions) {
				std::unique_ptr<FunctionTemplate> ft = std::make_unique<FunctionTemplate>();

				ft->ast_node = m.get();
				ft->parent = new_inst;
				ft->generic_ctx.generator = &new_inst->generic_inst;

				if (new_inst->name_table.find(m->name_string) != new_inst->name_table.end()) {
					Cursor c = load_cursor(m->name, src);
					return throw_specific_error(c, "Name already exists in the structure");
				}

				new_inst->name_table[m->name_string] = std::make_pair((std::uint8_t)2, (std::uint32_t)new_inst->subfunctions.size());
				new_inst->subfunctions.push_back(std::move(ft));
			}

			for (auto&& t : ast_node->structures) {
				std::unique_ptr<StructureTemplate> decl = std::make_unique<StructureTemplate>();
				decl->ast_node = t.get();
				decl->parent = new_inst;
				decl->generic_ctx.generator = &new_inst->generic_inst;

				if (new_inst->name_table.find(t->name_string) != new_inst->name_table.end()) {
					Cursor c = load_cursor(t->name, src);
					return throw_specific_error(c, "Name already exists in the structure");
				}

				new_inst->name_table[t->name_string] = std::make_pair((std::uint8_t)1, (std::uint32_t)new_inst->subtemplates.size());
				new_inst->subtemplates.push_back(std::move(decl));
			}

			for (auto&& s : ast_node->statics) {
				std::unique_ptr<StaticInstance> decl = std::make_unique<StaticInstance>();
				decl->ast_node = s.get();
				decl->parent = new_inst;
				decl->generator = &new_inst->generic_inst;

				if (new_inst->name_table.find(s->name_string) != new_inst->name_table.end()) {
					Cursor c = load_cursor(s->name, src);
					return throw_specific_error(c, "Name already exists in the structure");
				}

				new_inst->name_table[s->name_string] = std::make_pair((std::uint8_t)4, (std::uint32_t)new_inst->substatics.size());
				new_inst->substatics.push_back(std::move(decl));
			}


			for (auto&& m : ast_node->implementations) {

				CompileValue value;
				Cursor c = load_cursor(m->trait, src);
				Cursor err = c;
				if (!Expression::parse(c, value, CompileType::eval)) return err::fail;
				if (!Operand::deref(value, CompileType::eval)) return err::fail;
				if (!Expression::rvalue(value, CompileType::eval)) return err::fail;

				if (value.type != compiler->types()->t_type) {
					return throw_specific_error(err, "Expected type value");
				}

				Type* t;
				if (!compiler->evaluator()->pop_register_value<Type*>(t)) return err::fail;
				if (!Type::assert(err, t)) return err::fail;

				if (t->type() != TypeInstanceType::type_trait) {
					return throw_specific_error(err, "Expected trait instance type");
				}

				if(!t->compile()) return err::fail;

				if (t->context() == ILContext::compile) {
					if (new_inst->context != ILContext::runtime) {
						new_inst->context = ILContext::compile;
					}
					else {
						return throw_specific_error(err, "Type is marked as compile-time and therefore cannot be used inside runtime only structure");
					}
				}
				else if (t->context() == ILContext::runtime) {
					if (new_inst->context != ILContext::compile) {
						new_inst->context = ILContext::runtime;
					}
					else {
						return throw_specific_error(err, "Type is marked as runtime and therefore cannot be used inside compile-time structure");
					}
				}

				TypeTraitInstance* tt = (TypeTraitInstance*)t;
				if (new_inst->traitfunctions.find(tt->owner) != new_inst->traitfunctions.end()) {
					return throw_specific_error(err, "This trait was already implemented");
				}
				else {
					std::vector<std::unique_ptr<FunctionInstance>> trait(tt->owner->member_declarations.size());

					unsigned int func_count = 0;
					for (auto&& f : m->functions) {

						std::unique_ptr<FunctionInstance> ft = std::make_unique<FunctionInstance>();
						ft->compile_state = 0;
						ft->generic_inst.generator = &generic_ctx;
						ft->generic_inst.key = new_key;
						ft->ast_node = f.get();
						ft->parent = (Namespace*)new_inst;
						ft->context = new_inst->context;

						std::string_view name_str;
						Cursor name_c;
						if (!m->fast) {
							name_str = f->name_string;
							name_c = load_cursor(f->name, src);
						}
						else {
							name_str = tt->owner->ast_node->declarations[0]->name_string;
							name_c = load_cursor(tt->owner->ast_node->declarations[0]->name, src);
						}

						auto ttid = tt->owner->member_table.find(name_str);

						if (ttid == tt->owner->member_table.end()) {
							return throw_specific_error(name_c, "Implemented trait has no function with this name");
						}
						else if (trait[ttid->second] != nullptr) {
							return throw_specific_error(name_c, "Funtion with the same name already exists in the implementation");
						}


						func_count += 1;
						auto& fundecl = tt->owner->member_declarations[ttid->second];

						// sould be ok
						/*if (f->context != fundecl->ptr_context) {
							return throw_specific_error(name_c, "Funtion has different context");
						}*/

						auto& args = compiler->types()->argument_array_storage.get(fundecl->argument_array_id);

						c = load_cursor(f->type, src);
						c.move();

						if (c.tok != RecognizedToken::CloseParenthesis) {
							while (true) {
								if (c.tok != RecognizedToken::Symbol) {
									return throw_not_a_name_error(c);
								}
								Cursor name = c;
								c.move();
								if (c.tok != RecognizedToken::Colon) {
									return throw_wrong_token_error(c, "':'");
								}
								c.move();

								Cursor err = c;
								CompileValue res;
								if (!Expression::parse(c, res, CompileType::eval)) return err::fail;
								if (!Operand::deref(value, CompileType::eval)) return err::fail;
								if (!Expression::rvalue(res, CompileType::eval)) return err::fail;
								if (res.type != compiler->types()->t_type) {
									return throw_specific_error(err, "Expected type");
								}
								Type* argt;
								if (!compiler->evaluator()->pop_register_value<Type*>(argt)) return err::fail;

								if (!Type::assert(err,argt)) return err::fail;
								if (ft->arguments.size() == 0) {
									Type* this_type = new_inst->type->generate_reference();
									if (argt != this_type) {
										return throw_specific_error(err, "First argument in implementation of trait function must be self reference to the structure");
									}

									ft->arguments.push_back(argt);
								}
								else {
									if (ft->arguments.size() >= args.size()) {
										return throw_specific_error(err, "There are more arguments than in the original trait function");
									}

									Type* req_type = args[ft->arguments.size()];
									if (argt != req_type) {
										return throw_specific_error(err, "Argument does not match the type of the original trait function");
									}

									ft->arguments.push_back(argt);
								}

								if (c.tok == RecognizedToken::Comma) {
									c.move();
								}
								else if (c.tok == RecognizedToken::CloseParenthesis) {
									break;
								}
								else {
									return throw_wrong_token_error(c, "',' or ')'");
								}
							}
						}

						if (ft->arguments.size() != args.size()) {
							return throw_specific_error(c, "Trait function declaration lacks arguments from the original");
						}

						c.move();

						if (c.tok != RecognizedToken::OpenBrace) {
							Cursor err = c;
							CompileValue res;
							if (!Expression::parse(c, res, CompileType::eval)) return err::fail;
							if (!Operand::deref(value, CompileType::eval)) return err::fail;
							if (!Expression::rvalue(res, CompileType::eval)) return err::fail;

							if (res.type != compiler->types()->t_type) {
								return throw_specific_error(err, "Expected type");
							}
							Type* rett;
							if (!compiler->evaluator()->pop_register_value<Type*>(rett)) return err::fail;
							if (!Type::assert(err,rett)) return err::fail;
							Type* req_type = fundecl->return_type;
							if (rett != req_type) {
								return throw_specific_error(err, "Return type does not match the type of the original trait function");
							}

							ft->returns = rett;
						}
						else {
							ft->returns = compiler->types()->t_void;
						}

						std::vector<Type*> argtypes;
						for (auto&& a : ft->arguments) {
							argtypes.push_back(a);
						}

						TypeFunction* fun_type = compiler->types()->load_or_register_function_type(ft->ast_node->convention, std::move(argtypes), ft->returns, ft->ast_node->context);
						auto fun_inst_type = std::make_unique<TypeFunctionInstance>();
						fun_inst_type->owner = ft.get();
						fun_inst_type->function_type = fun_type;
						ft->type = std::move(fun_inst_type);
						ft->compile_state = 1;
						trait[ttid->second] = std::move(ft);

					}

					if (func_count != tt->owner->member_declarations.size()) {
						Cursor c = load_cursor(m->trait, src);
						return throw_specific_error(c, "Trait implementation is missing some functions");
					}
					

					new_inst->traitfunctions[tt->owner] = std::move(trait);

				}
			}

			for (auto&& m : ast_node->variables) {

				bool composite = m->alias;


				CompileValue value;

				Cursor c = load_cursor(m->type, src);
				Cursor err = c;
				if (!Expression::parse(c, value, CompileType::eval)) return err::fail;
				if (!Operand::deref(value, CompileType::eval)) return err::fail;
				if (!Expression::rvalue(value, CompileType::eval)) return err::fail;
				if (value.type != compiler->types()->t_type) {
					return throw_specific_error(err, "Expected type value");
				}

				Type* m_t;
				if (!compiler->evaluator()->pop_register_value<Type*>(m_t)) return err::fail;
				if (!Type::assert(err,m_t)) return err::fail;
				if(!m_t->compile()) return err::fail;
				if (m_t->context() == ILContext::compile) {
					if (new_inst->context != ILContext::runtime) {
						new_inst->context = ILContext::compile;
					}
					else {
						return throw_specific_error(err, "Type is marked as compile-time and therefore cannot be used inside runtime only structure");
					}
				}
				else if (m_t->context() == ILContext::runtime) {
					if (new_inst->context != ILContext::compile) {
						new_inst->context = ILContext::runtime;
					}
					else {
						return throw_specific_error(err, "Type is marked as runtime and therefore cannot be used inside compile-time structure");
					}
				}

				new_inst->member_table[m->name_string] = std::make_pair((std::uint16_t)new_inst->member_vars.size(), MemberTableEntryType::var);
				if (composite) {
					new_inst->member_composites.push_back((std::uint16_t)new_inst->member_vars.size());
				}
				new_inst->member_vars.push_back(std::make_pair(m_t, 0));
			}


			for (auto&& b : ast_node->compile_blocks) {
				auto scope = ScopeState().context(ILContext::compile).function(nullptr, nullptr);
				Cursor c = load_cursor(b, ast_node->get_source());
				if (c.tok == RecognizedToken::OpenBrace) {
					BlockTermination termination;
					if (!Statement::parse(c, termination, ForceCompile::single)) return err::fail;
				} else {
					CompileValue res;
					if (!Expression::parse(c, res, CompileType::eval, false)) return err::fail;
					if (c.tok != RecognizedToken::Semicolon) {
						return throw_wrong_token_error(c,"';'");
					}
					c.move();
				}
			}



		}

		return err::ok;
	}


	void StructureTemplate::var_wrapper(dword_t dw, Type* type) {
		Compiler* compiler = Compiler::current();
		std::uint8_t* data;
		if (!compiler->constant_manager()->register_generic_storage(data,(std::uint8_t*)dw.p1, (std::size_t)dw.p2, compiler->types()->t_u8)){ILEvaluator::ex_throw(); return;}
		std::basic_string_view<char> name((char*)data, (std::size_t)dw.p2);
		StructureInstance* sinst = (StructureInstance*)compiler->workspace();

		sinst->member_table[name] = std::make_pair((std::uint16_t)sinst->member_vars.size(), MemberTableEntryType::var);
		sinst->member_vars.push_back(std::make_pair(type, 0));
	}


	
	void StructureTemplate::include_wrapper(Type* type) {
		Compiler* compiler = Compiler::current();
		StructureInstance* sinst = (StructureInstance*)compiler->workspace();

		switch (type->type()) {
			case TypeInstanceType::type_function_template: {

				TypeFunctionTemplate* func = (TypeFunctionTemplate*)type;
				auto ft = std::make_unique<FunctionTemplate>();
				ft->ast_node = func->owner->ast_node;
				ft->compile_state = 0;
				ft->parent = sinst;
				ft->type = std::make_unique<TypeFunctionTemplate>();
				ft->type->owner = ft.get();
				ft->generic_ctx.generator = &sinst->generic_inst;
				if (!ft->compile()) { ILEvaluator::ex_throw(); return; }

				sinst->name_table[ft->ast_node->name_string] = std::make_pair((std::uint8_t)2, (std::uint32_t)sinst->subfunctions.size());
				sinst->subfunctions.push_back(std::move(ft));

			} break;

			case TypeInstanceType::type_function_instance: {
				TypeFunctionInstance* func = (TypeFunctionInstance*)type;
				if (!func->compile()) {ILEvaluator::ex_throw(); return;}

				auto fi = std::make_unique<FunctionInstance>();
				fi->arguments = func->owner->arguments;
				fi->returns = func->owner->returns;
				fi->ast_node = func->owner->ast_node;
				fi->compile_state = 3;
				fi->context = func->owner->context;
				fi->func = func->owner->func;
				fi->generic_inst = func->owner->generic_inst;
				fi->parent = sinst;
				fi->type = std::make_unique<TypeFunctionInstance>();
				fi->type->owner = fi.get();
				fi->type->function_type = func->owner->type->function_type;

				if (fi->arguments.size()>=1 && (fi->arguments[0]->type() == TypeInstanceType::type_reference) && ((TypeReference*)fi->arguments[0])->owner == sinst->type.get()) {
					sinst->member_table[fi->ast_node->name_string] = std::make_pair((tableelement_t)sinst->orphaned_functions.size(), MemberTableEntryType::orphan);
				}

				sinst->name_table[fi->ast_node->name_string] = std::make_pair((std::uint8_t)5, (std::uint32_t)sinst->orphaned_functions.size());
				sinst->orphaned_functions.push_back(std::move(fi));
			} break;

			default:
				throw_runtime_exception(Compiler::current()->evaluator(), "included type is not function instance or function template");
				ILEvaluator::ex_throw();
		}
	}

	void StructureTemplate::var_alias_wrapper(dword_t dw, Type* type) {
		Compiler* compiler = Compiler::current();
		std::uint8_t* data;
		if (!compiler->constant_manager()->register_generic_storage(data,(std::uint8_t*)dw.p1, (std::size_t)dw.p2, compiler->types()->t_u8)){ILEvaluator::ex_throw(); return;}
		std::basic_string_view<char> name((char*)data, (std::size_t)dw.p2);
		StructureInstance* sinst = (StructureInstance*)compiler->workspace();

		sinst->member_table[name] = std::make_pair((std::uint16_t)sinst->member_vars.size(), MemberTableEntryType::var); 
		sinst->member_composites.push_back((std::uint16_t)sinst->member_vars.size());
		sinst->member_vars.push_back(std::make_pair(type, 0));
	}


	errvoid TraitTemplate::generate(unsigned char* argdata, TraitInstance*& out) {
		TraitInstance* new_inst = nullptr;
		unsigned char* new_key = nullptr;
		Source* src = ast_node->get_source();

		auto state = ScopeState().workspace(parent).compiler_stack();

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
					std::size_t c_size = std::get<1>(*l)->size().eval(Compiler::current()->global_module());

					std::get<1>(*l)->copy_to_generic_storage(old_offset, new_offset);
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
			new_inst->generic_inst.key = new_key;
			new_inst->generic_inst.generator = &generic_ctx;
			new_inst->generic_inst.insert_key_on_stack();
			new_inst->ast_node = ast_node;
			new_inst->context = ast_node->context;
			Compiler* compiler = Compiler::current();
			std::vector<std::pair<Type*,std::vector<Type*>>> decls;


			for (auto&& m : ast_node->declarations) {

				std::vector<Type*> args;
				args.push_back(compiler->types()->t_ptr);
				Type* ret_type;

				Cursor c = load_cursor(m->type, src);
				c.move();

				if (c.tok != RecognizedToken::CloseParenthesis) {
					while (true) {
						Cursor err = c;
						CompileValue val;
						if (!Expression::parse(c, val, CompileType::eval)) return err::fail;
						if (!Operand::deref(val, CompileType::eval)) return err::fail;
						if (!Expression::rvalue(val, CompileType::eval)) return err::fail;

						if (val.type != compiler->types()->t_type) {
							return throw_specific_error(err, "Expected type");
						}
						Type* t;
						if (!compiler->evaluator()->pop_register_value<Type*>(t)) return err::fail;
						if (!Type::assert(err,t)) return err::fail;
						if(!t->compile()) return err::fail;

						if (t->context() == ILContext::compile) {
							if (new_inst->context != ILContext::runtime) {
								new_inst->context = ILContext::compile;
							}
							else {
								return throw_specific_error(err, "Type is marked as compile-time and therefore cannot be used inside runtime only trait");
							}
						}
						else if (t->context() == ILContext::runtime) {
							if (new_inst->context != ILContext::compile) {
								new_inst->context = ILContext::runtime;
							}
							else {
								return throw_specific_error(err, "Type is marked as runtime and therefore cannot be used inside compile-time trait");
							}
						}

						args.push_back(t);
						if (c.tok == RecognizedToken::Comma) {
							c.move();
						}
						else if (c.tok == RecognizedToken::CloseParenthesis) {
							break;
						}
						else {
							return throw_wrong_token_error(c, "',' or ')'");
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
					if (!Expression::parse(c, val, CompileType::eval)) return err::fail;
					if (!Operand::deref(val, CompileType::eval)) return err::fail;
					if (!Expression::rvalue(val, CompileType::eval)) return err::fail;

					if (val.type != compiler->types()->t_type) {
						return throw_specific_error(err, "Expected type");
					}
					Type* t;
					if (!compiler->evaluator()->pop_register_value<Type*>(t)) return err::fail;
					if (!Type::assert(err,t)) return err::fail;
					if(!t->compile()) return err::fail;

					if (t->context() == ILContext::compile) {
						if (new_inst->context != ILContext::runtime) {
							new_inst->context = ILContext::compile;
						}
						else {
							return throw_specific_error(err, "Type is marked as compile-time and therefore cannot be used inside runtime only trait");
						}
					}
					else if (t->context() == ILContext::runtime) {
						if (new_inst->context != ILContext::compile) {
							new_inst->context = ILContext::runtime;
						}
						else {
							return throw_specific_error(err, "Type is marked as runtime and therefore cannot be used inside compile-time trait");
						}
					}

					ret_type = t;
				}


				new_inst->member_table[m->name_string] = (std::uint16_t)decls.size();
				decls.push_back(std::make_pair(ret_type, std::move(args)));
			}

			for (auto&& d : decls) {
				TypeFunction* type = compiler->types()->load_or_register_function_type(ILCallingConvention::bytecode, std::move(d.second), d.first, new_inst->context);
				new_inst->member_declarations.push_back(type);
			}

		}

		return err::ok;
	}


	errvoid FunctionTemplate::generate(unsigned char* argdata, FunctionInstance*& out) {
		FunctionInstance* new_inst = nullptr;
		unsigned char* new_key = nullptr;

		auto state = ScopeState().workspace(parent).compiler_stack();

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
					std::size_t c_size = std::get<1>(*l)->size().eval(Compiler::current()->global_module());
					if (!std::get<1>(*l)->copy_to_generic_storage(old_offset, new_offset)) return err::fail;
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
			new_inst->ast_node = ast_node;
			new_inst->context = ast_node->context;

			new_inst->generic_inst.insert_key_on_stack();
			Compiler* compiler = Compiler::current();

			CompileValue cvres;

			Cursor c = load_cursor(ast_node->type, ast_node->get_source());
			c.move();
			if (c.tok != RecognizedToken::CloseParenthesis) {
				while (true) {

					Cursor argname = c;
					if (ast_node->has_body()) {
						if (c.tok != RecognizedToken::Symbol) {
							return throw_not_a_name_error(c);
						}
						c.move();
						if (c.tok != RecognizedToken::Colon) {
							return throw_wrong_token_error(c, "':'");
						}
						c.move();
					}

					Cursor err = c;
					if (!Expression::parse(c, cvres, CompileType::eval)) return err::fail;
					if (!Operand::deref(cvres, CompileType::eval)) return err::fail;
					if (!Expression::rvalue(cvres, CompileType::eval)) return err::fail;
					if (!Operand::cast(err, cvres, compiler->types()->t_type, CompileType::eval, true)) return err::fail;

					Type* t;
					if (!compiler->evaluator()->pop_register_value<Type*>(t)) return err::fail;
					if (!Type::assert(err,t)) return err::fail;

					if (t->context() == ILContext::compile) {
						if (new_inst->context != ILContext::runtime) {
							new_inst->context = ILContext::compile;
						}
						else {
							return throw_specific_error(err, "Type is marked as compile-time and therefore cannot be used as an argument to runtime-only function");
						}
					}
					else if (t->context() == ILContext::runtime) {
						if (new_inst->context != ILContext::compile) {
							new_inst->context = ILContext::runtime;
						}
						else {
							return throw_specific_error(err, "Type is marked as runtime only and therefore cannot be used as an argument to compile-time function");
						}
					}

					new_inst->arguments.push_back(t);

					if (c.tok == RecognizedToken::Comma) {
						c.move();
					}
					else if (c.tok == RecognizedToken::CloseParenthesis) {
						c.move();
						break;
					}
					else {
						return throw_wrong_token_error(c, "',' or ')'");
					}
				}
			}
			else { c.move(); }

			if (c.tok != RecognizedToken::OpenBrace && c.tok != RecognizedToken::Semicolon) {
				Cursor err = c;

				if (!Expression::parse(c, cvres, CompileType::eval)) return err::fail;
				if (!Operand::deref(cvres, CompileType::eval)) return err::fail;
				if (!Expression::rvalue(cvres, CompileType::eval)) return err::fail;
				if (!Operand::cast(err, cvres, compiler->types()->t_type, CompileType::eval, true)) return err::fail;

				Type* t;
				if (!compiler->evaluator()->pop_register_value<Type*>(t)) return err::fail;

				if (!Type::assert(err,t)) return err::fail;
				new_inst->returns = t;

				if (t->context() == ILContext::compile) {
					if (new_inst->context != ILContext::runtime) {
						new_inst->context = ILContext::compile;
					}
					else {
						return throw_specific_error(err, "Type is marked as compile-time and therefore cannot be used as an argument to runtime-only function");
					}
				}
				else if (t->context() == ILContext::runtime) {
					if (new_inst->context != ILContext::compile) {
						new_inst->context = ILContext::runtime;
					}
					else {
						return throw_specific_error(err, "Type is marked as runtime only and therefore cannot be used as an argument to compile-time function");
					}
				}

			}
			else {
				new_inst->returns = compiler->types()->t_void;
			}

			std::vector<Type*> argtypes;
			for (auto&& a : new_inst->arguments) {
				argtypes.push_back(a);
			}

			TypeFunction* fun_type = compiler->types()->load_or_register_function_type(ast_node->convention, std::move(argtypes), new_inst->returns, new_inst->context);
			auto fun_inst_type = std::make_unique<TypeFunctionInstance>();
			fun_inst_type->function_type = fun_type;
			fun_inst_type->owner = new_inst;

			new_inst->type = std::move(fun_inst_type);
			new_inst->compile_state = 1;
		}

		return err::ok;
	}

	errvoid FunctionInstance::compile() {
		if (compile_state == 1) {
			compile_state = 2;

			std::string name = std::string(ast_node->name_string);
			Namespace* nspc = parent;
			while(nspc && nspc->ast_node) {
				if (auto stct = dynamic_cast<StructureInstance*>(nspc)) {
					name.insert(0,"::");
					name.insert(0,((AstStructureNode*)stct->ast_node)->name_string);
				}else {
					name.insert(0,"::");
					name.insert(0,((AstNamedNamespaceNode*)nspc->ast_node)->name_string);
				}
				nspc = nspc->parent;
			}

			if (ast_node->has_body()) {
				Compiler* compiler = Compiler::current();
				if(!type->function_type->compile()) return err::fail;
				auto func = compiler->global_module()->create_function(context);

				func->name = std::move(name);
				this->func = func;
				func->decl_id = type->function_type->il_function_decl;

				ILBlock* b = func->create_and_append_block();

				{
					auto scope = ScopeState().function(func, returns).workspace(parent).context(context).compiler_stack().stack();
					generic_inst.insert_key_on_stack();

					if (!Statement::parse_inner_block_start(b)) return err::fail;

					bool ret_rval_stack = false;
					stackid_t return_ptr_local_id = 0;

					if(!returns->compile()) return err::fail;

					if (returns->rvalue_stacked()) {
						ret_rval_stack = true;
						return_ptr_local_id = func->local_stack_lifetime.append(compiler->types()->t_ptr->size());
					}

					
					for (std::size_t i = 0; i < arguments.size(); i++) {
						auto& a = arguments[i];

						if(!a->compile()) return err::fail;
						// context should be already ok or thrown

						stackid_t id = func->local_stack_lifetime.append(a->size());

						compiler->stack()->push_item(((AstFunctionNode*)ast_node)->argument_names[i].second, a, id);
					}



					std::uint16_t argid = (std::uint16_t)(arguments.size() - (ret_rval_stack ? 0 : 1));
					for (auto a = arguments.rbegin(); a != arguments.rend(); a++) {
						if (!ILBuilder::build_local(compiler->scope(), argid)) return err::fail;
						if (!Expression::copy_from_rvalue(*a, CompileType::compile)) return err::fail;

						argid--;
					}

					if (ret_rval_stack) {
						if (!ILBuilder::build_local(compiler->scope(), return_ptr_local_id)) return err::fail;
						if (!ILBuilder::build_store(compiler->scope(), ILDataType::word)) return err::fail;
					}

					// return type context should be already ok or thrown

					compile_state = 3;
					Source* src = ast_node->get_source();
					
					Cursor name = load_cursor(ast_node->name, src);
					Cursor cb = load_cursor(((AstFunctionNode*)ast_node)->block, src);
					BlockTermination term;
					cb.move();
					if (!Statement::parse_inner_block(cb, term, true, &name)) return err::fail;

					//func->dump();
					//std::cout << std::endl;
				}
			}
			else {
				if(!type->function_type->compile()) return err::fail;
				auto func = Compiler::current()->global_module()->create_native_function(std::move(name));
				this->func = func;
				func->decl_id = type->function_type->il_function_decl;
				compile_state = 3;
			}
		}
		else if (compile_state == 3) {

		}
		else if (compile_state == 2) {
			Cursor c = load_cursor(ast_node->name, ast_node->get_source());
			return throw_specific_error(c, "Build cycle");
		}
		else if (compile_state == 0) {
			Cursor c = load_cursor(ast_node->name, ast_node->get_source());
			return throw_specific_error(c, "Build cycle");
		}

		return err::ok;
	}

	std::uint32_t upper_power_of_two(std::uint32_t v)
	{
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		v++;
		return v;
	}

	errvoid StructureInstance::compile() {
		if (compile_state == 0) {
			compile_state = 1;
			
			auto scope = ScopeState().context(ILContext::compile).stack().compiler_stack();

			generic_inst.insert_key_on_stack();

			ILStructTable table;
			size = ILSize(ILSizeType::_0, 0);

			for (auto&& m : member_vars) {
				if(!m.first->compile()) return err::fail;

				ILSize m_s = m.first->size();

				if (m_s.type == ILSizeType::abs8) {
					if (size.type == ILSizeType::_0) size.type = ILSizeType::abs8;

					if (size.type == ILSizeType::abs8) {
						m.second = size.value;
						size.value += m_s.value;
					}
					else {
						size.type = ILSizeType::table;
					}
				}
				else if (m_s.type == ILSizeType::abs16) {
					if (size.type == ILSizeType::_0) size.type = ILSizeType::abs16;

					if (size.type == ILSizeType::abs16) {
						m.second = size.value*2;
						size.value += m_s.value;
					}
					else {
						size.type = ILSizeType::table;
					}
				}
				else if (m_s.type == ILSizeType::abs32) {
					if (size.type == ILSizeType::_0) size.type = ILSizeType::abs32;

					if (size.type == ILSizeType::abs32) {
						m.second = size.value*4;
						size.value += m_s.value;
					}
					else {
						size.type = ILSizeType::table;
					}
				}
				else if (m_s.type == ILSizeType::abs64) {
					if (size.type == ILSizeType::_0) size.type = ILSizeType::abs64;

					if (size.type == ILSizeType::abs64) {
						m.second = size.value*8;
						size.value += m_s.value;
					}
					else {
						size.type = ILSizeType::table;
					}
				}
				else if (m_s.type == ILSizeType::absf32) {
					if (size.type == ILSizeType::_0) size.type = ILSizeType::absf32;

					if (size.type == ILSizeType::absf32) {
						m.second = size.value*4;
						size.value += m_s.value;
					}
					else {
						size.type = ILSizeType::table;
					}
				}
				else if (m_s.type == ILSizeType::absf64) {
					if (size.type == ILSizeType::_0) size.type = ILSizeType::absf64;

					if (size.type == ILSizeType::absf64) {
						m.second = size.value*8;
						size.value += m_s.value;
					}
					else {
						size.type = ILSizeType::table;
					}
				}
				else if (m_s.type == ILSizeType::ptr) { // words are automatically aligned
					if (size.type == ILSizeType::_0) size.type = ILSizeType::ptr;

					if (size.type == ILSizeType::ptr) {
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

				if (table.elements.empty() || table.elements.back().type != m_s.type) {
					table.elements.push_back(m_s);
				} else if (m_s.type != ILSizeType::table && m_s.type!=ILSizeType::array) {
					table.elements.back().value += m_s.value;
				} else {
					table.elements.push_back(m_s);
				}
			}


			if (size.type == ILSizeType::table) {
				if (table.elements.size() > 0) {
					Compiler* compiler = Compiler::current();
					size.value = compiler->global_module()->register_structure_table();
					compiler->global_module()->structure_tables[size.value] = std::move(table);
				}
				else {
					size = table.elements.back();
					wrapper = true;
				}
			}


			for (auto&& m : subfunctions) {
				if(!m->compile()) return err::fail;
			}

			structure_type = StructureInstanceType::normal_structure;
			compile_state = 2;

			for (std::size_t i = 0; i < member_composites.size(); i++) {
				std::size_t comp = member_composites[i];
				auto& m = member_vars[comp];
				Type* t = m.first;

				if (t->type() == TypeInstanceType::type_reference && ((TypeReference*)t)->owner->type() == TypeInstanceType::type_structure_instance) {
					t = ((TypeReference*)t)->owner;
				}

				if (t->type() == TypeInstanceType::type_structure_instance) {
					TypeStructureInstance* ts = (TypeStructureInstance*)t;
					if(!ts->compile()) return err::fail;
					for (auto&& v : ts->owner->member_table) {
						member_table.insert(std::make_pair(v.first, std::make_pair<std::uint16_t, MemberTableEntryType>((std::uint16_t)comp, MemberTableEntryType::alias)));
					}
				}
				else if (t->type() == TypeInstanceType::type_slice) {
					TypeSlice* ts = (TypeSlice*)t;
					if(!ts->compile()) return err::fail;
					pass_array_operator = true;
					pass_array_id = (std::uint16_t)comp;
				}
			}


			for (std::uint32_t i = 0; i < subfunctions.size(); ++i) {
				auto gf = subfunctions[i].get();
				if (!(gf->ast_node->has_body() && ((AstFunctionNode*)gf->ast_node)->is_generic)) {
					if(!gf->compile()) return err::fail;
					FunctionInstance* fi;
					if (!gf->generate(nullptr, fi)) return err::fail;

					if (fi->arguments.size() > 0 && fi->arguments[0] == type.get()->generate_reference()) {
						member_table.insert(std::make_pair(fi->ast_node->name_string, std::make_pair<std::uint16_t, MemberTableEntryType>((std::uint16_t)i, MemberTableEntryType::func)));
					}
				}
			}

			compile_state = 3;

		}
		else if (compile_state == 3) {

		}
		else {
			Cursor c = load_cursor(((AstStructureNode*)ast_node)->name, ast_node->get_source());
			return throw_specific_error(c, "Build cycle");
		}

		return err::ok;
	}

	void GenericInstance::insert_key_on_stack() {
		if (generator != nullptr) {

			if (generator->generator != nullptr) {
				generator->generator->insert_key_on_stack();
			}
			Compiler* compiler = Compiler::current();

			unsigned char* key_ptr = key;
			for (auto key_l = generator->generic_layout.rbegin(); key_l != generator->generic_layout.rend(); key_l++) {

				stackid_t sid = compiler->mask_local(key_ptr);
				compiler->compiler_stack()->push_item(std::get<0>(*key_l).buffer(), std::get<1>(*key_l), sid);

				key_ptr += std::get<1>(*key_l)->size().eval(compiler->global_module());
			}
		}
	}


	errvoid TraitInstance::generate_vtable(StructureInstance* forinst, std::uint32_t& optid) {
		if(!forinst->compile()) return err::fail;

		std::unique_ptr<void* []> vtable = std::make_unique<void* []>(member_declarations.size());

		auto& f_table = forinst->traitfunctions[this];
		std::size_t id = 0;
		for (auto&& m_func : member_declarations) {
			FunctionInstance* finst = f_table[id].get();
			if(!finst->compile()) return err::fail;
			vtable[id] = finst->func;
			++id;
		}

		void** vt = vtable.get();
		std::uint32_t vtid = Compiler::current()->global_module()->register_vtable((std::uint32_t)member_declarations.size(), std::move(vtable));
		vtable_instances[forinst] = vtid;
		optid = vtid;

		return err::ok;
	}

	errvoid StaticInstance::compile() {
		if (compile_state == 0) {
			compile_state = 1;
			
			Compiler* compiler = Compiler::current();
			context = ast_node->context;

			if (generator != nullptr) {
				generator->insert_key_on_stack();
			}

			Cursor c = load_cursor(ast_node->type, ast_node->get_source());
			Cursor err = c;

			
			if (!ast_node->has_value) {
				auto scope = ScopeState().context(ILContext::compile).stack().compiler_stack();

				CompileValue typevalue;
				if (!Expression::parse(c, typevalue, CompileType::eval)) return err::fail;
				if (!Operand::deref(typevalue, CompileType::eval)) return err::fail;
				if (!Expression::rvalue(typevalue, CompileType::eval)) return err::fail;
				if (!Operand::cast(err, typevalue, compiler->types()->t_type, CompileType::eval, true)) return err::fail;

				if (!compiler->evaluator()->pop_register_value<Type*>(type)) return err::fail;
				if (!Type::assert(err,type)) return err::fail;
				if(!type->compile()) return err::fail;

				if (type->context() == ILContext::compile) {
					if (context != ILContext::runtime) {
						context = ILContext::compile;
					}
					else {
						return throw_specific_error(err, "Type is marked as compile-time and therefore cannot be used as an argument to runtime-only function");
					}
				}
				else if (type->context() == ILContext::runtime) {
					if (context != ILContext::compile) {
						context = ILContext::runtime;
					}
					else {
						return throw_specific_error(err, "Type is marked as runtime only and therefore cannot be used as an argument to compile-time function");
					}
				}


				sid = compiler->global_module()->register_static(nullptr, type->size(), nullptr);
			}
			else {
				//ILCallingConvention, ILDataType, std::vector<ILDataType>
				init_func = compiler->global_module()->create_function(ILContext::both);
				init_func->decl_id = compiler->global_module()->register_function_decl(std::make_tuple(ILCallingConvention::bytecode, ILDataType::none, std::vector<ILDataType>()));
				ILBlock* b = init_func->create_and_append_block();

				auto scope = ScopeState().function(init_func, compiler->types()->t_void).stack().compiler_stack().context(context);
				compiler->push_scope(b);
				CompileValue typevalue;
				if (!Expression::parse(c, typevalue, CompileType::compile)) return err::fail;
				if (!Operand::deref(typevalue, CompileType::compile)) return err::fail;
				if (!Expression::rvalue(typevalue, CompileType::compile)) return err::fail;
				
				
				sid = compiler->global_module()->register_static(nullptr, typevalue.type->size(),init_func);
				if (typevalue.lvalue || typevalue.type->rvalue_stacked()) {
					if (!ILBuilder::build_staticref(b, sid)) return err::fail;
					if (!ILBuilder::build_memcpy(b, typevalue.type->size())) return err::fail;
				} else {
					if (!ILBuilder::build_staticref(b, sid)) return err::fail;
					if (!ILBuilder::build_store(b, typevalue.type->rvalue())) return err::fail;
				}

				if (!ILBuilder::build_ret(b,ILDataType::none)) return err::fail;

				compiler->pop_scope();

				type = typevalue.type;
				if(!type->compile()) return err::fail;

				if (type->context() == ILContext::compile) {
					if (context != ILContext::runtime) {
						context = ILContext::compile;
					}
					else {
						return throw_specific_error(err, "Type is marked as compile-time and therefore cannot be used as an argument to runtime-only function");
					}
				}
				else if (type->context() == ILContext::runtime) {
					if (context != ILContext::compile) {
						context = ILContext::runtime;
					}
					else {
						return throw_specific_error(err, "Type is marked as runtime only and therefore cannot be used as an argument to compile-time function");
					}
				}

				if (!ILBuilder::eval_fncall(init_func)) return err::fail;				
			}

			compile_state = 2;
		}
		else if (compile_state == 1) {
			Cursor c = load_cursor(ast_node->name, ast_node->get_source());
			return throw_specific_error(c, "Build cycle");
		}

		return err::ok;
	}
}