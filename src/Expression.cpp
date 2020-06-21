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


	void Expression::copy_from_rvalue(Type* me, CompileType cpt, bool me_top) {
		if (cpt == CompileType::compile) {

			if (me->has_special_copy()) {
				if (me->rvalue_stacked()) {
					if (!me_top) {
						ILBuilder::build_swap(Ctx::scope(), ILDataType::ptr);
					}
					me->build_copy();
				}
				else {
					throw std::exception("Compiler error, structure with special copy should not have rvalue stacked");
				}
			}
			else {
				if (me_top) {
					if (me->rvalue_stacked()) {
						ILBuilder::build_memcpy_rev(Ctx::scope(), me->size());
					}
					else {
						ILBuilder::build_store_rev(Ctx::scope(), me->rvalue());
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
					throw std::exception("Compiler error, structure with special copy should not have rvalue stacked");
				}
			}
			else {

				if (me_top) {
					if (me->rvalue_stacked()) {
						ILBuilder::eval_memcpy_rev(Ctx::eval(), me->size().eval(Ctx::global_module(), compiler_arch));
					}
					else {
						ILBuilder::eval_store_rev(Ctx::eval(), me->rvalue());
					}
				}
				else {
					if (me->rvalue_stacked()) {
						ILBuilder::eval_memcpy(Ctx::eval(), me->size().eval(Ctx::global_module(), compiler_arch));
					}
					else {
						ILBuilder::eval_store(Ctx::eval(), me->rvalue());
					}
				}
			}
		}

	}

	void Expression::move_from_rvalue(Type* me, CompileType cpt, bool me_top) {
		if (cpt == CompileType::compile) {

			if (me->has_special_move()) {
				if (me->rvalue_stacked()) {
					if (!me_top) {
						ILBuilder::build_swap(Ctx::scope(), ILDataType::ptr);
					}
					me->build_move();
				}
				else {
					throw std::exception("Compiler error, structure with special copy should not have rvalue stacked");
				}
			}
			else {
				if (me_top) {
					if (me->rvalue_stacked()) {
						ILBuilder::build_memcpy_rev(Ctx::scope(), me->size());
					}
					else {
						ILBuilder::build_store_rev(Ctx::scope(), me->rvalue());
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
					throw std::exception("Compiler error, structure with special move should not have rvalue stacked");
				}
			}
			else {

				if (me_top) {
					if (me->rvalue_stacked()) {
						ILBuilder::eval_memcpy_rev(Ctx::eval(), me->size().eval(Ctx::global_module(), compiler_arch));
					}
					else {
						ILBuilder::eval_store_rev(Ctx::eval(), me->rvalue());
					}
				}
				else {
					if (me->rvalue_stacked()) {
						ILBuilder::eval_memcpy(Ctx::eval(), me->size().eval(Ctx::global_module(), compiler_arch));
					}
					else {
						ILBuilder::eval_store(Ctx::eval(), me->rvalue());
					}
				}
			}
		}
	}



	void Expression::rvalue(CompileValue& value, CompileType cpt) {
		if (value.lvalue && !value.t->rvalue_stacked()) {

			if (cpt == CompileType::compile) {
				value.lvalue = false;
				ILBuilder::build_load(Ctx::scope(), value.t->rvalue());
			}
			else if (cpt == CompileType::eval) {
				value.lvalue = false;
				ILBuilder::eval_load(Ctx::eval(), value.t->rvalue());
			}
		}
		/*else if (!value.lvalue && value.t->type() == TypeInstanceType::type_reference) {
			value.t = ((TypeReference*)value.t)->owner;
			value.lvalue = true;
		}*/
	}

	ILDataType Expression::arithmetic_type(Type* type) {
		if (type->type() != TypeInstanceType::type_structure_instance) {
			return ILDataType::none;
		}

		TypeStructureInstance* si = (TypeStructureInstance*)type;
		if (si->owner->structure_type != StructureInstanceType::primitive_structure) {
			return ILDataType::none;
		}

		return type->rvalue();
	}

	Type* Expression::arithmetic_result(Type* type_left, Type* type_right) {
		ILDataType lv = Expression::arithmetic_type(type_left);
		ILDataType rv = Expression::arithmetic_type(type_right);
		if (lv == ILDataType::none || rv == ILDataType::none) return nullptr;

		ILDataType rsv = ILBuilder::arith_result(lv, rv);
		return Ctx::types()->get_type_from_rvalue(rsv);
	}



	void Expression::emit(Cursor& c, CompileValue& res, int l, int op, CompileValue left, CompileValue right, CompileType cpt, int next_l, int next_op) {
		bool isf = false;
		bool sig = false;
		Type* result_type = nullptr;

		if (!(result_type = Expression::arithmetic_result(left.t, right.t))) {
			if ((l==1 || l==2) && left.t == right.t) {
				int8_t eval_value = 0;

				if (left.t->has_special_compare()) {
					if (left.lvalue || left.t->rvalue_stacked()) {
						if (cpt == CompileType::compile) {
							left.t->build_compare();
						}
						else {
							unsigned char* cmp_to = Ctx::eval()->pop_register_value<unsigned char*>();
							unsigned char* cmp_what = Ctx::eval()->pop_register_value<unsigned char*>();
							eval_value = left.t->compare(cmp_what,cmp_to);
						}
					}
					else {
						throw_specific_error(c, "Compiler error, type should not be rvalue with specific compare function");
					}
				}
				else {

					if (cpt == CompileType::compile) {
						if (left.lvalue || left.t->rvalue_stacked()) {
							ILBuilder::build_memcmp(Ctx::scope(), left.t->size());
						}
						else {
							ILBuilder::build_rmemcmp(Ctx::scope(), left.t->rvalue());
						}
					}
					else {
						if (left.lvalue || left.t->rvalue_stacked()) {
							ILBuilder::eval_memcmp(Ctx::eval(), left.t->size().eval(Ctx::global_module(), compiler_arch));
						}
						else {
							ILBuilder::eval_rmemcmp(Ctx::eval(), left.t->rvalue());
						}
						eval_value = Ctx::eval()->pop_register_value<int8_t>();
					}

				}

				if (cpt == CompileType::compile) {
					switch (l) {
						case 1: {
							switch (op) {
								case 0: {
									ILBuilder::build_const_i8(Ctx::scope(), 0);
									ILBuilder::build_eq(Ctx::scope(), ILDataType::i8, ILDataType::i8);
								}break;
								case 1: {
									ILBuilder::build_const_i8(Ctx::scope(), 0);
									ILBuilder::build_ne(Ctx::scope(), ILDataType::i8, ILDataType::i8);
								}break;
							}
						}break;
						case 2: {
							switch (op) {
								case 0: {
									ILBuilder::build_const_i8(Ctx::scope(), 1);
									ILBuilder::build_eq(Ctx::scope(), ILDataType::i8, ILDataType::i8);
								}break;
								case 1: {
									ILBuilder::build_const_i8(Ctx::scope(), -1);
									ILBuilder::build_eq(Ctx::scope(), ILDataType::i8, ILDataType::i8);
								} break;
								case 2: {
									ILBuilder::build_const_i8(Ctx::scope(), -1);
									ILBuilder::build_ne(Ctx::scope(), ILDataType::i8, ILDataType::i8);
								}break;
								case 3: {
									ILBuilder::build_const_i8(Ctx::scope(), 1);
									ILBuilder::build_ne(Ctx::scope(), ILDataType::i8, ILDataType::i8);
								}break;
							}
						}break;
					}
				}
				else {
					switch (l) {
						case 1: {
							switch (op) {
								case 0: {
									ILBuilder::eval_const_ibool(Ctx::eval(), eval_value == 0 ? 1 : 0);
								}break;
								case 1: {
									ILBuilder::eval_const_ibool(Ctx::eval(), eval_value == 0 ? 0 : 1);
								}break;
							}
						}break;
						case 2: {
							switch (op) {
								case 0: {
									ILBuilder::eval_const_ibool(Ctx::eval(), eval_value == 1 ? 1 : 0);
								}break;
								case 1: {
									ILBuilder::eval_const_ibool(Ctx::eval(), eval_value == -1 ? 1 : 0);
								} break;
								case 2: {
									ILBuilder::eval_const_ibool(Ctx::eval(), eval_value == -1 ? 0 : 1);
								}break;
								case 3: {
									ILBuilder::eval_const_ibool(Ctx::eval(), eval_value == 1 ? 0 : 1);
								}break;
							}
						}break;
					}
				}


				res.t = Ctx::types()->t_bool;
				res.lvalue = false;
			}
			else {
				throw_specific_error(c, "Operation is not defined on top of specified types");
			}
		}
		else {

			if (l == 1 || l == 2)
				result_type = Ctx::types()->t_bool;

			if (cpt == CompileType::compile) {
				switch (l) {
					case 0: {
						switch (op) {
							case 0: {
								ILBuilder::build_and(Ctx::scope(), left.t->rvalue(), right.t->rvalue());
							}break;
							case 1: {
								ILBuilder::build_or(Ctx::scope(), left.t->rvalue(), right.t->rvalue());
							}break;
							case 2: {
								ILBuilder::build_xor(Ctx::scope(), left.t->rvalue(), right.t->rvalue());
							}break;
						}
					}break;
					case 1: {
						switch (op) {
							case 0: {
								ILBuilder::build_eq(Ctx::scope(), left.t->rvalue(), right.t->rvalue());
							}break;
							case 1: {
								ILBuilder::build_ne(Ctx::scope(), left.t->rvalue(), right.t->rvalue());
							}break;
						}
					}break;
					case 2: {
						switch (op) {
							case 0: {
								ILBuilder::build_gt(Ctx::scope(), left.t->rvalue(), right.t->rvalue());
							}break;
							case 1: {
								ILBuilder::build_lt(Ctx::scope(), left.t->rvalue(), right.t->rvalue());
							} break;
							case 2: {
								ILBuilder::build_ge(Ctx::scope(), left.t->rvalue(), right.t->rvalue());
							}break;
							case 3: {
								ILBuilder::build_le(Ctx::scope(), left.t->rvalue(), right.t->rvalue());
							}break;
						}
					}break;
					case 3: {
						switch (op) {
							case 0: {
								ILBuilder::build_add(Ctx::scope(), left.t->rvalue(), right.t->rvalue());
							}break;
							case 1: {
								ILBuilder::build_sub(Ctx::scope(), left.t->rvalue(), right.t->rvalue());
							}break;
						}
					}break;
					case 4: {
						switch (op) {
							case 0: {
								ILBuilder::build_mul(Ctx::scope(), left.t->rvalue(), right.t->rvalue());
							}break;
							case 1: {
								ILBuilder::build_div(Ctx::scope(), left.t->rvalue(), right.t->rvalue());
							} break;
							case 2: {
								ILBuilder::build_rem(Ctx::scope(), left.t->rvalue(), right.t->rvalue());
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
								ILBuilder::eval_and(Ctx::eval(), left.t->rvalue(), right.t->rvalue());
							}break;
							case 1: {
								ILBuilder::eval_or(Ctx::eval(), left.t->rvalue(), right.t->rvalue());
							}break;
							case 2: {
								ILBuilder::eval_xor(Ctx::eval(), left.t->rvalue(), right.t->rvalue());
							}break;
						}
					}break;
					case 1: {
						switch (op) {
							case 0: {
								ILBuilder::eval_eq(Ctx::eval(), left.t->rvalue(), right.t->rvalue());
							}break;
							case 1: {
								ILBuilder::eval_ne(Ctx::eval(), left.t->rvalue(), right.t->rvalue());
							}break;
						}
					}break;
					case 2: {
						switch (op) {
							case 0: {
								ILBuilder::eval_gt(Ctx::eval(), left.t->rvalue(), right.t->rvalue());
							}break;
							case 1: {
								ILBuilder::eval_lt(Ctx::eval(), left.t->rvalue(), right.t->rvalue());
							} break;
							case 2: {
								ILBuilder::eval_ge(Ctx::eval(), left.t->rvalue(), right.t->rvalue());
							}break;
							case 3: {
								ILBuilder::eval_le(Ctx::eval(), left.t->rvalue(), right.t->rvalue());
							}break;
						}
					}break;
					case 3: {
						switch (op) {
							case 0: {
								ILBuilder::eval_add(Ctx::eval(), left.t->rvalue(), right.t->rvalue());
							}break;
							case 1: {
								ILBuilder::eval_sub(Ctx::eval(), left.t->rvalue(), right.t->rvalue());
							}break;
						}
					}break;
					case 4: {
						switch (op) {
							case 0: {
								ILBuilder::eval_mul(Ctx::eval(), left.t->rvalue(), right.t->rvalue());
							}break;
							case 1: {
								ILBuilder::eval_div(Ctx::eval(), left.t->rvalue(), right.t->rvalue());
							} break;
							case 2: {
								ILBuilder::eval_rem(Ctx::eval(), left.t->rvalue(), right.t->rvalue());
							}break;
						}
					}break;
				}
			}

			res.t = result_type;
			res.lvalue = false;
		}
	}

	void Expression::parse(Cursor& c, CompileValue& res, CompileType cpt, bool require_output) {
		CompileValue val;
		parse_or(c, val, cpt);

		if (c.tok == RecognizedToken::Equals || c.tok == RecognizedToken::BackArrow) {
			bool do_copy = c.tok == RecognizedToken::Equals;

			if (!val.lvalue) {
				throw_specific_error(c, "Left assignment must be modifiable lvalue");
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

			CompileValue val2;
			Expression::parse(c, val2, cpt);
			Operand::cast(err, val2, val.t, cpt, true);

			if (!val2.lvalue) {
				do_copy = false;
			}

			Expression::rvalue(val2, CompileType::compile);

			if (do_copy) {
				Expression::copy_from_rvalue(val.t, cpt);
			}
			else {
				Expression::move_from_rvalue(val.t, cpt);
			}

			if (!require_output) {
				val.lvalue = false;
				val.t = Ctx::types()->t_void;
			}
		}
		else if (c.tok == RecognizedToken::PlusEquals || c.tok == RecognizedToken::MinusEquals || c.tok == RecognizedToken::StarEquals || c.tok == RecognizedToken::SlashEquals) {
			unsigned char op = 0;
			if (c.tok == RecognizedToken::PlusEquals) {
				op = 1;
			}
			else if (c.tok == RecognizedToken::MinusEquals) {
				op = 2;
			}
			else if (c.tok == RecognizedToken::StarEquals) {
				op = 3;
			}
			else {
				op = 4;
			}

			if (!Operand::is_numeric_value(val.t)) {
				throw_specific_error(c, "Left assignment must be numeric type");
			}

			if (!val.lvalue) {
				throw_specific_error(c, "Left assignment must be modifiable lvalue");
			}

			if (require_output) {
				if (cpt == CompileType::compile) {
					ILBuilder::build_duplicate(Ctx::scope(), ILDataType::ptr);
				}
				else {
					ILBuilder::eval_duplicate(Ctx::eval(), ILDataType::ptr);
				}
			}
			
			if (cpt == CompileType::compile) {
				ILBuilder::build_duplicate(Ctx::scope(), ILDataType::ptr);
			}
			else {
				ILBuilder::eval_duplicate(Ctx::eval(), ILDataType::ptr);
			}
			Expression::rvalue(val, cpt);

			c.move();
			Cursor err = c;

			CompileValue val2;
			Expression::parse(c, val2, cpt);
			Expression::rvalue(val2, cpt);
			if (!Operand::is_numeric_value(val2.t)) {
				throw_specific_error(err, "Expected numeric value");
			}

			if (cpt == CompileType::compile) {
				if (op==1)
					ILBuilder::build_add(Ctx::scope(), val.t->rvalue(), val2.t->rvalue());
				else if(op==2)
					ILBuilder::build_sub(Ctx::scope(), val.t->rvalue(), val2.t->rvalue());
				else if (op==3)
					ILBuilder::build_mul(Ctx::scope(), val.t->rvalue(), val2.t->rvalue());
				else
					ILBuilder::build_div(Ctx::scope(), val.t->rvalue(), val2.t->rvalue());

				ILBuilder::build_cast(Ctx::scope(), ILBuilder::arith_result(val.t->rvalue(), val2.t->rvalue()), val.t->rvalue());
				ILBuilder::build_store_rev(Ctx::scope(), val.t->rvalue());
			}
			else {
				if (op == 1)
					ILBuilder::eval_add(Ctx::eval(), val.t->rvalue(), val2.t->rvalue());
				else if (op == 2)
					ILBuilder::eval_sub(Ctx::eval(), val.t->rvalue(), val2.t->rvalue());
				else if (op == 3)
					ILBuilder::eval_mul(Ctx::eval(), val.t->rvalue(), val2.t->rvalue());
				else
					ILBuilder::eval_div(Ctx::eval(), val.t->rvalue(), val2.t->rvalue());

				ILBuilder::eval_cast(Ctx::eval(), ILBuilder::arith_result(val.t->rvalue(), val2.t->rvalue()), val.t->rvalue());
				ILBuilder::eval_store_rev(Ctx::eval(), val.t->rvalue());
			}

			if (!require_output) {
				val.lvalue = false;
				val.t = Ctx::types()->t_void;
			}
		}

		res = val;
	}

	void Expression::parse_and(Cursor& c, CompileValue& res, CompileType cpt) {

		ILBlock* fallback = nullptr;
		CompileValue value;
		Cursor err = c;
		Expression::parse_operators(c, value, cpt);

		while (c.tok == RecognizedToken::DoubleAnd) {

			Operand::cast(err, value, Ctx::types()->t_bool, cpt, true);
			/*if (value.t != Ctx::types()->t_bool) {
				throw_specific_error(c, "Operation requires left operand to be boolean");
				return false;
			}*/

			Expression::rvalue(value, cpt);

			c.move();

			if (cpt == CompileType::eval) {
				uint8_t v = Ctx::eval()->read_register_value<uint8_t>();

				Ctx::eval()->pop_register_value<uint8_t>();
				CompileValue right;
				err = c;
				Expression::parse_operators(c, right, cpt);
				Expression::rvalue(right, cpt);
				Operand::cast(err, right, Ctx::types()->t_bool, cpt, true);

				uint8_t rv = Ctx::eval()->pop_register_value<uint8_t>();
				ILBuilder::eval_const_ibool(Ctx::eval(), v & rv);
			}
			else {
				if (!fallback) {
					fallback = Ctx::workspace_function()->create_block();
				}

				Expression::rvalue(value, cpt);

				ILBlock* positive_block = Ctx::workspace_function()->create_block();
				Ctx::workspace_function()->append_block(positive_block);

				ILBuilder::build_discard(positive_block, ILDataType::ibool);

				ILBuilder::build_const_ibool(Ctx::scope(), false);
				ILBuilder::build_yield(Ctx::scope(), ILDataType::ibool);

				ILBuilder::build_jmpz(Ctx::scope(), fallback, positive_block);

				Ctx::pop_scope();
				Ctx::push_scope(positive_block);

				err = c;
				Expression::parse_operators(c, value, cpt);
				//if (!Expression::rvalue(value, cpt)) return false;

			}

		}

		if (fallback != nullptr && cpt == CompileType::compile) {

			Operand::cast(err, value, Ctx::types()->t_bool, cpt, true);

			Expression::rvalue(value, cpt);
			ILBuilder::build_yield(Ctx::scope(), ILDataType::ibool);
			ILBuilder::build_jmp(Ctx::scope(), fallback);

			Ctx::workspace_function()->append_block(fallback);
			Ctx::pop_scope();
			Ctx::push_scope(fallback);
			fallback = nullptr;

			ILBuilder::build_accept(Ctx::scope(), ILDataType::ibool);
			value.t = Ctx::types()->t_bool;
			value.lvalue = false;
		}

		res = value;
	}



	void Expression::parse_or(Cursor& c, CompileValue& res, CompileType cpt) {

		ILBlock* fallback = nullptr;

		CompileValue value;
		Cursor err;
		Expression::parse_and(c, value, cpt);

		while (c.tok == RecognizedToken::DoubleOr) {
			Operand::cast(err, value, Ctx::types()->t_bool, cpt, true);
			/*if (value.t != Ctx::types()->t_bool) {
				throw_specific_error(c, "Operation requires left operand to be boolean");
				return false;
			}*/

			Expression::rvalue(value, cpt);

			c.move();


			if (cpt == CompileType::eval) {
				uint8_t v = Ctx::eval()->read_register_value<uint8_t>();

				Ctx::eval()->pop_register_value<uint8_t>();
				CompileValue right;
				Expression::parse_and(c, right, cpt);
				Expression::rvalue(right, cpt);
				Operand::cast(err, right, Ctx::types()->t_bool, cpt, true);

				uint8_t rv = Ctx::eval()->pop_register_value<uint8_t>();
				ILBuilder::eval_const_ibool(Ctx::eval(), v | rv);
			}
			else {
				if (!fallback) {
					fallback = Ctx::workspace_function()->create_block();
				}

				Expression::rvalue(value, cpt);

				ILBlock* positive_block = Ctx::workspace_function()->create_block();
				Ctx::workspace_function()->append_block(positive_block);

				ILBuilder::build_discard(positive_block, ILDataType::ibool);

				ILBuilder::build_const_ibool(Ctx::scope(), true);
				ILBuilder::build_yield(Ctx::scope(), ILDataType::ibool);

				ILBuilder::build_jmpz(Ctx::scope(), positive_block, fallback);

				Ctx::pop_scope();
				Ctx::push_scope(positive_block);

				err = c;
				Expression::parse_and(c, value, cpt);
			}

		}

		if (fallback != nullptr && cpt == CompileType::compile) {

			Operand::cast(err, value, Ctx::types()->t_bool, cpt, true);

			Expression::rvalue(value, cpt);

			ILBuilder::build_yield(Ctx::scope(), ILDataType::ibool);
			ILBuilder::build_jmp(Ctx::scope(), fallback);

			Ctx::workspace_function()->append_block(fallback);
			Ctx::pop_scope();
			Ctx::push_scope(fallback);

			fallback = nullptr;

			ILBuilder::build_accept(Ctx::scope(), ILDataType::ibool);

			value.t = Ctx::types()->t_bool;
			value.lvalue = false;
		}

		res = value;
	}

	void Expression::parse_operators(Cursor& c, CompileValue& res, CompileType cpt) {

		int op_type[5] = { -1 };
		Cursor op_cursors[5];
		CompileValue layer[5];
		memset(layer, 0, sizeof(layer));

		int current_layer = -1;

		while (true) {
			CompileValue value;
			Cursor err = c;
			Operand::parse(c, value, cpt);
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
				Expression::rvalue(value, cpt);
			}

			for (int i = current_layer; i >= std::max(op_v, 0); i--) {

				if (i >= 0 && layer[i].t != nullptr) {
					CompileValue& left = layer[i];
					CompileValue& right = value;

					Expression::emit(op_cursors[i], value, i, op_type[i], left, right, cpt, op_v, op_t);
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
				return;
			}
		}
	}
}