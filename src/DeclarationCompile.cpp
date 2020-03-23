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
					
					generate_heap_size += t->compile_time_size(ctx.eval);
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
				gen_template_cmp.eval = ctx.eval;
				instances = std::make_unique<std::map<std::pair<unsigned int,void*>, std::unique_ptr<StructureInstance>, GenericTemplateCompare>>(gen_template_cmp);
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



	bool StructureTemplate::generate(CompileContext& ctx,void* argdata, StructureInstance*& out) {
		StructureInstance* new_inst = nullptr;
		void* new_key = nullptr;

		if (!is_generic) {
			if (singe_instance == nullptr) {
				singe_instance = std::make_unique<StructureInstance>();
				new_inst = singe_instance.get();
			}
			out = singe_instance.get();
		}
		else {
			std::pair<unsigned int, void*> key = std::make_pair((unsigned int)generate_heap_size, argdata);


			auto f = instances->find(key);
			if (f == instances->end()) {
				std::unique_ptr<StructureInstance> inst = std::make_unique<StructureInstance>();

				

				new_inst = inst.get();
				out = inst.get();
				key.second = new unsigned char[generate_heap_size];
				memcpy(key.second, argdata, generate_heap_size);
				new_key = key.second;

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

				new_inst->member_vars.push_back(std::make_pair(m.name, m_t));
			}

			for (auto&& m : member_funcs) {
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

				Type* t = ctx.eval->pop_register_value<Type*>();

				ILFunction* func = ctx.module->create_function(0);
				ILBlock* block = func->create_block(ILDataType::none);
				func->append_block(block);

				CompileContext bctx = ctx;
				bctx.block = block;
				bctx.function = func;
				bctx.inside = new_inst;
				CompileValue res;
				c = m.block;
				Expression::parse(c, bctx, res, CompileType::compile);

				func->dump();

				new_inst->member_funcs.push_back(std::make_pair<ILFunction*, Type*>((ILFunction*)nullptr, (Type*)t));
			}
		}

		return true;
	}

	unsigned int _align_up(unsigned int value, unsigned int alignment) {
		return alignment == 0 ? value : ((value % alignment == 0) ? value : value + (alignment - (value % alignment)));
	}


	void StructureInstance::add_member(CompileContext& ctx,Type* t) {
		runtime_size = _align_up(runtime_size,t->runtime_alignment(ctx));
		runtime_size += t->runtime_size(ctx);
		compile_time_size_in_bytes += t->compile_time_size(ctx.eval);
	}

	bool StructureInstance::compile(CompileContext& ctx) {
		if (compile_state == 0) {
			compile_state = 1;

			compile_time_size_in_bytes = 0;
			runtime_alignment = 0;
			runtime_size = 0;

			for (auto&& m : member_vars) {
				if (!m.second->compile(ctx)) return false;
				

				if (m.second->compile_time_size(ctx.eval) > 0) {
					add_member(ctx,m.second);
				}
				else {
					throw_specific_error(m.first, "Specified type is not an instantiable type");
					std::cerr << " | \tType was '";
					m.second->print(std::cerr);
					std::cerr << "'\n";
					return false;
				}
			}

			runtime_size = _align_up(runtime_size, runtime_alignment);

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

		unsigned char* key_ptr = (unsigned char*)key;
		for (auto key_l = generator->generic_layout.rbegin(); key_l != generator->generic_layout.rend(); key_l++) {
			ctx.eval->stack_push_pointer(key_ptr);
			key_ptr += std::get<1>(*key_l)->compile_time_size(ctx.eval);
			CompileValue argval;
			argval.lvalue = true;
			argval.t = std::get<1>(*key_l);
			StackManager::stack_push<1>(std::get<0>(*key_l).buffer, argval, (unsigned int)StackManager::stack_state<1>());
		}

	}
}