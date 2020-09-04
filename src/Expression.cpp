#include "Expression.hpp"
#include "Operand.hpp"
#include "Error.hpp"
#include <algorithm>
#include "Declaration.hpp"
#include "BuiltIn.hpp"
#include <iostream>
#include <cstring>
#include "StackManager.hpp"

namespace Corrosive {


	errvoid Expression::copy_from_rvalue(Type* me, CompileType cpt) {
		if (cpt == CompileType::compile) {
			if (me->rvalue_stacked()) {
				if (!ILBuilder::build_memcpy(Compiler::current()->scope(), me->size())) return err::fail;
			}
			else {
				if (!ILBuilder::build_store(Compiler::current()->scope(), me->rvalue())) return err::fail;
			}
		}
		else {
			if (me->rvalue_stacked()) {
				if (!ILBuilder::eval_memcpy(Compiler::current()->evaluator(), me->size().eval(Compiler::current()->global_module()))) return err::fail;
			}
			else {
				if (!ILBuilder::eval_store(Compiler::current()->evaluator(), me->rvalue())) return err::fail;
			}
		}
		return err::ok;
	}

	errvoid Expression::copy_from_rvalue_reverse(Type* me, CompileType cpt) {
		if (cpt == CompileType::compile) {
			if (me->rvalue_stacked()) {
				if (!ILBuilder::build_memcpy_rev(Compiler::current()->scope(), me->size())) return err::fail;
			}
			else {
				if (!ILBuilder::build_store_rev(Compiler::current()->scope(), me->rvalue())) return err::fail;
			}
		}
		else {
			if (me->rvalue_stacked()) {
				if (!ILBuilder::eval_memcpy_rev(Compiler::current()->evaluator(), me->size().eval(Compiler::current()->global_module()))) return err::fail;
			}
			else {
				if (!ILBuilder::eval_store_rev(Compiler::current()->evaluator(), me->rvalue())) return err::fail;
			}
		}
		return err::ok;
	}

	errvoid Expression::rvalue(CompileValue& value, CompileType cpt) {
		if (value.lvalue && !value.type->rvalue_stacked()) {

			if (cpt == CompileType::compile) {
				value.lvalue = false;
				if (!ILBuilder::build_load(Compiler::current()->scope(), value.type->rvalue())) return err::fail;
			}
			else if (cpt == CompileType::eval) {
				value.lvalue = false;
				if (!ILBuilder::eval_load(Compiler::current()->evaluator(), value.type->rvalue())) return err::fail;
			}
		}
		return err::ok;
	}

	ILDataType Expression::arithmetic_type(Type* type) {
		if (type->type() != TypeInstanceType::type_structure_instance || type == Compiler::current()->types()->t_ptr) {
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
		return Compiler::current()->types()->get_type_from_rvalue(rsv);
	}



	errvoid Expression::emit(Cursor& c, CompileValue& res, int l, int op, CompileValue left, CompileValue right, CompileType cpt, int next_l, int next_op) {
		Compiler* compiler = Compiler::current();
		bool isf = false;
		bool sig = false;
		Type* result_type = nullptr;

		if (!(result_type = Expression::arithmetic_result(left.type, right.type))) {
			if ((l == 1 || l == 2) && (left.type == right.type)) {

				std::int8_t eval_value = 0;

				if (cpt == CompileType::compile) {
					if (left.lvalue || left.type->rvalue_stacked()) {
						if (!ILBuilder::build_memcmp(compiler->scope(), left.type->size())) return err::fail;
					}
					else {
						if (!ILBuilder::build_rmemcmp(compiler->scope(), left.type->rvalue())) return err::fail;
					}
				}
				else {
					if (left.lvalue || left.type->rvalue_stacked()) {
						if (!ILBuilder::eval_memcmp(compiler->evaluator(), left.type->size().eval(compiler->global_module()))) return err::fail;
					}
					else {
						if (!ILBuilder::eval_rmemcmp(compiler->evaluator(), left.type->rvalue())) return err::fail;
					}
					if (!compiler->evaluator()->pop_register_value<std::int8_t>(eval_value)) return err::fail;
				}

				if (cpt == CompileType::compile) {
					switch (l) {
						case 1: {
							switch (op) {
								case 0: {
									if (!ILBuilder::build_const_i8(compiler->scope(), 0)) return err::fail;
									if (!ILBuilder::build_eq(compiler->scope(), ILDataType::i8, ILDataType::i8)) return err::fail;
								}break;
								case 1: {
									if (!ILBuilder::build_const_i8(compiler->scope(), 0)) return err::fail;
									if (!ILBuilder::build_ne(compiler->scope(), ILDataType::i8, ILDataType::i8)) return err::fail;
								}break;
							}
						}break;
						case 2: {
							switch (op) {
								case 0: {
									if (!ILBuilder::build_const_i8(compiler->scope(), 1)) return err::fail;
									if (!ILBuilder::build_eq(compiler->scope(), ILDataType::i8, ILDataType::i8)) return err::fail;
								}break;
								case 1: {
									if (!ILBuilder::build_const_i8(compiler->scope(), -1)) return err::fail;
									if (!ILBuilder::build_eq(compiler->scope(), ILDataType::i8, ILDataType::i8)) return err::fail;
								} break;
								case 2: {
									if (!ILBuilder::build_const_i8(compiler->scope(), -1)) return err::fail;
									if (!ILBuilder::build_ne(compiler->scope(), ILDataType::i8, ILDataType::i8)) return err::fail;
								}break;
								case 3: {
									if (!ILBuilder::build_const_i8(compiler->scope(), 1)) return err::fail;
									if (!ILBuilder::build_ne(compiler->scope(), ILDataType::i8, ILDataType::i8)) return err::fail;
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
									if (!ILBuilder::eval_const_i8(compiler->evaluator(), eval_value == 0 ? 1 : 0)) return err::fail;
								}break;
								case 1: {
									if (!ILBuilder::eval_const_i8(compiler->evaluator(), eval_value == 0 ? 0 : 1)) return err::fail;
								}break;
							}
						}break;
						case 2: {
							switch (op) {
								case 0: {
									if (!ILBuilder::eval_const_i8(compiler->evaluator(), eval_value == 1 ? 1 : 0)) return err::fail;
								}break;
								case 1: {
									if (!ILBuilder::eval_const_i8(compiler->evaluator(), eval_value == -1 ? 1 : 0)) return err::fail;
								} break;
								case 2: {
									if (!ILBuilder::eval_const_i8(compiler->evaluator(), eval_value == -1 ? 0 : 1)) return err::fail;
								}break;
								case 3: {
									if (!ILBuilder::eval_const_i8(compiler->evaluator(), eval_value == 1 ? 0 : 1)) return err::fail;
								}break;
							}
						}break;
					}
				}


				res.type = compiler->types()->t_bool;
				res.lvalue = false;
			}
			else {
				return throw_specific_error(c, "Operation is not defined on top of specified types");
			}
		}
		else {

			if (l == 1 || l == 2)
				result_type = compiler->types()->t_bool;

			if (cpt == CompileType::compile) {
				switch (l) {
					case 0: {
						switch (op) {
							case 0: {
								if (!ILBuilder::build_and(compiler->scope(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
							case 1: {
								if (!ILBuilder::build_or(compiler->scope(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
							case 2: {
								if (!ILBuilder::build_xor(compiler->scope(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
						}
					}break;
					case 1: {
						switch (op) {
							case 0: {
								if (!ILBuilder::build_eq(compiler->scope(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
							case 1: {
								if (!ILBuilder::build_ne(compiler->scope(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
						}
					}break;
					case 2: {
						switch (op) {
							case 0: {
								if (!ILBuilder::build_gt(compiler->scope(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
							case 1: {
								if (!ILBuilder::build_lt(compiler->scope(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							} break;
							case 2: {
								if (!ILBuilder::build_ge(compiler->scope(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
							case 3: {
								if (!ILBuilder::build_le(compiler->scope(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
						}
					}break;
					case 3: {
						switch (op) {
							case 0: {
								if (!ILBuilder::build_add(compiler->scope(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
							case 1: {
								if (!ILBuilder::build_sub(compiler->scope(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
						}
					}break;
					case 4: {
						switch (op) {
							case 0: {
								if (!ILBuilder::build_mul(compiler->scope(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
							case 1: {
								if (!ILBuilder::build_div(compiler->scope(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							} break;
							case 2: {
								if (!ILBuilder::build_rem(compiler->scope(), left.type->rvalue(), right.type->rvalue())) return err::fail;
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
								if (!ILBuilder::eval_and(compiler->evaluator(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
							case 1: {
								if (!ILBuilder::eval_or(compiler->evaluator(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
							case 2: {
								if (!ILBuilder::eval_xor(compiler->evaluator(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
						}
					}break;
					case 1: {
						switch (op) {
							case 0: {
								if (!ILBuilder::eval_eq(compiler->evaluator(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
							case 1: {
								if (!ILBuilder::eval_ne(compiler->evaluator(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
						}
					}break;
					case 2: {
						switch (op) {
							case 0: {
								if (!ILBuilder::eval_gt(compiler->evaluator(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
							case 1: {
								if (!ILBuilder::eval_lt(compiler->evaluator(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							} break;
							case 2: {
								if (!ILBuilder::eval_ge(compiler->evaluator(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
							case 3: {
								if (!ILBuilder::eval_le(compiler->evaluator(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
						}
					}break;
					case 3: {
						switch (op) {
							case 0: {
								if (!ILBuilder::eval_add(compiler->evaluator(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
							case 1: {
								if (!ILBuilder::eval_sub(compiler->evaluator(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
						}
					}break;
					case 4: {
						switch (op) {
							case 0: {
								if (!ILBuilder::eval_mul(compiler->evaluator(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
							case 1: {
								if (!ILBuilder::eval_div(compiler->evaluator(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							} break;
							case 2: {
								if (!ILBuilder::eval_rem(compiler->evaluator(), left.type->rvalue(), right.type->rvalue())) return err::fail;
							}break;
						}
					}break;
				}
			}

			res.type = result_type;
			res.lvalue = false;
		}
		return err::ok;
	}

	errvoid Expression::parse(Cursor& c, CompileValue& res, CompileType cpt, bool require_output, Type* request) {
		Compiler* compiler = Compiler::current();
		CompileValue val;
		if (!Expression::parse_or(c, val, cpt, request)) return err::fail;
		unsigned char op = 0;
		bool assign = false;
		switch (c.tok) {
			case RecognizedToken::Equals:
				if (!Operand::deref(val, cpt)) return err::fail;
			case RecognizedToken::ColonEquals: {

				if (!val.lvalue) {
					return throw_specific_error(c, "Left assignment must be modifiable lvalue");
				}

				if (require_output) {
					if (cpt == CompileType::compile) {
						if (!ILBuilder::build_duplicate(compiler->scope(), ILDataType::word)) return err::fail;
					}
					else {
						if (!ILBuilder::eval_duplicate(compiler->evaluator(), ILDataType::word)) return err::fail;
					}
				}

				c.move();
				Cursor err = c;

				CompileValue val2;
				if (!Expression::parse(c, val2, cpt)) return err::fail;
				if (!Operand::deref(val2, cpt)) return err::fail;
				if (!Operand::cast(err, val2, val.type, cpt, true)) return err::fail;
				if (!Expression::rvalue(val2, cpt)) return err::fail;
				if (!Expression::copy_from_rvalue_reverse(val.type, cpt)) return err::fail;

				if (!require_output) {
					val.lvalue = false;
					val.type = compiler->types()->t_void;
				}
			} break;
			case RecognizedToken::PlusEquals:
			case RecognizedToken::MinusEquals:
			case RecognizedToken::StarEquals:
			case RecognizedToken::SlashEquals: {
				if (!Operand::deref(val, cpt)) return err::fail;

				switch (c.tok) {
					case RecognizedToken::PlusEquals: op = 1; break;
					case RecognizedToken::MinusEquals: op = 2; break;
					case RecognizedToken::StarEquals: op = 3; break;
					case RecognizedToken::SlashEquals: op = 4; break;
					default: break;
				}

				if (!Operand::is_numeric_value(val.type)) {
					return throw_specific_error(c, "Left assignment must be numeric type");
				}

				if (!val.lvalue) {
					return throw_specific_error(c, "Left assignment must be modifiable lvalue");
				}

				if (require_output) {
					if (cpt == CompileType::compile) {
						if (!ILBuilder::build_duplicate(compiler->scope(), ILDataType::word)) return err::fail;
					}
					else {
						if (!ILBuilder::eval_duplicate(compiler->evaluator(), ILDataType::word)) return err::fail;
					}
				}

				if (cpt == CompileType::compile) {
					if (!ILBuilder::build_duplicate(compiler->scope(), ILDataType::word)) return err::fail;
				}
				else {
					if (!ILBuilder::eval_duplicate(compiler->evaluator(), ILDataType::word)) return err::fail;
				}
				if (!Expression::rvalue(val, cpt)) return err::fail;

				c.move();
				Cursor err = c;

				CompileValue val2;
				if (!Expression::parse(c, val2, cpt)) return err::fail;
				if (!Operand::deref(val2, cpt)) return err::fail;
				if (!Expression::rvalue(val2, cpt)) return err::fail;
				
				if (!Operand::is_numeric_value(val2.type)) {
					return throw_specific_error(err, "Expected numeric value");
				}

				if (cpt == CompileType::compile) {
					if (op == 1) {
						if (!ILBuilder::build_add(compiler->scope(), val.type->rvalue(), val2.type->rvalue())) return err::fail;
					}
					else if (op == 2) {
						if (!ILBuilder::build_sub(compiler->scope(), val.type->rvalue(), val2.type->rvalue())) return err::fail;
					}
					else if (op == 3) {
						if (!ILBuilder::build_mul(compiler->scope(), val.type->rvalue(), val2.type->rvalue())) return err::fail;
					}
					else {
						if (!ILBuilder::build_div(compiler->scope(), val.type->rvalue(), val2.type->rvalue())) return err::fail;
					}

					if (!ILBuilder::build_cast(compiler->scope(), ILBuilder::arith_result(val.type->rvalue(), val2.type->rvalue()), val.type->rvalue())) return err::fail;
					if (!ILBuilder::build_store_rev(compiler->scope(), val.type->rvalue())) return err::fail;
				}
				else {
					if (op == 1) {
						if (!ILBuilder::eval_add(compiler->evaluator(), val.type->rvalue(), val2.type->rvalue())) return err::fail;
					}
					else if (op == 2){
						if (!ILBuilder::eval_sub(compiler->evaluator(), val.type->rvalue(), val2.type->rvalue())) return err::fail;
					}
					else if (op == 3){
						if (!ILBuilder::eval_mul(compiler->evaluator(), val.type->rvalue(), val2.type->rvalue())) return err::fail;
					}
					else {
						if (!ILBuilder::eval_div(compiler->evaluator(), val.type->rvalue(), val2.type->rvalue())) return err::fail;
					}

					if (!ILBuilder::eval_cast(compiler->evaluator(), ILBuilder::arith_result(val.type->rvalue(), val2.type->rvalue()), val.type->rvalue())) return err::fail;
					if (!ILBuilder::eval_store_rev(compiler->evaluator(), val.type->rvalue())) return err::fail;
				}

				if (!require_output) {
					val.lvalue = false;
					val.type = compiler->types()->t_void;
				}
			} break;
			default: break;
		}

		res = val;
		if (!require_output && val.type != compiler->types()->t_void) {
			if (res.lvalue || res.type->rvalue_stacked()) {
				if (cpt == CompileType::compile) {
					if (!ILBuilder::build_forget(compiler->scope(), ILDataType::word)) return err::fail;
				}
				else {
					if (!ILBuilder::eval_forget(compiler->evaluator(), ILDataType::word)) return err::fail;
				}
			}
			else {
				if (cpt == CompileType::compile) {
					if (!ILBuilder::build_forget(compiler->scope(), res.type->rvalue())) return err::fail;
				}
				else {
					if (!ILBuilder::eval_forget(compiler->evaluator(), res.type->rvalue())) return err::fail;
				}
			}

			res.lvalue = false;
			res.type = compiler->types()->t_void;
		}
		return err::ok;
	}

	errvoid Expression::parse_and(Cursor& c, CompileValue& res, CompileType cpt, Type* request) {
		Compiler* compiler = Compiler::current();
		ILBlock* fallback = nullptr;
		CompileValue value;
		Cursor err = c;
		if (!Expression::parse_operators(c, value, cpt, request)) return err::fail;

		while (c.tok == RecognizedToken::DoubleAnd) {

			if (!Operand::deref(value, cpt)) return err::fail;
			if (!Operand::cast(err, value, compiler->types()->t_bool, cpt, true)) return err::fail;
			if (!Expression::rvalue(value, cpt)) return err::fail;

			c.move();

			if (cpt == CompileType::eval) {
				std::uint8_t v;
				if (!compiler->evaluator()->pop_register_value<std::uint8_t>(v)) return err::fail;

				CompileValue right;
				err = c;
				if (!Expression::parse_operators(c,right, cpt, request)) return err::fail;
				if (!Operand::deref(right, cpt)) return err::fail;
				if (!Operand::cast(err, right, compiler->types()->t_bool, cpt, true)) return err::fail;
				if (!Expression::rvalue(right, cpt)) return err::fail;
				
				std::uint8_t rv;
				if (!compiler->evaluator()->pop_register_value<std::uint8_t>(rv)) return err::fail;
				if (!ILBuilder::eval_const_i8(compiler->evaluator(), v & rv)) return err::fail;
			}
			else {
				if (!fallback) {
					fallback = compiler->target()->create_block();
				}

				if (!Expression::rvalue(value, cpt)) return err::fail;

				ILBlock* positive_block = compiler->target()->create_block();
				compiler->target()->append_block(positive_block);

				if (!ILBuilder::build_discard(positive_block, ILDataType::i8)) return err::fail;

				if (!ILBuilder::build_const_i8(compiler->scope(), false)) return err::fail;
				if (!ILBuilder::build_yield(compiler->scope(), ILDataType::i8)) return err::fail;

				if (!ILBuilder::build_jmpz(compiler->scope(), fallback, positive_block)) return err::fail;

				compiler->pop_scope();
				compiler->push_scope(positive_block);

				err = c;
				if (!Expression::parse_operators(c,value, cpt, request)) return err::fail;

			}

		}

		if (fallback != nullptr && cpt == CompileType::compile) {

			if (!Operand::deref(value, cpt)) return err::fail;
			if (!Operand::cast(err, value, compiler->types()->t_bool, cpt, true)) return err::fail;

			if (!Expression::rvalue(value, cpt)) return err::fail;
			if (!ILBuilder::build_yield(compiler->scope(), ILDataType::i8)) return err::fail;
			if (!ILBuilder::build_jmp(compiler->scope(), fallback)) return err::fail;

			compiler->target()->append_block(fallback);
			compiler->pop_scope();
			compiler->push_scope(fallback);
			fallback = nullptr;

			if (!ILBuilder::build_accept(compiler->scope(), ILDataType::i8)) return err::fail;
			value.type = compiler->types()->t_bool;
			value.lvalue = false;
		}

		res = value;
		return err::ok;
	}



	errvoid Expression::parse_or(Cursor& c, CompileValue& res, CompileType cpt, Type* request) {
		Compiler* compiler = Compiler::current();
		ILBlock* fallback = nullptr;

		CompileValue value;
		Cursor err = c;
		if (!Expression::parse_and(c, value, cpt, request)) return err::fail;

		while (c.tok == RecognizedToken::DoubleOr) {
			if (!Operand::deref(value, cpt)) return err::fail;
			if (!Operand::cast(err, value, compiler->types()->t_bool, cpt, true)) return err::fail;
			if (!Expression::rvalue(value, cpt)) return err::fail;

			c.move();

			if (cpt == CompileType::eval) {
				std::uint8_t v;
				if (!compiler->evaluator()->pop_register_value<std::uint8_t>(v)) return err::fail;

				CompileValue right;
				if (!Expression::parse_and(c, right, cpt, request)) return err::fail;
				if (!Operand::deref(right, cpt)) return err::fail;
				if (!Operand::cast(err, right, compiler->types()->t_bool, cpt, true)) return err::fail;
				if (!Expression::rvalue(right, cpt)) return err::fail;

				std::uint8_t rv;
				if (!compiler->evaluator()->pop_register_value<std::uint8_t>(rv)) return err::fail;
				if (!ILBuilder::eval_const_i8(compiler->evaluator(), v | rv)) return err::fail;
			}
			else {
				if (!fallback) {
					fallback = compiler->target()->create_block();
				}

				if (!Expression::rvalue(value, cpt)) return err::fail;

				ILBlock* positive_block = compiler->target()->create_block();
				compiler->target()->append_block(positive_block);

				if (!ILBuilder::build_discard(positive_block, ILDataType::i8)) return err::fail;

				if (!ILBuilder::build_const_i8(compiler->scope(), true)) return err::fail;
				if (!ILBuilder::build_yield(compiler->scope(), ILDataType::i8)) return err::fail;

				if (!ILBuilder::build_jmpz(compiler->scope(), positive_block, fallback)) return err::fail;

				compiler->pop_scope();
				compiler->push_scope(positive_block);

				err = c;
				if (!Expression::parse_and(c, value, cpt, request)) return err::fail;
			}

		}

		if (fallback != nullptr && cpt == CompileType::compile) {

			if (!Operand::deref(value, cpt)) return err::fail;
			if (!Operand::cast(err, value, compiler->types()->t_bool, cpt, true)) return err::fail;

			if (!Expression::rvalue(value, cpt)) return err::fail;

			if (!ILBuilder::build_yield(compiler->scope(), ILDataType::i8)) return err::fail;
			if (!ILBuilder::build_jmp(compiler->scope(), fallback)) return err::fail;

			compiler->target()->append_block(fallback);
			compiler->pop_scope();
			compiler->push_scope(fallback);

			fallback = nullptr;

			if (!ILBuilder::build_accept(compiler->scope(), ILDataType::i8)) return err::fail;

			value.type = compiler->types()->t_bool;
			value.lvalue = false;
		}

		res = value;
		
		return err::ok;
	}

	errvoid Expression::parse_operators(Cursor& c, CompileValue& res, CompileType cpt, Type* request) {

		int op_type[5] = { -1 };
		Cursor op_cursors[5];
		CompileValue layer[5];
		memset(layer, 0, sizeof(layer));

		int current_layer = -1;

		while (true) {
			CompileValue value;
			Cursor err = c;
			if (!Operand::parse(c, value, cpt, false, request)) return err::fail;

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
				default: break;
			}

			if (op_v >= 0 || current_layer >= 0) {
				if (!Operand::deref(value, cpt)) return err::fail;
				if (!Expression::rvalue(value, cpt)) return err::fail;
			}

			for (int i = current_layer; i >= std::max(op_v, 0); i--) {

				if (i >= 0 && layer[i].type != nullptr) {
					CompileValue& left = layer[i];
					CompileValue& right = value;

					if (!Expression::emit(op_cursors[i], value, i, op_type[i], left, right, cpt, op_v, op_t)) return err::fail;
					layer[i].type = nullptr;
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
				return err::ok;
			}
		}

		
		return err::ok;
	}
}