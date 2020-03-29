#include "Declaration.h"
#include "Error.h"
#include <iostream>
#include "Contents.h"
#include <string>
#include "PredefinedTypes.h"
#include "Expression.h"
#include "StackManager.h"
#include "Type.h"
#include <algorithm>

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

					generate_heap_size += t->size(ctx);
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
				instances = std::make_unique<std::map<std::pair<unsigned int, ILPtr>, std::unique_ptr<FunctionInstance>, GenericTemplateCompare>>(gen_template_cmp);
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
					
					generate_heap_size += t->size(ctx);
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
				instances = std::make_unique<std::map<ILPtr, std::unique_ptr<StructureInstance>, GenericTemplateCompare>>(gen_template_cmp);
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



	bool StructureTemplate::generate(CompileContext& ctx, ILPtr argdata, StructureInstance*& out) {
		StructureInstance* new_inst = nullptr;
		ILPtr new_key = ilnullptr;

		if (!is_generic) {
			if (singe_instance == nullptr) {
				singe_instance = std::make_unique<StructureInstance>();
				new_inst = singe_instance.get();
			}
			out = singe_instance.get();
		}
		else {
			ILPtr key = argdata;


			auto f = instances->find(key);
			if (f == instances->end()) {
				std::unique_ptr<StructureInstance> inst = std::make_unique<StructureInstance>();

				new_inst = inst.get();
				out = inst.get();
				key = ctx.eval->malloc(generate_heap_size);
				memcpy(ctx.eval->map(key), ctx.eval->map(argdata), generate_heap_size);
				new_key = key;

				instances->emplace(key, std::move(inst));

			}
			else {
				out = f->second.get();
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

			for (auto&& m : member_vars) {
				Cursor c = m.type;

				CompileValue value;

				CompileContext nctx = ctx;
				nctx.inside = parent;
				if (!Expression::parse(c, nctx, value, CompileType::eval)) return false;
				if (value.t != ctx.default_types->t_type) {
					throw_specific_error(m.type, "Expected type value");
					std::cerr << " | \tType was '";
					value.t->print(std::cerr);
					std::cerr << "'\n";
					return false;
				}

				Type* m_t = ctx.eval->pop_register_value<Type*>();

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


	bool FunctionTemplate::generate(CompileContext& ctx, ILPtr argdata, FunctionInstance*& out) {
		FunctionInstance* new_inst = nullptr;
		ILPtr new_key = ilnullptr;

		if (!is_generic) {
			if (singe_instance == nullptr) {
				singe_instance = std::make_unique<FunctionInstance>();
				new_inst = singe_instance.get();
			}
			out = singe_instance.get();
		}
		else {
			std::pair<unsigned int, ILPtr> key = std::make_pair((unsigned int)generate_heap_size, argdata);

			auto f = instances->find(key);

			if (f == instances->end()) {
				std::unique_ptr<FunctionInstance> inst = std::make_unique<FunctionInstance>();

				new_inst = inst.get();
				out = inst.get();
				key.second = ctx.eval->malloc(generate_heap_size);
				memcpy(ctx.eval->map(key.second), ctx.eval->map(argdata), generate_heap_size);
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

			auto ss = std::move(StackManager::move_stack_out<0>());
			for (auto&& a : arguments) {
				CompileValue argval;
				argval.t = a.second;
				argval.lvalue = true;
				StackManager::stack_push<0>(ctx,a.first.buffer, argval);
			}

			CompileContext nctx = ctx;
			nctx.inside = generator->parent;

			ILFunction* nf = ctx.module->create_function(returns->size(ctx));
			ILBlock* b = nf->create_block(ILDataType::none);
			nf->append_block(b);
			nctx.block = b;
			nctx.function = nf;

			Cursor cb = block;
			CompileValue cvres;
			if (!Expression::parse(cb, nctx, cvres, CompileType::compile)) return false;

			func = nf;
			nf->dump();

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


	bool StructureInstance::compile(CompileContext& ctx) {
		if (compile_state == 0) {
			compile_state = 1;

			alignment = 0;
			size = 0;

			for (auto&& m : member_vars) {
				if (!m.type->compile(ctx)) return false;
				

				if (m.type->size(ctx) > 0) {
					size = _align_up(size, m.type->alignment(ctx));
					m.offset = size;
					size += m.type->size(ctx);

					alignment = std::max(alignment, m.type->alignment(ctx));
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




	void StructureInstance::insert_key_on_stack(CompileContext& ctx) {
		if (generator->template_parent != nullptr) {
			generator->template_parent->insert_key_on_stack(ctx);
		}

		ILPtr key_ptr = key;
		for (auto key_l = generator->generic_layout.rbegin(); key_l != generator->generic_layout.rend(); key_l++) {
			
			/*ctx.eval->stack_push_pointer(key_ptr);
			key_ptr += std::get<1>(*key_l)->size(ctx);
			CompileValue argval;
			argval.lvalue = true;
			argval.t = std::get<1>(*key_l);
			StackManager::stack_push<1>(std::get<0>(*key_l).buffer, argval, (unsigned int)StackManager::stack_state<1>());*/

			CompileValue res;
			res.lvalue = true;
			res.t = std::get<1>(*key_l);

			ILPtr data_place = ctx.eval->stack_reserve(res.t->size(ctx));
			StackManager::stack_push<1>(ctx,std::get<0>(*key_l).buffer, res);


			res.t->move(ctx, key_ptr, data_place); // TODO! there will be copy

			/*bool stacked = res.t->rvalue_stacked();
			if (stacked) {
				res.t->move(ctx, key_ptr, data_place);
			}
			else {
				memcpy(ctx.eval->map(data_place), ctx.eval->map(key_ptr), res.t->size(ctx));
			}*/

			key_ptr += res.t->size(ctx);
		}

	}
}