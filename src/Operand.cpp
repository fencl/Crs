#include "Operand.h"
#include "Error.h"
#include "Utilities.h"
#include "PredefinedTypes.h"
#include "Expression.h"
#include "StackManager.h"

#include <iostream>

namespace Corrosive {


	bool Operand::parse(Cursor& c, CompileContext& ctx, CompileValue& res, CompileType cpt) {
		CompileValue ret;
		ret.lvalue = false;
		ret.t = Type::null;

		unsigned int type_ref_count = 0;
		while (c.tok == RecognizedToken::Star) {
			type_ref_count++;
			c.move();
		}

		switch (c.tok) {
			case RecognizedToken::OpenParenthesis: {
					if (!parse_expression(ret, c, ctx, cpt)) return false;
				}break;

			case RecognizedToken::Symbol: {
					if (!parse_symbol(ret, c, ctx, cpt, type_ref_count)) return false;
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



	bool Operand::parse_symbol(CompileValue& ret, Cursor& c, CompileContext& ctx, CompileType cpt, unsigned int type_ref_count) {
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
			
			
			if (auto sitm = StackManager::stack_find(c.buffer)) {

				ILBuilder::build_local(ctx.block, sitm->ir_local);
				ret = sitm->value;

				c.move();
			}
			else {
				Namespace* inst = ctx.inside->find_name(c.buffer);

				if (inst == nullptr) {
					throw_specific_error(c, "Path start point not found");
					return false;
				}

				c.move();
				while (c.tok == RecognizedToken::DoubleColon) {
					c.move();
					if (c.tok != RecognizedToken::Symbol)
					{
						throw_not_a_name_error(c);
						return false;
					}
					auto f = inst->subnamespaces.find(c.buffer);
					if (f != inst->subnamespaces.end()) {
						inst = f->second.get();
					}
					else {
						throw_specific_error(c, "Namespace path is not valid");
						return false;
					}
					c.move();
				}

				if (auto struct_inst = dynamic_cast<Structure*>(inst)) {
					if (type_ref_count == 0) {
						if (!struct_inst->compile(ctx)) return false;
					}
					
					if (struct_inst->singe_instance == nullptr) {
						ILCtype ct = { (void*)struct_inst->type.get(),(uint32_t)type_ref_count };
						if (cpt == CompileType::eval) {
							if (!ILBuilder::eval_const_ctype(ctx.eval, ct)) return false;
						}
						else if (cpt == CompileType::compile) {
							if (!ILBuilder::build_const_ctype(ctx.block, ct)) return false;
						}
					}
					else {
						ILCtype ct = { (void*)struct_inst->singe_instance->type.get(),(uint32_t)type_ref_count };

						if (cpt == CompileType::eval) {
							if (!ILBuilder::eval_const_ctype(ctx.eval, ct)) return false;
						}
						else if (cpt == CompileType::compile) {
							if (!ILBuilder::build_const_ctype(ctx.block, ct)) return false;
						}
					}

					ret.lvalue = false;
					ret.t = ctx.default_types->t_type;
				}
				else {
					throw_specific_error(c, "Path is pointing to a namespace");
					return false;
				}
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

			if (cpt == CompileType::eval) {
				ctx.eval->pop_register_type();
				ctx.eval->pop_register_type();
				uint32_t rc = ctx.eval->pop_register_value<uint32_t>();
				AbstractType* at = ctx.eval->pop_register_value<AbstractType*>();

				Type t = { at,rc };

				if (auto dt = dynamic_cast<DirectType*>(at)) {
					
				}
				else {
					throw_specific_error(c, "this type is not a generic type");
					return false;
				}
			}
			else if (cpt == CompileType::compile) {
				throw_specific_error(c, "types are not alowed in runtime operations");
				return false;
			}

		}

		c.move();
		c.move();
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