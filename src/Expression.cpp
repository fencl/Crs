#include "Expression.h"
#include "Operand.h"
#include "Error.h"
#include <algorithm>
#include "Declaration.h"
#include "PredefinedTypes.h"
#include <iostream>
#include <llvm/Target.h>

namespace Corrosive {
	bool Expression::rvalue(CompileContext& ctx,CompileValue& value,CompileType cpt) {
		if (value.lvalue) {
			if (cpt == CompileType::compile) {
				value.lvalue = false;
				if (!ILBuilder::build_load(ctx.scope, value.t->rvalue)) return false;
			}
			else if (cpt == CompileType::eval) {
				value.lvalue = false;
				if (!ILBuilder::eval_load(ctx.eval, value.t->rvalue)) return false;
			}
		}

		return true;
	}

	unsigned int _crs_expr_arith_val(CompileValue v) {
		if (v.t->type() != TypeInstanceType::type_instance) return 0;

		switch (v.t->rvalue)
		{
			case ILDataType::ibool: return 1;
			case ILDataType::u8: return 2;
			case ILDataType::i8: return 3;
			case ILDataType::u16: return 4;
			case ILDataType::i16: return 5;
			case ILDataType::u32: return 6;
			case ILDataType::i32: return 7;
			case ILDataType::u64: return 8;
			case ILDataType::i64: return 9;
			case ILDataType::f32: return 10;
			case ILDataType::f64: return 11;
			default:
				return 0;
		}
	}
	bool _crs_expr_arith_res(CompileContext& ctx,CompileValue left, CompileValue right, CompileValue& ret) {
		unsigned int lv = _crs_expr_arith_val(left);
		unsigned int rv = _crs_expr_arith_val(right);
		if (lv == 0 || rv == 0) return false;

		unsigned int rsv = std::max(lv, rv); 
		ret.lvalue = false;
		switch (rsv)
		{
			case 1: ret.t = ctx.default_types->t_bool; break;
			case 2: ret.t = ctx.default_types->t_u8; break;
			case 3: ret.t = ctx.default_types->t_i8; break;
			case 4: ret.t = ctx.default_types->t_u16; break;
			case 5: ret.t = ctx.default_types->t_i16; break;
			case 6: ret.t = ctx.default_types->t_u32; break;
			case 7: ret.t = ctx.default_types->t_i32; break;
			case 8: ret.t = ctx.default_types->t_u64; break;
			case 9: ret.t = ctx.default_types->t_i64; break;
			case 10: ret.t = ctx.default_types->t_f32; break;
			case 11: ret.t = ctx.default_types->t_f64; break;
			default:
				break;
		}

		return true;
	}

	

	bool Expression::emit(Cursor& c, CompileContext& ctx, CompileValue& res, int l, int op, CompileValue left, CompileValue right,CompileType cpt,int next_l,int next_op) {
		bool isf = false;
		bool sig = false;

		CompileValue ret;
		if (!_crs_expr_arith_res(ctx,left, right,ret)) {
			throw_specific_error(c, "Operation is not defined on top of specified types");
			std::cerr << " |\tTypes were ";
			left.t->print(std::cerr);
			std::cerr << " and ";
			right.t->print(std::cerr);
			std::cerr << "\n";

			return false;
		}

		ret.lvalue = false;
		if (l == 1 || l == 2)
			ret.t = ctx.default_types->t_bool;

		if(cpt == CompileType::compile) {
			switch (l) {
				case 0: {
						switch (op) {
							case 0: {
									if (!ILBuilder::build_and(ctx.scope, left.t->rvalue, right.t->rvalue)) return false;
								}break;
							case 1: {
									if (!ILBuilder::build_or(ctx.scope, left.t->rvalue, right.t->rvalue)) return false;
								}break;
							case 2: {
									if (!ILBuilder::build_xor(ctx.scope, left.t->rvalue, right.t->rvalue)) return false;
								}break;
						}
					}break;
				case 1: {
						switch (op) {
							case 0: {
									if (!ILBuilder::build_eq(ctx.scope, left.t->rvalue, right.t->rvalue)) return false;
								}break;
							case 1: {
									if (!ILBuilder::build_ne(ctx.scope, left.t->rvalue, right.t->rvalue)) return false;
								}break;
						}
					}break;
				case 2: {
						switch (op) {
							case 0: {
									if (!ILBuilder::build_gt(ctx.scope, left.t->rvalue, right.t->rvalue)) return false;
								}break;
							case 1: {
									if (!ILBuilder::build_lt(ctx.scope, left.t->rvalue, right.t->rvalue)) return false;
								} break;
							case 2: {
									if (!ILBuilder::build_ge(ctx.scope, left.t->rvalue, right.t->rvalue)) return false;
								}break;
							case 3: {
									if (!ILBuilder::build_le(ctx.scope, left.t->rvalue, right.t->rvalue)) return false;
								}break;
						}
					}break;
				case 3: {
						switch (op) {
							case 0: {
									if (!ILBuilder::build_add(ctx.scope, left.t->rvalue, right.t->rvalue)) return false;
								}break;
							case 1: {
									if (!ILBuilder::build_sub(ctx.scope, left.t->rvalue, right.t->rvalue)) return false;
								}break;
						}
					}break;
				case 4: {
						switch (op) {
							case 0: {
									if (!ILBuilder::build_mul(ctx.scope, left.t->rvalue, right.t->rvalue)) return false;
								}break;
							case 1: {
									if (!ILBuilder::build_div(ctx.scope, left.t->rvalue, right.t->rvalue)) return false;
								} break;
							case 2: {
									if (!ILBuilder::build_rem(ctx.scope, left.t->rvalue, right.t->rvalue)) return false;
								}break;
						}
					}break;
			}
		}
		else if (cpt == CompileType::eval) {
			switch (l) {
				case 0: {
						switch (op) {
							case 0: {
									if (!ILBuilder::eval_and(ctx.eval,left.t->rvalue, right.t->rvalue)) return false;
								}break;
							case 1: {
									if (!ILBuilder::eval_or(ctx.eval, left.t->rvalue, right.t->rvalue)) return false;
								}break;
							case 2: {
									if (!ILBuilder::eval_xor(ctx.eval, left.t->rvalue, right.t->rvalue)) return false;
								}break;
						}
					}break;
				case 1: {
						switch (op) {
							case 0: {
									if (!ILBuilder::eval_eq(ctx.eval, left.t->rvalue, right.t->rvalue)) return false;
								}break;
							case 1: {
									if (!ILBuilder::eval_ne(ctx.eval, left.t->rvalue, right.t->rvalue)) return false;
								}break;
						}
					}break;
				case 2: {
						switch (op) {
							case 0: {
									if (!ILBuilder::eval_gt(ctx.eval, left.t->rvalue, right.t->rvalue)) return false;
								}break;
							case 1: {
									if (!ILBuilder::eval_lt(ctx.eval, left.t->rvalue, right.t->rvalue)) return false;
								} break;
							case 2: {
									if (!ILBuilder::eval_ge(ctx.eval, left.t->rvalue, right.t->rvalue)) return false;
								}break;
							case 3: {
									if (!ILBuilder::eval_le(ctx.eval, left.t->rvalue, right.t->rvalue)) return false;
								}break;
						}
					}break;
				case 3: {
						switch (op) {
							case 0: {
									if (!ILBuilder::eval_add(ctx.eval, left.t->rvalue, right.t->rvalue)) return false;
								}break;
							case 1: {
									if (!ILBuilder::eval_sub(ctx.eval, left.t->rvalue, right.t->rvalue)) return false;
								}break;
						}
					}break;
				case 4: {
						switch (op) {
							case 0: {
									if (!ILBuilder::eval_mul(ctx.eval, left.t->rvalue, right.t->rvalue)) return false;
								}break;
							case 1: {
									if (!ILBuilder::eval_div(ctx.eval, left.t->rvalue, right.t->rvalue)) return false;
								} break;
							case 2: {
									if (!ILBuilder::eval_rem(ctx.eval, left.t->rvalue, right.t->rvalue)) return false;
								}break;
						}
					}break;
			}
		}

		res = ret;
		return true;
	}

	bool Expression::parse(Cursor& c, CompileContext& ctx,CompileValue& res, CompileType cpt) {
		if (!parse_or(c, ctx, res, cpt)) return false;
		return true;
	}

	bool Expression::parse_and(Cursor& c, CompileContext& ctx, CompileValue& res, CompileType cpt) {

		ILBlock* fallback = nullptr;
		CompileValue value;
		if (!Expression::parse_operators(c, ctx, value, cpt)) return false;
		

		while (c.tok == RecognizedToken::DoubleAnd) {
			if (value.t != ctx.default_types->t_bool) {
				throw_specific_error(c, "Operation requires left operand to be boolean");
				return false;
			}

			c.move();

			if (cpt == CompileType::eval) {
				uint8_t v = ctx.eval->read_register_value<uint8_t>();
				
				ctx.eval->pop_register_value<uint8_t>();
				CompileValue right;
				if (!Expression::parse_operators(c, ctx, right, cpt)) return false;

				if (right.t != ctx.default_types->t_bool) {
					throw_specific_error(c, "Operation requires right operand to be boolean");
					return false;
				}
				uint8_t rv = ctx.eval->pop_register_value<uint8_t>();
				ILBuilder::eval_const_ibool(ctx.eval, v & rv);
			}
			else {
				if (!fallback) {
					fallback = ctx.function->func->create_block(ILDataType::ibool);
				}

				if (!rvalue(ctx, value, cpt)) return false;

				ILBlock* positive_block = ctx.function->func->create_block(ILDataType::ibool);
				ctx.function->func->append_block(positive_block);

				if (!ILBuilder::build_discard(positive_block,ILDataType::ibool)) return false;

				ILBuilder::build_const_ibool(ctx.scope, false);
				if (!ILBuilder::build_yield(ctx.scope,ILDataType::ibool)) return false;

				if (!ILBuilder::build_jmpz(ctx.scope, fallback, positive_block)) return false;
				ctx.scope = positive_block;

				if (!Expression::parse_operators(c, ctx, value, cpt)) return false;
			}
			
		}

		if (fallback != nullptr && cpt == CompileType::compile) {
			if (value.t != ctx.default_types->t_bool) {
				throw_specific_error(c, "Operation requires right operand to be boolean");
				return false;
			}

			if (!rvalue(ctx, value, cpt)) return false;
			if (!ILBuilder::build_yield(ctx.scope,ILDataType::ibool)) return false;
			if (!ILBuilder::build_jmp(ctx.scope, fallback)) return false;
			ctx.function->func->append_block(fallback);

			ctx.scope = fallback;
			fallback = nullptr;

			if (!ILBuilder::build_accept(ctx.scope,ILDataType::ibool)) return false;
			value.t = ctx.default_types->t_bool;
			value.lvalue = false;
		}

		res = value;
		return true;
	}



	bool Expression::parse_or(Cursor& c, CompileContext& ctx, CompileValue& res, CompileType cpt) {

		ILBlock* fallback = nullptr;

		CompileValue value;
		if (!Expression::parse_and(c, ctx, value, cpt)) return false;

		while (c.tok == RecognizedToken::DoubleOr) {
			if (value.t != ctx.default_types->t_bool) {
				throw_specific_error(c, "Operation requires left operand to be boolean");
				return false;
			}

			c.move();

			
			if (cpt == CompileType::eval) {
				uint8_t v = ctx.eval->read_register_value<uint8_t>();
				
				ctx.eval->pop_register_value<uint8_t>();
				CompileValue right;
				if (!Expression::parse_and(c, ctx, right, cpt)) return false;

				if (right.t != ctx.default_types->t_bool) {
					throw_specific_error(c, "Operation requires right operand to be boolean");
					return false;
				}
				uint8_t rv = ctx.eval->pop_register_value<uint8_t>();
				ILBuilder::eval_const_ibool(ctx.eval, v | rv);
			}
			else {
				if (!fallback) {
					fallback = ctx.function->func->create_block(ILDataType::ibool);
				}

				if (!rvalue(ctx, value, cpt)) return false;

				ILBlock* positive_block = ctx.function->func->create_block(ILDataType::ibool);
				ctx.function->func->append_block(positive_block);

				if (!ILBuilder::build_discard(positive_block, ILDataType::ibool)) return false;

				ILBuilder::build_const_ibool(ctx.scope, true);
				if (!ILBuilder::build_yield(ctx.scope,ILDataType::ibool)) return false;

				if (!ILBuilder::build_jmpz(ctx.scope, positive_block, fallback)) return false;
				ctx.scope = positive_block;

				if (!Expression::parse_and(c, ctx,value, cpt)) return false;
			}
			
		}

		if (fallback != nullptr && cpt == CompileType::compile) {
			if (value.t != ctx.default_types->t_bool) {
				throw_specific_error(c, "Operation requires right operand to be boolean");
				return false;
			}

			if (!rvalue(ctx, value, cpt)) return false;

			if (!ILBuilder::build_yield(ctx.scope,ILDataType::ibool)) return false;
			if (!ILBuilder::build_jmp(ctx.scope, fallback)) return false;
			ctx.function->func->append_block(fallback);

			ctx.scope = fallback;
			fallback = nullptr;

			if (!ILBuilder::build_accept(ctx.scope, ILDataType::ibool)) return false;

			value.t = ctx.default_types->t_bool;
			value.lvalue = false;
		}

		res = value;
		return true;
	}

	bool Expression::parse_operators(Cursor& c, CompileContext& ctx, CompileValue& res, CompileType cpt) {

		int op_type[5] = { -1 };
		Cursor op_cursors[5];
		CompileValue layer[5];
		memset(layer, 0, sizeof(layer));

		int current_layer = -1;

		while (true) {
			CompileValue value;
			if (!Operand::parse(c, ctx, value, cpt)) return false;
			int op_v = -1;
			int op_t = -1;

			Cursor opc = c;

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

					if (!emit(op_cursors[i], ctx, value, i, op_type[i], left, right, cpt, op_v, op_t)) return false;
					layer[i].t = nullptr;
				}
			}
			
			

			if (op_v >= 0) {

				layer[op_v] = value;
				op_type[op_v] = op_t;
				op_cursors[op_v] = opc;
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