#include "Expression.h"
#include "Operand.h"
#include "Error.h"
#include <algorithm>
#include "Declaration.h"
#include "PredefinedTypes.h"
#include <iostream>
#include <llvm/Target.h>

namespace Corrosive {
	bool Expression::rvalue(CompileContextExt& ctx,CompileValue& value,CompileType cpt) {
		if (value.lvalue) {
			if (cpt == CompileType::compile) {
				value.lvalue = false;
				if (!ILBuilder::build_load(ctx.block, value.t->iltype->rvalue)) return false;
			}
			else if (cpt == CompileType::Eval) {
				
			}
		}

		return true;
	}

	void Expression::arith_promote(CompileValue& value,int from, int to) {
		switch (to) {
			case 0: value.t = t_bool; break;
			case 1: value.t = t_u8; break;
			case 2: value.t = t_i8; break;
			case 3: value.t = t_u16; break;
			case 4: value.t = t_i16; break;
			case 5: value.t = t_u32; break;
			case 6: value.t = t_i32; break;
			case 7: value.t = t_u64; break;
			case 8: value.t = t_i64; break;
			case 9: value.t = t_f32; break;
			case 10: value.t = t_f64; break;
		}
	}


	bool Expression::arith_value(CompileContextExt& ctx, const PrimitiveType* pt, int& res) {
		if (!pt->compile(ctx.basic)) return false;
		StructDeclarationType sdt = pt->structure->decl_type;

		switch (sdt)
		{
		case Corrosive::StructDeclarationType::t_bool:
			res = 0; return true;
		case Corrosive::StructDeclarationType::t_u8:
			res = 1; return true;
		case Corrosive::StructDeclarationType::t_u16:
			res = 3; return true;
		case Corrosive::StructDeclarationType::t_u32:
			res = 5; return true;
		case Corrosive::StructDeclarationType::t_u64:
			res = 7; return true;
		case Corrosive::StructDeclarationType::t_i8:
			res = 2; return true;
		case Corrosive::StructDeclarationType::t_i16:
			res = 4; return true;
		case Corrosive::StructDeclarationType::t_i32:
			res = 6; return true;
		case Corrosive::StructDeclarationType::t_i64:
			res = 8; return true;
		case Corrosive::StructDeclarationType::t_f32:
			res = 9; return true;
		case Corrosive::StructDeclarationType::t_f64:
			res = 10; return true;
		default:
			res = 11; return true;
		}
	}

	bool Expression::arith_cast(CompileContextExt& ctx, CompileValue& left, CompileValue& right, bool& isfloat,bool& issigned,bool& res) {
		auto ltp = dynamic_cast<const PrimitiveType*>(left.t);
		auto rtp = dynamic_cast<const PrimitiveType*>(right.t);

		if (ltp == nullptr || rtp == nullptr) {
			res= false;
			return true;
		}
		else {
			int arith_value_left;
			if (!arith_value(ctx, ltp, arith_value_left)) return false;
			int arith_value_right;
			if (!arith_value(ctx, rtp, arith_value_right)) return false;

			int arith_value_res = std::max(arith_value_left, arith_value_right);
			if (arith_value_res == 11) { res = false;  return true; }

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

			res= true;
			return true;
		}
	}

	bool Expression::emit(Cursor& c, CompileContextExt& ctx, CompileValue& res, int l, int op, CompileValue left, CompileValue right,CompileType cpt,int next_l,int next_op) {
		bool isf = false;
		bool sig = false;

		bool ccast;
		if (!arith_cast(ctx, left, right, isf, sig,ccast)) return false;

		if (!ccast) {
			throw_specific_error(c, "Types of operands cannot be used in this operation");
			return false;
		}

		CompileValue ret = left;

		ret.lvalue = false;
		if (l == 1 || l == 2)
			ret.t = t_bool;

		if(cpt != CompileType::ShortCircuit) {
			switch (l) {
				case 0: {
						switch (op) {
							case 0: {
									if (!ILBuilder::build_and(ctx.block)) return false;
								}break;
							case 1: {
									if (!ILBuilder::build_or(ctx.block)) return false;
								}break;
							case 2: {
									if (!ILBuilder::build_xor(ctx.block)) return false;
								}break;
						}
					}break;
				case 1: {
						switch (op) {
							case 0: {
									if (!ILBuilder::build_eq(ctx.block)) return false;
								}break;
							case 1: {
									if (!ILBuilder::build_ne(ctx.block)) return false;
								}break;
						}
					}break;
				case 2: {
						switch (op) {
							case 0: {
									if (!ILBuilder::build_gt(ctx.block)) return false;
								}break;
							case 1: {
									if (!ILBuilder::build_lt(ctx.block)) return false;
								} break;
							case 2: {
									if (!ILBuilder::build_ge(ctx.block)) return false;
								}break;
							case 3: {
									if (!ILBuilder::build_le(ctx.block)) return false;
								}break;
						}
					}break;
				case 3: {
						switch (op) {
							case 0: {
									if (!ILBuilder::build_add(ctx.block)) return false;
								}break;
							case 1: {
									if (!ILBuilder::build_sub(ctx.block)) return false;
								}break;
						}
					}break;
				case 4: {
						switch (op) {
							case 0: {
									if (!ILBuilder::build_mul(ctx.block)) return false;
								}break;
							case 1: {
									if (!ILBuilder::build_div(ctx.block)) return false;
								} break;
							case 2: {
									if (!ILBuilder::build_rem(ctx.block)) return false;
								}break;
						}
					}break;
			}
		}

		res = ret;
		return true;
	}

	bool Expression::parse(Cursor& c, CompileContextExt& ctx,CompileValue& res, CompileType cpt) {
		if (!parse_or(c, ctx, res, cpt)) return false;
		return true;
	}

	bool Expression::parse_and(Cursor& c, CompileContextExt& ctx, CompileValue& res, CompileType cpt) {

		ILBlock* fallback = nullptr;
		CompileValue value;
		if (!Expression::parse_operators(c, ctx,res, cpt)) return false;
		

		while (c.tok == RecognizedToken::DoubleAnd) {
			if (value.t != t_bool) {
				throw_specific_error(c, "Operation requires left operand to be boolean");
				return false;
			}

			c.move();

			if (cpt == CompileType::ShortCircuit) {
				CompileValue tmp;
				if (!parse_operators(c, ctx, tmp, CompileType::ShortCircuit)) return false;
			}
			else {
				if (cpt == CompileType::Eval) {
					CompileValue right;
					if (!Expression::parse_operators(c, ctx, right, cpt)) return false;
				}
				else {
					if (!fallback) {
						fallback = ctx.function->create_block(ILDataType::ibool);
					}

					if (!rvalue(ctx, value, cpt)) return false;
					ILBlock* positive_block = ctx.function->create_block(ILDataType::ibool);
					ctx.function->append_block(positive_block);

					if (!ILBuilder::build_discard(positive_block)) return false;

					ILBuilder::build_const_ibool(ctx.block, false);
					if (!ILBuilder::build_yield(ctx.block)) return false;
					if (!ILBuilder::build_jmpz(ctx.block, fallback, positive_block)) return false;
					ctx.block = positive_block;

					if (!Expression::parse_operators(c, ctx, value, cpt)) return false;
				}
			}
		}

		if (fallback != nullptr && cpt == CompileType::compile) {
			if (value.t != t_bool) {
				throw_specific_error(c, "Operation requires right operand to be boolean");
				return false;
			}

			if (!rvalue(ctx, value, cpt)) return false;
			if (!ILBuilder::build_yield(ctx.block)) return false;
			if (!ILBuilder::build_jmp(ctx.block, fallback)) return false;
			ctx.function->append_block(fallback);

			ctx.block = fallback;
			fallback = nullptr;

			if (!ILBuilder::build_accept(ctx.block)) return false;
			value.t = t_bool;
			value.lvalue = false;
		}

		res = value;
		return true;
	}



	bool Expression::parse_or(Cursor& c, CompileContextExt& ctx, CompileValue& res, CompileType cpt) {

		ILBlock* fallback = nullptr;

		CompileValue value;
		if (!Expression::parse_and(c, ctx, value, cpt)) return false;

		while (c.tok == RecognizedToken::DoubleOr) {
			if (value.t != t_bool) {
				throw_specific_error(c, "Operation requires left operand to be boolean");
				return false;
			}

			c.move();

			if (cpt == CompileType::ShortCircuit) {
				CompileValue tmp;
				if (!parse_and(c, ctx,tmp, CompileType::ShortCircuit)) return false;
			}
			else {
				
				if (cpt == CompileType::Eval) {
					CompileValue right;
					if (!Expression::parse_and(c, ctx,right, cpt)) return false;
				}
				else {
					if (!fallback) {
						fallback = ctx.function->create_block(ILDataType::ibool);
					}

					if (!rvalue(ctx, value, cpt)) return false;

					ILBlock* positive_block = ctx.function->create_block(ILDataType::ibool);
					ctx.function->append_block(positive_block);

					ILBuilder::build_const_ibool(ctx.block, true);
					if (!ILBuilder::build_yield(ctx.block)) return false;

					if (!ILBuilder::build_jmpz(ctx.block, positive_block, fallback)) return false;
					ctx.block = positive_block;

					if (!Expression::parse_and(c, ctx,value, cpt)) return false;
				}
			}
		}

		if (fallback != nullptr && cpt == CompileType::compile) {
			if (value.t != t_bool) {
				throw_specific_error(c, "Operation requires right operand to be boolean");
				return false;
			}

			if (!rvalue(ctx, value, cpt)) return false;

			if (!ILBuilder::build_yield(ctx.block)) return false;
			if (!ILBuilder::build_jmp(ctx.block, fallback)) return false;
			ctx.function->append_block(fallback);

			ctx.block = fallback;
			fallback = nullptr;

			if (!ILBuilder::build_accept(ctx.block)) return false;

			value.t = t_bool;
			value.lvalue = false;
		}

		res = value;
		return true;
	}

	bool Expression::parse_operators(Cursor& c, CompileContextExt& ctx, CompileValue& res, CompileType cpt) {

		int op_type[5] = { -1 };
		CompileValue layer[5];
		memset(layer, 0, sizeof(layer));

		int current_layer = -1;

		while (true) {
			CompileValue value;
			if (!Operand::parse(c, ctx, value,cpt)) return false;
			int op_v = -1;
			int op_t = -1;

			switch (c.tok)
			{

				case RecognizedToken::And: {
						op_v = 0;
						op_t = 0;
					}break;
				case RecognizedToken::Or: {
						op_v = 0;
						op_t = 1;
					} break;
				case RecognizedToken::Xor: {
						op_v = 0;
						op_t = 2;
					}break;
				case RecognizedToken::DoubleEquals: {
						op_v = 1;
						op_t = 0;
					} break;
				case RecognizedToken::NotEquals: {
						op_v = 1;
						op_t = 1;
					}break;
				case RecognizedToken::GreaterThan: {
						op_v = 2;
						op_t = 0;
					}break;
				case RecognizedToken::LessThan: {
						op_v = 2;
						op_t = 1;
					}break;
				case RecognizedToken::GreaterOrEqual: {
						op_v = 2;
						op_t = 2;
					}break;
				case RecognizedToken::LessOrEqual: {
						op_v = 2;
						op_t = 3;
					}break;
				case RecognizedToken::Plus: {
						op_v = 3;
						op_t = 0;
					}break;
				case RecognizedToken::Minus: {
						op_v = 3;
						op_t = 1;
					}break;
				case RecognizedToken::Star: {
						op_v = 4;
						op_t = 0;
					}break;
				case RecognizedToken::Slash: {
						op_v = 4;
						op_t = 1;
					}break;
				case RecognizedToken::Percent: {
						op_v = 4;
						op_t = 2;
					}break;
			}

			if (!rvalue(ctx, value, cpt)) return false;
			
			for (int i = current_layer; i >= std::max(op_v, 0); i--) {

				if (i>=0 && layer[i].t != nullptr) {
					CompileValue& left = layer[i];
					CompileValue& right = value;
					CompileType cpt2 = cpt;

					if (!emit(c, ctx, value, i, op_type[i], left, right, cpt2, op_v, op_t)) return false;
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
				res= value;
				return true;
			}
		}


		return true;
	}
}