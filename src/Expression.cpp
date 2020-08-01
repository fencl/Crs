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
			if (me_top) {
				if (me->rvalue_stacked()) {
					ILBuilder::build_memcpy_rev(me->compiler()->scope(), me->size());
				}
				else {
					ILBuilder::build_store_rev(me->compiler()->scope(), me->rvalue());
				}
			}
			else {
				if (me->rvalue_stacked()) {
					ILBuilder::build_memcpy(me->compiler()->scope(), me->size());
				}
				else {
					ILBuilder::build_store(me->compiler()->scope(), me->rvalue());
				}
			}
		}
		else {
			if (me_top) {
				if (me->rvalue_stacked()) {
					ILBuilder::eval_memcpy_rev(me->compiler()->evaluator(), me->size().eval(me->compiler()->global_module(), compiler_arch));
				}
				else {
					ILBuilder::eval_store_rev(me->compiler()->evaluator(), me->rvalue());
				}
			}
			else {
				if (me->rvalue_stacked()) {
					ILBuilder::eval_memcpy(me->compiler()->evaluator(), me->size().eval(me->compiler()->global_module(), compiler_arch));
				}
				else {
					ILBuilder::eval_store(me->compiler()->evaluator(), me->rvalue());
				}
			}
		}
	}



	void Expression::rvalue(Compiler& compiler, CompileValue& value, CompileType cpt) {
		if (value.lvalue && !value.type->rvalue_stacked()) {

			if (cpt == CompileType::compile) {
				value.lvalue = false;
				ILBuilder::build_load(compiler.scope(), value.type->rvalue());
			}
			else if (cpt == CompileType::eval) {
				value.lvalue = false;
				ILBuilder::eval_load(compiler.evaluator(), value.type->rvalue());
			}
		}
	}

	ILDataType Expression::arithmetic_type(Type* type) {
		if (type->type() != TypeInstanceType::type_structure_instance && type->type() != TypeInstanceType::type_reference) {
			return ILDataType::none;
		}

		TypeStructureInstance* si = (TypeStructureInstance*)type;
		if (si->owner->structure_type != StructureInstanceType::primitive_structure) {
			return ILDataType::none;
		}

		return type->rvalue();
	}

	Type* Expression::arithmetic_result(Compiler& compiler, Type* type_left, Type* type_right) {
		ILDataType lv = Expression::arithmetic_type(type_left);
		ILDataType rv = Expression::arithmetic_type(type_right);
		if (lv == ILDataType::none || rv == ILDataType::none) return nullptr;

		ILDataType rsv = ILBuilder::arith_result(lv, rv);
		return compiler.types()->get_type_from_rvalue(rsv);
	}



	void Expression::emit(Compiler& compiler, Cursor& c, CompileValue& res, int l, int op, CompileValue left, CompileValue right, CompileType cpt, int next_l, int next_op) {
		bool isf = false;
		bool sig = false;
		Type* result_type = nullptr;

		if (!(result_type = Expression::arithmetic_result(compiler, left.type, right.type))) {
			if ((l == 1 || l == 2) && left.type == right.type) {
				int8_t eval_value = 0;

				if (cpt == CompileType::compile) {
					if (left.lvalue || left.type->rvalue_stacked()) {
						ILBuilder::build_memcmp(compiler.scope(), left.type->size());
					}
					else {
						ILBuilder::build_rmemcmp(compiler.scope(), left.type->rvalue());
					}
				}
				else {
					if (left.lvalue || left.type->rvalue_stacked()) {
						ILBuilder::eval_memcmp(compiler.evaluator(), left.type->size().eval(compiler.global_module(), compiler_arch));
					}
					else {
						ILBuilder::eval_rmemcmp(compiler.evaluator(), left.type->rvalue());
					}
					eval_value = compiler.evaluator()->pop_register_value<int8_t>();
				}

				if (cpt == CompileType::compile) {
					switch (l) {
						case 1: {
							switch (op) {
								case 0: {
									ILBuilder::build_const_i8(compiler.scope(), 0);
									ILBuilder::build_eq(compiler.scope(), ILDataType::i8, ILDataType::i8);
								}break;
								case 1: {
									ILBuilder::build_const_i8(compiler.scope(), 0);
									ILBuilder::build_ne(compiler.scope(), ILDataType::i8, ILDataType::i8);
								}break;
							}
						}break;
						case 2: {
							switch (op) {
								case 0: {
									ILBuilder::build_const_i8(compiler.scope(), 1);
									ILBuilder::build_eq(compiler.scope(), ILDataType::i8, ILDataType::i8);
								}break;
								case 1: {
									ILBuilder::build_const_i8(compiler.scope(), -1);
									ILBuilder::build_eq(compiler.scope(), ILDataType::i8, ILDataType::i8);
								} break;
								case 2: {
									ILBuilder::build_const_i8(compiler.scope(), -1);
									ILBuilder::build_ne(compiler.scope(), ILDataType::i8, ILDataType::i8);
								}break;
								case 3: {
									ILBuilder::build_const_i8(compiler.scope(), 1);
									ILBuilder::build_ne(compiler.scope(), ILDataType::i8, ILDataType::i8);
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
									ILBuilder::eval_const_i8(compiler.evaluator(), eval_value == 0 ? 1 : 0);
								}break;
								case 1: {
									ILBuilder::eval_const_i8(compiler.evaluator(), eval_value == 0 ? 0 : 1);
								}break;
							}
						}break;
						case 2: {
							switch (op) {
								case 0: {
									ILBuilder::eval_const_i8(compiler.evaluator(), eval_value == 1 ? 1 : 0);
								}break;
								case 1: {
									ILBuilder::eval_const_i8(compiler.evaluator(), eval_value == -1 ? 1 : 0);
								} break;
								case 2: {
									ILBuilder::eval_const_i8(compiler.evaluator(), eval_value == -1 ? 0 : 1);
								}break;
								case 3: {
									ILBuilder::eval_const_i8(compiler.evaluator(), eval_value == 1 ? 0 : 1);
								}break;
							}
						}break;
					}
				}


				res.type = compiler.types()->t_bool;
				res.lvalue = false;
			}
			else {
				throw_specific_error(c, "Operation is not defined on top of specified types");
			}
		}
		else {

			if (l == 1 || l == 2)
				result_type = compiler.types()->t_bool;

			if (cpt == CompileType::compile) {
				switch (l) {
					case 0: {
						switch (op) {
							case 0: {
								ILBuilder::build_and(compiler.scope(), left.type->rvalue(), right.type->rvalue());
							}break;
							case 1: {
								ILBuilder::build_or(compiler.scope(), left.type->rvalue(), right.type->rvalue());
							}break;
							case 2: {
								ILBuilder::build_xor(compiler.scope(), left.type->rvalue(), right.type->rvalue());
							}break;
						}
					}break;
					case 1: {
						switch (op) {
							case 0: {
								ILBuilder::build_eq(compiler.scope(), left.type->rvalue(), right.type->rvalue());
							}break;
							case 1: {
								ILBuilder::build_ne(compiler.scope(), left.type->rvalue(), right.type->rvalue());
							}break;
						}
					}break;
					case 2: {
						switch (op) {
							case 0: {
								ILBuilder::build_gt(compiler.scope(), left.type->rvalue(), right.type->rvalue());
							}break;
							case 1: {
								ILBuilder::build_lt(compiler.scope(), left.type->rvalue(), right.type->rvalue());
							} break;
							case 2: {
								ILBuilder::build_ge(compiler.scope(), left.type->rvalue(), right.type->rvalue());
							}break;
							case 3: {
								ILBuilder::build_le(compiler.scope(), left.type->rvalue(), right.type->rvalue());
							}break;
						}
					}break;
					case 3: {
						switch (op) {
							case 0: {
								ILBuilder::build_add(compiler.scope(), left.type->rvalue(), right.type->rvalue());
							}break;
							case 1: {
								ILBuilder::build_sub(compiler.scope(), left.type->rvalue(), right.type->rvalue());
							}break;
						}
					}break;
					case 4: {
						switch (op) {
							case 0: {
								ILBuilder::build_mul(compiler.scope(), left.type->rvalue(), right.type->rvalue());
							}break;
							case 1: {
								ILBuilder::build_div(compiler.scope(), left.type->rvalue(), right.type->rvalue());
							} break;
							case 2: {
								ILBuilder::build_rem(compiler.scope(), left.type->rvalue(), right.type->rvalue());
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
								ILBuilder::eval_and(compiler.evaluator(), left.type->rvalue(), right.type->rvalue());
							}break;
							case 1: {
								ILBuilder::eval_or(compiler.evaluator(), left.type->rvalue(), right.type->rvalue());
							}break;
							case 2: {
								ILBuilder::eval_xor(compiler.evaluator(), left.type->rvalue(), right.type->rvalue());
							}break;
						}
					}break;
					case 1: {
						switch (op) {
							case 0: {
								ILBuilder::eval_eq(compiler.evaluator(), left.type->rvalue(), right.type->rvalue());
							}break;
							case 1: {
								ILBuilder::eval_ne(compiler.evaluator(), left.type->rvalue(), right.type->rvalue());
							}break;
						}
					}break;
					case 2: {
						switch (op) {
							case 0: {
								ILBuilder::eval_gt(compiler.evaluator(), left.type->rvalue(), right.type->rvalue());
							}break;
							case 1: {
								ILBuilder::eval_lt(compiler.evaluator(), left.type->rvalue(), right.type->rvalue());
							} break;
							case 2: {
								ILBuilder::eval_ge(compiler.evaluator(), left.type->rvalue(), right.type->rvalue());
							}break;
							case 3: {
								ILBuilder::eval_le(compiler.evaluator(), left.type->rvalue(), right.type->rvalue());
							}break;
						}
					}break;
					case 3: {
						switch (op) {
							case 0: {
								ILBuilder::eval_add(compiler.evaluator(), left.type->rvalue(), right.type->rvalue());
							}break;
							case 1: {
								ILBuilder::eval_sub(compiler.evaluator(), left.type->rvalue(), right.type->rvalue());
							}break;
						}
					}break;
					case 4: {
						switch (op) {
							case 0: {
								ILBuilder::eval_mul(compiler.evaluator(), left.type->rvalue(), right.type->rvalue());
							}break;
							case 1: {
								ILBuilder::eval_div(compiler.evaluator(), left.type->rvalue(), right.type->rvalue());
							} break;
							case 2: {
								ILBuilder::eval_rem(compiler.evaluator(), left.type->rvalue(), right.type->rvalue());
							}break;
						}
					}break;
				}
			}

			res.type = result_type;
			res.lvalue = false;
		}
	}

	void Expression::parse(Compiler& compiler, Cursor& c,RecognizedToken& tok, CompileValue& res, CompileType cpt, bool require_output) {
		CompileValue val;
		parse_or(compiler,c,tok, val, cpt);
		unsigned char op = 0;
		switch (tok) {
			case RecognizedToken::Equals: {

				if (!val.lvalue) {
					throw_specific_error(c, "Left assignment must be modifiable lvalue");
				}

				if (require_output) {
					if (cpt == CompileType::compile) {
						ILBuilder::build_duplicate(compiler.scope(), ILDataType::word);
					}
					else {
						ILBuilder::eval_duplicate(compiler.evaluator(), ILDataType::word);
					}
				}

				c.move(tok);
				Cursor err = c;

				CompileValue val2;
				Expression::parse(compiler, c, tok, val2, cpt);
				Operand::cast(compiler, err, val2, val.type, cpt, true);
				Expression::rvalue(compiler, val2, CompileType::compile);
				Expression::copy_from_rvalue(val.type, cpt);

				if (!require_output) {
					val.lvalue = false;
					val.type = compiler.types()->t_void;
				}
			} break;
			case RecognizedToken::PlusEquals:
			case RecognizedToken::MinusEquals:
			case RecognizedToken::StarEquals:
			case RecognizedToken::SlashEquals: {

				switch (tok) {
					case RecognizedToken::PlusEquals: op = 1; break;
					case RecognizedToken::MinusEquals: op = 2; break;
					case RecognizedToken::StarEquals: op = 3; break;
					case RecognizedToken::SlashEquals: op = 4; break;
				}

				if (!Operand::is_numeric_value(val.type)) {
					throw_specific_error(c, "Left assignment must be numeric type");
				}

				if (!val.lvalue) {
					throw_specific_error(c, "Left assignment must be modifiable lvalue");
				}

				if (require_output) {
					if (cpt == CompileType::compile) {
						ILBuilder::build_duplicate(compiler.scope(), ILDataType::word);
					}
					else {
						ILBuilder::eval_duplicate(compiler.evaluator(), ILDataType::word);
					}
				}

				if (cpt == CompileType::compile) {
					ILBuilder::build_duplicate(compiler.scope(), ILDataType::word);
				}
				else {
					ILBuilder::eval_duplicate(compiler.evaluator(), ILDataType::word);
				}
				Expression::rvalue(compiler, val, cpt);

				c.move(tok);
				Cursor err = c;

				CompileValue val2;
				Expression::parse(compiler, c, tok, val2, cpt);
				Expression::rvalue(compiler, val2, cpt);
				if (!Operand::is_numeric_value(val2.type)) {
					throw_specific_error(err, "Expected numeric value");
				}

				if (cpt == CompileType::compile) {
					if (op == 1)
						ILBuilder::build_add(compiler.scope(), val.type->rvalue(), val2.type->rvalue());
					else if (op == 2)
						ILBuilder::build_sub(compiler.scope(), val.type->rvalue(), val2.type->rvalue());
					else if (op == 3)
						ILBuilder::build_mul(compiler.scope(), val.type->rvalue(), val2.type->rvalue());
					else
						ILBuilder::build_div(compiler.scope(), val.type->rvalue(), val2.type->rvalue());

					ILBuilder::build_cast(compiler.scope(), ILBuilder::arith_result(val.type->rvalue(), val2.type->rvalue()), val.type->rvalue());
					ILBuilder::build_store_rev(compiler.scope(), val.type->rvalue());
				}
				else {
					if (op == 1)
						ILBuilder::eval_add(compiler.evaluator(), val.type->rvalue(), val2.type->rvalue());
					else if (op == 2)
						ILBuilder::eval_sub(compiler.evaluator(), val.type->rvalue(), val2.type->rvalue());
					else if (op == 3)
						ILBuilder::eval_mul(compiler.evaluator(), val.type->rvalue(), val2.type->rvalue());
					else
						ILBuilder::eval_div(compiler.evaluator(), val.type->rvalue(), val2.type->rvalue());

					ILBuilder::eval_cast(compiler.evaluator(), ILBuilder::arith_result(val.type->rvalue(), val2.type->rvalue()), val.type->rvalue());
					ILBuilder::eval_store_rev(compiler.evaluator(), val.type->rvalue());
				}

				if (!require_output) {
					val.lvalue = false;
					val.type = compiler.types()->t_void;
				}
			} break;
		}

		res = val;
		if (!require_output && val.type != compiler.types()->t_void) {
			if (res.lvalue || res.type->rvalue_stacked()) {
				ILBuilder::build_forget(compiler.scope(), ILDataType::word);
			}
			else {
				ILBuilder::build_forget(compiler.scope(), res.type->rvalue());
			}

			res.lvalue = false;
			res.type = compiler.types()->t_void;
		}
	}

	void Expression::parse_and(Compiler& compiler, Cursor& c, RecognizedToken& tok, CompileValue& res, CompileType cpt) {

		ILBlock* fallback = nullptr;
		CompileValue value;
		Cursor err = c;
		Expression::parse_operators(compiler,c,tok, value, cpt);

		while (tok == RecognizedToken::DoubleAnd) {

			Operand::cast(compiler,err, value, compiler.types()->t_bool, cpt, true);
			/*if (value.t != compiler.types()->t_bool) {
				throw_specific_error(c, "Operation requires left operand to be boolean");
				return false;
			}*/

			Expression::rvalue(compiler,value, cpt);

			c.move(tok);

			if (cpt == CompileType::eval) {
				uint8_t v = compiler.evaluator()->read_register_value<uint8_t>();

				compiler.evaluator()->pop_register_value<uint8_t>();
				CompileValue right;
				err = c;
				Expression::parse_operators(compiler,c,tok, right, cpt);
				Expression::rvalue(compiler,right, cpt);
				Operand::cast(compiler,err, right, compiler.types()->t_bool, cpt, true);

				uint8_t rv = compiler.evaluator()->pop_register_value<uint8_t>();
				ILBuilder::eval_const_i8(compiler.evaluator(), v & rv);
			}
			else {
				if (!fallback) {
					fallback = compiler.target()->create_block();
				}

				Expression::rvalue(compiler,value, cpt);

				ILBlock* positive_block = compiler.target()->create_block();
				compiler.target()->append_block(positive_block);

				ILBuilder::build_discard(positive_block, ILDataType::i8);

				ILBuilder::build_const_i8(compiler.scope(), false);
				ILBuilder::build_yield(compiler.scope(), ILDataType::i8);

				ILBuilder::build_jmpz(compiler.scope(), fallback, positive_block);

				compiler.pop_scope();
				compiler.push_scope(positive_block);

				err = c;
				Expression::parse_operators(compiler,c,tok, value, cpt);

			}

		}

		if (fallback != nullptr && cpt == CompileType::compile) {

			Operand::cast(compiler,err, value, compiler.types()->t_bool, cpt, true);

			Expression::rvalue(compiler,value, cpt);
			ILBuilder::build_yield(compiler.scope(), ILDataType::i8);
			ILBuilder::build_jmp(compiler.scope(), fallback);

			compiler.target()->append_block(fallback);
			compiler.pop_scope();
			compiler.push_scope(fallback);
			fallback = nullptr;

			ILBuilder::build_accept(compiler.scope(), ILDataType::i8);
			value.type = compiler.types()->t_bool;
			value.lvalue = false;
		}

		res = value;
	}



	void Expression::parse_or(Compiler& compiler, Cursor& c,RecognizedToken& tok, CompileValue& res, CompileType cpt) {

		ILBlock* fallback = nullptr;

		CompileValue value;
		Cursor err;
		Expression::parse_and(compiler,c,tok, value, cpt);

		while (tok == RecognizedToken::DoubleOr) {
			Operand::cast(compiler,err, value, compiler.types()->t_bool, cpt, true);
			Expression::rvalue(compiler,value, cpt);

			c.move(tok);


			if (cpt == CompileType::eval) {
				uint8_t v = compiler.evaluator()->read_register_value<uint8_t>();

				compiler.evaluator()->pop_register_value<uint8_t>();
				CompileValue right;
				Expression::parse_and(compiler,c,tok, right, cpt);
				Expression::rvalue(compiler,right, cpt);
				Operand::cast(compiler,err, right, compiler.types()->t_bool, cpt, true);

				uint8_t rv = compiler.evaluator()->pop_register_value<uint8_t>();
				ILBuilder::eval_const_i8(compiler.evaluator(), v | rv);
			}
			else {
				if (!fallback) {
					fallback = compiler.target()->create_block();
				}

				Expression::rvalue(compiler,value, cpt);

				ILBlock* positive_block = compiler.target()->create_block();
				compiler.target()->append_block(positive_block);

				ILBuilder::build_discard(positive_block, ILDataType::i8);

				ILBuilder::build_const_i8(compiler.scope(), true);
				ILBuilder::build_yield(compiler.scope(), ILDataType::i8);

				ILBuilder::build_jmpz(compiler.scope(), positive_block, fallback);

				compiler.pop_scope();
				compiler.push_scope(positive_block);

				err = c;
				Expression::parse_and(compiler,c,tok, value, cpt);
			}

		}

		if (fallback != nullptr && cpt == CompileType::compile) {

			Operand::cast(compiler,err, value, compiler.types()->t_bool, cpt, true);

			Expression::rvalue(compiler,value, cpt);

			ILBuilder::build_yield(compiler.scope(), ILDataType::i8);
			ILBuilder::build_jmp(compiler.scope(), fallback);

			compiler.target()->append_block(fallback);
			compiler.pop_scope();
			compiler.push_scope(fallback);

			fallback = nullptr;

			ILBuilder::build_accept(compiler.scope(), ILDataType::i8);

			value.type = compiler.types()->t_bool;
			value.lvalue = false;
		}

		res = value;
	}

	void Expression::parse_operators(Compiler& compiler, Cursor& c, RecognizedToken& tok, CompileValue& res, CompileType cpt) {

		int op_type[5] = { -1 };
		Cursor op_cursors[5];
		CompileValue layer[5];
		memset(layer, 0, sizeof(layer));

		int current_layer = -1;

		while (true) {
			CompileValue value;
			Cursor err = c;
			Operand::parse(compiler,c,tok, value, cpt);
			int op_v = -1;
			int op_t = -1;

			Cursor opc = c;

			switch (tok)
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

			if (op_v >= 0 || current_layer >= 0) {
				Expression::rvalue(compiler,value, cpt);
			}

			for (int i = current_layer; i >= std::max(op_v, 0); i--) {

				if (i >= 0 && layer[i].type != nullptr) {
					CompileValue& left = layer[i];
					CompileValue& right = value;

					Expression::emit(compiler,op_cursors[i], value, i, op_type[i], left, right, cpt, op_v, op_t);
					layer[i].type = nullptr;
				}
			}



			if (op_v >= 0) {

				layer[op_v] = value;
				op_type[op_v] = op_t;
				op_cursors[op_v] = opc;
				current_layer = op_v;

				c.move(tok);
			}
			else {
				res = value;
				return;
			}
		}
	}
}