#include "Declaration.h"
#include "Error.h"
#include <iostream>
#include "Contents.h"
#include <string>
#include "PredefinedTypes.h"
#include "Expression.h"
#include "StackManager.h"

namespace Corrosive {

	bool Structure::compile(CompileContext& ctx) {
		if (compile_state == 0) {
			compile_state = 1;

			if (is_generic) {
				Namespace* i_push = ctx.inside;
				ctx.inside = this;

				Cursor c = generic_types;
				unsigned int arg_i = 0;
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
					if (!Expression::parse(c, ctx, value, CompileType::eval)) return false;
					if (value.t != ctx.default_types->t_type) {
						throw_specific_error(err, "Expected type value");
						return false;
					}

					ILCtype ctype = ctx.eval->pop_register_value<ILCtype>();
					Type t = { (AbstractType*)ctype.type,ctype.ptr };

					if (!dynamic_cast<InstanceType*>(t.type)) {
						throw_specific_error(err, "Type does not point to instance");
						return false;
					}

					if (!t.compile(ctx)) return false;
					
					generate_heap_size += t.size(ctx);
					generic_layout.push_back(std::make_tuple(name,t.size(ctx),t));

					


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


				/*for (auto&& m : members) {

					CompileValue value;
					Cursor m_type = m.type;
					if (!Expression::parse(m_type, nctx, value, CompileType::compile)) return false;
					
					if (value.t != ctx.default_types->t_type) {
						throw_specific_error(m.type, "Expected type value");
						return false;
					}

					if (value.lvalue) {
						ILBuilder::build_load(nctx.block, ILDataType::ctype);
					}
					ILBuilder::build_yield_type(nctx.block, m.name.buffer);

				}*/

				gen_template_cmp.parent = this;
				instances = std::make_unique<std::map<std::pair<unsigned int,void*>, std::unique_ptr<StructureInstance>, GenericTemplateCompare>>(gen_template_cmp);
				

				ctx.inside = i_push;

			}
			else {
				singe_instance = std::make_unique<StructureInstance>();
				singe_instance->type = std::make_unique<InstanceType>();
				singe_instance->type->owner = singe_instance.get();
				singe_instance->generator = this;
				singe_instance->type->rvalue = ILDataType::ptr;

				compile_state = 2;

				for (auto&& m : member_vars) {
					Cursor c = m.type;

					CompileValue value;

					CompileContext nctx = ctx;
					nctx.inside = this;
					if (!Expression::parse(c, nctx, value, CompileType::eval)) return false;
					if (value.t != ctx.default_types->t_type) {
						throw_specific_error(m.type, "Expected type value");
						return false;
					}

					ILCtype ctype = ctx.eval->pop_register_value<ILCtype>();

					Type m_t = { (AbstractType*)ctype.type,ctype.ptr };
					singe_instance->member_vars[m.name.buffer] = std::make_pair(m.name, m_t);
				}

				for (auto&& m : member_funcs) {
					Cursor c = m.type;

					CompileValue value;
					if (!Expression::parse(c, ctx, value, CompileType::eval)) return false;
					if (value.t != ctx.default_types->t_type) {
						throw_specific_error(m.type, "Expected type value");
						return false;
					}

					ILCtype ctype = ctx.eval->pop_register_value<ILCtype>();

					ILFunction* func = ctx.module->create_function(ctx.module->t_void);
					ILBlock* block = func->create_block(ILDataType::none);
					func->append_block(block);
					
					CompileContext nctx = ctx;
					nctx.block = block;
					nctx.function = func;
					nctx.inside = this;
					CompileValue res;
					c = m.block;
					Expression::parse(c, nctx, res, CompileType::compile);

					func->dump();

					singe_instance->member_funcs[m.name.buffer] = std::make_pair<ILFunction*,Type>(nullptr,{ (AbstractType*)ctype.type,ctype.ptr });
				}
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



	bool Structure::generate(CompileContext& ctx,void* argdata, StructureInstance*& out) {
		if (!is_generic) {
			return false;
		}

		std::pair<unsigned int, void*> key = std::make_pair((unsigned int)generate_heap_size, argdata);


		auto f = instances->find(key);
		if (f == instances->end()) {
			std::unique_ptr<StructureInstance> inst = std::make_unique<StructureInstance>();

			inst->type = std::make_unique<InstanceType>();
			inst->type->owner = inst.get();
			inst->generator = this;
			inst->type->rvalue = ILDataType::ptr;

			for (auto&& m : member_vars) {
				Cursor c = m.type;

				CompileValue value;

				CompileContext nctx = ctx;
				nctx.inside = this;
				if (!Expression::parse(c, nctx, value, CompileType::eval)) return false;
				if (value.t != ctx.default_types->t_type) {
					throw_specific_error(m.type, "Expected type value");
					return false;
				}

				ILCtype ctype = ctx.eval->pop_register_value<ILCtype>();
				Type m_t = { (AbstractType*)ctype.type,ctype.ptr };
				inst->member_vars[m.name.buffer] = std::make_pair(m.name, m_t);
			}

			for (auto&& m : member_funcs) {
				Cursor c = m.type;

				CompileValue value;
				if (!Expression::parse(c, ctx, value, CompileType::eval)) return false;
				if (value.t != ctx.default_types->t_type) {
					throw_specific_error(m.type, "Expected type value");
					return false;
				}

				ILCtype ctype = ctx.eval->pop_register_value<ILCtype>();

				ILFunction* func = ctx.module->create_function(ctx.module->t_void);
				ILBlock* block = func->create_block(ILDataType::none);
				func->append_block(block);

				CompileContext nctx = ctx;
				nctx.block = block;
				nctx.function = func;
				nctx.inside = this;
				CompileValue res;
				c = m.block;
				Expression::parse(c, nctx, res, CompileType::compile);

				func->dump();

				inst->member_funcs[m.name.buffer] = std::make_pair<ILFunction*, Type>(nullptr, { (AbstractType*)ctype.type,ctype.ptr });
			}


			out = inst.get();
			key.second = new unsigned char[generate_heap_size];
			memcpy(key.second, argdata, generate_heap_size);

			instances->emplace(key,std::move(inst));


			//ctx.eval->stack_pop();
			//StackManager::move_stack_in<1>(std::move(ss));
		}
		else {
			out = f->second.get();
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

				if (auto it = dynamic_cast<InstanceType*>(m.second.second.type)) {
					((ILStruct*)iltype)->add_member(it->owner->iltype);
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
}