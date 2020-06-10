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


	bool Expression::copy_from_rvalue(Type* me, CompileType cpt, bool me_top) {
		if (cpt == CompileType::compile) {

			if (me->has_special_copy()) {
				if (me->rvalue_stacked()) {
					if (!me_top) {
						ILBuilder::build_swap(Ctx::scope(), ILDataType::ptr);
					}
					me->build_copy();
				}
				else {
					std::cerr << "Compiler error, structure with special copy should not have rvalue stacked\n";
					return false;
				}
			}
			else {
				if (me_top) {
					if (me->rvalue_stacked()) {
						ILBuilder::build_memcpy2(Ctx::scope(), me->size());
					}
					else {
						ILBuilder::build_store2(Ctx::scope(), me->rvalue());
					}
				}
				else {
					if (me->rvalue_stacked()) {
						ILBuilder::build_memcpy(Ctx::scope(), me->size());
					}
					else {
						ILBuilder::build_store(Ctx::scope(), me->rvalue());
					}
				}
			}
		}
		else {
			if (me->has_special_copy()) {
				if (me->rvalue_stacked()) {
					if (me_top) {
						unsigned char* copy_from = Ctx::eval()->pop_register_value<unsigned char*>();
						unsigned char* copy_to = Ctx::eval()->pop_register_value<unsigned char*>();
						me->copy(copy_to, copy_from);
					}
					else {
						unsigned char* copy_to = Ctx::eval()->pop_register_value<unsigned char*>();
						unsigned char* copy_from = Ctx::eval()->pop_register_value<unsigned char*>();
						me->copy(copy_to, copy_from);
					}
				}
				else {
					std::cerr << "Compiler error, structure with special copy should not have rvalue stacked\n";
					return false;
				}
			}
			else {

				if (me_top) {
					if (me->rvalue_stacked()) {
						ILBuilder::eval_memcpy2(Ctx::eval(), me->size().eval(compiler_arch));
					}
					else {
						ILBuilder::eval_store2(Ctx::eval(), me->rvalue());
					}
				}
				else {
					if (me->rvalue_stacked()) {
						ILBuilder::eval_memcpy(Ctx::eval(), me->size().eval(compiler_arch));
					}
					else {
						ILBuilder::eval_store(Ctx::eval(), me->rvalue());
					}
				}
			}
		}

		return true;
	}

	bool Expression::move_from_rvalue(Type* me, CompileType cpt, bool me_top) {
		if (cpt == CompileType::compile) {

			if (me->has_special_move()) {
				if (me->rvalue_stacked()) {
					if (!me_top) {
						ILBuilder::build_swap(Ctx::scope(), ILDataType::ptr);
					}
					me->build_move();
				}
				else {
					std::cerr << "Compiler error, structure with special copy should not have rvalue stacked\n";
					return false;
				}
			}
			else {
				if (me_top) {
					if (me->rvalue_stacked()) {
						ILBuilder::build_memcpy2(Ctx::scope(), me->size());
					}
					else {
						ILBuilder::build_store2(Ctx::scope(), me->rvalue());
					}
				}
				else {
					if (me->rvalue_stacked()) {
						ILBuilder::build_memcpy(Ctx::scope(), me->size());
					}
					else {
						ILBuilder::build_store(Ctx::scope(), me->rvalue());
					}
				}
			}
		}
		else {
			if (me->has_special_move()) {
				if (me->rvalue_stacked()) {
					if (me_top) {
						unsigned char* copy_from = Ctx::eval()->pop_register_value<unsigned char*>();
						unsigned char* copy_to = Ctx::eval()->pop_register_value<unsigned char*>();
						me->move(copy_to, copy_from);
					}
					else {
						unsigned char* copy_to = Ctx::eval()->pop_register_value<unsigned char*>();
						unsigned char* copy_from = Ctx::eval()->pop_register_value<unsigned char*>();
						me->move(copy_to, copy_from);
					}
				}
				else {
					std::cerr << "Compiler error, structure with special move should not have rvalue stacked\n";
					return false;
				}
			}
			else {

				if (me_top) {
					if (me->rvalue_stacked()) {
						ILBuilder::eval_memcpy2(Ctx::eval(), me->size().eval(compiler_arch));
					}
					else {
						ILBuilder::eval_store2(Ctx::eval(), me->rvalue());
					}
				}
				else {
					if (me->rvalue_stacked()) {
						ILBuilder::eval_memcpy(Ctx::eval(), me->size().eval(compiler_arch));
					}
					else {
						ILBuilder::eval_store(Ctx::eval(), me->rvalue());
					}
				}
			}
		}

		return true;
	}



	bool Expression::rvalue(CompileValue& value, CompileType cpt) {
		if (value.lvalue && !value.t->rvalue_stacked()) {

			if (cpt == CompileType::compile) {
				value.lvalue = false;
				if (!ILBuilder::build_load(Ctx::scope(), value.t->rvalue())) return false;
			}
			else if (cpt == CompileType::eval) {
				value.lvalue = false;
				if (!ILBuilder::eval_load(Ctx::eval(), value.t->rvalue())) return false;
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


		ILDataType rsv = ILBuilder::arith_result(lv, rv);
		ret.lvalue = false;
		ret.t = Ctx::types()->get_type_from_rvalue(rsv);
		return true;
	}



	bool Expression::emit(Cursor& c, CompileValue& res, int l, int op, CompileValue left, CompileValue right, CompileType cpt, int next_l, int next_op) {
		bool isf = false;
		bool sig = false;

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
			ret.t = Ctx::types()->t_bool;

		if (cpt == CompileType::compile) {
			switch (l) {
				case 0: {
					switch (op) {
						case 0: {
							if (!ILBuilder::build_and(Ctx::scope(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 1: {
							if (!ILBuilder::build_or(Ctx::scope(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 2: {
							if (!ILBuilder::build_xor(Ctx::scope(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
					}
				}break;
				case 1: {
					switch (op) {
						case 0: {
							if (!ILBuilder::build_eq(Ctx::scope(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 1: {
							if (!ILBuilder::build_ne(Ctx::scope(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
					}
				}break;
				case 2: {
					switch (op) {
						case 0: {
							if (!ILBuilder::build_gt(Ctx::scope(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 1: {
							if (!ILBuilder::build_lt(Ctx::scope(), left.t->rvalue(), right.t->rvalue())) return false;
						} break;
						case 2: {
							if (!ILBuilder::build_ge(Ctx::scope(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 3: {
							if (!ILBuilder::build_le(Ctx::scope(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
					}
				}break;
				case 3: {
					switch (op) {
						case 0: {
							if (!ILBuilder::build_add(Ctx::scope(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 1: {
							if (!ILBuilder::build_sub(Ctx::scope(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
					}
				}break;
				case 4: {
					switch (op) {
						case 0: {
							if (!ILBuilder::build_mul(Ctx::scope(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 1: {
							if (!ILBuilder::build_div(Ctx::scope(), left.t->rvalue(), right.t->rvalue())) return false;
						} break;
						case 2: {
							if (!ILBuilder::build_rem(Ctx::scope(), left.t->rvalue(), right.t->rvalue())) return false;
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
							if (!ILBuilder::eval_and(Ctx::eval(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 1: {
							if (!ILBuilder::eval_or(Ctx::eval(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 2: {
							if (!ILBuilder::eval_xor(Ctx::eval(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
					}
				}break;
				case 1: {
					switch (op) {
						case 0: {
							if (!ILBuilder::eval_eq(Ctx::eval(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 1: {
							if (!ILBuilder::eval_ne(Ctx::eval(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
					}
				}break;
				case 2: {
					switch (op) {
						case 0: {
							if (!ILBuilder::eval_gt(Ctx::eval(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 1: {
							if (!ILBuilder::eval_lt(Ctx::eval(), left.t->rvalue(), right.t->rvalue())) return false;
						} break;
						case 2: {
							if (!ILBuilder::eval_ge(Ctx::eval(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 3: {
							if (!ILBuilder::eval_le(Ctx::eval(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
					}
				}break;
				case 3: {
					switch (op) {
						case 0: {
							if (!ILBuilder::eval_add(Ctx::eval(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 1: {
							if (!ILBuilder::eval_sub(Ctx::eval(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
					}
				}break;
				case 4: {
					switch (op) {
						case 0: {
							if (!ILBuilder::eval_mul(Ctx::eval(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
						case 1: {
							if (!ILBuilder::eval_div(Ctx::eval(), left.t->rvalue(), right.t->rvalue())) return false;
						} break;
						case 2: {
							if (!ILBuilder::eval_rem(Ctx::eval(), left.t->rvalue(), right.t->rvalue())) return false;
						}break;
					}
				}break;
			}
		}

		res = ret;
		return true;
	}

	bool Expression::parse(Cursor& c, CompileValue& res, CompileType cpt, bool require_output) {
		CompileValue val;
		if (!parse_or(c, val, cpt)) return false;

		if (c.tok == RecognizedToken::Equals || c.tok == RecognizedToken::BackArrow) {
			bool do_copy = c.tok == RecognizedToken::Equals;

			CompileValue val2;
			if (!val.lvalue) {
				throw_specific_error(c, "Left assignment must be modifiable lvalue");
				return false;
			}

			if (require_output) {
				if (cpt == CompileType::compile) {
					ILBuilder::build_duplicate(Ctx::scope(), ILDataType::ptr);
				}
				else {
					ILBuilder::eval_duplicate(Ctx::eval(), ILDataType::ptr);
				}
			}

			c.move();
			Cursor err = c;

			if (!Expression::parse(c, val2, cpt)) return false;
			if (!Operand::cast(err, val2, val.t, cpt, true)) return false;

			if (!val2.lvalue) {
				do_copy = false;
			}

			if (!Expression::rvalue(val2, CompileType::compile)) return false;

			if (do_copy) {
				if (!Expression::copy_from_rvalue(val.t, cpt)) return false;
			}
			else {
				if (!Expression::move_from_rvalue(val.t, cpt)) return false;
			}

			if (!require_output) {
				val.lvalue = false;
				val.t = Ctx::types()->t_void;
			}
		}

		res = val;
		
		return true;
	}

	bool Expression::parse_and(Cursor& c, CompileValue& res, CompileType cpt) {

		ILBlock* fallback = nullptr;
		CompileValue value;
		Cursor err = c;
		if (!Expression::parse_operators(c, value, cpt)) return false;

		while (c.tok == RecognizedToken::DoubleAnd) {

			if (!Operand::cast(err, value, Ctx::types()->t_bool, cpt, true)) return false;
			/*if (value.t != Ctx::types()->t_bool) {
				throw_specific_error(c, "Operation requires left operand to be boolean");
				return false;
			}*/

			if (!Expression::rvalue(value, cpt)) return false;

			c.move();

			if (cpt == CompileType::eval) {
				uint8_t v = Ctx::eval()->read_register_value<uint8_t>();

				Ctx::eval()->pop_register_value<uint8_t>();
				CompileValue right;
				if (!Expression::parse_operators(c, right, cpt)) return false;
				if (!Expression::rvalue(right, cpt)) return false;

				if (right.t != Ctx::types()->t_bool) {
					throw_specific_error(c, "Operation requires right operand to be boolean");
					return false;
				}
				uint8_t rv = Ctx::eval()->pop_register_value<uint8_t>();
				ILBuilder::eval_const_ibool(Ctx::eval(), v & rv);
			}
			else {
				if (!fallback) {
					fallback = Ctx::workspace_function()->create_block();
				}

				if (!rvalue(value, cpt)) return false;

				ILBlock* positive_block = Ctx::workspace_function()->create_block();
				Ctx::workspace_function()->append_block(positive_block);

				if (!ILBuilder::build_discard(positive_block, ILDataType::ibool)) return false;

				ILBuilder::build_const_ibool(Ctx::scope(), false);
				if (!ILBuilder::build_yield(Ctx::scope(), ILDataType::ibool)) return false;

				if (!ILBuilder::build_jmpz(Ctx::scope(),fallback, positive_block)) return false;

				Ctx::pop_scope();
				Ctx::push_scope(positive_block);

				err = c;
				if (!Expression::parse_operators(c, value, cpt)) return false;
				//if (!Expression::rvalue(value, cpt)) return false;

			}

		}

		if (fallback != nullptr && cpt == CompileType::compile) {
			if (value.t != Ctx::types()->t_bool) {
				throw_specific_error(c, "Operation requires right operand to be boolean");
				return false;
			}

			if (!rvalue(value, cpt)) return false;
			if (!ILBuilder::build_yield(Ctx::scope(), ILDataType::ibool)) return false;
			if (!ILBuilder::build_jmp(Ctx::scope(), fallback)) return false;

			Ctx::workspace_function()->append_block(fallback);
			Ctx::pop_scope();
			Ctx::push_scope(fallback);
			fallback = nullptr;

			if (!ILBuilder::build_accept(Ctx::scope(), ILDataType::ibool)) return false;
			value.t = Ctx::types()->t_bool;
			value.lvalue = false;
		}

		res = value;
		return true;
	}



	bool Expression::parse_or(Cursor& c, CompileValue& res, CompileType cpt) {

		ILBlock* fallback = nullptr;

		CompileValue value;
		Cursor err;
		if (!Expression::parse_and(c, value, cpt)) return false;

		while (c.tok == RecognizedToken::DoubleOr) {
			if (!Operand::cast(err, value, Ctx::types()->t_bool, cpt, true)) return false;
			/*if (value.t != Ctx::types()->t_bool) {
				throw_specific_error(c, "Operation requires left operand to be boolean");
				return false;
			}*/

			if (!Expression::rvalue(value, cpt)) return false;

			c.move();


			if (cpt == CompileType::eval) {
				uint8_t v = Ctx::eval()->read_register_value<uint8_t>();

				Ctx::eval()->pop_register_value<uint8_t>();
				CompileValue right;
				if (!Expression::parse_and(c, right, cpt)) return false;
				if (!Expression::rvalue(right,cpt)) return false;

				if (right.t != Ctx::types()->t_bool) {
					throw_specific_error(c, "Operation requires right operand to be boolean");
					return false;
				}
				uint8_t rv = Ctx::eval()->pop_register_value<uint8_t>();
				ILBuilder::eval_const_ibool(Ctx::eval(), v | rv);
			}
			else {
				if (!fallback) {
					fallback = Ctx::workspace_function()->create_block();
				}

				if (!rvalue(value, cpt)) return false;

				ILBlock* positive_block = Ctx::workspace_function()->create_block();
				Ctx::workspace_function()->append_block(positive_block);

				if (!ILBuilder::build_discard(positive_block, ILDataType::ibool)) return false;

				ILBuilder::build_const_ibool(Ctx::scope(), true);
				if (!ILBuilder::build_yield(Ctx::scope(), ILDataType::ibool)) return false;

				if (!ILBuilder::build_jmpz(Ctx::scope(), positive_block, fallback)) return false;

				Ctx::pop_scope();
				Ctx::push_scope(positive_block);

				err = c;
				if (!Expression::parse_and(c, value, cpt)) return false;
			}

		}

		if (fallback != nullptr && cpt == CompileType::compile) {
			if (value.t != Ctx::types()->t_bool) {
				throw_specific_error(c, "Operation requires right operand to be boolean");
				return false;
			}

			if (!rvalue(value, cpt)) return false;

			if (!ILBuilder::build_yield(Ctx::scope(), ILDataType::ibool)) return false;
			if (!ILBuilder::build_jmp(Ctx::scope(), fallback)) return false;

			Ctx::workspace_function()->append_block(fallback);
			Ctx::pop_scope();
			Ctx::push_scope(fallback);

			fallback = nullptr;

			if (!ILBuilder::build_accept(Ctx::scope(), ILDataType::ibool)) return false;

			value.t = Ctx::types()->t_bool;
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