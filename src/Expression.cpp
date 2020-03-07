#include "Expression.h"
#include "Operand.h"
#include "Error.h"
#include <algorithm>
#include "Declaration.h"
#include "PredefinedTypes.h"
#include <iostream>
#include <llvm/Target.h>

namespace Corrosive {
	void Expression::rvalue(CompileContextExt& ctx,CompileValue& value,CompileType cpt) {
		if (value.lvalue) {
			if (cpt == CompileType::compile) {
				value.lvalue = false;
				ILBuilder::build_load(ctx.block, value.t->iltype->rvalue);
			}
			else if (cpt == CompileType::Eval) {
				
			}
		}
	}

	void Expression::arith_promote(CompileValue& value,int from, int to) {
		if (to == 10) {
			value.t = t_f64;
		}
		else if (to == 9) {
			value.t = t_f32;
		}
		else if (to == 8 || to == 7) {
			value.t = (to == 8 ? t_i64 : t_u64);
		}
		else if (to == 6 || to == 5) {
			value.t = (to == 6 ? t_i32 : t_u32);
		}
		else if (to == 4 || to == 3) {
			value.t = (to == 4 ? t_i16 : t_u16);
		}
		else if (to == 2 || to == 1) {
			value.t = (to == 2 ? t_i8 : t_u8);
		}
	}


	int Expression::arith_value(CompileContextExt& ctx, const PrimitiveType* pt) {
		pt->compile(ctx.basic);
		StructDeclarationType sdt = pt->structure->decl_type;

		switch (sdt)
		{
		case Corrosive::StructDeclarationType::t_bool:
			return 0;
		case Corrosive::StructDeclarationType::t_u8:
			return 1;
		case Corrosive::StructDeclarationType::t_u16:
			return 3;
		case Corrosive::StructDeclarationType::t_u32:
			return 5;
		case Corrosive::StructDeclarationType::t_u64:
			return 7;
		case Corrosive::StructDeclarationType::t_i8:
			return 2;
		case Corrosive::StructDeclarationType::t_i16:
			return 4;
		case Corrosive::StructDeclarationType::t_i32:
			return 6;
		case Corrosive::StructDeclarationType::t_i64:
			return 8;
		case Corrosive::StructDeclarationType::t_f32:
			return 9;
		case Corrosive::StructDeclarationType::t_f64:
			return 10;
		default:
			return 11;
			break;
		}
	}

	bool Expression::arith_cast(CompileContextExt& ctx, CompileValue& left, CompileValue& right, bool& isfloat,bool& issigned) {
		auto ltp = dynamic_cast<const PrimitiveType*>(left.t);
		auto rtp = dynamic_cast<const PrimitiveType*>(right.t);

		if (ltp == nullptr || rtp == nullptr) {
			return false;
		}
		else {
			int arith_value_left = arith_value(ctx,ltp);
			int arith_value_right = arith_value(ctx,rtp);

			int arith_value_res = std::max(arith_value_left, arith_value_right);
			if (arith_value_res == 11) return false;

			if (arith_value_left == arith_value_right) {

			}
			else if (arith_value_left > arith_value_right) {
				arith_promote(right, arith_value_right, arith_value_left);
			}
			else {
				arith_promote(left, arith_value_left, arith_value_right);
			}

			issigned = (arith_value_res % 2 == 0);
			isfloat = (arith_value_res >= 9);

			return true;
		}
	}

	CompileValue Expression::emit(Cursor& c, CompileContextExt& ctx, int l, int op, CompileValue left, CompileValue right,CompileType cpt,int next_l,int next_op) {
		bool isf = false;
		bool sig = false;

		if (!arith_cast(ctx,left, right, isf, sig)) {
			throw_specific_error(c, "Types of operands cannot be used in this operation");
		}

		CompileValue ret = left;

		ret.lvalue = false;
		if (l == 1 || l == 2)
			ret.t = t_bool;

		if(cpt != CompileType::ShortCircuit) {

			if (l == 0) {
				if (op == 0) {
					ILBuilder::build_and(ctx.block);
				}
				else if (op == 1) {
					ILBuilder::build_or(ctx.block);
				}
				else if (op == 2) {
					ILBuilder::build_xor(ctx.block);
				}
			}
			else if (l == 1) {
				if (op == 0)
					ILBuilder::build_eq(ctx.block);
				else if (op == 1)
					ILBuilder::build_ne(ctx.block);
			}
			else if (l == 2) {
				if (op == 0)
					ILBuilder::build_gt(ctx.block);
				else if (op == 1)
					ILBuilder::build_lt(ctx.block);
				else if (op == 2)
					ILBuilder::build_ge(ctx.block);
				else if (op == 3)
					ILBuilder::build_le(ctx.block);
			}
			else if (l == 3) {
				if (op == 0)
					ILBuilder::build_add(ctx.block);
				else if (op == 1)
					ILBuilder::build_sub(ctx.block);
			}
			else if (l == 4) {
				if (op == 0)
					ILBuilder::build_mul(ctx.block);
				else if (op == 1)
					ILBuilder::build_div(ctx.block);
				else if (op == 2)
					ILBuilder::build_rem(ctx.block);
			}
		}

		return ret;
	}

	CompileValue Expression::parse(Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		CompileValue ret = parse_or(c, ctx, cpt);
		return ret;
	}

	CompileValue Expression::parse_and(Cursor& c, CompileContextExt& ctx, CompileType cpt) {

		ILBlock* fallback = nullptr;
		CompileValue value = Expression::parse_operators(c, ctx, cpt);
		

		while (c.tok == RecognizedToken::DoubleAnd) {
			if (value.t != t_bool) {
				throw_specific_error(c, "Operation requires left operand to be boolean");
			}

			c.move();

			if (cpt == CompileType::ShortCircuit) {
				parse_operators(c, ctx, CompileType::ShortCircuit);
			}
			else {
				if (cpt == CompileType::Eval) {
					CompileValue right = Expression::parse_operators(c, ctx, cpt);
				}
				else {
					if (!fallback) {
						fallback = ctx.function->create_block(ILDataType::ibool);
					}

					rvalue(ctx, value, cpt);
					ILBlock* positive_block = ctx.function->create_block(ILDataType::ibool);
					ctx.function->append_block(positive_block);

					ILBuilder::build_discard(positive_block);

					ILBuilder::build_const_ibool(ctx.block, false);
					ILBuilder::build_yield(ctx.block);
					ILBuilder::build_jmpz(ctx.block, fallback, positive_block);
					ctx.block = positive_block;

					value = Expression::parse_operators(c, ctx, cpt);
				}
			}
		}

		if (fallback != nullptr && cpt == CompileType::compile) {
			if (value.t != t_bool) {
				throw_specific_error(c, "Operation requires right operand to be boolean");
			}

			rvalue(ctx, value, cpt);
			ILBuilder::build_yield(ctx.block);
			ILBuilder::build_jmp(ctx.block, fallback);
			ctx.function->append_block(fallback);

			ctx.block = fallback;
			fallback = nullptr;

			ILBuilder::build_accept(ctx.block);
			value.t = t_bool;
			value.lvalue = false;
		}

		return value;
	}



	CompileValue Expression::parse_or(Cursor& c, CompileContextExt& ctx, CompileType cpt) {

		ILBlock* fallback = nullptr;

		CompileValue value = Expression::parse_and(c, ctx, cpt);

		while (c.tok == RecognizedToken::DoubleOr) {
			if (value.t != t_bool) {
				throw_specific_error(c, "Operation requires left operand to be boolean");
			}

			c.move();

			if (cpt == CompileType::ShortCircuit) {
				parse_and(c, ctx, CompileType::ShortCircuit);
			}
			else {
				
				if (cpt == CompileType::Eval) {
					CompileValue right = Expression::parse_and(c, ctx, cpt);
				}
				else {
					if (!fallback) {
						fallback = ctx.function->create_block(ILDataType::ibool);
					}

					rvalue(ctx, value, cpt);

					ILBlock* positive_block = ctx.function->create_block(ILDataType::ibool);
					ctx.function->append_block(positive_block);

					ILBuilder::build_const_ibool(ctx.block, true);
					ILBuilder::build_yield(ctx.block);

					ILBuilder::build_jmpz(ctx.block, positive_block, fallback);
					ctx.block = positive_block;

					value = Expression::parse_and(c, ctx, cpt);
				}
			}
		}

		if (fallback != nullptr && cpt == CompileType::compile) {
			if (value.t != t_bool) {
				throw_specific_error(c, "Operation requires right operand to be boolean");
			}

			rvalue(ctx, value, cpt);

			ILBuilder::build_yield(ctx.block);
			ILBuilder::build_jmp(ctx.block, fallback);
			ctx.function->append_block(fallback);

			ctx.block = fallback;
			fallback = nullptr;

			ILBuilder::build_accept(ctx.block);

			value.t = t_bool;
			value.lvalue = false;
		}

		return value;
	}

	CompileValue Expression::parse_operators(Cursor& c, CompileContextExt& ctx, CompileType cpt) {

		int op_type[5] = { -1 };
		CompileValue layer[5];
		memset(layer, 0, sizeof(layer));

		int current_layer = -1;

		while (true) {
			CompileValue value = Operand::parse(c, ctx, cpt);
			int op_v = -1;
			int op_t = -1;

			if (c.tok == RecognizedToken::And) {
				op_v = 0;
				op_t = 0;
			}
			else if (c.tok == RecognizedToken::Or) {
				op_v = 0;
				op_t = 1;
			}
			else if(c.tok == RecognizedToken::Xor) {
				op_v = 0;
				op_t = 2;
			}
			else if (c.tok == RecognizedToken::DoubleEquals) {
				op_v = 1;
				op_t = 0;
			}
			else if (c.tok == RecognizedToken::NotEquals) {
				op_v = 1;
				op_t = 1;
			}
			else  if (c.tok == RecognizedToken::GreaterThan) {
				op_v = 2;
				op_t = 0;
			}
			else if (c.tok == RecognizedToken::LessThan) {
				op_v = 2;
				op_t = 1;
			}
			else if(c.tok == RecognizedToken::GreaterOrEqual) {
				op_v = 2;
				op_t = 2;
			}
			else if (c.tok == RecognizedToken::LessOrEqual) {
				op_v = 2;
				op_t = 3;
			}
			else if (c.tok == RecognizedToken::Plus) {
				op_v = 3;
				op_t = 0;
			}
			else if (c.tok == RecognizedToken::Minus) {
				op_v = 3;
				op_t = 1;
			}
			else if (c.tok == RecognizedToken::Star) {
				op_v = 4;
				op_t = 0;
			}
			else if (c.tok == RecognizedToken::Slash) {
				op_v = 4;
				op_t = 1;
			}
			else if (c.tok == RecognizedToken::Percent) {
				op_v = 4;
				op_t = 2;
			}

			rvalue(ctx, value, cpt);
			
			for (int i = current_layer; i >= std::max(op_v, 0); i--) {

				if (i>=0 && layer[i].t != nullptr) {
					CompileValue& left = layer[i];
					CompileValue& right = value;
					CompileType cpt2 = cpt;

					value = emit(c, ctx, i, op_type[i], left, right, cpt2, op_v, op_t);
					layer[i].t = nullptr;
				}
			}
			
			

			if (op_v >= 0) {

				layer[op_v] = value;
				op_type[op_v] = op_t;
				current_layer = op_v;

				c.move();
			}
			else {
				return value;
			}
		}
	}
}