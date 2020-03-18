#include "Operand.h"
#include "Error.h"
#include "Utilities.h"
#include "PredefinedTypes.h"
#include "Expression.h"
#include "StackManager.h"

#include <iostream>

namespace Corrosive {

	bool Operand::cast(Cursor& err, CompileContext& ctx, CompileValue& res,Type to, CompileType copm_type) {
		if (res.t != to) {
			throw_cannot_cast_error(err, res.t, to);
			return false;
		}
		return true;
	}

	bool Operand::parse(Cursor& c, CompileContext& ctx, CompileValue& res, CompileType cpt) {
		CompileValue ret;
		ret.lvalue = false;
		ret.t = Type::null;

		unsigned int type_ref_count = 0;
		while (c.tok == RecognizedToken::And || c.tok == RecognizedToken::DoubleAnd) {
			type_ref_count++;
			if (c.tok == RecognizedToken::DoubleAnd)
				type_ref_count++;
			c.move();
		}

		if (type_ref_count > 0) {
			Cursor err = c;
			CompileValue value;
			if (!Operand::parse(c, ctx, value, cpt)) return false;
			Expression::rvalue(ctx, value, cpt);

			if (value.t != ctx.default_types->t_type) {
				throw_specific_error(err, "operator expected to recieve type");
				return false;
			}

			if (cpt == CompileType::eval) {
				ILCtype t = ctx.eval->pop_register_value<ILCtype>();
				t.ptr += type_ref_count;
				ILBuilder::eval_const_ctype(ctx.eval, t);
			}
			else if (cpt == CompileType::compile) {
				throw_specific_error(err, "Operator cannot be used in compile context");
				return true;
			}

			res.t = ctx.default_types->t_type;
			res.lvalue = false;
			return true;
		}

		switch (c.tok) {
			case RecognizedToken::OpenParenthesis: {
					if (!parse_expression(ret, c, ctx, cpt)) return false;
				}break;

			case RecognizedToken::Symbol: {
					if (!parse_symbol(ret, c, ctx, cpt)) return false;
				}break;

			case RecognizedToken::Number:
			case RecognizedToken::UnsignedNumber: {
					if (!parse_number(ret, c, ctx, cpt)) return false;
				}break;

			case RecognizedToken::LongNumber:
			case RecognizedToken::UnsignedLongNumber: {
					if (!parse_long_number(ret, c, ctx, cpt)) return false;
				}break;

			case RecognizedToken::FloatNumber:
			case RecognizedToken::DoubleNumber: {
					if (!parse_float_number(ret, c, ctx, cpt)) return false;
				}break;

			default: {
					throw_specific_error(c, "Expected to parse operand");
					return false;
				} break;
		}

		while (true) {
			switch (c.tok) {
				case RecognizedToken::OpenParenthesis: {
						if (!parse_call_operator(ret, c, ctx, cpt)) return false;
					}break;
				case RecognizedToken::OpenBracket: {
						if (!parse_array_operator(ret, c, ctx, cpt)) return false;
					}break;
				case RecognizedToken::Dot: {
						if (!parse_dot_operator(ret, c, ctx, cpt)) return false;
					}break;
				default: goto break_while;
			}

			continue;
		break_while:
			break;
		}

		res = ret;
		return true;
	}







	bool Operand::parse_expression(CompileValue& ret, Cursor& c, CompileContext& ctx, CompileType cpt) {
		c.move();
		if (!Expression::parse(c, ctx, ret, cpt)) return false;
		if (c.tok != RecognizedToken::CloseParenthesis) {
			throw_wrong_token_error(c, "')'");
			return false;
		}
		c.move();

		return true;
	}



	bool Operand::parse_symbol(CompileValue& ret, Cursor& c, CompileContext& ctx, CompileType cpt) {
		if (c.buffer == "true") {
			c.move();
			ret.lvalue = false;
			ret.t = ctx.default_types->t_bool;
			if (cpt == CompileType::compile) {
				if (!ILBuilder::build_const_ibool(ctx.block, true)) return false;
			}
			else if (cpt == CompileType::eval) {
				if (!ILBuilder::eval_const_ibool(ctx.eval, true)) return false;
			}
		}
		else if (c.buffer == "false") {
			c.move();
			ret.lvalue = false;
			ret.t = ctx.default_types->t_bool;
			if (cpt == CompileType::compile) {
				if (!ILBuilder::build_const_ibool(ctx.block, false)) return false;
			}
			else if (cpt == CompileType::eval) {
				if (!ILBuilder::eval_const_ibool(ctx.eval, false)) return false;
			}
		}
		else if (c.buffer == "type") {
			c.move();
			ret.lvalue = false;
			ret.t = ctx.default_types->t_type;
			ILCtype ct = { ctx.default_types->t_type.type,0 };

			if (cpt == CompileType::compile) {
				if (!ILBuilder::build_const_ctype(ctx.block, ct)) return false;
			}
			else if (cpt == CompileType::eval) {
				if (!ILBuilder::eval_const_ctype(ctx.eval, ct)) return false;
			}
		}
		else {
			
			StackItem* sitm;

			if (cpt != CompileType::eval && (sitm = StackManager::stack_find<0>(c.buffer))) {
				if (cpt == CompileType::compile) {
					ILBuilder::build_local(ctx.block, sitm->ir_local);
					ret = sitm->value;
					ret.lvalue = true;
				}
				c.move();
			}
			else if (cpt != CompileType::compile && (sitm = StackManager::stack_find<1>(c.buffer))) {
				if (cpt == CompileType::eval) {
					ILBuilder::eval_local(ctx.eval, sitm->ir_local);
					ret = sitm->value;
					ret.lvalue = true;
				}
				c.move();
			}
			else {
				Namespace* inst = nullptr;
				
				if (cpt != CompileType::short_circuit) {
					inst = ctx.inside->find_name(c.buffer);

					if (inst == nullptr) {
						throw_specific_error(c, "Path start point not found");
						return false;
					}
				}

				c.move();
				while (c.tok == RecognizedToken::DoubleColon) {
					c.move();
					if (c.tok != RecognizedToken::Symbol)
					{
						throw_not_a_name_error(c);
						return false;
					}

					if (cpt != CompileType::short_circuit) {
						auto f = inst->subnamespaces.find(c.buffer);
						if (f != inst->subnamespaces.end()) {
							inst = f->second.get();
						}
						else {
							throw_specific_error(c, "Namespace path is not valid");
							return false;
						}
					}
					c.move();
				}

				if (cpt != CompileType::short_circuit) {
					if (auto struct_inst = dynamic_cast<Structure*>(inst)) {
						if (!struct_inst->compile(ctx)) return false;

						if (struct_inst->is_generic) {
							ILCtype ct = { (void*)struct_inst->type.get(),0 };
							if (cpt == CompileType::eval) {
								if (!ILBuilder::eval_const_ctype(ctx.eval, ct)) return false;
							}
							else if (cpt == CompileType::compile) {
								if (!ctx.function->is_const) {
									throw_specific_error(c, "Use of type in runtime context");
									return false;
								}

								if (!ILBuilder::build_const_ctype(ctx.block, ct)) return false;
							}
						}
						else {
							ILCtype ct = { (void*)struct_inst->singe_instance->type.get(),0 };

							if (cpt == CompileType::eval) {
								if (!ILBuilder::eval_const_ctype(ctx.eval, ct)) return false;
							}
							else if (cpt == CompileType::compile) {
								if (!ctx.function->is_const) {
									throw_specific_error(c, "Use of type in runtime context");
									return false;
								}
								if (!ILBuilder::build_const_ctype(ctx.block, ct)) return false;
							}
						}
					}
					else {
						throw_specific_error(c, "Path is pointing to a namespace");
						return false;
					}
				}

				ret.lvalue = false;
				ret.t = ctx.default_types->t_type;
			}

		}

		return true;
	}




	bool Operand::parse_number(CompileValue& ret, Cursor& c, CompileContext& ctx, CompileType cpt) {
		bool usg = c.tok == RecognizedToken::UnsignedNumber;

		std::string_view ndata;
		if (usg)
			ndata = c.buffer.substr(0, c.buffer.size() - 1);
		else
			ndata = c.buffer;

		unsigned long long d = svtoi(ndata);
		c.move();

		if (cpt == CompileType::compile) {
			if (usg)
				ILBuilder::build_const_u32(ctx.block, (uint32_t)d);
			else
				ILBuilder::build_const_i32(ctx.block, (int32_t)d);
		}
		else if (cpt == CompileType::eval) {
			if (usg)
				ILBuilder::eval_const_u32(ctx.eval, (uint32_t)d);
			else
				ILBuilder::eval_const_i32(ctx.eval, (int32_t)d);

		}

		ret.t = usg ? ctx.default_types->t_u32 : ctx.default_types->t_i32;
		ret.lvalue = false;
		return true;
	}





	bool Operand::parse_long_number(CompileValue& ret, Cursor& c, CompileContext& ctx, CompileType cpt) {
		bool usg = c.tok == RecognizedToken::UnsignedLongNumber;

		std::string_view ndata;
		if (usg)
			ndata = c.buffer.substr(0, c.buffer.size() - 2);
		else
			ndata = c.buffer.substr(0, c.buffer.size() - 1);

		unsigned long long d = svtoi(ndata);
		c.move();

		if (cpt == CompileType::compile) {
			if (usg)
				ILBuilder::build_const_u64(ctx.block, d);
			else
				ILBuilder::build_const_i64(ctx.block, d);
		}
		else if (cpt == CompileType::eval) {
			if (usg)
				ILBuilder::eval_const_u64(ctx.eval, d);
			else
				ILBuilder::eval_const_i64(ctx.eval, d);

		}

		ret.t = usg ? ctx.default_types->t_u64 : ctx.default_types->t_i64;
		ret.lvalue = false;
		return true;
	}





	bool Operand::parse_float_number(CompileValue& ret, Cursor& c, CompileContext& ctx, CompileType cpt) {
		bool dbl = c.tok == RecognizedToken::DoubleNumber;

		std::string_view ndata;
		if (dbl)
			ndata = c.buffer.substr(0, c.buffer.size() - 1);
		else
			ndata = c.buffer;

		double d = svtod(ndata);
		c.move();

		if (cpt == CompileType::compile) {
			if (dbl)
				ILBuilder::build_const_f64(ctx.block, d);
			else
				ILBuilder::build_const_f32(ctx.block, (float)d);
		}
		else if (cpt == CompileType::eval) {
			if (dbl)
				ILBuilder::eval_const_f64(ctx.eval, d);
			else
				ILBuilder::eval_const_f32(ctx.eval, (float)d);

		}

		ret.t = dbl ? ctx.default_types->t_f64 : ctx.default_types->t_f32;
		ret.lvalue = false;
		return true;
	}



	bool Operand::parse_call_operator(CompileValue& ret, Cursor& c, CompileContext& ctx, CompileType cpt) {
		if (ret.t == ctx.default_types->t_type) {

			Expression::rvalue(ctx, ret, cpt);

			if (cpt != CompileType::compile) {
				TypeInstance* dt = nullptr;

				if (cpt != CompileType::short_circuit) {
					ILCtype ct = ctx.eval->pop_register_value<ILCtype>();
					Type t = { (TypeInstance*)ct.type,ct.ptr };

					dt = t.type;
					if (dt->type != TypeInstanceType::Structure) {
						throw_specific_error(c, "this type is not a generic type");
						return false;
					}
				}

				c.move();
				
				std::vector<std::tuple<Cursor, Type>>::iterator layout;
				
				if (cpt != CompileType::short_circuit)
					layout = ((Structure*)dt->owner_ptr)->generic_layout.begin();

				std::vector<CompileValue> results;
				if (c.tok != RecognizedToken::CloseParenthesis) {
					while (true) {
						CompileValue res;
						Cursor err = c;
						if (!Expression::parse(c, ctx, res, cpt)) return false;
						results.push_back(res);


						if (cpt != CompileType::short_circuit) {
							if (!Operand::cast(err, ctx, res, std::get<1>(*layout), cpt)) return false;
							layout++;
						}

						if (c.tok == RecognizedToken::Comma) {
							c.move();
						}
						else if (c.tok == RecognizedToken::CloseParenthesis) {
							break;
						}
						else {
							throw_wrong_token_error(c, "')' or ','");
							return false;
						}
					}
				}

				c.move();


				if (cpt != CompileType::short_circuit) {
					auto ss = std::move(StackManager::move_stack_out<1>());
					auto sp = ctx.eval->stack_push();

					unsigned int local_i = 0;
					for (size_t arg_i = results.size() - 1; arg_i >= 0 && arg_i < results.size(); arg_i--) {
						CompileValue res = results[arg_i];
						unsigned char* data_place = ctx.eval->stack_reserve(res.t.compile_time_size(ctx.eval));

						bool stacked = res.t.rvalue_stacked();
						if (stacked) {
							res.t.move(ctx, ctx.eval->pop_register_value<void*>(), data_place);
						}
						else {
							res.t.move(ctx, ctx.eval->read_last_register_value_pointer(res.t.type->rvalue), data_place);
							ctx.eval->discard_register_by_type(res.t.type->rvalue);
						}

						StackManager::stack_push<1>(std::get<0>(((Structure*)dt->owner_ptr)->generic_layout[arg_i]).buffer, res, local_i);
						local_i++;
					}

					StructureInstance* inst = nullptr;
					if (!((Structure*)dt->owner_ptr)->generate(ctx, sp, inst)) return false;

					ILCtype ct = { inst->type.get(),0 };
					ILBuilder::eval_const_ctype(ctx.eval, ct);
					
					//DESTRUCTOR: there are values on the stack, they need to be potentionaly released if they hold memory
					//FUTURE: DO NOT CALL DESTRUCTORS IF GENERATE ACTUALLY GENERATED INSTANCE -> there might be allocated memory on the heap that is used to compare later types

					ctx.eval->stack_pop(sp);
					StackManager::move_stack_in<1>(std::move(ss));
				}

				ret.lvalue = false;
				ret.t = ctx.default_types->t_type;
				
			}
			else if (cpt == CompileType::compile) {
				throw_specific_error(c, "types are not alowed in runtime operations");
				return false;
			}

		}
		else {
			throw_specific_error(c, "not implemented yet");
			return false;
		}

		return true;
	}


	bool Operand::parse_array_operator(CompileValue& ret, Cursor& c, CompileContext& ctx, CompileType cpt) {
		/*auto array_type = dynamic_cast<const ArrayType*>(ret.t);
		if (array_type == nullptr) {
			throw_specific_error(c, "Operator requires array type");
			return false;
		}
		c.move();
		CompileValue v;
		if (!Expression::parse(c, ctx,v, cpt)) return false;
		//check for integer type
		if (c.tok != RecognizedToken::CloseBracket) {
			throw_wrong_token_error(c, "']'");
			return false;
		}
		c.move();

		//LLVMValueRef ind[] = { v.v };
		//ret.v = LLVMBuildGEP2(ctx.builder, array_type->base->LLVMType(), ret.v, ind, 1, "");

		ret.t = array_type->base;
		ret.lvalue = true;*/
		return true;
	}





	bool Operand::parse_dot_operator(CompileValue& ret, Cursor& c, CompileContext& ctx, CompileType cpt) {
		/*auto prim_type = dynamic_cast<const PrimitiveType*>(ret.t);
		if (prim_type == nullptr) {
			throw_specific_error(c, "Operator requires primitive type");
			return false;
		}
		c.move();

		StructDeclaration* sd = prim_type->structure;*/

		

		c.move();
		return true;
	}
}