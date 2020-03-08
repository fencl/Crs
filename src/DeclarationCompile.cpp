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

				auto ss = StackManager::move_stack_out();

				generator = ctx.module->create_function(ctx.module->t_void);
				ILBlock* block = generator->create_block(ILDataType::none);
				generator->append_block(block);

				CompileContext nctx = ctx;
				nctx.block = block;
				nctx.function = generator;
				nctx.inside = this;

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
					if (!Expression::parse(c, nctx, value, CompileType::eval)) return false;
					if (value.t != ctx.default_types->t_type) {
						throw_specific_error(err, "Expected type value");
						return false;
					}

					ILCtype ctype = nctx.eval->pop_register_value<ILCtype>();
					Type t = { (AbstractType*)ctype.type,ctype.ptr };
					if (t.ref_count == 0) {
						if (!t.type->compile(nctx)) return false;
					}
					
					generate_heap_size += t.size(nctx);

					value.lvalue = true;
					value.t = t;
					StackManager::stack_push(name.buffer, value, arg_i);
					arg_i++;


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


				for (auto&& m : members) {

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

				}


				ILBuilder::build_ret(nctx.block);

				StackManager::move_stack_in(std::move(ss));


				if (!nctx.function->assert_flow()) return false;

				generator->dump();
				std::cout << generate_heap_size << std::endl;
			}
			else {
				singe_instance = std::make_unique<StructureInstance>();
				singe_instance->type = std::make_unique<InstanceType>();
				singe_instance->type->owner = singe_instance.get();
				singe_instance->generator = this;
				singe_instance->type->rvalue = ILDataType::ptr;

				compile_state = 2;

				for (auto&& m : members) {
					Cursor c = m.type;

					CompileValue value;
					if (!Expression::parse(c, ctx, value, CompileType::eval)) return false;
					if (value.t != ctx.default_types->t_type) {
						throw_specific_error(m.type, "Expected type value");
						return false;
					}

					ILCtype ctype = ctx.eval->pop_register_value<ILCtype>();

					singe_instance->members[m.name.buffer] = { (AbstractType*)ctype.type,ctype.ptr };
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




	bool StructureInstance::compile(CompileContext& ctx) {
		if (compile_state == 0) {
			compile_state = 1;
			iltype = ctx.module->create_struct_type();

			for (auto&& m : members) {
				if (m.second.ref_count == 0) {
					if (!m.second.type->compile(ctx)) return false;
				}

				if (auto it = dynamic_cast<InstanceType*>(m.second.type)) {
					((ILStruct*)iltype)->add_member(it->owner->iltype);
				}
				else {
					throw_specific_error(generator->name, "member is not instantiable type");
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