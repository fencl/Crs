#include "Operand.hpp"
#include "Error.hpp"
#include "BuiltIn.hpp"
#include "Expression.hpp"
#include "StackManager.hpp"
#include "ConstantManager.hpp"
#include <iostream>
#include <cstring>

namespace Corrosive {


	errvoid Operand::deref(CompileValue& val, CompileType cpt) {
		if (!val.reflock && val.type->type() == TypeInstanceType::type_reference && ((TypeReference*)val.type)->owner != Compiler::current()->types()->t_void) {
			if (!Expression::rvalue(val, cpt)) return err::fail;
			val.lvalue = true;
			val.type = ((TypeReference*)val.type)->owner;	
		}

		val.reflock = false;

		return err::ok;
	}

	errvoid Operand::type_template_cast_crsr(ILEvaluator* eval, Cursor& err) {
		Compiler* compiler = Compiler::current();
		Type* template_cast;
		if (!eval->pop_register_value<Type*>(template_cast)) return err::fail;
		if (!Type::assert(err,template_cast)) return err::fail;
		Type* template_type; 
		if (!eval->read_register_value<Type*>(template_type)) return err::fail;
		if (!Type::assert(err,template_type)) return err::fail;

		if (template_type->type() == TypeInstanceType::type_structure_template) {
			TypeStructureTemplate* st = (TypeStructureTemplate*)template_type;
			TypeTemplate* tt = (TypeTemplate*)template_cast;

			if (st->owner->generic_ctx.generic_layout.size() != compiler->types()->argument_array_storage.get(tt->argument_array_id).size()) {
				return throw_cannot_cast_error(err, template_type, template_cast);
			}
			else {
				for (std::size_t i = 0; i < st->owner->generic_ctx.generic_layout.size(); i++) {
					if (std::get<1>(st->owner->generic_ctx.generic_layout[i]) != compiler->types()->argument_array_storage.get(tt->argument_array_id)[i]) {
						return throw_cannot_cast_error(err, template_type, template_cast);
					}
				}
			}
		}
		else if (template_type->type() == TypeInstanceType::type_trait_template) {
			TypeTraitTemplate* st = (TypeTraitTemplate*)template_type;
			TypeTemplate* tt = (TypeTemplate*)template_cast;

			if (st->owner->generic_ctx.generic_layout.size() != compiler->types()->argument_array_storage.get(tt->argument_array_id).size()) {
				return throw_cannot_cast_error(err, template_type, template_cast);
			}
			else {
				for (std::size_t i = 0; i < st->owner->generic_ctx.generic_layout.size(); i++) {
					if (std::get<1>(st->owner->generic_ctx.generic_layout[i]) != compiler->types()->argument_array_storage.get(tt->argument_array_id)[i]) {
						return throw_cannot_cast_error(err, template_type, template_cast);
					}
				}
			}
		}
		else {
			return throw_cannot_cast_error(err, template_type, template_cast);
		}

		return err::ok;
	}

	errvoid Operand::type_template_cast(ILEvaluator* eval) {
		Compiler* compiler = Compiler::current();
		Type* template_cast;
		if (!eval->pop_register_value<Type*>(template_cast)) return err::fail;
		if (!Type::assert(template_cast)) return err::fail;
		Type* template_type;
		if (!eval->read_register_value<Type*>(template_type)) return err::fail;
		if (!Type::assert(template_type)) return err::fail;

		if (template_type->type() == TypeInstanceType::type_structure_template) {
			TypeStructureTemplate* st = (TypeStructureTemplate*)template_type;
			TypeTemplate* tt = (TypeTemplate*)template_cast;

			if (st->owner->generic_ctx.generic_layout.size() != compiler->types()->argument_array_storage.get(tt->argument_array_id).size()) {
				return throw_runtime_exception(eval, "Template cannot be casted to this generic type");
			}
			else {
				for (std::size_t i = 0; i < st->owner->generic_ctx.generic_layout.size(); i++) {
					if (std::get<1>(st->owner->generic_ctx.generic_layout[i]) != compiler->types()->argument_array_storage.get(tt->argument_array_id)[i]) {
						return throw_runtime_exception(eval, "Template cannot be casted to this generic type");
					}
				}
			}
		} else if (template_type->type() == TypeInstanceType::type_function_template) {
			TypeFunctionTemplate* st = (TypeFunctionTemplate*)template_type;
			TypeTemplate* tt = (TypeTemplate*)template_cast;

			if (st->owner->generic_ctx.generic_layout.size() != compiler->types()->argument_array_storage.get(tt->argument_array_id).size()) {
				return throw_runtime_exception(eval, "Template cannot be casted to this generic type");
			}
			else {
				for (std::size_t i = 0; i < st->owner->generic_ctx.generic_layout.size(); i++) {
					if (std::get<1>(st->owner->generic_ctx.generic_layout[i]) != compiler->types()->argument_array_storage.get(tt->argument_array_id)[i]) {
						return throw_runtime_exception(eval, "Template cannot be casted to this generic type");
					}
				}
			}
		}
		else if (template_type->type() == TypeInstanceType::type_trait_template) {
			TypeTraitTemplate* st = (TypeTraitTemplate*)template_type;
			TypeTemplate* tt = (TypeTemplate*)template_cast;

			if (st->owner->generic_ctx.generic_layout.size() != compiler->types()->argument_array_storage.get(tt->argument_array_id).size()) {
				return throw_runtime_exception(eval, "Template cannot be casted to this generic type");
			}
			else {
				for (std::size_t i = 0; i < st->owner->generic_ctx.generic_layout.size(); i++) {
					if (std::get<1>(st->owner->generic_ctx.generic_layout[i]) != compiler->types()->argument_array_storage.get(tt->argument_array_id)[i]) {
						return throw_runtime_exception(eval, "Template cannot be casted to this generic type");
					}
				}
			}
		}
		else {
			return throw_runtime_exception(eval, "Template cannot be casted to this generic type");
		}

		
		return err::ok;
	}

	bool Operand::is_numeric_value(Type* t) {
		return t->type()==TypeInstanceType::type_structure_instance && t!=Compiler::current()->types()->t_ptr && t!= Compiler::current()->types()->t_bool && ((TypeStructureInstance*)t)->owner->structure_type==StructureInstanceType::primitive_structure && !t->rvalue_stacked() && t->rvalue() < ILDataType::none;
	}

	errvoid Operand::cast(Cursor& err, CompileValue& res, Type*& to, CompileType cpt, bool implicit) {
		Compiler* compiler = Compiler::current();
		
		if (res.type == compiler->types()->t_type && to->type() == TypeInstanceType::type_template) {
			if (cpt == CompileType::eval) {
				if (!ILBuilder::eval_const_word(compiler->evaluator(), to)) return err::fail;
				if (!Operand::type_template_cast_crsr(compiler->evaluator(), err)) return err::fail;

				res.type = to;

			}
			else {
				if (!ILBuilder::build_const_word(compiler->scope(), to)) return err::fail;
				if (!ILBuilder::build_insintric(compiler->scope(), ILInsintric::type_dynamic_cast)) return err::fail;
				res.type = to;
			}
		}
		else if (to == compiler->types()->t_type && res.type->type() == TypeInstanceType::type_template) {
			res.type = to;
		}
		else if ((res.type->type() == TypeInstanceType::type_reference || (res.type->type() == TypeInstanceType::type_structure_instance && res.lvalue)) && to->type() == TypeInstanceType::type_trait) {

			TypeTraitInstance* tt = (TypeTraitInstance*)to;

			TypeStructureInstance* ts = nullptr;
			if (res.type->type() == TypeInstanceType::type_reference) {
				TypeReference* tr = (TypeReference*)res.type;
				if (tr->owner->type() == TypeInstanceType::type_structure_instance) {
					ts = (TypeStructureInstance*)tr->owner;
					if (!Expression::rvalue(res, cpt)) return err::fail;
				}
				else {
					return throw_cannot_cast_error(err, res.type, to);

				}
			}
			else {
				ts = (TypeStructureInstance*)res.type;
			}


			if (!ts->compile()) return err::fail;

			auto tti = ts->owner->traitfunctions.find(tt->owner);
			if (tti == ts->owner->traitfunctions.end()) {
				return throw_cannot_cast_error(err, res.type, to);
			}

			if (!tt->compile()) return err::fail;

			std::uint32_t vtableid = 0;
			auto vtbl = tt->owner->vtable_instances.find(ts->owner);
			if (vtbl == tt->owner->vtable_instances.end()) {
				if (!tt->owner->generate_vtable(ts->owner, vtableid)) return err::fail;
			}
			else {
				vtableid = vtbl->second;
			}

			if (cpt == CompileType::eval) {

				if (!ILBuilder::eval_vtable(compiler->evaluator(), vtableid)) return err::fail;
				if (!ILBuilder::eval_combine_dword(compiler->evaluator())) return err::fail;

			}
			else {

				if (!ILBuilder::build_vtable(compiler->scope(), vtableid)) return err::fail;
				if (!ILBuilder::build_combine_dword(compiler->scope())) return err::fail;

			}
			res.lvalue = false;
			res.type = to;
		}
		else if (to->type() == TypeInstanceType::type_reference && ((TypeReference*)to)->owner == res.type) {
			TypeReference* tr = (TypeReference*)to;
			if (tr->owner != res.type) {
				return throw_cannot_cast_error(err, res.type, to);
			}

			if (!res.lvalue) {
				return throw_cannot_cast_error(err, res.type, to);
			}

			res.type = to;
			res.lvalue = false;
		}
		else if (res.type->type() == TypeInstanceType::type_array && to->type() == TypeInstanceType::type_slice && ((TypeArray*)res.type)->owner == ((TypeSlice*)to)->owner) {
			if (cpt == CompileType::compile) {
				if (!ILBuilder::build_const_size(compiler->scope(), res.type->size())) return err::fail;
				if (!ILBuilder::build_combine_dword(compiler->scope())) return err::fail;
			}
			else {
				if (!ILBuilder::eval_const_size(compiler->evaluator(), res.type->size().eval(compiler->global_module()))) return err::fail;
				if (!ILBuilder::eval_combine_dword(compiler->evaluator())) return err::fail;
			}

			res.type = to;
			res.lvalue = false;
		}
		else if (res.type->type() == TypeInstanceType::type_slice && to->type() == TypeInstanceType::type_array && ((TypeArray*)res.type)->owner == ((TypeSlice*)to)->owner) {
			if (!to->compile()) return err::fail;
			if (cpt == CompileType::compile) {
				stackid_t local_id = compiler->target()->local_stack_lifetime.append(to->size());
				compiler->temp_stack()->push_item("$tmp", to, local_id);

				if (!ILBuilder::build_bitcast(compiler->scope(), ILDataType::dword, ILDataType::word)) return err::fail;
				if (!ILBuilder::build_local(compiler->scope(), local_id)) return err::fail;
				if (!ILBuilder::build_memcpy(compiler->scope(), to->size())) return err::fail;
				if (!ILBuilder::build_local(compiler->scope(), local_id)) return err::fail;
			}
			else {
				stackid_t local_id = compiler->push_local(to->size());
				compiler->compiler_stack()->push_item("$tmp", to, local_id);
				dword_t dw;
				if (!compiler->evaluator()->read_register_value<dword_t>(dw)) return err::fail;
				std::size_t slice_size = (std::size_t)dw.p2;
				std::size_t array_size = to->size().eval(compiler->global_module());
				if (slice_size != array_size) {
					return throw_specific_error(err, "The array has different size than casted slice");
				}

				if (!ILBuilder::eval_bitcast(compiler->evaluator(), ILDataType::dword, ILDataType::word)) return err::fail;
				compiler->eval_local(local_id);
				if (!ILBuilder::eval_memcpy(compiler->evaluator(), array_size)) return err::fail;
				compiler->eval_local(local_id);
			}

			res.type = to;
			res.lvalue = false;
		}
		else if (Operand::is_numeric_value(res.type) && to == compiler->types()->t_bool) {
			if (!Expression::rvalue(res, cpt)) return err::fail;
			if (cpt == CompileType::eval) {
				if (res.type->rvalue() != to->rvalue()) {
					if (!ILBuilder::eval_bitcast(compiler->evaluator(), res.type->rvalue(), to->rvalue())) return err::fail;
				}
				

				res.type = to;
				res.lvalue = false;
			}
			else {
				if (res.type->rvalue() != to->rvalue()) {
					if (!ILBuilder::build_bitcast(compiler->scope(), res.type->rvalue(), to->rvalue())) return err::fail;
				}

				res.type = to;
				res.lvalue = false;
			}
		}
		else if (Operand::is_numeric_value(res.type) && Operand::is_numeric_value(to)) {
			if (!Expression::rvalue(res, cpt)) return err::fail;
			if (cpt == CompileType::eval) {
				if (res.type->rvalue() != to->rvalue()) {
					if (!ILBuilder::eval_cast(compiler->evaluator(), res.type->rvalue(), to->rvalue())) return err::fail;
				}

				res.type = to;
				res.lvalue = false;

			}
			else {
				if (res.type->rvalue() != to->rvalue()) {
					if (!ILBuilder::build_cast(compiler->scope(), res.type->rvalue(), to->rvalue())) return err::fail;
				}

				res.type = to;
				res.lvalue = false;

			}
		}
		else if (res.type != to) {
			return throw_cannot_cast_error(err, res.type, to);
			return err::fail;
		}

		res.type = to;
		return err::ok;
	}

	errvoid Operand::parse(Cursor& c, CompileValue& res, CompileType cpt, bool targets_defer, Type* request) {
		Compiler* compiler = Compiler::current();
		res.lvalue = false;
		res.reflock = false;
		res.type = nullptr;

		switch (c.tok) {
			case RecognizedToken::And:
			case RecognizedToken::DoubleAnd: {
				if (!Operand::parse_reference(res, c, cpt,targets_defer)) return err::fail;
				return err::ok;
			}
			case RecognizedToken::OpenBracket: {
				if (!Operand::parse_array_type(res, c, cpt,targets_defer)) return err::fail;
				return err::ok;
			}

			case RecognizedToken::OpenParenthesis: {
				if (!Operand::parse_expression(res, c, cpt, request)) return err::fail;
			}break;

			case RecognizedToken::OpenBrace: {
				if (!Operand::parse_const_decl(res, c, cpt, request)) return err::fail;
				return err::ok;
			}

			case RecognizedToken::Symbol: {
				if (c.buffer() == "cast") {
					c.move();
					if (c.tok != RecognizedToken::OpenParenthesis) {
						return throw_wrong_token_error(c, "'('");
					}
					c.move();

					Cursor err = c;
					CompileValue t_res;

					{
						auto state = ScopeState().context(ILContext::compile);

						if (!Expression::parse(c, t_res, CompileType::eval)) return err::fail;
						if (!Operand::deref(t_res, cpt)) return err::fail;
						if (!Expression::rvalue(t_res, CompileType::eval)) return err::fail;
						if (t_res.type != compiler->types()->t_type) {
							return throw_specific_error(err, "Expected type");
						}
					}

					Type* to;
					if (!compiler->evaluator()->pop_register_value<Type*>(to)) return err::fail;

					if (!Type::assert(err, to)) return err::fail;
					if (!to->compile()) return err::fail;

					if (to->context()==ILContext::compile && compiler->scope_context() != ILContext::compile) {
						return throw_specific_error(err,"Type cannot be used outside compile context");
					}

					if (to->context()==ILContext::runtime && compiler->scope_context() != ILContext::runtime) {
						return throw_specific_error(err,"Type cannot be used outside runtime context");
					}

					if (c.tok != RecognizedToken::CloseParenthesis) {
						return throw_wrong_token_error(c, "')'");
					}
					c.move();

					err = c;
					if (!Operand::parse(c, res, cpt,targets_defer, to)) return err::fail;
					if (!Operand::deref(res, cpt)) return err::fail;
					if (!Operand::cast(err, res, to, cpt, false)) return err::fail;
					res.reflock = true;

					return err::ok;
				} else if (c.buffer() == "bitcast") {
					c.move();
					if (c.tok != RecognizedToken::OpenParenthesis) {
						return throw_wrong_token_error(c, "'('");
					}
					c.move();

					Cursor err = c;
					CompileValue t_res;

					{
						auto state = ScopeState().context(ILContext::compile);

						if (!Expression::parse(c, t_res, CompileType::eval)) return err::fail;
						if (!Operand::deref(t_res, cpt)) return err::fail;
						if (!Expression::rvalue(t_res, CompileType::eval)) return err::fail;
						if (t_res.type != compiler->types()->t_type) {
							return throw_specific_error(err, "Expected type");
						}
					}

					Type* to;
					if (!compiler->evaluator()->pop_register_value<Type*>(to)) return err::fail;
					if (!Type::assert(err, to)) return err::fail;
					if (!to->compile()) return err::fail;

					if (to->context()==ILContext::compile && compiler->scope_context() != ILContext::compile) {
						return throw_specific_error(err,"Type cannot be used outside compile context");
					}

					if (to->context()==ILContext::runtime && compiler->scope_context() != ILContext::runtime) {
						return throw_specific_error(err,"Type cannot be used outside runtime context");
					}

					if (c.tok != RecognizedToken::CloseParenthesis) {
						return throw_wrong_token_error(c, "')'");
					}
					c.move();

					err = c;
					if (!Operand::parse(c, res, cpt, targets_defer)) return err::fail;
					if (!Operand::deref(res, cpt)) return err::fail;
					if (!Expression::rvalue(res, cpt)) return err::fail;

					if (res.type->rvalue_stacked() || to->rvalue_stacked()) {
						return throw_specific_error(err, "This type cannot be bitcasted (yet)");
					}

					if (cpt == CompileType::compile) {
						if (!ILBuilder::build_bitcast(compiler->scope(), res.type->rvalue(), to->rvalue())) return err::fail;
					}
					else {
						if (!ILBuilder::eval_bitcast(compiler->evaluator(), res.type->rvalue(), to->rvalue())) return err::fail;
					}

					res.type = to;
					res.lvalue = false;
					res.reflock = true;
					return err::ok;
				}
				else {
					if (!Operand::parse_symbol(res, c, cpt,targets_defer)) return err::fail;
				}
			}break;

			case RecognizedToken::Minus: {
				Cursor err = c;
				c.move();
				if (!Expression::parse(c, res, cpt)) return err::fail;
				if (!Operand::deref(res, cpt)) return err::fail;
				if (!Expression::rvalue(res, cpt)) return err::fail;

				if (!Operand::is_numeric_value(res.type)) {
					return throw_specific_error(err, "Operation requires number operand");
				}

				if (cpt == CompileType::compile) {
					if (!ILBuilder::build_negative(compiler->scope(), res.type->rvalue())) return err::fail;
				}
				else {
					if (!ILBuilder::eval_negative(compiler->evaluator(), res.type->rvalue())) return err::fail;
				}
			}return err::ok;

			case RecognizedToken::ExclamationMark: {
				c.move();
				Cursor err = c;
				if(!Expression::parse(c, res, cpt)) return err::fail;
				if(!Operand::deref(res, cpt)) return err::fail;
				if(!Expression::rvalue(res, cpt)) return err::fail;
				if(!Operand::cast(err, res, compiler->types()->t_bool, cpt, true)) return err::fail;

				if (cpt == CompileType::compile) {
					if (!ILBuilder::build_negate(compiler->scope())) return err::fail;
				}
				else {
					if (!ILBuilder::eval_negate(compiler->evaluator())) return err::fail;
				}
			}return err::ok;

			case RecognizedToken::Number:
			case RecognizedToken::UnsignedNumber: {
				if (!Operand::parse_number(res, c, cpt)) return err::fail;
			}break;

			case RecognizedToken::LongNumber:
			case RecognizedToken::UnsignedLongNumber: {
				if (!Operand::parse_long_number(res, c, cpt)) return err::fail;
			}break;

			case RecognizedToken::FloatNumber:
			case RecognizedToken::DoubleNumber: {
				if(!Operand::parse_float_number(res, c, cpt)) return err::fail;
			}break;

			case RecognizedToken::String: {
				if (!Operand::parse_string_literal(res, c, cpt)) return err::fail;
			}break;


			default: {
				return throw_specific_error(c, "Expected to parse operand");
			} break;
		}

		while (true) {
			switch (c.tok) {
				case RecognizedToken::Symbol: {
					if (c.buffer() == "cast") {
						//if (!Expression::rvalue(ret, cpt)) return err::fail;

						c.move();
						if (c.tok != RecognizedToken::OpenParenthesis) {
							return throw_wrong_token_error(c, "'('");
						}
						c.move();



						Cursor err = c;

						CompileValue t_res;
						{
							auto state = ScopeState().context(ILContext::compile);
							if (!Expression::parse(c, t_res, CompileType::eval)) return err::fail;
							if (!Operand::deref(t_res, cpt)) return err::fail;
							if (!Expression::rvalue(t_res, CompileType::eval)) return err::fail;
						}

						if (t_res.type != compiler->types()->t_type) {
							return throw_specific_error(err, "Expected type");
						}

						Type* to;
						if (!compiler->evaluator()->pop_register_value<Type*>(to)) return err::fail;

						if (!Type::assert(err, to)) return err::fail;
						if (!to->compile()) return err::fail;

						if (to->context()==ILContext::compile && compiler->scope_context() != ILContext::compile) {
							return throw_specific_error(err,"Type cannot be used outside compile context");
						}

						if (to->context()==ILContext::runtime && compiler->scope_context	() != ILContext::runtime) {
							return throw_specific_error(err,"Type cannot be used outside runtime context");
						}

						if (c.tok != RecognizedToken::CloseParenthesis) {
							return throw_wrong_token_error(c, "')'");
						}
						c.move();


						if (!Operand::deref(res, cpt)) return err::fail;
						if (!Operand::cast(err, res, to, cpt, false)) return err::fail;

						res.reflock = true;
					}
					else if (c.buffer() == "bitcast") {
						c.move();
						if (c.tok != RecognizedToken::OpenParenthesis) {
							return throw_wrong_token_error(c, "'('");
						}
						c.move();

						Cursor err = c;
						CompileValue t_res;

						{
							auto state = ScopeState().context(ILContext::compile);

							if (!Expression::parse(c, t_res, CompileType::eval)) return err::fail;
							if (!Operand::deref(t_res, cpt)) return err::fail;
							if (!Expression::rvalue(t_res, CompileType::eval)) return err::fail;
							if (t_res.type != compiler->types()->t_type) {
								return throw_specific_error(err, "Expected type");
							}
						}

						Type* to;
						if (!compiler->evaluator()->pop_register_value<Type*>(to)) return err::fail;
						if (!Type::assert(err, to)) return err::fail;
						if (!to->compile()) return err::fail;

						if (to->context()==ILContext::compile && compiler->scope_context() != ILContext::compile) {
							return throw_specific_error(err,"Type cannot be used outside compile context");
						}

					if (to->context()==ILContext::runtime && compiler->scope_context() != ILContext::runtime) {
							return throw_specific_error(err,"Type cannot be used outside runtime context");
						}

						if (c.tok != RecognizedToken::CloseParenthesis) {
							return throw_wrong_token_error(c, "')'");
						}
						c.move();

						err = c;
						if (!Operand::deref(res, cpt)) return err::fail;
						if (!Expression::rvalue(res, cpt)) return err::fail;

						if (res.type->rvalue_stacked() || to->rvalue_stacked()) {
							return throw_specific_error(err, "This type cannot be bitcasted (yet)");
						}

						if (cpt == CompileType::compile) {
							if (!ILBuilder::build_bitcast(compiler->scope(), res.type->rvalue(), to->rvalue())) return err::fail;
						}
						else {
							if (!ILBuilder::eval_bitcast(compiler->evaluator(), res.type->rvalue(), to->rvalue())) return err::fail;
						}

						res.type = to;
						res.lvalue = false;
						res.reflock = true;
					}
					else goto break_while;
				}break;
				case RecognizedToken::OpenParenthesis: {
					if (!parse_call_operator(res, c, cpt, targets_defer)) return err::fail;
				}break;
				case RecognizedToken::OpenBracket: {
					if (!parse_array_operator(res, c, cpt)) return err::fail;
				}break;
				case RecognizedToken::Dot: {
					if (!parse_dot_operator(res, c, cpt, targets_defer)) return err::fail;
				}break;
				case RecognizedToken::DoubleColon: {
					if (!parse_double_colon_operator(res, c, cpt, targets_defer)) return err::fail;
				}break;
				default: goto break_while;
			}

			continue;
		break_while:
			break;
		}

		return err::ok;
	}

	errvoid Operand::parse_const_decl(CompileValue& ret, Cursor& c, CompileType cpt, Type* request) {
		if (request == nullptr) {
			return throw_specific_error(c,"Constant array had no requested type");
		} else {
			Compiler* compiler = Compiler::current();

			if (request->type() == TypeInstanceType::type_slice || request->type() == TypeInstanceType::type_array) {
				auto scope = ScopeState().context(ILContext::compile);
				Type* target;
				std::uint32_t req_count = 0;
				if (request->type() == TypeInstanceType::type_slice) {
					target = ((TypeSlice*)request)->owner;
				} else {
					target = ((TypeArray*)request)->owner;
					req_count = ((TypeArray*)request)->count;
				}

				ILSize target_size = target->size();
				std::size_t target_size_value = target_size.eval(compiler->global_module());
				std::string storage;
				std::uint32_t count = 0;
				Cursor err = c;
				c.move();
				if (c.tok != RecognizedToken::CloseBrace) {
					while (true) {
						Cursor err = c;
						CompileValue cv;
						if (!Expression::parse(c, cv, CompileType::eval, true, target)) return err::fail;
						if (!Operand::deref(cv, CompileType::eval)) return err::fail;
						if (!Operand::cast(err, cv, target, CompileType::eval, true)) return err::fail;
						if (!Expression::rvalue(cv, CompileType::eval)) return err::fail;

								
						storage.resize(storage.size() + target_size_value);

						if (cpt == CompileType::compile) {
							if (cv.type->rvalue_stacked()) {
								unsigned char* src;
								if (!compiler->evaluator()->pop_register_value<unsigned char*>(src)) return err::fail;
								if (!cv.type->constantize(err, (unsigned char*)&storage.data()[storage.size()-target_size_value], src)) return err::fail;
							} else {
								ilsize_t srcstorage;
								if (!compiler->evaluator()->pop_register_value_indirect(compiler->evaluator()->compile_time_register_size(cv.type->rvalue()), &srcstorage)) return err::fail;
								if (!cv.type->constantize(err, (unsigned char*)&storage.data()[storage.size()-target_size_value], (unsigned char*)&srcstorage)) return err::fail;
							}
						} else {
							if (cv.type->rvalue_stacked()) {
								unsigned char* src;
								if (!compiler->evaluator()->pop_register_value<unsigned char*>(src)) return err::fail;
								std::memcpy((unsigned char*)&storage.data()[storage.size()-target_size_value], src, target_size_value); // TODO wrap
							} else {
								ilsize_t srcstorage;
								if (!compiler->evaluator()->pop_register_value_indirect(compiler->evaluator()->compile_time_register_size(cv.type->rvalue()), &srcstorage)) return err::fail;
								std::memcpy((unsigned char*)&storage.data()[storage.size()-target_size_value], (unsigned char*)&srcstorage,target_size_value); // TODO wrap
							}
						}

						++count;

						if (c.tok == RecognizedToken::CloseBrace) {
							break;
						}else if (c.tok == RecognizedToken::Comma) {
							c.move();
						}else {
							return throw_wrong_token_error(c, "',' or '}'");
						}
					}
				}
				c.move();

				if (req_count > 0 && req_count < count) {
					return throw_specific_error(err,"Not enough elements");
				}

				if (req_count > 0 && req_count > count) {
					return throw_specific_error(err,"Too many elements");
				}

				ILSize s;
				if (target_size.type == ILSizeType::table || target_size.type == ILSizeType::array) {
					s.type = ILSizeType::array;
					s.value = compiler->global_module()->register_array_table(target_size,count);
				}else if (target_size.type == ILSizeType::_0) {
					s.type = ILSizeType::_0;
				}else{
					s = target_size;
					s.value *= count;
				}

				if (cpt == CompileType::compile) {
					auto cid = compiler->constant_manager()->register_constant(std::move(storage), s);
					if (request->type() == TypeInstanceType::type_slice) {
						if (!ILBuilder::build_const_slice(compiler->scope(), cid.second, s)) return err::fail;
					} else {
						if (!ILBuilder::build_constref(compiler->scope(), cid.second)) return err::fail;
					}
				} else {
					stackid_t local_id = compiler->push_local(s);
					compiler->compiler_stack()->push_item("$tmp", target->generate_array(count), local_id);
					unsigned char* dst = compiler->stack_ptr(local_id);
					memcpy(dst, storage.data(), storage.size()); // wrap
					
					if (request->type() == TypeInstanceType::type_slice) {
						dword_t v;
						v.p1 = dst;
						v.p2 = (void*)(target_size_value*count);
						if (!ILBuilder::eval_const_dword(compiler->evaluator(),v)) return err::fail;
					} else {
						if (!ILBuilder::eval_const_word(compiler->evaluator(), dst)) return err::fail;
					}
				}

				ret.type = request;
				ret.lvalue = false;
				ret.reflock = false;
			} else {
				return throw_specific_error(c,"Not yet implemented");
			}
		}

		return err::ok;
	}

	errvoid Operand::parse_const_type_function(Cursor& c, CompileValue& res) {
		Compiler* compiler = Compiler::current();
		auto state = ScopeState().context(ILContext::compile);
		Cursor err = c;
		FindNameResult found;

		StackItem sitm;
		if (compiler->compiler_stack()->find(c.buffer(), sitm)) {
			if (sitm.type == compiler->types()->t_type) {
				compiler->eval_local(sitm.id);
				if (!ILBuilder::eval_load(compiler->evaluator(), ILDataType::word)) return err::fail;
				Type* rec_type;
				if (!compiler->evaluator()->pop_register_value<Type*>(rec_type)) return err::fail;
				if (!Type::assert(err, rec_type)) return err::fail;


				if (rec_type->type() == TypeInstanceType::type_structure_instance) {
					c.move();
					if (c.tok != RecognizedToken::DoubleColon) {
						if (!ILBuilder::build_const_word(compiler->scope(), rec_type)) return err::fail;
						res.type = compiler->types()->t_type;
						res.lvalue = false;
						return err::ok;
					}
					c.move();

					found = ((TypeStructureInstance*)rec_type)->owner->find_name(c.buffer());
				}
				else if (rec_type->type() == TypeInstanceType::type_structure_template) {
					found = ((TypeStructureTemplate*)rec_type)->owner;
				}
				else if (rec_type->type() == TypeInstanceType::type_function_template) {
					found = ((TypeFunctionTemplate*)rec_type)->owner;
				}
				else {
					return throw_specific_error(c, "Expected structure type");
				}

				c.move();
			}
			else {
				std::uint8_t* source = compiler->stack_ptr(sitm.id);
				if (!sitm.type->constantize(c, nullptr, source)) return err::fail;

				res.type = sitm.type; res.lvalue = false;
				c.move();
				return err::ok;
			}
		}
		else {
			found = compiler->workspace()->find_name(c.buffer(),true);

			if (found.type() == FindNameResultType::None) {
				return throw_specific_error(c, "Path start point not found");
			}


			c.move();
			while (c.tok == RecognizedToken::DoubleColon && found.type() == FindNameResultType::Namespace) {
				c.move();
				if (c.tok != RecognizedToken::Symbol)
				{
					return throw_not_a_name_error(c);
				}
				err = c;

				found = found.get_namespace()->find_name(c.buffer());
				c.move();
			}
		}


		while (found.type() == FindNameResultType::Structure) {
			auto struct_inst = found.get_structure();
			if (!struct_inst->compile()) return err::fail;

			StructureInstance* inst;
			if (struct_inst->ast_node->is_generic) {
				if (c.tok != RecognizedToken::OpenParenthesis) {
					if (!ILBuilder::build_const_word(compiler->scope(), struct_inst->type.get())) return err::fail;
					res.type = compiler->types()->t_type;
					res.lvalue = false;
					return err::ok;
				}
				c.move();

				if (!Operand::parse_generate_template(c, struct_inst, inst)) return err::fail;
			}
			else {
				struct_inst->generate(nullptr, inst);
			}



			if (c.tok != RecognizedToken::DoubleColon) {
				if (!ILBuilder::build_const_word(compiler->scope(), inst->type.get())) return err::fail;
				res.type = compiler->types()->t_type;
				res.lvalue = false;
				return err::ok;
			}

			c.move();
			found = inst->find_name(c.buffer());
			if (found.type() == FindNameResultType::None) {
				return throw_specific_error(c, "This symbol is not a member of provided structure");
			}
			c.move();
		}


		if (found.type() == FindNameResultType::Function) {
			auto func_inst = found.get_function();
			if (!func_inst->compile()) return err::fail;

			if (!(func_inst->ast_node->has_body() && ((AstFunctionNode*)func_inst->ast_node)->is_generic)) {
				FunctionInstance* inst;
				if (!func_inst->generate(nullptr, inst)) return err::fail;
				if (!inst->compile()) return err::fail;


				if (!ILBuilder::build_fnptr(compiler->scope(), inst->func)) return err::fail;
				res.type = inst->type->function_type;
				res.lvalue = false;

				return err::ok;
			}
			else {
				FunctionInstance* inst;
				if (c.tok != RecognizedToken::OpenParenthesis) {
					if (!ILBuilder::build_const_word(compiler->scope(), func_inst->type.get())) return err::fail;
					res.type = compiler->types()->t_type;
					res.lvalue = false;
					return err::ok;
				}
				c.move();

				if (!Operand::parse_generate_template(c, func_inst, inst)) return err::fail;
				if (!inst->compile()) return err::fail;

				if (!ILBuilder::build_fnptr(compiler->scope(), inst->func)) return err::fail;
				res.type = inst->type->function_type;
				res.lvalue = false;
				return err::ok;
			}

		} else if (found.type() == FindNameResultType::Orphan) {
			auto func_inst = found.get_orphan();
			if (!func_inst->compile()) return err::fail;
			if (!ILBuilder::build_fnptr(compiler->scope(), func_inst->func)) return err::fail;
			res.type = func_inst->type->function_type;
			res.lvalue = false;
			return err::ok;
		}
		else if (found.type() == FindNameResultType::Static) {
			auto static_inst = found.get_static();
			if (!static_inst->compile()) return err::fail;

			if (!ILBuilder::build_staticref(compiler->scope(), static_inst->sid)) return err::fail;
			res.type = static_inst->type;
			res.lvalue = true;
		}
		else {
			return throw_specific_error(err, "Only function or static variable may be brought in the runtime context");
		}
		return err::ok;
	}



	errvoid Operand::parse_expression(CompileValue& ret, Cursor& c, CompileType cpt, Type* request) {
		c.move();
		if (!Expression::parse(c, ret, cpt, true, request)) return err::fail;
		if (c.tok != RecognizedToken::CloseParenthesis) {
			return throw_wrong_token_error(c, "')'");
		}
		c.move();
		return err::ok;
	}



	errvoid Operand::parse_symbol(CompileValue& ret, Cursor& c, CompileType cpt, bool targets_defer) {
		Compiler* compiler = Compiler::current();
		auto buf = c.buffer();

		if (buf == "true") {
			c.move();
			ret.lvalue = false;
			ret.type = compiler->types()->t_bool;
			if (cpt == CompileType::compile) {
				if (!ILBuilder::build_const_i8(compiler->scope(), true)) return err::fail;
			}
			else if (cpt == CompileType::eval) {
				if (!ILBuilder::eval_const_i8(compiler->evaluator(), true)) return err::fail;
			}
		}
		else if (buf == "false") {
			c.move();
			ret.lvalue = false;
			ret.type = compiler->types()->t_bool;
			if (cpt == CompileType::compile) {
				if (!ILBuilder::build_const_i8(compiler->scope(), false)) return err::fail;
			}
			else if (cpt == CompileType::eval) {
				if (!ILBuilder::eval_const_i8(compiler->evaluator(), false)) return err::fail;
			}
		}
		else if (buf == "self") {
			ret.lvalue = false;
			ret.type = compiler->types()->t_type;

			Namespace* nspc = compiler->workspace();
			StructureInstance* s = dynamic_cast<StructureInstance*>(nspc);

			if (!s) {
				return throw_specific_error(c, "Self must be used inside structure");
			}
			else {

				if (compiler->scope_context() != ILContext::compile) {
					return throw_specific_error(c, "Self type can be used only in compile context");
				}

				if (cpt == CompileType::compile) {
					if (!ILBuilder::build_const_word(compiler->scope(), s->type.get())) return err::fail;
				}
				else if (cpt == CompileType::eval) {
					if (!ILBuilder::eval_const_word(compiler->evaluator(), s->type.get())) return err::fail;
				}
			}

			c.move();
		}
		else if (buf == "null") {
			c.move();
			ret.lvalue = false;
			ret.type = compiler->types()->t_ptr;

			if (cpt == CompileType::compile) {
				if (!ILBuilder::build_null(compiler->scope())) return err::fail;
			}
			else if (cpt == CompileType::eval) {
				if (!ILBuilder::eval_null(compiler->evaluator())) return err::fail;
			}
		}
		else if (buf == "typesize") {
			c.move();
			auto state = ScopeState().context(ILContext::compile);

			CompileValue type_val;
			Cursor err = c;
			if (!Operand::parse(c, type_val, CompileType::eval, targets_defer)) return err::fail;
			if (!Expression::rvalue(type_val, CompileType::eval)) return err::fail;

			if (type_val.type != compiler->types()->t_type) {
				return throw_specific_error(err, "Expected type");
			}

			Type* tp;
			if (!compiler->evaluator()->pop_register_value<Type*>(tp)) return err::fail;

			if (!Type::assert(err, tp)) return err::fail;
			if (!tp->compile()) return err::fail;

			if (cpt == CompileType::compile) {
				if (!ILBuilder::build_const_size(compiler->scope(), tp->size())) return err::fail;
			}
			else {
				if (!ILBuilder::eval_const_size(compiler->evaluator(), tp->size().eval(compiler->global_module()))) return err::fail;
			}

			ret.lvalue = false;
			ret.type = compiler->types()->t_size;
		}
		else if (buf == "type") {

			c.move();
			if (c.tok != RecognizedToken::OpenParenthesis) {
				compiler->evaluator()->write_register_value(compiler->types()->t_type);
			}
			else {
				c.move();
				std::vector<Type*> arg_types;

				if (c.tok != RecognizedToken::CloseParenthesis) {
					while (true) {
						Cursor err = c;
						CompileValue arg;
						if (!Expression::parse(c, arg, CompileType::eval)) return err::fail;
						if (!Operand::deref(arg, cpt)) return err::fail;
						if (!Expression::rvalue(arg, CompileType::eval)) return err::fail;

						if (arg.type != compiler->types()->t_type) {
							return throw_specific_error(err, "Expected type");
						}

						Type* type;
						if (!compiler->evaluator()->pop_register_value<Type*>(type)) return err::fail;
						if (!Type::assert(err, type)) return err::fail;
						arg_types.push_back(type);

						if (c.tok == RecognizedToken::Comma) {
							c.move();
						}
						else if (c.tok == RecognizedToken::CloseParenthesis) {
							break;
						}
						else {
							return throw_wrong_token_error(c, "',' or ')'");
						}
					}
				}
				c.move();


				Type* ftype = compiler->types()->load_or_register_template_type(std::move(arg_types));

				compiler->evaluator()->write_register_value(ftype);
			}

			ret.lvalue = false;
			ret.type = compiler->types()->t_type;
		}
		else if (buf == "fn") {
			if (cpt == CompileType::compile) {
				if (compiler->scope_context() != ILContext::compile) {
					return throw_specific_error(c, "Function type cannot be created in runtime context");

				}
			}

			c.move();
			ILCallingConvention call_conv = ILCallingConvention::bytecode;
			ILContext t_context = ILContext::both;

			while (true)
			{
				auto buf = c.buffer();
				if (c.tok == RecognizedToken::Symbol && buf == "compile") {
					t_context = ILContext::compile;
					c.move();
				}
				else if (c.tok == RecognizedToken::Symbol && buf == "runtime") {
					t_context = ILContext::runtime;
					c.move();
				}
				else if (c.tok == RecognizedToken::Symbol && buf == "native") {
					call_conv = ILCallingConvention::native;
					c.move();
				}
				else if (c.tok == RecognizedToken::Symbol && buf == "stdcall") {
					call_conv = ILCallingConvention::stdcall;
					c.move();
				}
				else {
					break;
				}
			}

			if (c.tok != RecognizedToken::OpenParenthesis) {
				return throw_wrong_token_error(c, "(");
			}
			c.move();

			std::vector<Type*> arg_types;

			if (c.tok != RecognizedToken::CloseParenthesis) {
				while (true) {
					Cursor err = c;
					CompileValue arg;
					if (!Expression::parse(c, arg, CompileType::eval)) return err::fail;
					if (!Operand::deref(arg, cpt)) return err::fail;
					if (!Expression::rvalue(arg, CompileType::eval)) return err::fail;

					if (arg.type != compiler->types()->t_type) {
						return throw_specific_error(err, "Expected type");

					}

					Type* type;
					if (!compiler->evaluator()->pop_register_value<Type*>(type)) return err::fail;
					if (!Type::assert(err, type)) return err::fail;
					arg_types.push_back(type);

					if (c.tok == RecognizedToken::Comma) {
						c.move();
					}
					else if (c.tok == RecognizedToken::CloseParenthesis) {
						break;
					}
					else {
						return throw_wrong_token_error(c, "',' or ')'");
					}
				}
			}
			c.move();
			Type* ret_type = compiler->types()->t_void;

			if (c.tok == RecognizedToken::Symbol || c.tok == RecognizedToken::And || c.tok == RecognizedToken::DoubleAnd) {
				Cursor err = c;
				CompileValue rt;
				if (!Expression::parse(c, rt, CompileType::eval)) return err::fail;
				if (!Operand::deref(rt, cpt)) return err::fail;
				if (!Expression::rvalue(rt, CompileType::eval)) return err::fail;

				if (rt.type != compiler->types()->t_type) {
					return throw_specific_error(err, "Expected type");

				}
				if (!compiler->evaluator()->pop_register_value<Type*>(ret_type)) return err::fail;
				if (!Type::assert(err, ret_type)) return err::fail;
			}

			Type* ftype = compiler->types()->load_or_register_function_type(call_conv, std::move(arg_types), ret_type, t_context);

			compiler->evaluator()->write_register_value(ftype);
			ret.lvalue = false;
			ret.type = compiler->types()->t_type;
		}
		else {

			StackItem sitm;

			if (cpt == CompileType::compile && compiler->stack()->find(c.buffer(), sitm)) {
				if (!ILBuilder::build_local(compiler->scope(), sitm.id)) return err::fail;
				ret.type = sitm.type;
				ret.lvalue = true;
				ret.reflock = false;
				c.move();
			}
			else if (cpt == CompileType::eval && compiler->compiler_stack()->find(c.buffer(), sitm)) {
				compiler->eval_local(sitm.id);
				ret.type = sitm.type;
				ret.lvalue = true;
				ret.reflock = false;
				c.move();
			}
			else if (cpt == CompileType::compile) {
				
				Cursor err = c;
				if (!Operand::parse_const_type_function(c, ret)) return err::fail;
			}
			else {


				Cursor err = c;

				auto res = compiler->workspace()->find_name(c.buffer(),true);

				if (res.type() == FindNameResultType::None) {
					return throw_specific_error(c, "Path start point not found");
				}


				Cursor nm_err = c;
				c.move();
				while (c.tok == RecognizedToken::DoubleColon && res.type() == FindNameResultType::Namespace) {
					c.move();
					if (c.tok != RecognizedToken::Symbol)
					{
						return throw_not_a_name_error(c);
					}
					nm_err = c;

					res = res.get_namespace()->find_name(c.buffer());
					c.move();
				}


				if (auto struct_inst = res.get_structure()) {
					if (!struct_inst->compile()) return err::fail;

					if (struct_inst->ast_node->is_generic) {
						if (!ILBuilder::eval_const_word(compiler->evaluator(), struct_inst->type.get())) return err::fail;
					}
					else {
						StructureInstance* inst;
						if (!struct_inst->generate(nullptr, inst)) return err::fail;
						if (!ILBuilder::eval_const_word(compiler->evaluator(), inst->type.get())) return err::fail;
					}

					ret.lvalue = false;
					ret.type = compiler->types()->t_type;
				}
				else if (auto func_inst = res.get_function()) {

					if (!func_inst->compile()) return err::fail;

					if (!(func_inst->ast_node->has_body() && ((AstFunctionNode*)func_inst->ast_node)->is_generic)) {
						FunctionInstance* inst;
						if (!func_inst->generate(nullptr, inst)) return err::fail;

						if (!ILBuilder::eval_const_word(compiler->evaluator(), inst->type.get())) return err::fail;
						ret.lvalue = false;
						ret.type = compiler->types()->t_type;
					}
					else {
						if (!ILBuilder::eval_const_word(compiler->evaluator(), func_inst->type.get())) return err::fail;

						ret.lvalue = false;
						ret.type = compiler->types()->t_type;
					}
				}else if (auto func_inst = res.get_orphan()) {
					if (!func_inst->compile()) return err::fail;
					if (!ILBuilder::eval_const_word(compiler->evaluator(), func_inst->type.get())) return err::fail;
					ret.lvalue = false;
					ret.type = compiler->types()->t_type;
				}
				else if (auto trait_inst = res.get_trait()) {
					if (!trait_inst->compile()) return err::fail;

					if (trait_inst->ast_node->is_generic) {
						if (!ILBuilder::eval_const_word(compiler->evaluator(), trait_inst->type.get())) return err::fail;
					}
					else {
						TraitInstance* inst;
						if (!trait_inst->generate(nullptr, inst)) return err::fail;
						if (!ILBuilder::eval_const_word(compiler->evaluator(), inst->type.get())) return err::fail;
					}

					ret.lvalue = false;
					ret.type = compiler->types()->t_type;
				}
				else if (auto static_inst = res.get_static()) {
					if (!static_inst->compile()) return err::fail;

					if (!ILBuilder::eval_staticref(compiler->evaluator(), static_inst->sid)) return err::fail;
					
					ret.lvalue = true;
					ret.type = static_inst->type;
				}
				else {
					return throw_specific_error(nm_err, "Symbol not found");
				}

			}

		}

		return err::ok;

	}


	
	unsigned long long svtoi(std::string_view sv) {
		unsigned long long r = 0;
		for (std::size_t i = 0; i < sv.length(); i++) {
			r *= 10;
			r += (unsigned char)(sv[i] - '0');
		}
		return r;
	}
	
	double svtod(std::string_view sv)
	{
		char buffer[256] = {'\0'};
		std::memcpy(buffer, sv.data(),sv.size());

		//double dbl;
		//auto result = std::from_chars((const char*)sv.data(), (const char*)(sv.data() + sv.size()), dbl);
		return atof(buffer);
	}


	errvoid Operand::parse_number(CompileValue& ret, Cursor& c, CompileType cpt) {
		Compiler* compiler = Compiler::current();
		bool usg = c.tok == RecognizedToken::UnsignedNumber;

		std::string_view ndata;
		if (usg)
			ndata = c.buffer().substr(0, c.length - 1);
		else
			ndata = c.buffer();

		unsigned long long d = svtoi(ndata);
		c.move();

		if (cpt == CompileType::compile) {
			if (usg) {
				if (!ILBuilder::build_const_u32(compiler->scope(), (std::uint32_t)d)) return err::fail;
			}
			else {
				if (!ILBuilder::build_const_i32(compiler->scope(), (std::int32_t)d)) return err::fail;
			}
		}
		else if (cpt == CompileType::eval) {
			if (usg) {
				if (!ILBuilder::eval_const_u32(compiler->evaluator(), (std::uint32_t)d)) return err::fail;
			}
			else {
				if (!ILBuilder::eval_const_i32(compiler->evaluator(), (std::int32_t)d)) return err::fail;
			}

		}

		ret.type = usg ? compiler->types()->t_u32 : compiler->types()->t_i32;
		ret.lvalue = false;
		return err::ok;
	}





	errvoid Operand::parse_long_number(CompileValue& ret, Cursor& c, CompileType cpt) {
		Compiler* compiler = Compiler::current();
		bool usg = c.tok == RecognizedToken::UnsignedLongNumber;

		std::string_view ndata;
		if (usg)
			ndata = c.buffer().substr(0, c.length - 2);
		else
			ndata = c.buffer().substr(0, c.length - 1);

		unsigned long long d = svtoi(ndata);
		c.move();

		if (cpt == CompileType::compile) {
			if (usg) {
				if (!ILBuilder::build_const_u64(compiler->scope(), d)) return err::fail;
			}
			else {
				if (!ILBuilder::build_const_i64(compiler->scope(), d)) return err::fail;
			}
		}
		else if (cpt == CompileType::eval) {
			if (usg) {
				if (!ILBuilder::eval_const_u64(compiler->evaluator(), d)) return err::fail;
			}
			else {
				if (!ILBuilder::eval_const_i64(compiler->evaluator(), d)) return err::fail;
			}

		}

		ret.type = usg ? compiler->types()->t_u64 : compiler->types()->t_i64;
		ret.lvalue = false;
		return err::ok;
	}





	errvoid Operand::parse_float_number(CompileValue& ret, Cursor& c, CompileType cpt) {
		Compiler* compiler = Compiler::current();
		bool dbl = c.tok == RecognizedToken::DoubleNumber;

		std::string_view ndata;
		if (dbl)
			ndata = c.buffer().substr(0, c.length - 1);
		else
			ndata = c.buffer();

		double d = svtod(ndata);
		c.move();

		if (cpt == CompileType::compile) {
			if (dbl) {
				if (!ILBuilder::build_const_f64(compiler->scope(), d)) return err::fail;
			}
			else {
				if (!ILBuilder::build_const_f32(compiler->scope(), (float)d)) return err::fail;
			}
		}
		else if (cpt == CompileType::eval) {
			if (dbl) {
				if (!ILBuilder::eval_const_f64(compiler->evaluator(), d)) return err::fail;
			}
			else {
				if (!ILBuilder::eval_const_f32(compiler->evaluator(), (float)d)) return err::fail;
			}

		}

		ret.type = dbl ? compiler->types()->t_f64 : compiler->types()->t_f32;
		ret.lvalue = false;
		return err::ok;
	}


	template<typename T, typename S>
	errvoid Operand::parse_generate_template(Cursor& c, T* generating, S*& out) {
		Compiler* compiler = Compiler::current();
		auto layout = generating->generic_ctx.generic_layout.begin();

		if (c.tok != RecognizedToken::CloseParenthesis) {
			while (true) {
				if (layout == generating->generic_ctx.generic_layout.end()) {
					return throw_specific_error(c, "Too much arguments");
				}

				Type* target = std::get<1>(*layout);
				CompileValue res;
				Cursor err = c;
				if (!Expression::parse(c, res, CompileType::eval, true, target)) return err::fail;
				if (!Operand::deref(res, CompileType::eval)) return err::fail;
				if (!Operand::cast(err, res, target, CompileType::eval, false)) return err::fail;
				if (!Expression::rvalue(res, CompileType::eval)) return err::fail;

				layout++;

				if (c.tok == RecognizedToken::Comma) {
					c.move();
				}
				else if (c.tok == RecognizedToken::CloseParenthesis) {
					break;
				}
				else {
					return throw_wrong_token_error(c, "')' or ','");
				}
			}
		}

		if (layout != generating->generic_ctx.generic_layout.end()) {
			return throw_specific_error(c, "Not enough arguments");
		}

		c.move();



		auto state = ScopeState().compiler_stack();


		std::vector<std::tuple<Cursor, Type*>>::reverse_iterator act_layout;
		std::size_t act_layout_size = 0;

		//if (generating->generic_ctx.generator != nullptr) {
		//	generating->generic_ctx.generator->insert_key_on_stack(compiler->evaluator());
		//}

		act_layout = generating->generic_ctx.generic_layout.rbegin();
		act_layout_size = generating->generic_ctx.generic_layout.size();

		unsigned char* key_base = compiler->local_stack_base.back();


		for (std::size_t arg_i = act_layout_size - 1; arg_i >= 0 && arg_i < act_layout_size; arg_i--) {

			Type* type = std::get<1>(*act_layout);
			if (!type->compile()) return err::fail;

			stackid_t local_id = compiler->push_local(type->size());
			compiler->compiler_stack()->push_item(std::get<0>(*act_layout).buffer(), type, local_id);

			compiler->eval_local(local_id);
			if (!Expression::copy_from_rvalue(type, CompileType::eval)) return err::fail;

			act_layout++;
		}

		if (!generating->generate(key_base, out)) return err::fail;

		StackItem sitm;
		while (compiler->compiler_stack()->pop_item(sitm)) {}
		return err::ok;

	}

	errvoid Operand::push_template(ILEvaluator* eval) {
		Type* t;
		if (!eval->pop_register_value<Type*>(t)) return err::fail;
		if (!Type::assert(t)) return err::fail;
		template_stack[template_sp] = t;
		template_sp++;
		return err::ok;

	}

	errvoid Operand::build_template(ILEvaluator* eval) {
		Compiler* compiler = Compiler::current();
		Type* gen_type = template_stack[template_sp - 1];
		if (!gen_type->compile()) return err::fail;

		auto state = ScopeState().compiler_stack();

		std::vector<std::tuple<Cursor, Type*>>::reverse_iterator act_layout;
		std::vector<std::tuple<Cursor, Type*>>::reverse_iterator act_layout_it;
		std::size_t gen_types = 0;

		if (gen_type->type() == TypeInstanceType::type_structure_template) {
			act_layout = ((TypeStructureTemplate*)gen_type)->owner->generic_ctx.generic_layout.rbegin();
			gen_types = ((TypeStructureTemplate*)gen_type)->owner->generic_ctx.generic_layout.size();
		}
		else if (gen_type->type() == TypeInstanceType::type_trait_template) {
			act_layout = ((TypeTraitTemplate*)gen_type)->owner->generic_ctx.generic_layout.rbegin();
			gen_types = ((TypeTraitTemplate*)gen_type)->owner->generic_ctx.generic_layout.size();
		}
		else if (gen_type->type() == TypeInstanceType::type_function_template) {
			act_layout = ((TypeFunctionTemplate*)gen_type)->owner->generic_ctx.generic_layout.rbegin();
			gen_types = ((TypeFunctionTemplate*)gen_type)->owner->generic_ctx.generic_layout.size();
		}




		unsigned char* key_base = compiler->local_stack_base.back() + compiler->local_stack_size.back();


		act_layout_it = act_layout;
		for (std::size_t arg_i = gen_types - 1; arg_i >= 0 && arg_i < gen_types; arg_i--) {

			Type* type = std::get<1>((*act_layout_it));

			stackid_t local_id = compiler->push_local(type->size());
			unsigned char* data_place = compiler->stack_ptr(local_id);
			compiler->compiler_stack()->push_item(std::get<0>(*act_layout_it).buffer(), type, local_id);


			eval->write_register_value(data_place);
			if (!Expression::copy_from_rvalue(type, CompileType::eval)) return err::fail;

			act_layout_it++;
		}

		if (gen_type->type() == TypeInstanceType::type_structure_template) {
			StructureInstance* out = nullptr;
			((TypeStructureTemplate*)gen_type)->owner->generate(key_base, out);
			eval->write_register_value(out->type.get());
		}
		else if (gen_type->type() == TypeInstanceType::type_trait_template) {
			TraitInstance* out = nullptr;
			((TypeTraitTemplate*)gen_type)->owner->generate(key_base, out);
			eval->write_register_value(out->type.get());
		}
		else if (gen_type->type() == TypeInstanceType::type_function_template) {
			FunctionInstance* out = nullptr;
			((TypeFunctionTemplate*)gen_type)->owner->generate(key_base, out);
			eval->write_register_value(out->type.get());
		}

		StackItem sitm;
		while (compiler->compiler_stack()->pop_item(sitm)) {}
		return err::ok;
	}

	Type* Operand::template_stack[1024];
	std::uint16_t Operand::template_sp = 0;


	errvoid Operand::read_arguments(Cursor& c, unsigned int& argi, TypeFunction* ft, CompileType cpt) {
		Compiler* compiler = Compiler::current();
		if (c.tok != RecognizedToken::CloseParenthesis) {
			while (true) {
				if (argi >= compiler->types()->argument_array_storage.get(ft->argument_array_id).size()) {
					return throw_specific_error(c, "Wrong number of arguments");
				}
				Type* target = compiler->types()->argument_array_storage.get(ft->argument_array_id)[argi];
				CompileValue arg;
				Cursor err = c;
				if (!Expression::parse(c, arg, cpt, true, target)) return err::fail;
				if (!Operand::deref(arg, cpt)) return err::fail;
				if (!Operand::cast(err, arg, target, cpt, true)) return err::fail;
				if (!Expression::rvalue(arg, cpt)) return err::fail;
				argi++;

				if (c.tok == RecognizedToken::Comma) {
					c.move();
				}
				else if (c.tok == RecognizedToken::CloseParenthesis) {
					break;
				}
				else {
					return throw_wrong_token_error(c, "',' or ')'");
				}
			}
		}

		return err::ok;
	}

	errvoid Operand::parse_call_operator(CompileValue& ret, Cursor& c, CompileType cpt, bool targets_defer) {
		Compiler* compiler = Compiler::current();
		if (ret.type == compiler->types()->t_type || ret.type->type() == TypeInstanceType::type_template) {
			Cursor nm_err = c;
			if (!Expression::rvalue(ret, cpt)) return err::fail;

			if (cpt != CompileType::compile) {
				Type* dt;
				if (!compiler->evaluator()->pop_register_value<Type*>(dt)) return err::fail;
				if (!Type::assert(nm_err, dt)) return err::fail;

				if (dt->type() != TypeInstanceType::type_structure_template && dt->type() != TypeInstanceType::type_trait_template && dt->type() != TypeInstanceType::type_function_template && dt->type() != TypeInstanceType::type_function_instance) {
					return throw_specific_error(c, "this type is not a generic type or function instance");
				}

				if (dt->type() == TypeInstanceType::type_structure_template) {
					c.move();
					StructureInstance* inst;
					if (!Operand::parse_generate_template(c, ((TypeStructureTemplate*)dt)->owner, inst)) return err::fail;
					if (!ILBuilder::eval_const_word(compiler->evaluator(), inst->type.get())) return err::fail;

					ret.lvalue = false;
					ret.type = compiler->types()->t_type;

				}
				else if (dt->type() == TypeInstanceType::type_trait_template) {
					c.move();
					TraitInstance* inst;
					if (!Operand::parse_generate_template(c, ((TypeTraitTemplate*)dt)->owner, inst)) return err::fail;
					if (!ILBuilder::eval_const_word(compiler->evaluator(), inst->type.get())) return err::fail;

					ret.lvalue = false;
					ret.type = compiler->types()->t_type;
				}
				else if (dt->type() == TypeInstanceType::type_function_template) {
					c.move();
					FunctionInstance* inst;
					if (!Operand::parse_generate_template(c, ((TypeFunctionTemplate*)dt)->owner, inst)) return err::fail;
					if (!ILBuilder::eval_const_word(compiler->evaluator(), inst->type.get())) return err::fail;

					ret.lvalue = false;
					ret.type = compiler->types()->t_type;
				}else if (dt->type() == TypeInstanceType::type_function_instance) {
					FunctionInstance* inst = ((TypeFunctionInstance*)dt)->owner;
					if (!inst->compile()) return err::fail;
					if (!ILBuilder::eval_fnptr(compiler->evaluator(), inst->func)) return err::fail;

					ret.lvalue = false;
					ret.type = ((TypeFunctionInstance*)dt)->function_type;

					if (!Operand::function_call(ret, c, CompileType::eval, 0, targets_defer)) return err::fail;
				}

			}
			else if (ret.type->type() == TypeInstanceType::type_template) {
				if (!Expression::rvalue(ret, cpt)) return err::fail;

				if (cpt == CompileType::compile) {
					if (!ILBuilder::build_const_word(compiler->scope(), ret.type)) return err::fail;

					if (!ILBuilder::build_insintric(compiler->scope(), ILInsintric::type_dynamic_cast)) return err::fail;
					if (!ILBuilder::build_insintric(compiler->scope(), ILInsintric::push_template)) return err::fail;
				}
				else {
					if (!ILBuilder::eval_const_word(compiler->evaluator(), ret.type)) return err::fail;
					if (!ILBuilder::eval_insintric(compiler->evaluator(), ILInsintric::type_dynamic_cast)) return err::fail;
					if (!ILBuilder::eval_insintric(compiler->evaluator(), ILInsintric::push_template)) return err::fail;
				}

				TypeTemplate* tt = (TypeTemplate*)ret.type;
				auto layout = compiler->types()->argument_array_storage.get(tt->argument_array_id).begin();

				if (c.tok != RecognizedToken::OpenParenthesis) {
					return throw_wrong_token_error(c, "'('");
				}
				c.move();

				std::vector<CompileValue> results;
				if (c.tok != RecognizedToken::CloseParenthesis) {
					while (true) {
						Type* target = *layout;
						CompileValue res;
						Cursor err = c;
						if (!Expression::parse(c, res, cpt,true, target)) return err::fail;
						if (!Operand::deref(res, cpt)) return err::fail;
						if (!Expression::rvalue(res, cpt)) return err::fail;
						if (!Operand::cast(err, res, target, cpt, true)) return err::fail;

						results.push_back(res);
						layout++;

						if (c.tok == RecognizedToken::Comma) {
							c.move();
						}
						else if (c.tok == RecognizedToken::CloseParenthesis) {
							break;
						}
						else {
							return throw_wrong_token_error(c, "')' or ','");
						}
					}
				}

				c.move();

				if (cpt == CompileType::compile) {
					if (!ILBuilder::build_insintric(compiler->scope(), ILInsintric::build_template)) return err::fail;
				}
				else {
					if (!ILBuilder::eval_insintric(compiler->evaluator(), ILInsintric::build_template)) return err::fail;
				}

				ret.lvalue = false;
				ret.type = compiler->types()->t_type;
			}
			else {
				return throw_specific_error(c, "Operation not supported on plain type, please cast to generic type");
			}

		}
		else if (ret.type->type() == TypeInstanceType::type_function) {
			if (!Operand::function_call(ret, c, cpt, 0,targets_defer)) return err::fail;
		} else if (ret.type->type() == TypeInstanceType::type_function_instance) {
			if (!((TypeFunctionInstance*)ret.type)->owner->compile()) return err::fail;

			if (cpt == CompileType::compile) {
				if (!ILBuilder::build_fnptr(compiler->scope(), ((TypeFunctionInstance*)ret.type)->owner->func)) return err::fail;
			} else {
				if (!ILBuilder::eval_fnptr(compiler->evaluator(), ((TypeFunctionInstance*)ret.type)->owner->func)) return err::fail;	
			}

			ret.type = ((TypeFunctionInstance*)ret.type)->function_type;

			if (!Operand::function_call(ret, c, cpt, 0,targets_defer)) return err::fail;
		} else {
			return throw_specific_error(c, "Operator () cant be used on this type");
		}
		return err::ok;


	}

	errvoid Operand::parse_reference(CompileValue& ret, Cursor& c, CompileType cpt, bool targets_defer) {
		Compiler* compiler = Compiler::current();
		Cursor operr = c;

		unsigned int type_ref_count = 0;
		while (c.tok == RecognizedToken::And || c.tok == RecognizedToken::DoubleAnd) {
			type_ref_count++;
			if (c.tok == RecognizedToken::DoubleAnd)
				type_ref_count++;
			c.move();
		}

		Cursor err = c;
		if (!Operand::parse(c, ret, cpt, targets_defer)) return err::fail;

		if (ret.type == compiler->types()->t_type) {
			if (!Expression::rvalue(ret, cpt)) return err::fail;

			if (cpt == CompileType::eval) {
				Type* t;
				if (!compiler->evaluator()->pop_register_value<Type*>(t)) return err::fail;
				if (!Type::assert(err, t)) return err::fail;
				for (unsigned int i = 0; i < type_ref_count; i++)
					t = t->generate_reference();

				compiler->evaluator()->write_register_value(t);
			}
			else if (cpt == CompileType::compile) {
				for (unsigned int i = 0; i < type_ref_count; i++) {
					if (!ILBuilder::build_fncall(compiler->scope(), compiler->types()->f_build_reference->func)) return err::fail;
				}
			}

			ret.type = compiler->types()->t_type;
			ret.lvalue = false;
		}
		else if (ret.type->type() == TypeInstanceType::type_reference) {
			ret.reflock = true;
		}
		else {
			return throw_specific_error(err, "operator expected to recieve type or reference");
		}
		return err::ok;
	}

	errvoid Operand::parse_array_operator(CompileValue& ret, Cursor& c, CompileType cpt) {
		Compiler* compiler = Compiler::current();
		while (ret.type->type() == TypeInstanceType::type_structure_instance || ret.type->type() == TypeInstanceType::type_reference) {

			if (ret.type->type() == TypeInstanceType::type_reference) {
				TypeReference* tr = (TypeReference*)ret.type;
				if (tr->owner->type() != TypeInstanceType::type_structure_instance && tr->owner->type() != TypeInstanceType::type_reference) break;

				if (ret.lvalue) {
					if (cpt == CompileType::compile) {
						if (!ILBuilder::build_load(compiler->scope(), ret.type->rvalue())) return err::fail;
					}
					else if (cpt == CompileType::eval) {
						if (!ILBuilder::eval_load(compiler->evaluator(), ret.type->rvalue())) return err::fail;
					}
				}

				ret.type = ((TypeReference*)ret.type)->owner;
				ret.lvalue = true;
				continue;
			}

			TypeStructureInstance* ti = (TypeStructureInstance*)ret.type;
			if (!ti->compile()) return err::fail;
			StructureInstance* si = ti->owner;

			if (!si->pass_array_operator) {
				return throw_specific_error(c, "Offset can be applied only on slices or passed to slice by alias");
			}

			if (!Operand::structure_element_offset(ret, si->pass_array_id, cpt)) return err::fail;
		}

		if (ret.type->type() == TypeInstanceType::type_reference) {
			TypeReference* tr = (TypeReference*)ret.type;

			if (tr->owner->type() == TypeInstanceType::type_slice || tr->owner->type() == TypeInstanceType::type_array) {
				if (ret.lvalue) {
					if (cpt == CompileType::compile) {
						if (!ILBuilder::build_load(compiler->scope(), ret.type->rvalue())) return err::fail;
					}
					else if (cpt == CompileType::eval) {
						if (!ILBuilder::eval_load(compiler->evaluator(), ret.type->rvalue())) return err::fail;
					}
				}

				ret.type = ((TypeReference*)ret.type)->owner;
				ret.lvalue = true;
			}
		}

		if (ret.type->type() != TypeInstanceType::type_slice && ret.type->type() != TypeInstanceType::type_array) {
			return throw_specific_error(c, "Offset can be applied only on slices or passed to slice by alias");
		}
		c.move();

		Type* base_slice;

		if (ret.type->type() != TypeInstanceType::type_slice) {
			base_slice = ((TypeSlice*)ret.type)->owner;
		} else if (ret.type->type() != TypeInstanceType::type_array) {
			base_slice = ((TypeArray*)ret.type)->owner;
		}

		if (ret.type->type() == TypeInstanceType::type_slice) {
			if (ret.lvalue) {
				if (cpt == CompileType::compile) {
					if (!ILBuilder::build_load(compiler->scope(), ILDataType::dword)) return err::fail;
				}
				else {
					if (!ILBuilder::eval_load(compiler->evaluator(), ILDataType::dword)) return err::fail;
				}
			}

			if (cpt == CompileType::compile) {
				if (!ILBuilder::build_low_word(compiler->scope())) return err::fail;
			}
			else {
				if (!ILBuilder::eval_low_word(compiler->evaluator())) return err::fail;
			}
		}

		Cursor err = c;
		CompileValue index;
		if (!Expression::parse(c, index, cpt)) return err::fail;
		if (!Operand::deref(index, cpt)) return err::fail;
		if (!Expression::rvalue(index, cpt)) return err::fail;
		if (!Operand::cast(err, index, compiler->types()->t_size, cpt, true)) return err::fail;

		if (c.tok == RecognizedToken::DoubleDot) {
			c.move();

			if (!Expression::parse(c, index, cpt)) return err::fail;
			if (!Operand::deref(index, cpt)) return err::fail;
			if (!Expression::rvalue(index, cpt)) return err::fail;
			if (!Operand::cast(err, index, compiler->types()->t_size, cpt, true)) return err::fail;

			if (cpt == CompileType::compile) {
				if (!ILBuilder::build_cut(compiler->scope(), base_slice->size())) return err::fail;
			}
			else {
				if (!ILBuilder::eval_cut(compiler->evaluator(), base_slice->size())) return err::fail;
			}
			
			ret.lvalue = false;
			ret.type = base_slice->generate_slice();
		} else {
			if (cpt == CompileType::compile) {
				if (!ILBuilder::build_extract(compiler->scope(), base_slice->size())) return err::fail;
			}
			else {
				if (!ILBuilder::eval_extract(compiler->evaluator(), base_slice->size())) return err::fail;
			}
			
			ret.lvalue = true;
			ret.type = base_slice;
		}

		

		if (c.tok != RecognizedToken::CloseBracket) {
			return throw_wrong_token_error(c, "']'");
		}

		c.move();

		return err::ok;


	}

	errvoid Operand::parse_array_type(CompileValue& ret, Cursor& c, CompileType cpt, bool targets_defer) {
		Compiler* compiler = Compiler::current();
		Cursor err = c;
		CompileValue res;
		c.move();

		if (c.tok == RecognizedToken::CloseBracket) {
			c.move();

			Cursor t_err = c;

			if (!Operand::parse(c, res, cpt, targets_defer)) return err::fail;
			if (!Expression::rvalue(res, cpt)) return err::fail;

			if (res.type != compiler->types()->t_type) {
				return throw_specific_error(t_err, "Expected type");

			}

			if (cpt == CompileType::eval) {
				Type* t;
				if (!compiler->evaluator()->pop_register_value<Type*>(t)) return err::fail;
				if (!Type::assert(t_err, t)) return err::fail;
				Type* nt = t->generate_slice();
				if (!ILBuilder::eval_const_word(compiler->evaluator(), nt)) return err::fail;
			}
			else {
				if (!ILBuilder::build_fncall(compiler->scope(), compiler->types()->f_build_slice->func)) return err::fail;
			}
		}
		else {
			if (!Expression::parse(c, res, cpt)) return err::fail;
			if (!Operand::deref(res, cpt)) return err::fail;
			if (!Expression::rvalue(res, cpt)) return err::fail;
			if (!Operand::cast(err, res, compiler->types()->t_u32, cpt, true)) return err::fail;


			if (c.tok != RecognizedToken::CloseBracket) {
				return throw_wrong_token_error(c, "']'");
			}

			c.move();

			Cursor t_err = c;

			if (!Operand::parse(c, res, cpt, targets_defer)) return err::fail;
			if (!Expression::rvalue(res, cpt)) return err::fail;

			if (res.type != compiler->types()->t_type) {
				return throw_specific_error(t_err, "Expected type");
			}

			if (cpt == CompileType::eval) {
				Type* t;
				if (!compiler->evaluator()->pop_register_value<Type*>(t)) return err::fail;
				if (!Type::assert(t_err, t)) return err::fail;
				std::uint32_t val;
				if (!compiler->evaluator()->pop_register_value<std::uint32_t>(val)) return err::fail;
				TypeArray* nt = t->generate_array(val);
				if (!ILBuilder::eval_const_word(compiler->evaluator(), nt)) return err::fail;
			}
			else {
				if (!ILBuilder::build_fncall(compiler->scope(), compiler->types()->f_build_array->func)) return err::fail;
			}
		}

		ret.type = compiler->types()->t_type;
		ret.lvalue = false;
		return err::ok;

	}


	errvoid Operand::parse_double_colon_operator(CompileValue& ret, Cursor& c, CompileType cpt, bool targets_defer) {
		Compiler* compiler = Compiler::current();
		if (ret.type->type() != TypeInstanceType::type_structure_instance && ret.type != compiler->types()->t_type) {
			return throw_specific_error(c, "Operator :: is only supported on structure instances");
		}

		if (cpt == CompileType::compile) {
			return throw_specific_error(c, "Operator :: is not supported in runtime context");
		}

		if (ret.type != compiler->types()->t_type) {
			return throw_specific_error(c, "left operator is not a type instance");
		}

		c.move();

		if (cpt == CompileType::eval) {
			if (!Expression::rvalue(ret, cpt)) return err::fail;
			TypeStructureInstance* ti;
			if (!compiler->evaluator()->pop_register_value<TypeStructureInstance*>(ti)) return err::fail;
			if (ti->type() != TypeInstanceType::type_structure_instance) {
				return throw_specific_error(c, "Type is not structure instance");
			}

			StructureInstance* struct_inst = ti->owner;

			auto f = struct_inst->name_table.find(c.buffer());
			if (f != struct_inst->name_table.end()) {



				switch (f->second.first)
				{
					case 1: {
						StructureTemplate* tplt = struct_inst->subtemplates[f->second.second].get();
						auto state = ScopeState().compiler_stack();

						/*if (tplt->generic_ctx.generator)
							tplt->generic_ctx.generator->insert_key_on_stack(compiler->evaluator());*/


						if (!tplt->compile()) return err::fail;

						ret.lvalue = false;
						ret.type = compiler->types()->t_type;

						if (tplt->ast_node->is_generic) {
							if (!ILBuilder::eval_const_word(compiler->evaluator(), tplt->type.get())) return err::fail;
						}
						else {
							StructureInstance* inst = nullptr;

							tplt->generate(compiler->local_stack_base.back(), inst);
							if (!ILBuilder::eval_const_word(compiler->evaluator(), inst->type.get())) return err::fail;
						}


						c.move();
					}break;
					case 2: {

						c.move();
						if (c.tok != RecognizedToken::OpenParenthesis) {
							return throw_wrong_token_error(c, "'('");
						}

						FunctionInstance* finst;
						if (!struct_inst->subfunctions[f->second.second]->generate(nullptr, finst)) return err::fail;
						if (!finst->compile()) return err::fail;

						if (!ILBuilder::eval_fnptr(compiler->evaluator(), finst->func)) return err::fail;

						ret.type = finst->type->function_type;
						ret.lvalue = false;
						if (!Operand::function_call(ret, c, cpt, 0, targets_defer)) return err::fail;
					}break;

					case 4: {
						c.move();
						StaticInstance* sinst = struct_inst->substatics[f->second.second].get();
						if (!sinst->compile()) return err::fail;
						ret.lvalue = true;
						ret.type = sinst->type;
						if (!ILBuilder::eval_staticref(compiler->evaluator(), sinst->sid)) return err::fail;
					} break;

					default:
						return throw_specific_error(c, "Target is not a structure or static");
						break;
				}

				
			}
			else {
				return throw_specific_error(c, "Structure instance does not contain a structure with this name");
			}
		}
		return err::ok;
	}

	errvoid Operand::function_call(CompileValue& ret, Cursor& c, CompileType cpt, unsigned int argi, bool targets_defer) {
		if (ret.type->type() != TypeInstanceType::type_function) {
			return throw_specific_error(c, "Compiler error, passed wrong type to function call");
		}

		Compiler* compiler = Compiler::current();
		if (!Expression::rvalue(ret, cpt)) return err::fail;

		TypeFunction* ft = (TypeFunction*)ret.type;		 

		if (ft->context() != ILContext::both && compiler->scope_context() != ft->context()) {
			return throw_specific_error(c, "Cannot call function with different context specifier");
		}

		if (c.tok != RecognizedToken::OpenParenthesis) {
			return throw_wrong_token_error(c, "'('");
		}
		c.move();

		CompileValue retval;
		retval.type = ft->return_type;
		retval.lvalue = true;
		if (!ft->return_type->compile()) return err::fail;

		if (cpt == CompileType::compile) {

			stackid_t local_return_id = 0;


			if (!ILBuilder::build_callstart(compiler->scope())) return err::fail;
			if (ft->return_type->rvalue_stacked()) {
				local_return_id = compiler->target()->local_stack_lifetime.append(retval.type->size());
				compiler->temp_stack()->push_item("$tmp", retval.type, local_return_id);

				if (!ILBuilder::build_local(compiler->scope(), local_return_id)) return err::fail;
			}

			if (!Operand::read_arguments(c, argi, ft, cpt)) return err::fail;

			if (argi != compiler->types()->argument_array_storage.get(ft->argument_array_id).size()) {
				return throw_specific_error(c, "Wrong number of arguments");
			}
			c.move();
			if (!ft->compile()) return err::fail;

			if (!targets_defer || c.tok != RecognizedToken::Semicolon) {
				if (!ILBuilder::build_call(compiler->scope(), ft->il_function_decl)) return err::fail;

				if (ft->return_type->rvalue_stacked()) {
					if (!ILBuilder::build_local(compiler->scope(), local_return_id)) return err::fail;
				}


				ret.type = ft->return_type;
				ret.lvalue = false;
			}
			else {
				compiler->defer_scope().push_back(ft);

				ret.type = compiler->types()->t_void;
				ret.lvalue = true;
			}

		}
		else {

			stackid_t local_stack_item;

			if (!ILBuilder::eval_callstart(compiler->evaluator())) return err::fail;

			if (ft->return_type->rvalue_stacked()) {
				local_stack_item = compiler->push_local(retval.type->size());
				unsigned char* memory_place = compiler->stack_ptr(local_stack_item);

				compiler->compiler_stack()->push_item("$tmp", retval.type, local_stack_item);
				compiler->eval_local(local_stack_item);
			}

			if (!Operand::read_arguments(c, argi, ft, cpt)) return err::fail;

			if (argi != compiler->types()->argument_array_storage.get(ft->argument_array_id).size()) {
				return throw_specific_error(c, "Wrong number of arguments");
			}

			c.move();
			if (!ft->compile()) return err::fail;

			if (!targets_defer || c.tok != RecognizedToken::Semicolon) {

				if (!ILBuilder::eval_call(compiler->evaluator(), ft->il_function_decl)) return err::fail;

				if (ft->return_type->rvalue_stacked()) {
					compiler->eval_local(local_stack_item);
				}


				ret.type = ft->return_type;
				ret.lvalue = false;
			}
			else {
				compiler->compile_defer_scope().push_back(ft);
				ret.type = compiler->types()->t_void;
				ret.lvalue = true;
			}
		}
		return err::ok;
	}

	errvoid Operand::structure_element_offset(CompileValue& ret, tableelement_t id, CompileType cpt) {
		Compiler* compiler = Compiler::current();
		TypeStructureInstance* ti = (TypeStructureInstance*)ret.type;
		if (!ti->compile()) return err::fail;

		tableelement_t elem_id = 0;
		Type* mem_type = nullptr;
		StructureInstance* si = ti->owner;
		ILSize elem_size(si->size.type, 0);

		auto& member = si->member_vars[id];
		if (si->size.type == ILSizeType::table) {
			elem_id = id;
			elem_size.value = si->size.value;
		}
		else {
			elem_size.value = member.second;
		}

		mem_type = member.first;

		if (ret.lvalue || ret.type->rvalue_stacked())
		{
			if (cpt == CompileType::compile) {
				if (!si->wrapper) {

					switch (elem_size.type)
					{
						case ILSizeType::table:
							if (!ILBuilder::build_tableoffset(compiler->scope(), elem_size.value, elem_id)) return err::fail; break;
						case ILSizeType::abs8: 
							if (!ILBuilder::build_aoffset(compiler->scope(), (std::uint32_t)elem_size.value)) return err::fail; break;
						case ILSizeType::abs16: 
							if (!ILBuilder::build_aoffset(compiler->scope(), (std::uint32_t)(elem_size.value*2))) return err::fail; break;
						case ILSizeType::abs32: 
							if (!ILBuilder::build_aoffset(compiler->scope(), (std::uint32_t)(elem_size.value*4))) return err::fail; break;
						case ILSizeType::abs64: 
							if (!ILBuilder::build_aoffset(compiler->scope(), (std::uint32_t)(elem_size.value*8))) return err::fail; break;
						case ILSizeType::ptr:
							if (!ILBuilder::build_woffset(compiler->scope(), (std::uint32_t)elem_size.value)) return err::fail; break;
						default:
							break;
					}
					if (!ret.lvalue && !mem_type->rvalue_stacked()) {
						if (!ILBuilder::build_load(compiler->scope(), mem_type->rvalue())) return err::fail;
					}
				}
			}
			else if (cpt == CompileType::eval) {
				if (!si->wrapper) {
					switch (elem_size.type)
					{
						case ILSizeType::table:
							if (!ILBuilder::eval_tableoffset(compiler->evaluator(), elem_size.value, elem_id)) return err::fail; break;
						case ILSizeType::abs8: 
							if (!ILBuilder::eval_aoffset(compiler->evaluator(), (std::uint32_t)elem_size.value)) return err::fail; break;
						case ILSizeType::abs16: 
							if (!ILBuilder::eval_aoffset(compiler->evaluator(), (std::uint32_t)(elem_size.value*2))) return err::fail; break;
						case ILSizeType::abs32: 
							if (!ILBuilder::eval_aoffset(compiler->evaluator(), (std::uint32_t)(elem_size.value*4))) return err::fail; break;
						case ILSizeType::abs64: 
							if (!ILBuilder::eval_aoffset(compiler->evaluator(), (std::uint32_t)(elem_size.value*8))) return err::fail; break;
						case ILSizeType::ptr:
							if (!ILBuilder::eval_woffset(compiler->evaluator(), (std::uint32_t)elem_size.value)) return err::fail; break;
						default:
							break;
					}	
					if (!ret.lvalue && !mem_type->rvalue_stacked()) {
						if (!ILBuilder::eval_load(compiler->evaluator(), mem_type->rvalue())) return err::fail;
					}
				}	
			}
		}
		else {
			if (cpt == CompileType::compile) {
				if (!si->wrapper) {
					switch (elem_size.type)
					{
						case ILSizeType::table:
							if (!ILBuilder::build_tableroffset(compiler->scope(), ret.type->rvalue(), mem_type->rvalue(), elem_size.value, elem_id)) return err::fail; break;
						case ILSizeType::abs8: 
							if (!ILBuilder::build_aroffset(compiler->scope(), ret.type->rvalue(), mem_type->rvalue(), (std::uint32_t)elem_size.value)) return err::fail; break;
						case ILSizeType::abs16: 
							if (!ILBuilder::build_aroffset(compiler->scope(), ret.type->rvalue(), mem_type->rvalue(), (std::uint32_t)(elem_size.value*2))) return err::fail; break;
						case ILSizeType::abs32: 
							if (!ILBuilder::build_aroffset(compiler->scope(), ret.type->rvalue(), mem_type->rvalue(), (std::uint32_t)(elem_size.value*4))) return err::fail; break;
						case ILSizeType::abs64: 
							if (!ILBuilder::build_aroffset(compiler->scope(), ret.type->rvalue(), mem_type->rvalue(), (std::uint32_t)(elem_size.value*8))) return err::fail; break;
						case ILSizeType::ptr:
							if (!ILBuilder::build_wroffset(compiler->scope(), ret.type->rvalue(), mem_type->rvalue(), (std::uint32_t)elem_size.value)) return err::fail; break;
						default:
							break;
					}
				}

			}
			else if (cpt == CompileType::eval) {

				if (!si->wrapper) {

					switch (elem_size.type)
					{
						case ILSizeType::table:
							if (!ILBuilder::eval_tableroffset(compiler->evaluator(), ret.type->rvalue(), mem_type->rvalue(), elem_size.value, elem_id)) return err::fail; break;
						case ILSizeType::abs8: 
							if (!ILBuilder::eval_aroffset(compiler->evaluator(), ret.type->rvalue(), mem_type->rvalue(), (std::uint32_t)elem_size.value)) return err::fail; break;
						case ILSizeType::abs16: 
							if (!ILBuilder::eval_aroffset(compiler->evaluator(), ret.type->rvalue(), mem_type->rvalue(), (std::uint32_t)(elem_size.value*2))) return err::fail; break;
						case ILSizeType::abs32: 
							if (!ILBuilder::eval_aroffset(compiler->evaluator(), ret.type->rvalue(), mem_type->rvalue(), (std::uint32_t)(elem_size.value*4))) return err::fail; break;
						case ILSizeType::abs64: 
							if (!ILBuilder::eval_aroffset(compiler->evaluator(), ret.type->rvalue(), mem_type->rvalue(), (std::uint32_t)(elem_size.value*8))) return err::fail; break;
						case ILSizeType::ptr:
							if (!ILBuilder::eval_wroffset(compiler->evaluator(), ret.type->rvalue(), mem_type->rvalue(), (std::uint32_t)elem_size.value)) return err::fail; break;
						default:
							break;
					}
				}

			}

		}

		// ret.lvalue stays the same
		ret.type = mem_type;
		return err::ok;
	}


	errvoid Operand::parse_dot_operator(CompileValue& ret, Cursor& c, CompileType cpt, bool targets_defer) {
		Compiler* compiler = Compiler::current();
		c.move();

		if (!Operand::deref(ret, cpt)) return err::fail;

		if (ret.type->type() == TypeInstanceType::type_slice) {
			TypeSlice* ts = (TypeSlice*)ret.type;

			auto buf = c.buffer();
			if (buf == "count") {
				if (cpt == CompileType::compile) {
					if (ret.lvalue || ret.type->rvalue_stacked()) {
						if (!ILBuilder::build_woffset(compiler->scope(), 1)) return err::fail;
						if (!ILBuilder::build_load(compiler->scope(), ILDataType::word)) return err::fail;
					}
					else {
						if (!ILBuilder::build_high_word(compiler->scope())) return err::fail;
					}
				}
				else {
					if (ret.lvalue || ret.type->rvalue_stacked()) {
						if (!ILBuilder::eval_woffset(compiler->evaluator(), 1)) return err::fail;
						if (!ILBuilder::eval_load(compiler->evaluator(), ILDataType::word)) return err::fail;
					}
					else {
						if (!ILBuilder::eval_high_word(compiler->evaluator())) return err::fail;
					}
				}

				if ((ts->owner->size().type != ILSizeType::table && ts->owner->size().type != ILSizeType::array) || ts->owner->size().value > 1) {
					if (cpt == CompileType::compile) {
						if (!ILBuilder::build_const_size(compiler->scope(), ts->owner->size())) return err::fail;
						if (!ILBuilder::build_div(compiler->scope(), ILDataType::word, ILDataType::word)) return err::fail;
					}
					else {
						if (!ILBuilder::eval_const_size(compiler->evaluator(), ts->owner->size().eval(compiler->global_module()))) return err::fail;
						if (!ILBuilder::eval_div(compiler->evaluator(), ILDataType::word, ILDataType::word)) return err::fail;
					}
				}

				c.move();

				ret.lvalue = false;
				ret.type = compiler->types()->t_size;

			}
			else if (buf == "size") {

				if (cpt == CompileType::compile) {

					if (ret.lvalue) {
						if (!ILBuilder::build_woffset(compiler->scope(), 1)) return err::fail;
					}
					else if (ret.type->rvalue_stacked()) {
						if (!ILBuilder::build_woffset(compiler->scope(), 1)) return err::fail;
						if (!ILBuilder::build_load(compiler->scope(), ILDataType::word)) return err::fail;
					}
					else {

						if (!ILBuilder::build_high_word(compiler->scope())) return err::fail;
					}
				}
				else {

					if (ret.lvalue) {
						if (!ILBuilder::eval_woffset(compiler->evaluator(), 1)) return err::fail;
					}
					else if (ret.type->rvalue_stacked()) {
						if (!ILBuilder::eval_woffset(compiler->evaluator(), 1)) return err::fail;
						if (!ILBuilder::eval_load(compiler->evaluator(), ILDataType::word)) return err::fail;
					}
					else {
						if (!ILBuilder::eval_high_word(compiler->evaluator())) return err::fail;
					}
				}


				c.move();

				ret.type = compiler->types()->t_size;

			}
			else if (buf == "ptr") {
				if (cpt == CompileType::compile) {
					if (ret.lvalue) {

					}
					else if (ret.type->rvalue_stacked()) {
						if (!ILBuilder::build_load(compiler->scope(), ILDataType::word)) return err::fail;
					}
					else {
						if (!ILBuilder::build_bitcast(compiler->scope(), ret.type->rvalue(), ILDataType::word)) return err::fail;
					}
				}
				else {
					if (ret.lvalue) {

					}
					else if (ret.type->rvalue_stacked()) {
						if (!ILBuilder::eval_load(compiler->evaluator(), ILDataType::word)) return err::fail;
					}
					else {

						if (!ILBuilder::eval_bitcast(compiler->evaluator(), ret.type->rvalue(), ILDataType::word)) return err::fail;
					}
				}

				c.move();

				ret.type = compiler->types()->t_ptr;

			}
			else {
				return throw_specific_error(c, "Indentifier not recognized as a value of slice");

			}
		}
		else if (ret.type->type() == TypeInstanceType::type_trait) {

			TypeTraitInstance* tti = (TypeTraitInstance*)ret.type;
			TraitInstance* ti = tti->owner;


			auto off_f = ti->member_table.find(c.buffer());
			if (off_f == ti->member_table.end()) {
				return throw_specific_error(c, "Trait function not found");

			}
			std::uint32_t off = (std::uint32_t)off_f->second;
			auto& mf = ti->member_declarations[off];

			if (mf->ptr_context != ILContext::both && compiler->scope_context() != mf->ptr_context) {
				return throw_specific_error(c, "Cannot access trait function with different context");
			}

			if (!Expression::rvalue(ret, cpt)) return err::fail;

			c.move();
			if (c.tok == RecognizedToken::OpenParenthesis) {
				if (cpt == CompileType::compile) {
					if (!ILBuilder::build_split_dword(compiler->scope())) return err::fail;
					if (!ILBuilder::build_woffset(compiler->scope(), (std::uint32_t)off)) return err::fail;
					if (!ILBuilder::build_load(compiler->scope(), ILDataType::word)) return err::fail;
					if (!ILBuilder::build_callstart(compiler->scope())) return err::fail;
				}
				else {
					if (!ILBuilder::eval_split_dword(compiler->evaluator())) return err::fail;
					if (!ILBuilder::eval_woffset(compiler->evaluator(), (std::uint32_t)off)) return err::fail;
					if (!ILBuilder::eval_load(compiler->evaluator(), ILDataType::word)) return err::fail;
					if (!ILBuilder::eval_callstart(compiler->evaluator())) return err::fail;
				}

				stackid_t local_return_id = 0;

				if (cpt == CompileType::compile) {
					if (mf->return_type->rvalue_stacked()) {
						local_return_id = compiler->target()->local_stack_lifetime.append(mf->return_type->size());
						compiler->temp_stack()->push_item("$tmp", mf->return_type, local_return_id);
						if (!ILBuilder::build_local(compiler->scope(), local_return_id)) return err::fail;
					}
				}
				else {
					if (mf->return_type->rvalue_stacked()) {
						local_return_id = compiler->push_local(mf->return_type->size());
						compiler->compiler_stack()->push_item("$tmp", mf->return_type, local_return_id);
						compiler->eval_local(local_return_id);
					}
				}

				c.move();
				unsigned int argi = 1;
				if (!Operand::read_arguments(c, argi, mf, cpt)) return err::fail;

				if (!mf->compile()) return err::fail;

				c.move();

				if (!targets_defer || c.tok!=RecognizedToken::Semicolon) {
					if (cpt == CompileType::compile) {
						if (!ILBuilder::build_call(compiler->scope(), mf->il_function_decl)) return err::fail;

						if (mf->return_type->rvalue_stacked()) {
							if (!ILBuilder::build_local(compiler->scope(), local_return_id)) return err::fail;
						}
					}
					else {
						if (!ILBuilder::eval_call(compiler->evaluator(), mf->il_function_decl)) return false;

						if (mf->return_type->rvalue_stacked()) {
							compiler->eval_local(local_return_id);
						}
					}

					ret.lvalue = false;
					ret.type = mf->return_type;
				}
				else {
					if (cpt == CompileType::compile) {
						compiler->defer_scope().push_back(mf);
					}
					else {
						compiler->compile_defer_scope().push_back(mf);

					}

					ret.lvalue = true;
					ret.type = compiler->types()->t_void;
				}


			}
			else {
				if (cpt == CompileType::compile) {
					if (!ILBuilder::build_high_word(compiler->scope())) return err::fail;
					if (!ILBuilder::build_woffset(compiler->scope(), (std::uint32_t)off)) return err::fail;
					if (!ILBuilder::build_load(compiler->scope(), ILDataType::word)) return err::fail;
				}
				else {
					if (!ILBuilder::eval_high_word(compiler->evaluator())) return err::fail;
					if (!ILBuilder::eval_woffset(compiler->evaluator(), (std::uint32_t)off)) return err::fail;
					if (!ILBuilder::eval_load(compiler->evaluator(), ILDataType::word)) return err::fail;
				}

				ret.lvalue = false;
				ret.type = mf;
			}
		}
		else {

			Cursor err = c;
			c.move();

			bool continue_deeper = true;

			while (continue_deeper) {
				if (!Operand::deref(ret, cpt)) return err::fail;
				if (ret.type->type() != TypeInstanceType::type_structure_instance) {
					return throw_specific_error(err, "Operator cannot be used on this type");
				}


				TypeStructureInstance* ti = (TypeStructureInstance*)ret.type;
				if (!ti->compile()) return err::fail;
				StructureInstance* si = ti->owner;

				auto table_element = si->member_table.find(err.buffer());
				if (table_element != si->member_table.end()) {
					switch (table_element->second.second)
					{
						case MemberTableEntryType::alias:
							continue_deeper = true;
							if (!Operand::structure_element_offset(ret, table_element->second.first, cpt)) return err::fail;
							break;
						case MemberTableEntryType::var:
							continue_deeper = false;
							if (!Operand::structure_element_offset(ret, table_element->second.first, cpt)) return err::fail;
							break;
						case MemberTableEntryType::func: {

							// rvalues will be stored in temporary memory location
							if (!ret.lvalue && !ret.type->rvalue_stacked()) {
								if (cpt == CompileType::compile) {
									stackid_t local_id = compiler->target()->local_stack_lifetime.append(ret.type->size());
									compiler->temp_stack()->push_item("$tmp", ret.type, local_id);
									
									if (!ILBuilder::build_store(compiler->scope(), ret.type->rvalue())) return err::fail;
									if (!ILBuilder::build_local(compiler->scope(), local_id)) return err::fail;
									ret.lvalue = true;
								}
								else {
									stackid_t local_id = compiler->push_local(ret.type->size());
									compiler->compiler_stack()->push_item("$tmp", ret.type, local_id);
									if (!ILBuilder::eval_store(compiler->evaluator(), ret.type->rvalue())) return err::fail;
									compiler->eval_local(local_id);
									ret.lvalue = true;
								}
							}

							if (!ret.lvalue && !ret.type->rvalue_stacked()) {
								return throw_wrong_token_error(c, "This function can be called only from lvalue object or reference");
							}

							if (c.tok != RecognizedToken::OpenParenthesis) {
								return throw_wrong_token_error(c, "'('");
							}

							FunctionInstance* finst;
							if (!si->subfunctions[table_element->second.first]->generate(nullptr, finst)) return err::fail;
							if (!finst->compile()) return err::fail;

							if (cpt == CompileType::compile) {
								if (!ILBuilder::build_fnptr(compiler->scope(), finst->func)) return err::fail;
							}
							else {
								if (!ILBuilder::eval_fnptr(compiler->evaluator(), finst->func)) return err::fail;
							}

							ret.type = finst->type->function_type;
							ret.lvalue = false;
							if (!Operand::function_call(ret, c, cpt, 1, targets_defer)) return err::fail;
							return err::ok;
						}
						
						case MemberTableEntryType::orphan: {

							// rvalues will be stored in temporary memory location
							if (!ret.lvalue && !ret.type->rvalue_stacked()) {
								if (cpt == CompileType::compile) {
									stackid_t local_id = compiler->target()->local_stack_lifetime.append(ret.type->size());
									compiler->temp_stack()->push_item("$tmp", ret.type, local_id);
									
									if (!ILBuilder::build_store(compiler->scope(), ret.type->rvalue())) return err::fail;
									if (!ILBuilder::build_local(compiler->scope(), local_id)) return err::fail;
									ret.lvalue = true;
								}
								else {
									stackid_t local_id = compiler->push_local(ret.type->size());
									compiler->compiler_stack()->push_item("$tmp", ret.type, local_id);
									if (!ILBuilder::eval_store(compiler->evaluator(), ret.type->rvalue())) return err::fail;
									compiler->eval_local(local_id);
									ret.lvalue = true;
								}
							}

							if (!ret.lvalue && !ret.type->rvalue_stacked()) {
								return throw_wrong_token_error(c, "This function can be called only from lvalue object or reference");
							}

							if (c.tok != RecognizedToken::OpenParenthesis) {
								return throw_wrong_token_error(c, "'('");
							}

							FunctionInstance* finst = si->orphaned_functions[table_element->second.first].get();
							if (!finst->compile()) return err::fail;

							if (cpt == CompileType::compile) {
								if (!ILBuilder::build_fnptr(compiler->scope(), finst->func)) return err::fail;
							}
							else {
								if (!ILBuilder::eval_fnptr(compiler->evaluator(), finst->func)) return err::fail;
							}

							ret.type = finst->type->function_type;
							ret.lvalue = false;
							if (!Operand::function_call(ret, c, cpt, 1, targets_defer)) return err::fail;
							return err::ok;
						}
					}
				}
				else {
					return throw_specific_error(err, "Instance does not contain member with this name");
				}
			}

		}
		return err::ok;


	}


	errvoid Operand::parse_string_literal(CompileValue& ret, Cursor& c, CompileType cpt) {
		Compiler* compiler = Compiler::current();
		auto lit = compiler->constant_manager()->register_string_literal(c);

		Type* slice = compiler->types()->t_u8->generate_slice();
		if (!slice->compile()) return err::fail;

		if (cpt == CompileType::compile) {
			if (!ILBuilder::build_const_slice(compiler->scope(), lit.second, ILSize(ILSizeType::abs8,(std::uint32_t)lit.first.length()))) return err::fail;
		}
		else {
			if (!ILBuilder::eval_const_slice(compiler->evaluator(), lit.second, ILSize(ILSizeType::abs8, (std::uint32_t)lit.first.length()))) return err::fail;
		}

		c.move();
		ret.type = slice;
		ret.lvalue = false;
		return err::ok;
	}
}