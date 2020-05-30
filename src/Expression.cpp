#include "Expression.h"
#include "Operand.h"
#include "Error.h"
#include <algorithm>
#include "Declaration.h"
#include "PredefinedTypes.h"
#include <iostream>
#include <llvm/Target.h>
#include "StackManager.h"

namespace Corrosive {
	bool Expression::rvalue(CompileValue& value, CompileType cpt) {
		if (value.lvalue && !value.t->rvalue_stacked()) {
			CompileContext& nctx = CompileContext::get();

			if (cpt == CompileType::compile) {
				value.lvalue = false;
				if (!ILBuilder::build_load(nctx.scope, value.t->rvalue())) return false;
			}
			else if (cpt == CompileType::eval) {
				value.lvalue = false;
				if (!ILBuilder::eval_load(nctx.eval, value.t->rvalue())) return false;
			}
		}

		return true;
	}

	ILDataType _crs_expr_arith_val(CompileValue v) {
		if (v.t->type() != TypeInstanceType::type_structure_instance) {
			return ILDataType::none;
		}

		TypeStructureInstance* si = (TypeStructureInstance*)v.t;
		if (si->owner->structure_type != StructureInstanceType::primitive_structure) {
			return ILDataType::none;
		}

		return v.t->rvalue();
	}

	bool _crs_expr_arith_res(CompileValue left, CompileValue right, CompileValue& ret) {
		ILDataType lv = _crs_expr_arith_val(left);
		ILDataType rv = _crs_expr_arith_val(right);
		if (lv == ILDataType::none || rv == ILDataType::none) return false;


		CompileContext& nctx = CompileContext::get();
		ILDataType rsv = ILBuilder::arith_result(lv, rv);
		ret.lvalue = false;
		ret.t = nctx.default_types->get_type_from_rvalue(rsv);
		return true;
	}



	bool Expression::emit(Cursor& c, CompileValue& res, int l, int op, CompileValue left, CompileValue right, CompileType cpt, int next_l, int next_op) {
		bool isf = false;
		bool sig = false;

		CompileContext& nctx = CompileContext::get();

		CompileValue ret;
		if (!_crs_expr_arith_res(left, right, ret)) {
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
			ret.t = nctx.default_types->t_bool;

		if (cpt == CompileType::compile) {
			switch (l) {
				case 0: {
					switch (op) {
						case 0: {
							if (!ILBuilder::build_and(nctx.scope, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 1: {
							if (!ILBuilder::build_or(nctx.scope, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 2: {
							if (!ILBuilder::build_xor(nctx.scope, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
					}
				}break;
				case 1: {
					switch (op) {
						case 0: {
							if (!ILBuilder::build_eq(nctx.scope, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 1: {
							if (!ILBuilder::build_ne(nctx.scope, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
					}
				}break;
				case 2: {
					switch (op) {
						case 0: {
							if (!ILBuilder::build_gt(nctx.scope, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 1: {
							if (!ILBuilder::build_lt(nctx.scope, left.t->rvalue(), right.t->rvalue())) return false;
						} break;
						case 2: {
							if (!ILBuilder::build_ge(nctx.scope, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 3: {
							if (!ILBuilder::build_le(nctx.scope, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
					}
				}break;
				case 3: {
					switch (op) {
						case 0: {
							if (!ILBuilder::build_add(nctx.scope, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 1: {
							if (!ILBuilder::build_sub(nctx.scope, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
					}
				}break;
				case 4: {
					switch (op) {
						case 0: {
							if (!ILBuilder::build_mul(nctx.scope, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 1: {
							if (!ILBuilder::build_div(nctx.scope, left.t->rvalue(), right.t->rvalue())) return false;
						} break;
						case 2: {
							if (!ILBuilder::build_rem(nctx.scope, left.t->rvalue(), right.t->rvalue())) return false;
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
							if (!ILBuilder::eval_and(nctx.eval, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 1: {
							if (!ILBuilder::eval_or(nctx.eval, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 2: {
							if (!ILBuilder::eval_xor(nctx.eval, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
					}
				}break;
				case 1: {
					switch (op) {
						case 0: {
							if (!ILBuilder::eval_eq(nctx.eval, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 1: {
							if (!ILBuilder::eval_ne(nctx.eval, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
					}
				}break;
				case 2: {
					switch (op) {
						case 0: {
							if (!ILBuilder::eval_gt(nctx.eval, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 1: {
							if (!ILBuilder::eval_lt(nctx.eval, left.t->rvalue(), right.t->rvalue())) return false;
						} break;
						case 2: {
							if (!ILBuilder::eval_ge(nctx.eval, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 3: {
							if (!ILBuilder::eval_le(nctx.eval, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
					}
				}break;
				case 3: {
					switch (op) {
						case 0: {
							if (!ILBuilder::eval_add(nctx.eval, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 1: {
							if (!ILBuilder::eval_sub(nctx.eval, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
					}
				}break;
				case 4: {
					switch (op) {
						case 0: {
							if (!ILBuilder::eval_mul(nctx.eval, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 1: {
							if (!ILBuilder::eval_div(nctx.eval, left.t->rvalue(), right.t->rvalue())) return false;
						} break;
						case 2: {
							if (!ILBuilder::eval_rem(nctx.eval, left.t->rvalue(), right.t->rvalue())) return false;
						}break;
					}
				}break;
			}
		}

		res = ret;
		return true;
	}

	bool Expression::parse(Cursor& c, CompileValue& res, CompileType cpt) {

		CompileContext& nctx = CompileContext::get();
		CompileValue val;
		if (!parse_or(c, val, cpt)) return false;

		if (c.tok == RecognizedToken::Equals) {
			CompileValue val2;
			if (!val.lvalue) {
				throw_specific_error(c, "Left assignment must be modifiable lvalue");
				return false;
			}

			if (cpt == CompileType::compile) {
				ILBuilder::build_duplicate(nctx.scope, ILDataType::ptr);
			}
			else {
				ILBuilder::eval_duplicate(nctx.eval, ILDataType::ptr);
			}

			c.move();
			Cursor err = c;

			if (!Expression::parse(c, val2, cpt)) return false;
			
			if (!Operand::cast(err, val2, val.t, cpt, true)) return false;

			if (cpt == CompileType::compile) {

				if (val.t->has_special_copy()) {
					val.t->build_copy(!val.lvalue);
				}
				else {
					// simple copy
					if (!Expression::rvalue(val2, CompileType::compile)) return false;

					if (val.t->rvalue_stacked()) {
						ILBuilder::build_memcpy2(nctx.scope, val.t->size());
					}
					else {
						ILBuilder::build_store2(nctx.scope, val.t->rvalue());
					}
				}
			}
			else {

				if (val.t->has_special_copy()) {

					if (!val.lvalue && !val.t->rvalue_stacked()) {
						ilsize_t storage;
						size_t reg_size = nctx.eval->compile_time_register_size(val.t->rvalue());
						nctx.eval->pop_register_value_indirect(reg_size, &storage);
						unsigned char* me = nctx.eval->pop_register_value<unsigned char*>();
						val.t->copy(nctx.eval, me, (unsigned char*)&storage);
					}
					else {
						unsigned char* copy_from = nctx.eval->pop_register_value<unsigned char*>();
						unsigned char* me = nctx.eval->pop_register_value<unsigned char*>();
						val.t->copy(nctx.eval, me, copy_from);
					}
				}
				else {
					//simple copy
					if (!Expression::rvalue(val2, CompileType::eval)) return false;

					if (val.t->rvalue_stacked()) {
						ILBuilder::eval_memcpy2(nctx.eval, val.t->size().eval(compiler_arch));
					}
					else {
						ILBuilder::eval_store2(nctx.eval, val.t->rvalue());
					}
				}
			}
		}

		//if (!Expression::rvalue(val, cpt)) return false;
		res = val;
		return true;
	}

	bool Expression::parse_and(Cursor& c, CompileValue& res, CompileType cpt) {

		ILBlock* fallback = nullptr;
		CompileValue value;
		if (!Expression::parse_operators(c, value, cpt)) return false;


		CompileContext& nctx = CompileContext::get();

		while (c.tok == RecognizedToken::DoubleAnd) {
			if (value.t != nctx.default_types->t_bool) {
				throw_specific_error(c, "Operation requires left operand to be boolean");
				return false;
			}

			if (!Expression::rvalue(value, cpt)) return false;

			c.move();

			if (cpt == CompileType::eval) {
				uint8_t v = nctx.eval->read_register_value<uint8_t>();

				nctx.eval->pop_register_value<uint8_t>();
				CompileValue right;
				if (!Expression::parse_operators(c, right, cpt)) return false;
				if (!Expression::rvalue(right, cpt)) return false;

				if (right.t != nctx.default_types->t_bool) {
					throw_specific_error(c, "Operation requires right operand to be boolean");
					return false;
				}
				uint8_t rv = nctx.eval->pop_register_value<uint8_t>();
				ILBuilder::eval_const_ibool(nctx.eval, v & rv);
			}
			else {
				if (!fallback) {
					fallback = nctx.function->create_block(ILDataType::ibool);
				}

				if (!rvalue(value, cpt)) return false;

				ILBlock* positive_block = nctx.function->create_block(ILDataType::ibool);
				nctx.function->append_block(positive_block);

				if (!ILBuilder::build_discard(positive_block, ILDataType::ibool)) return false;

				ILBuilder::build_const_ibool(nctx.scope, false);
				if (!ILBuilder::build_yield(nctx.scope, ILDataType::ibool)) return false;

				if (!ILBuilder::build_jmpz(nctx.scope, fallback, positive_block)) return false;

				nctx.scope = positive_block;
				if (!Expression::parse_operators(c, value, cpt)) return false;
				if (!Expression::rvalue(value, cpt)) return false;

			}

		}

		if (fallback != nullptr && cpt == CompileType::compile) {
			if (value.t != nctx.default_types->t_bool) {
				throw_specific_error(c, "Operation requires right operand to be boolean");
				return false;
			}

			if (!rvalue(value, cpt)) return false;
			if (!ILBuilder::build_yield(nctx.scope, ILDataType::ibool)) return false;
			if (!ILBuilder::build_jmp(nctx.scope, fallback)) return false;

			nctx.function->append_block(fallback);
			nctx.scope = fallback;
			fallback = nullptr;

			if (!ILBuilder::build_accept(nctx.scope, ILDataType::ibool)) return false;
			value.t = nctx.default_types->t_bool;
			value.lvalue = false;
		}

		res = value;
		return true;
	}



	bool Expression::parse_or(Cursor& c, CompileValue& res, CompileType cpt) {

		ILBlock* fallback = nullptr;

		CompileValue value;
		if (!Expression::parse_and(c, value, cpt)) return false;

		CompileContext& nctx = CompileContext::get();

		while (c.tok == RecognizedToken::DoubleOr) {
			if (value.t != nctx.default_types->t_bool) {
				throw_specific_error(c, "Operation requires left operand to be boolean");
				return false;
			}

			if (!Expression::rvalue(value, cpt)) return false;

			c.move();


			if (cpt == CompileType::eval) {
				uint8_t v = nctx.eval->read_register_value<uint8_t>();

				nctx.eval->pop_register_value<uint8_t>();
				CompileValue right;
				if (!Expression::parse_and(c, right, cpt)) return false;
				if (!Expression::rvalue(right,cpt)) return false;

				if (right.t != nctx.default_types->t_bool) {
					throw_specific_error(c, "Operation requires right operand to be boolean");
					return false;
				}
				uint8_t rv = nctx.eval->pop_register_value<uint8_t>();
				ILBuilder::eval_const_ibool(nctx.eval, v | rv);
			}
			else {
				if (!fallback) {
					fallback = nctx.function->create_block(ILDataType::ibool);
				}

				if (!rvalue(value, cpt)) return false;

				ILBlock* positive_block = nctx.function->create_block(ILDataType::ibool);
				nctx.function->append_block(positive_block);

				if (!ILBuilder::build_discard(positive_block, ILDataType::ibool)) return false;

				ILBuilder::build_const_ibool(nctx.scope, true);
				if (!ILBuilder::build_yield(nctx.scope, ILDataType::ibool)) return false;

				if (!ILBuilder::build_jmpz(nctx.scope, positive_block, fallback)) return false;
				nctx.scope = positive_block;

				if (!Expression::parse_and(c, value, cpt)) return false;
				if (!Expression::rvalue(value, cpt)) return false;
			}

		}

		if (fallback != nullptr && cpt == CompileType::compile) {
			if (value.t != nctx.default_types->t_bool) {
				throw_specific_error(c, "Operation requires right operand to be boolean");
				return false;
			}

			if (!rvalue(value, cpt)) return false;

			if (!ILBuilder::build_yield(nctx.scope, ILDataType::ibool)) return false;
			if (!ILBuilder::build_jmp(nctx.scope, fallback)) return false;

			nctx.function->append_block(fallback);
			nctx.scope = fallback;

			fallback = nullptr;

			if (!ILBuilder::build_accept(nctx.scope, ILDataType::ibool)) return false;

			value.t = nctx.default_types->t_bool;
			value.lvalue = false;
		}

		res = value;
		return true;
	}

	bool Expression::parse_operators(Cursor& c, CompileValue& res, CompileType cpt) {

		int op_type[5] = { -1 };
		Cursor op_cursors[5];
		CompileValue layer[5];
		memset(layer, 0, sizeof(layer));

		int current_layer = -1;

		while (true) {
			CompileValue value;
			if (!Operand::parse(c, value, cpt)) return false;
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

			if (op_v >= 0 || current_layer>=0) {
				if (!Expression::rvalue(value, cpt)) return false;
			}

			for (int i = current_layer; i >= std::max(op_v, 0); i--) {

				if (i >= 0 && layer[i].t != nullptr) {
					CompileValue& left = layer[i];
					CompileValue& right = value;

					if (!Expression::emit(op_cursors[i], value, i, op_type[i], left, right, cpt, op_v, op_t)) return false;
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
				res = value;
				return true;
			}
		}


		return true;
	}
}