#include "Declaration.h"
#include "Error.h"
#include <iostream>
#include "Contents.h"
#include <string>
#include "PredefinedTypes.h"
#include "Expression.h"
#include "StackManager.h"
#include "Type.h"

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

					ILCtype ctype = ctx.eval->pop_register_value<ILCtype>();
					Type t = { (TypeInstance*)ctype.type,ctype.ptr };

					if (t.type->type != TypeInstanceType::type_instance) {
						throw_specific_error(err, "Type does not point to instance");
						return false;
					}

					if (!t.compile(ctx)) return false;
					
					generate_heap_size += t.compile_time_size(ctx.eval);
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
			new_inst->type->type = TypeInstanceType::type_instance;
			new_inst->type->owner_ptr = (void*)new_inst;
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
					return false;
				}

				ILCtype ctype = ctx.eval->pop_register_value<ILCtype>();
				Type m_t = { (TypeInstance*)ctype.type,ctype.ptr };
				new_inst->member_vars[m.name.buffer] = std::make_pair(m.name, m_t);
			}

			for (auto&& m : member_funcs) {
				Cursor c = m.type;

				CompileValue value;

				CompileContext nctx = ctx;
				nctx.inside = parent;

				if (!Expression::parse(c, nctx, value, CompileType::eval)) return false;
				if (value.t != ctx.default_types->t_type) {
					throw_specific_error(m.type, "Expected type value");
					return false;
				}

				ILCtype ctype = ctx.eval->pop_register_value<ILCtype>();

				ILFunction* func = ctx.module->create_function(ctx.module->t_void);
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

				new_inst->member_funcs[m.name.buffer] = std::make_pair<ILFunction*, Type>(nullptr, { (TypeInstance*)ctype.type,ctype.ptr });
			}
		}

		return true;
	}



	bool StructureInstance::compile(CompileContext& ctx) {
		if (compile_state == 0) {
			compile_state = 1;
			iltype = ctx.module->create_struct_type();

			for (auto&& m : member_vars) {
				if (m.second.second.ref_count == 0) {
					if (!m.second.second.type->compile(ctx)) return false;
				}

				if (m.second.second.type->type == TypeInstanceType::type_instance) {
					((ILStruct*)iltype)->add_member(((StructureInstance*)m.second.second.type->owner_ptr)->iltype);
				}
				else {
					throw_specific_error(m.second.first, "Specified type is not an instance type");
					std::cerr << " | \tType was '";
					m.second.second.type->print(std::cerr);
					std::cerr << "'\n";
					return false;
				}
			}

			((ILStruct*)iltype)->align_size();

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
			key_ptr += std::get<1>(*key_l).compile_time_size(ctx.eval);
			CompileValue argval;
			argval.lvalue = true;
			argval.t = std::get<1>(*key_l);
			StackManager::stack_push<1>(std::get<0>(*key_l).buffer, argval, (unsigned int)StackManager::stack_state<1>());
		}

	}
}