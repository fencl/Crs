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

	bool FunctionTemplate::compile(CompileContext& ctx) {
		if (compile_state == 0) {
			compile_state = 1;

			if (is_generic) {
				Cursor c = annotation;

				while (true) {
					CompileContext nctx = ctx;
					nctx.inside = parent;

					Cursor name = c;
					c.move();
					if (c.tok != RecognizedToken::Colon) {
						throw_wrong_token_error(c, "':'");
						return false;
					}
					c.move();
					Cursor err = c;
					CompileValue value;
					if (!Expression::parse(c, nctx, value, CompileType::eval)) return false;
					if (value.t != ctx.default_types->t_type) {
						throw_specific_error(err, "Expected type value");
						return false;
					}

					Type* t = ctx.eval->pop_register_value<Type*>();

					if (t->type() != TypeInstanceType::type_instance) {
						throw_specific_error(err, "Type does not point to instance");
						return false;
					}

					if (!t->compile(ctx)) return false;

					generate_heap_size += t->compile_size(ctx);
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
				gen_template_cmp.ctx = ctx;
				instances = std::make_unique<std::map<std::pair<unsigned int, unsigned char*>, std::unique_ptr<FunctionInstance>, GenericTemplateCompare>>(gen_template_cmp);
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


	bool StructureTemplate::compile(CompileContext& ctx) {
		if (compile_state == 0) {
			compile_state = 1;

			type = std::make_unique<TypeStructure>();
			type->owner = this;
			type->rvalue = ILDataType::ptr;

			if (is_generic) {
				Cursor c = annotation;

				while (true) {
					CompileContext nctx = ctx;
					nctx.inside = parent;

					Cursor name = c;
					c.move();
					if (c.tok != RecognizedToken::Colon) {
						throw_wrong_token_error(c, "':'");
						return false;
					}
					c.move();
					Cursor err = c;
					CompileValue value;
					if (!Expression::parse(c, nctx, value, CompileType::eval)) return false;
					if (value.t != ctx.default_types->t_type) {
						throw_specific_error(err, "Expected type value");
						return false;
					}

					Type* t = ctx.eval->pop_register_value<Type*>();

					if (t->type() != TypeInstanceType::type_instance) {
						throw_specific_error(err, "Type does not point to instance");
						return false;
					}

					if (!t->compile(ctx)) return false;
					
					generate_heap_size += t->compile_size(ctx);
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
				gen_template_cmp.ctx = ctx;
				instances = std::make_unique<std::map<unsigned char*, std::pair<std::unique_ptr<unsigned char[]>,std::unique_ptr<StructureInstance>>, GenericTemplateCompare>>(gen_template_cmp);
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

	bool TraitTemplate::compile(CompileContext& ctx) {
		if (compile_state == 0) {
			compile_state = 1;
			
			type = std::make_unique<TypeTrait>();
			type->rvalue = ILDataType::ptr;
			type->owner = this;

			if (is_generic) {
				Cursor c = annotation;

				while (true) {
					CompileContext nctx = ctx;
					nctx.inside = parent;

					Cursor name = c;
					c.move();
					if (c.tok != RecognizedToken::Colon) {
						throw_wrong_token_error(c, "':'");
						return false;
					}
					c.move();
					Cursor err = c;
					CompileValue value;
					if (!Expression::parse(c, nctx, value, CompileType::eval)) return false;
					if (value.t != ctx.default_types->t_type) {
						throw_specific_error(err, "Expected type value");
						return false;
					}

					Type* t = ctx.eval->pop_register_value<Type*>();

					if (t->type() != TypeInstanceType::type_instance) {
						throw_specific_error(err, "Type does not point to instance");
						return false;
					}

					if (!t->compile(ctx)) return false;

					generate_heap_size += t->compile_size(ctx);
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
				gen_template_cmp.ctx = ctx;
				instances = std::make_unique<std::map<unsigned char*, std::pair<std::unique_ptr<unsigned char[]>, std::unique_ptr<TraitInstance>>, GenericTemplateCompare>>(gen_template_cmp);
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



	bool StructureTemplate::generate(CompileContext& ctx, unsigned char* argdata, StructureInstance*& out) {
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
			new_inst->type = std::make_unique<TypeInstance>();
			new_inst->type->owner = new_inst;
			new_inst->generator = this;
			new_inst->parent = parent;
			new_inst->name = name;
			new_inst->type->rvalue = ILDataType::ptr;
			new_inst->namespace_type = NamespaceType::t_struct_instance;
			new_inst->key = new_key;

			for (auto&& t : member_templates) {
				Cursor tc = t.cursor;
				std::unique_ptr<StructureTemplate> decl;
				if (!StructureTemplate::parse(tc, ctx, new_inst, decl)) return false;
				new_inst->subtemplates[decl->name.buffer] = std::move(decl);
			}


			for (auto&& m : member_implementation) {
				

				Cursor c = m.type;
				CompileValue value;

				CompileContext nctx = ctx;
				nctx.inside = parent;
				Cursor err = c;
				if (!Expression::parse(c, nctx, value, CompileType::eval)) return false;
				if (value.t != ctx.default_types->t_type) {
					throw_specific_error(m.type, "Expected type value");
					std::cerr << " | \tType was '";
					value.t->print(std::cerr);
					std::cerr << "'\n";
					return false;
				}

				Type* t = ctx.eval->pop_register_value<Type*>();

				if (t->type() != TypeInstanceType::type_trait) {
					throw_specific_error(m.type, "Expected trait instance type");
					return false;
				}

				if (!t->compile(nctx)) return false;

				TypeTraitInstance* tt = (TypeTraitInstance*)t;
				if (new_inst->traitfunctions.find(tt->owner) != new_inst->traitfunctions.end()) {
					throw_specific_error(err, "This trait was already implemented");
					return false;
				}
				else {
					std::map<std::string_view, std::unique_ptr<FunctionTemplate>> trait;
					for (auto&& f : m.functions) {
						std::unique_ptr<FunctionTemplate> ft = std::make_unique<FunctionTemplate>();
						ft->name = f.name;
						ft->is_generic = false;
						ft->parent = new_inst;
						ft->template_parent = new_inst;
						ft->type = m.type;
						ft->block = f.block;
						if (trait.find(f.name.buffer) != trait.end()) {
							throw_specific_error(f.name, "Funtion with the same name already exists in the implementation");
							return false;
						}

						if (tt->owner->member_table.find(f.name.buffer) == tt->owner->member_table.end()) {
							throw_specific_error(f.name, "Implemented trait has no function with this name");
							return false;
						}

						trait[f.name.buffer] = std::move(ft);
					}

					new_inst->traitfunctions[tt->owner] = std::move(trait);
				}
			}

			for (auto&& m : member_vars) {
				Cursor c = m.type;

				CompileValue value;

				CompileContext nctx = ctx;
				nctx.inside = parent;
				Cursor err = c;
				if (!Expression::parse(c, nctx, value, CompileType::eval)) return false;
				if (value.t != ctx.default_types->t_type) {
					throw_specific_error(m.type, "Expected type value");
					std::cerr << " | \tType was '";
					value.t->print(std::cerr);
					std::cerr << "'\n";
					return false;
				}

				Type* m_t = ctx.eval->pop_register_value<Type*>();
				if (m_t == nullptr) {
					std::cout << "error";
				}

				new_inst->member_table[m.name.buffer] = new_inst->member_vars.size();
				StructureInstanceMemberRecord rec;
				rec.definition = m.name;
				rec.type = m_t;

				new_inst->member_vars.push_back(rec);
			}

			for (auto&& m : member_funcs) {
				Cursor c = m.type;
				//TODO we could probably skip the catalogue stage and build the functiontemplate directly in the structure template
				
				CompileContext nctx = ctx;
				nctx.inside = parent;

				std::unique_ptr<FunctionTemplate> ft = std::make_unique<FunctionTemplate>();
				ft->name = m.name;
				ft->annotation = m.annotation;
				ft->is_generic = m.annotation.tok != RecognizedToken::Eof;
				ft->parent = new_inst;
				ft->template_parent = new_inst;
				ft->type = m.type;
				ft->block = m.block;
				if (new_inst->subfunctions.find(m.name.buffer) != new_inst->subfunctions.end()) {
					throw_specific_error(m.name, "Funtion with the same name already exists in the structure");
					return false;
				}

				new_inst->subfunctions[m.name.buffer] = std::move(ft);
			}
		}

		return true;
	}


	bool TraitTemplate::generate(CompileContext& ctx, unsigned char* argdata, TraitInstance*& out) {
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

			for (auto&& m : member_funcs) {



				TraitInstanceMemberRecord member;

				member.definition = m.type;
				Cursor c = member.definition;

				if (c.tok != RecognizedToken::OpenParenthesis) {
					throw_wrong_token_error(c, "'('");
					return false;
				}

				c.move();

				CompileContext nctx = ctx;
				nctx.inside = parent;

				if (c.tok != RecognizedToken::CloseParenthesis) {
					while (true) {
						Cursor err = c;
						CompileValue val;
						if (!Expression::parse(c, nctx, val, CompileType::eval)) return false;

						if (val.t != ctx.default_types->t_type) {
							throw_specific_error(err, "Expected type");
							return false;
						}
						Type* t = ctx.eval->pop_register_value<Type*>();
						member.arg_types.push_back(t);
						if (c.tok == RecognizedToken::Comma) {
							c.move();
						}
						else if (c.tok == RecognizedToken::CloseParenthesis) {
							c.move();
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
					member.return_type = ctx.default_types->t_void;
				}
				else {
					Cursor err = c;
					CompileValue val;
					if (!Expression::parse(c, nctx, val, CompileType::eval)) return false;

					if (val.t != ctx.default_types->t_type) {
						throw_specific_error(err, "Expected type");
						return false;
					}
					Type* t = ctx.eval->pop_register_value<Type*>();
					member.return_type = t;
				}



				
				new_inst->member_table[m.name.buffer] = new_inst->member_funcs.size();
				new_inst->member_funcs.push_back(std::move(member));
			}
		}

		return true;
	}


	bool FunctionTemplate::generate(CompileContext& ctx, unsigned char* argdata, FunctionInstance*& out) {
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
			new_inst->generator = this;
			new_inst->key = new_key;

			CompileContext nctx = ctx;
			nctx.inside = parent;

			CompileValue cvres;

			Cursor cc = type;
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
					if (!Expression::parse(cc, nctx, cvres, CompileType::eval))return false;
					if (cvres.t != ctx.default_types->t_type) {
						throw_cannot_cast_error(err, cvres.t, ctx.default_types->t_type);
						return false;
					}
					Type* t = ctx.eval->pop_register_value<Type*>();
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
				if (!Expression::parse(cc, nctx, cvres, CompileType::eval))return false;
				if (cvres.t != ctx.default_types->t_type) {
					throw_cannot_cast_error(err, cvres.t, ctx.default_types->t_type);
					return false;
				}
				Type* t = ctx.eval->pop_register_value<Type*>();
				new_inst->returns = t;
			}
			else {
				new_inst->returns = ctx.default_types->t_void;
			}

			new_inst->block = block;
		}

		return true;
	}

	bool FunctionInstance::compile(CompileContext& ctx) {
		if (compile_state == 0) {
			compile_state = 1;

			func = ctx.module->create_function();

			auto ss = std::move(StackManager::move_stack_out<0>());
			for (auto&& a : arguments) {
				CompileValue argval;
				argval.t = a.second;
				argval.lvalue = true;
				//uint16_t id = func->register_local(argval.t->compile_size(ctx), argval.t->size(ctx));

				StackManager::stack_push<0>(ctx,a.first.buffer, argval);
			}

			CompileContext nctx = ctx;
			nctx.inside = generator->parent;

			

			ILBlock* b = func->create_and_append_block(ILDataType::none);
			b->alias = "entry";

			nctx.scope = b;

			nctx.function = this;

			Cursor cb = block;
			CompileValue cvres;
			bool terminated;
			if (!Statement::parse_inner_block(cb, nctx, cvres, CompileType::compile,terminated)) return false;


			ILBlock* b_exit = func->create_and_append_block(returns->rvalue);

			ILBuilder::build_yield(nctx.scope, cvres.t->rvalue);
			ILBuilder::build_jmp(nctx.scope, b_exit);


			b_exit->alias = "entry_exit";
			ILBuilder::build_accept(b_exit,returns->rvalue);
			ILBuilder::build_ret(b_exit,returns->rvalue);




			func->dump();

			if (!func->assert_flow()) return false;

			StackManager::move_stack_in<0>(std::move(ss));

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



	unsigned int _align_up(unsigned int value, unsigned int alignment) {
		return alignment == 0 ? value : ((value % alignment == 0) ? value : value + (alignment - (value % alignment)));
	}

	bool TraitInstance::compile(CompileContext& ctx) {
		if (compile_state == 0) {
			compile_state = 1;

			/*auto ss = std::move(StackManager::move_stack_out<1>());
			auto sp = ctx.eval->stack_push();

			insert_key_on_stack(ctx);*/

			alignment = 0;
			size = 0;

			compile_alignment = 0;
			compile_size = 0;

			for (auto&& m : member_funcs) {
				size = _align_up(size, ctx.default_types->t_ptr->alignment(ctx));
				compile_size = _align_up(compile_size, ctx.default_types->t_ptr->compile_alignment(ctx));

				m.offset = size;
				m.compile_offset = compile_size;


				size += ctx.default_types->t_ptr->size(ctx);
				compile_size += ctx.default_types->t_ptr->compile_size(ctx);

				alignment = std::max(alignment, ctx.default_types->t_ptr->alignment(ctx));
				compile_alignment = std::max(compile_alignment, ctx.default_types->t_ptr->compile_alignment(ctx));

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

	bool StructureInstance::compile(CompileContext& ctx) {
		if (compile_state == 0) {
			compile_state = 1;

			alignment = 0;
			size = 0;

			compile_alignment = 0;
			compile_size = 0;

			for (auto&& m : member_vars) {
				if (!m.type->compile(ctx)) return false;
				

				if (m.type->size(ctx) > 0) {
					size = _align_up(size, m.type->alignment(ctx));
					compile_size = _align_up(compile_size, m.type->compile_alignment(ctx));
					m.offset = size;
					m.compile_offset = compile_size;
					size += m.type->size(ctx);
					compile_size += m.type->compile_size(ctx);

					alignment = std::max(alignment, m.type->alignment(ctx));
					compile_alignment = std::max(compile_alignment, m.type->compile_alignment(ctx));
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
				if (!m.second->compile(ctx)) return false;
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

	void TraitInstance::insert_key_on_stack(CompileContext& ctx) {
		if (generator->template_parent != nullptr) {
			generator->template_parent->insert_key_on_stack(ctx);
		}

		unsigned char* key_ptr = key;
		for (auto key_l = generator->generic_layout.rbegin(); key_l != generator->generic_layout.rend(); key_l++) {

			CompileValue res;
			res.lvalue = true;
			res.t = std::get<1>(*key_l);

			unsigned char* data_place = ctx.eval->stack_reserve(res.t->compile_size(ctx));

			StackManager::stack_push<1>(ctx, std::get<0>(*key_l).buffer, res);

			res.t->move(ctx, key_ptr, data_place); // TODO! there will be copy

			key_ptr += res.t->compile_size(ctx);
		}

	}



	void StructureInstance::insert_key_on_stack(CompileContext& ctx) {
		if (generator->template_parent != nullptr) {
			generator->template_parent->insert_key_on_stack(ctx);
		}

		unsigned char* key_ptr = key;
		for (auto key_l = generator->generic_layout.rbegin(); key_l != generator->generic_layout.rend(); key_l++) {
			
		
			CompileValue res;
			res.lvalue = true;
			res.t = std::get<1>(*key_l);

			unsigned char* data_place = ctx.eval->stack_reserve(res.t->compile_size(ctx));
			
			StackManager::stack_push<1>(ctx,std::get<0>(*key_l).buffer, res);

			res.t->move(ctx, key_ptr, data_place); // TODO! there will be copy

			key_ptr += res.t->compile_size(ctx);
		}

	}
}