#include "Operand.h"
#include "Error.h"
#include "Utilities.h"
#include "PredefinedTypes.h"
#include "Expression.h"
#include "StackManager.h"

#include <iostream>

namespace Corrosive {

	bool Operand::cast(Cursor& err, CompileContext& ctx, CompileValue& res,Type*& to, CompileType copm_type) {
		if (res.t != to) {
			throw_cannot_cast_error(err, res.t, to);
			return false;
		}
		return true;
	}

	bool Operand::parse(Cursor& c, CompileContext& ctx, CompileValue& res, CompileType cpt) {
		CompileValue ret;

		ret.lvalue = false;
		ret.t = nullptr;

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
			if (!Expression::rvalue(ctx, value, cpt)) return false;

			if (value.t != ctx.default_types->t_type) {
				throw_specific_error(err, "operator expected to recieve type");
				return false;
			}

			if (cpt == CompileType::eval) {
				Type* t = ctx.eval->pop_register_value<Type*>();
				for(unsigned int tr = 0;tr< type_ref_count;tr++)
					t = t->generate_reference();
				ILBuilder::eval_const_type(ctx.eval, t);
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
			case RecognizedToken::Arrow: {
					c.move();
					Cursor err = c;
					CompileValue value;
					if (!Operand::parse(c, ctx, value, cpt)) return false;
					if (!value.lvalue) {
						throw_specific_error(err, "Target is not lvalue, and cannot be referenced");
						return false;
					}

					res.lvalue = false;
					res.t = value.t->generate_reference();
					return true;
				}
			case RecognizedToken::BackArrow: {
					c.move();
					Cursor err = c;
					CompileValue value;
					if (!Operand::parse(c, ctx, value, cpt)) return false;
					if (!Expression::rvalue(ctx, value, cpt)) return false;
					if (value.t->type() != TypeInstanceType::type_reference) {
						throw_specific_error(err, "Target is not reference, and cannot be dereferenced");
						return false;
					}

					res.lvalue = true;
					res.t = ((TypeReference*)value.t)->owner;
					return true;
				}
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
				case RecognizedToken::BackArrow: {
						if (!ret.lvalue) {
							throw_specific_error(c, "Target is not lvalue, and cannot be referenced");
							return false;
						}
						c.move();

						ret.t = ret.t->generate_reference();
						ret.lvalue = false;
					}break;
				case RecognizedToken::Arrow: {
						if (!Expression::rvalue(ctx, ret, cpt)) return false;
						if (ret.t->type() != TypeInstanceType::type_reference) {
							throw_specific_error(c, "Target is not reference, and cannot be dereferenced");
							return false;
						}
						c.move();

						ret.t = ((TypeReference*)ret.t)->owner;
						ret.lvalue = true;
					}break;
				case RecognizedToken::OpenParenthesis: {
						if (!parse_call_operator(ret, c, ctx, cpt)) return false;
					}break;
				case RecognizedToken::OpenBracket: {
						if (!parse_array_operator(ret, c, ctx, cpt)) return false;
					}break;
				case RecognizedToken::Dot: {
						if (!parse_dot_operator(ret, c, ctx, cpt)) return false;
					}break;
				case RecognizedToken::DoubleColon: {
						if (!parse_double_colon_operator(ret, c, ctx, cpt)) return false;
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
		else {
			
			StackItem* sitm;

			if (cpt == CompileType::compile && (sitm = StackManager::stack_find<0>(c.buffer))) {
				
				ILBuilder::build_local(ctx.block, sitm->ir_local);
				ret = sitm->value;
				ret.lvalue = true;
				
				c.move();
			}
			else if (cpt != CompileType::compile && (sitm = StackManager::stack_find<1>(c.buffer))) {
				if (cpt == CompileType::eval) {
					ILBuilder::eval_local(ctx.eval, sitm->ir_local);
				}

				ret = sitm->value;
				ret.lvalue = true;
				c.move();
			}
			else {
				Namespace* namespace_inst = nullptr;
				StructureTemplate* struct_inst = nullptr;
				

				Cursor err = c;

				ctx.inside->find_name(c.buffer, namespace_inst, struct_inst);

				if (namespace_inst == nullptr && struct_inst == nullptr) {
					throw_specific_error(c, "Path start point not found");
					return false;
				}
				

				Cursor nm_err = c;
				c.move();
				while (c.tok == RecognizedToken::DoubleColon && namespace_inst!=nullptr) {
					c.move();
					if (c.tok != RecognizedToken::Symbol)
					{
						throw_not_a_name_error(c);
						return false;
					}
					nm_err = c;

					if (cpt != CompileType::short_circuit) {
						auto f = namespace_inst->subnamespaces.find(c.buffer);
						if (f != namespace_inst->subnamespaces.end()) {
							namespace_inst = f->second.get();
						}
						else {
							auto f2 = namespace_inst->subtemplates.find(c.buffer);
							if (f2 != namespace_inst->subtemplates.end()) {
								struct_inst = f2->second.get();
								namespace_inst = nullptr;
							}
							else {

								throw_specific_error(c, "Namespace path is not valid");
								return false;
							}
						}
					}
					c.move();
				}


				if (struct_inst) {
					if (cpt != CompileType::short_circuit) {
						if (!struct_inst->compile(ctx)) return false;


						if (struct_inst->is_generic) {
							if (cpt == CompileType::eval) {
								if (!ILBuilder::eval_const_type(ctx.eval, struct_inst->type.get())) return false;
							}
							else if (cpt == CompileType::compile) {
								if (!ctx.function->is_const) {
									throw_specific_error(err, "Use of a type in runtime context");
									return false;
								}

								if (!ILBuilder::build_const_type(ctx.block, struct_inst->type.get())) return false;
							}
						}
						else {
							StructureInstance* inst;
							if (!struct_inst->generate(ctx, nullptr, inst)) return false;

							if (cpt == CompileType::eval) {
								if (!ILBuilder::eval_const_type(ctx.eval, inst->type.get())) return false;
							}
							else if (cpt == CompileType::compile) {
								if (!ctx.function->is_const) {
									throw_specific_error(err, "Use of a type in runtime context");
									return false;
								}
								if (!ILBuilder::build_const_type(ctx.block, inst->type.get())) return false;
							}
						}
					}
				}
				else {
					throw_specific_error(nm_err, "Path is pointing to a namespace");
					return false;
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
				TypeStructure* dt = nullptr;

				if (cpt != CompileType::short_circuit) {
					dt = ctx.eval->pop_register_value<TypeStructure*>();

					if (dt->type() != TypeInstanceType::type_template) {
						throw_specific_error(c, "this type is not a generic type");
						return false;
					}
				}

				c.move();
				
				std::vector<std::tuple<Cursor, Type*>>::iterator layout;
				
				if (cpt != CompileType::short_circuit)
					layout = dt->owner->generic_layout.begin();

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
					StructureTemplate* generating = dt->owner;
					
					
					if (generating->template_parent != nullptr) {
						unsigned char* key_ptr = (unsigned char*)generating->template_parent->key;
						for (auto key_l = generating->template_parent->generator->generic_layout.rbegin(); key_l != generating->template_parent->generator->generic_layout.rend(); key_l++) {
							ctx.eval->stack_push_pointer(key_ptr);
							key_ptr += std::get<1>(*key_l)->size(ctx);
							CompileValue argval;
							argval.lvalue = true;
							argval.t = std::get<1>(*key_l);

							StackManager::stack_push<1>(std::get<0>(*key_l).buffer, argval, (unsigned int)StackManager::stack_state<1>());
						}
					}

					auto act_layout = generating->generic_layout.rbegin();

					for (size_t arg_i = results.size() - 1; arg_i >= 0 && arg_i < results.size(); arg_i--) {

						CompileValue res = results[arg_i];

						unsigned char* data_place = ctx.eval->stack_reserve(res.t->size(ctx));
						StackManager::stack_push<1>(std::get<0>(*act_layout).buffer, res, (unsigned int)StackManager::stack_state<1>());

						bool stacked = res.t->rvalue_stacked();
						if (stacked) {
							res.t->move(ctx, ctx.eval->map(ctx.eval->pop_register_value_ilptr()), data_place);
						}
						else {
							res.t->move(ctx, ctx.eval->read_last_register_value_indirect(res.t->rvalue), data_place);
							ctx.eval->discard_last_register_type(res.t->rvalue);
						}


						act_layout++;
					}

					StructureInstance* inst = nullptr;
					if (!generating->generate(ctx, sp, inst)) return false;

					ILBuilder::eval_const_type(ctx.eval, inst->type.get());
					
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

		if (ret.t == ctx.default_types->t_type) {
			c.move();
			Cursor err = c;
			Expression::rvalue(ctx, ret, cpt);

			if (cpt != CompileType::short_circuit) {
				Type* t = ctx.eval->pop_register_value<Type*>();

				CompileValue res;
				if (!Expression::parse(c, ctx, res, cpt)) return false;
				if (!Operand::cast(err, ctx, res, ctx.default_types->t_u32, cpt)) return false;
				uint32_t c = ctx.eval->pop_register_value<uint32_t>();
				TypeArray* nt = t->generate_array(c);
				ILBuilder::eval_const_type(ctx.eval, nt);
			}

			ret.t = ctx.default_types->t_type;
			ret.lvalue = false;
		}
		else {
			throw_specific_error(c, "Not yet implemented");
			return false;
		}

		if (c.tok != RecognizedToken::CloseBracket) {
			throw_wrong_token_error(c, "']'");
			return false;
		}
		c.move();

		return true;
	}


	bool Operand::parse_double_colon_operator(CompileValue& ret, Cursor& c, CompileContext& ctx, CompileType cpt) {
		c.move();

		if (cpt == CompileType::compile) {
			throw_specific_error(c, "Operation :: is not supported in runtime context");
			return false;
		}

		if (ret.t != ctx.default_types->t_type) {
			throw_specific_error(c, "left operator is not a type instance");
			return false;
		}

		if (cpt == CompileType::eval) {
			Expression::rvalue(ctx, ret, cpt);
			TypeInstance* ti = ctx.eval->pop_register_value<TypeInstance*>();
			if (ti->type() != TypeInstanceType::type_instance) {
				throw_specific_error(c, "Type is not structure instance");
				return false;
			}

			StructureInstance* struct_inst = ti->owner;

			auto f = struct_inst->subtemplates.find(c.buffer);
			if (f != struct_inst->subtemplates.end()) {
				StructureTemplate* tplt = f->second.get();
				if (!tplt->compile(ctx)) return false;

				ret.lvalue = false;
				if (tplt->is_generic) {
					ILBuilder::eval_const_type(ctx.eval, tplt->type.get());
				}
				else {
					StructureInstance* inst = nullptr;

					if (tplt->template_parent!=nullptr) {
						auto ss = std::move(StackManager::move_stack_out<1>());
						auto sp = ctx.eval->stack_push();

						tplt->template_parent->insert_key_on_stack(ctx);

						if (!tplt->generate(ctx, sp, inst)) return false;
						ILBuilder::eval_const_type(ctx.eval, inst->type.get());

						//DO NOT COLLECT STACK THERE, ALL THE VARIABLES ARE ALREADY REGISTERED KEY

						ctx.eval->stack_pop(sp);
						StackManager::move_stack_in<1>(std::move(ss));
					}
					else {
						if (!tplt->generate(ctx, nullptr, inst)) return false;
						ILBuilder::eval_const_type(ctx.eval, inst->type.get());
					}


				}
			}
			else {
				throw_specific_error(c, "Structure instance does not contain a structure with this name");
				return false;
			}
		}

		ret.lvalue = false;
		ret.t = ctx.default_types->t_type;

		c.move();

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
		if (ret.t->type() == TypeInstanceType::type_reference) {

			ret.t = ((TypeReference*)ret.t)->owner;
			ret.lvalue = true;

			if (cpt == CompileType::compile) {
				ILBuilder::build_load(ctx.block, ret.t->rvalue);
			}
			else if (cpt == CompileType::eval) {
				ILBuilder::eval_load(ctx.eval, ret.t->rvalue);
			}
		}

		if (ret.t->type() == TypeInstanceType::type_instance) {
			if (!ret.lvalue) {
				throw_specific_error(c, "Left side of operator must be lvalue");
				return false;
			}

			c.move();

			TypeInstance* ti = (TypeInstance*)ret.t;
			StructureInstance* si = ti->owner;
			auto table_element = si->member_table.find(c.buffer);
			if (table_element != si->member_table.end()) {
				size_t id = table_element->second;
				auto& member = si->member_vars[id];
				if (cpt == CompileType::compile) {
					ILBuilder::build_member(ctx.block, member.offset);
				}
				else if (cpt == CompileType::eval) {
					ILBuilder::eval_member(ctx.eval, member.offset);
				}

				ret.lvalue = true;
				ret.t = member.type;
				c.move();
			}
			else {
				throw_specific_error(c, "Instance does not contain member with this name");
				return false;
			}
		}
		else {
			throw_specific_error(c, "Operator cannot be used on this type");
			return false;
		}

		
		return true;
	}
}