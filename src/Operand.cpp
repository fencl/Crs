#include "Operand.h"
#include "Error.h"
#include "Utilities.h"
#include "PredefinedTypes.h"
#include "Expression.h"
#include "StackManager.h"
#include "ConstantManager.h"
#include <iostream>

namespace Corrosive {


	void Operand::deref(CompileValue& val, CompileType cpt) {
		if (!val.reflock && val.type->type() == TypeInstanceType::type_reference && ((TypeReference*)val.type)->owner != Compiler::current()->types()->t_void) {
			Expression::rvalue(val, cpt);
			val.lvalue = true;
			val.type = ((TypeReference*)val.type)->owner;	
		}

		val.reflock = false;
	}

	size_t Operand::priv_type_size(Type* t) {
		return t->size().eval(Compiler::current()->global_module(), compiler_arch);
	}

	void Operand::priv_type_template_cast_crsr(ILEvaluator* eval, Cursor& err) {

		Type* template_cast = eval->pop_register_value<Type*>();
		Type* template_type = eval->read_register_value<Type*>();

		if (template_type->type() == TypeInstanceType::type_structure_template) {
			TypeStructureTemplate* st = (TypeStructureTemplate*)template_type;
			TypeTemplate* tt = (TypeTemplate*)template_cast;

			if (st->owner->generic_ctx.generic_layout.size() != Compiler::current()->types()->argument_array_storage.get(tt->argument_array_id).size()) {
				throw_cannot_cast_error(err, template_type, template_cast);
			}
			else {
				for (size_t i = 0; i < st->owner->generic_ctx.generic_layout.size(); i++) {
					if (std::get<1>(st->owner->generic_ctx.generic_layout[i]) != Compiler::current()->types()->argument_array_storage.get(tt->argument_array_id)[i]) {
						throw_cannot_cast_error(err, template_type, template_cast);
					}
				}
			}
		}
		else if (template_type->type() == TypeInstanceType::type_trait_template) {
			TypeTraitTemplate* st = (TypeTraitTemplate*)template_type;
			TypeTemplate* tt = (TypeTemplate*)template_cast;

			if (st->owner->generic_ctx.generic_layout.size() != Compiler::current()->types()->argument_array_storage.get(tt->argument_array_id).size()) {
				throw_cannot_cast_error(err, template_type, template_cast);
			}
			else {
				for (size_t i = 0; i < st->owner->generic_ctx.generic_layout.size(); i++) {
					if (std::get<1>(st->owner->generic_ctx.generic_layout[i]) != Compiler::current()->types()->argument_array_storage.get(tt->argument_array_id)[i]) {
						throw_cannot_cast_error(err, template_type, template_cast);
					}
				}
			}
		}
		else {
			throw_cannot_cast_error(err, template_type, template_cast);
		}
	}

	void Operand::priv_type_template_cast(ILEvaluator* eval) {
		Type* template_cast = eval->pop_register_value<Type*>();
		Type* template_type = eval->read_register_value<Type*>();

		if (template_type->type() == TypeInstanceType::type_structure_template) {
			TypeStructureTemplate* st = (TypeStructureTemplate*)template_type;
			TypeTemplate* tt = (TypeTemplate*)template_cast;

			if (st->owner->generic_ctx.generic_layout.size() != Compiler::current()->types()->argument_array_storage.get(tt->argument_array_id).size()) {
				throw_runtime_exception(eval, "Template cannot be casted to this generic type");
			}
			else {
				for (size_t i = 0; i < st->owner->generic_ctx.generic_layout.size(); i++) {
					if (std::get<1>(st->owner->generic_ctx.generic_layout[i]) != Compiler::current()->types()->argument_array_storage.get(tt->argument_array_id)[i]) {
						throw_runtime_exception(eval, "Template cannot be casted to this generic type");
					}
				}
			}
		}
		else if (template_type->type() == TypeInstanceType::type_trait_template) {
			TypeTraitTemplate* st = (TypeTraitTemplate*)template_type;
			TypeTemplate* tt = (TypeTemplate*)template_cast;

			if (st->owner->generic_ctx.generic_layout.size() != Compiler::current()->types()->argument_array_storage.get(tt->argument_array_id).size()) {
				throw_runtime_exception(eval, "Template cannot be casted to this generic type");
			}
			else {
				for (size_t i = 0; i < st->owner->generic_ctx.generic_layout.size(); i++) {
					if (std::get<1>(st->owner->generic_ctx.generic_layout[i]) != Compiler::current()->types()->argument_array_storage.get(tt->argument_array_id)[i]) {
						throw_runtime_exception(eval, "Template cannot be casted to this generic type");
					}
				}
			}
		}
		else {
			throw_runtime_exception(eval, "Template cannot be casted to this generic type");
		}
	}

	bool Operand::is_numeric_value(Type* t) {
		return t->type()==TypeInstanceType::type_structure_instance && t!=Compiler::current()->types()->t_ptr && t!=Compiler::current()->types()->t_bool && ((TypeStructureInstance*)t)->owner->structure_type==StructureInstanceType::primitive_structure && !t->rvalue_stacked() && t->rvalue() < ILDataType::none;
	}

	void Operand::cast(Cursor& err, CompileValue& res, Type*& to, CompileType cpt, bool implicit) {

		if (res.type == Compiler::current()->types()->t_type && to->type() == TypeInstanceType::type_template) {
			if (cpt == CompileType::eval) {
				ILBuilder::eval_const_type(Compiler::current()->evaluator(), to);
				priv_type_template_cast_crsr(Compiler::current()->evaluator(), err);

				res.type = to;

			}
			else {
				ILBuilder::build_const_type(Compiler::current()->scope(), to);
				ILBuilder::build_insintric(Compiler::current()->scope(), ILInsintric::type_dynamic_cast);
				res.type = to;
			}
		}
		else if (to == Compiler::current()->types()->t_type && res.type->type() == TypeInstanceType::type_template) {
			res.type = to;
		}
		else if ((res.type->type() == TypeInstanceType::type_reference || (res.type->type() == TypeInstanceType::type_structure_instance && res.lvalue)) && to->type() == TypeInstanceType::type_trait) {

			TypeTraitInstance* tt = (TypeTraitInstance*)to;

			TypeStructureInstance* ts = nullptr;
			if (res.type->type() == TypeInstanceType::type_reference) {
				TypeReference* tr = (TypeReference*)res.type;
				if (tr->owner->type() == TypeInstanceType::type_structure_instance) {
					ts = (TypeStructureInstance*)tr->owner;
					Expression::rvalue(res, cpt);
				}
				else {
					throw_cannot_cast_error(err, res.type, to);

				}
			}
			else {
				ts = (TypeStructureInstance*)res.type;
			}


			ts->compile();

			auto tti = ts->owner->traitfunctions.find(tt->owner);
			if (tti == ts->owner->traitfunctions.end()) {
				throw_cannot_cast_error(err, res.type, to);
				std::cerr << " |\trequested trait is not implemented in the structure\n";

			}

			tt->compile();

			uint32_t vtableid = 0;
			auto vtbl = tt->owner->vtable_instances.find(ts->owner);
			if (vtbl == tt->owner->vtable_instances.end()) {
				tt->owner->generate_vtable(ts->owner, vtableid);
			}
			else {
				vtableid = vtbl->second;
			}

			if (cpt == CompileType::eval) {

				ILBuilder::eval_vtable(Compiler::current()->evaluator(), vtableid);
				ILBuilder::eval_combine_dword(Compiler::current()->evaluator());

			}
			else {

				ILBuilder::build_vtable(Compiler::current()->scope(), vtableid);
				ILBuilder::build_combine_dword(Compiler::current()->scope());

			}
			res.lvalue = false;
			res.type = to;
		}
		else if (res.type == Compiler::current()->types()->t_ptr && to->type() == TypeInstanceType::type_function) {
			Expression::rvalue(res, cpt);
			res.lvalue = false;
			res.type = to;
		}
		else if (to == Compiler::current()->types()->t_ptr && res.type->type() == TypeInstanceType::type_function) {
			Expression::rvalue(res, cpt);
			res.lvalue = false;
			res.type = to;
		}
		/*else if (res.type->type() == TypeInstanceType::type_reference && to->type() == TypeInstanceType::type_reference) {
			if (res.type != to && implicit) {
				if (((TypeReference*)to)->owner != Compiler::current()->types()->t_void) {
					throw_cannot_implicit_cast_error(err, res.type, to);
				}
			}

			Expression::rvalue(res, cpt);
			res.lvalue = false;
			res.type = to;
		}*/
		else if (to->type() == TypeInstanceType::type_reference && ((TypeReference*)to)->owner == res.type) {
			TypeReference* tr = (TypeReference*)to;
			if (tr->owner != res.type) {
				throw_cannot_cast_error(err, res.type, to);
			}

			if (!res.lvalue) {
				throw_cannot_cast_error(err, res.type, to);
				std::cerr << " |\tType was not lvalue\n";

			}

			res.type = to;
			res.lvalue = false;
		}
		else if (res.type->type() == TypeInstanceType::type_array && to->type() == TypeInstanceType::type_slice && ((TypeArray*)res.type)->owner == ((TypeSlice*)to)->owner) {
			if (cpt == CompileType::compile) {
				ILBuilder::build_const_size(Compiler::current()->scope(), res.type->size());
				ILBuilder::build_combine_dword(Compiler::current()->scope());
			}
			else {
				ILBuilder::eval_const_size(Compiler::current()->evaluator(), res.type->size().eval(Compiler::current()->global_module(), compiler_arch));
				ILBuilder::eval_combine_dword(Compiler::current()->evaluator());
			}

			res.type = to;
			res.lvalue = false;
		}
		else if (res.type->type() == TypeInstanceType::type_slice && to->type() == TypeInstanceType::type_array && ((TypeArray*)res.type)->owner == ((TypeSlice*)to)->owner) {
			to->compile();
			if (cpt == CompileType::compile) {
				stackid_t local_id = Compiler::current()->target()->local_stack_lifetime.append(to->size());
				Compiler::current()->temp_stack()->push_item("$tmp", to, local_id);

				ILBuilder::build_bitcast(Compiler::current()->scope(), ILDataType::dword, ILDataType::word);
				ILBuilder::build_local(Compiler::current()->scope(), local_id);
				ILBuilder::build_memcpy(Compiler::current()->scope(), to->size());
				ILBuilder::build_local(Compiler::current()->scope(), local_id);
			}
			else {
				stackid_t local_id = Compiler::current()->push_local(to->size());
				Compiler::current()->compiler_stack()->push_item("$tmp", to, local_id);
				size_t slice_size = (size_t)Compiler::current()->evaluator()->read_register_value<dword_t>().p2;
				size_t array_size = to->size().eval(Compiler::current()->global_module(), compiler_arch);
				if (slice_size != array_size) {
					throw_specific_error(err, "The array has different size than casted slice");
				}

				ILBuilder::eval_bitcast(Compiler::current()->evaluator(), ILDataType::dword, ILDataType::word);
				Compiler::current()->eval_local(local_id);
				ILBuilder::eval_memcpy(Compiler::current()->evaluator(), array_size);
				Compiler::current()->eval_local(local_id);
			}

			res.type = to;
			res.lvalue = false;
		}
		else if (Operand::is_numeric_value(res.type) && to == Compiler::current()->types()->t_bool) {
			Expression::rvalue(res, cpt);
			if (cpt == CompileType::eval) {
				if (res.type->rvalue() != to->rvalue()) 
					ILBuilder::eval_bitcast(Compiler::current()->evaluator(), res.type->rvalue(), to->rvalue());
				

				res.type = to;
				res.lvalue = false;
			}
			else {
				if (res.type->rvalue() != to->rvalue())
					ILBuilder::build_bitcast(Compiler::current()->scope(), res.type->rvalue(), to->rvalue());

				res.type = to;
				res.lvalue = false;
			}
		}
		else if (Operand::is_numeric_value(res.type) && Operand::is_numeric_value(to)) {
			Expression::rvalue(res, cpt);
			if (cpt == CompileType::eval) {
				if (res.type->rvalue() != to->rvalue()) {
					ILBuilder::eval_cast(Compiler::current()->evaluator(), res.type->rvalue(), to->rvalue());
				}

				res.type = to;
				res.lvalue = false;

			}
			else {
				if (res.type->rvalue() != to->rvalue())
					ILBuilder::build_cast(Compiler::current()->scope(), res.type->rvalue(), to->rvalue());

				res.type = to;
				res.lvalue = false;

			}
		}
		else if (res.type != to) {
			throw_cannot_cast_error(err, res.type, to);
		}

		res.type = to;

	}

	void Operand::parse(Cursor& c, RecognizedToken& tok, CompileValue& res, CompileType cpt, bool targets_defer) {

		res.lvalue = false;
		res.reflock = false;
		res.type = nullptr;

		switch (tok) {
			case RecognizedToken::And:
			case RecognizedToken::DoubleAnd: {
				Operand::parse_reference(res, c,tok, cpt,targets_defer);
				return;
			}
			case RecognizedToken::OpenBracket: {
				Operand::parse_array_type(res, c,tok, cpt,targets_defer);
				return;
			}

			case RecognizedToken::OpenParenthesis: {
				Operand::parse_expression(res, c, tok, cpt);
			}break;

			case RecognizedToken::Symbol: {
				if (c.buffer() == "cast") {
					c.move(tok);
					if (tok != RecognizedToken::OpenParenthesis) {
						throw_wrong_token_error(c, "'('");

					}
					c.move(tok);

					Cursor err = c;
					CompileValue t_res;

					{
						auto state = ScopeState().context(ILContext::compile);

						Expression::parse(c, tok, t_res, CompileType::eval);
						Operand::deref(t_res, cpt);
						Expression::rvalue(t_res, CompileType::eval);
						if (t_res.type != Compiler::current()->types()->t_type) {
							throw_specific_error(err, "Expected type");
						}
					}

					auto to = Compiler::current()->evaluator()->pop_register_value<Type*>();
					to->compile();

					if (tok != RecognizedToken::CloseParenthesis) {
						throw_wrong_token_error(c, "')'");
					}
					c.move(tok);

					err = c;
					Operand::parse(c, tok, res, cpt,targets_defer);
					Operand::deref(res, cpt);
					Operand::cast(err, res, to, cpt, false);
					res.reflock = true;

					return;
				} else if (c.buffer() == "bitcast") {
					c.move(tok);
					if (tok != RecognizedToken::OpenParenthesis) {
						throw_wrong_token_error(c, "'('");

					}
					c.move(tok);

					Cursor err = c;
					CompileValue t_res;

					{
						auto state = ScopeState().context(ILContext::compile);

						Expression::parse(c, tok, t_res, CompileType::eval);
						Operand::deref(t_res, cpt);
						Expression::rvalue(t_res, CompileType::eval);
						if (t_res.type != Compiler::current()->types()->t_type) {
							throw_specific_error(err, "Expected type");
						}
					}

					auto to = Compiler::current()->evaluator()->pop_register_value<Type*>();
					to->compile();

					if (tok != RecognizedToken::CloseParenthesis) {
						throw_wrong_token_error(c, "')'");
					}
					c.move(tok);

					err = c;
					Operand::parse(c, tok, res, cpt, targets_defer);
					Operand::deref(res, cpt);
					Expression::rvalue(res, cpt);
					if (res.type->rvalue_stacked() || to->rvalue_stacked()) {
						throw_specific_error(err, "This type cannot be bitcasted (yet)");
					}

					if (cpt == CompileType::compile) {
						ILBuilder::build_bitcast(Compiler::current()->scope(), res.type->rvalue(), to->rvalue());
					}
					else {
						ILBuilder::eval_bitcast(Compiler::current()->evaluator(), res.type->rvalue(), to->rvalue());
					}

					res.type = to;
					res.lvalue = false;
					res.reflock = true;
					return;
				}
				else {
					Operand::parse_symbol(res, c,tok, cpt,targets_defer);
				}
			}break;

			case RecognizedToken::Minus: {
				Cursor err = c;
				c.move(tok);
				Expression::parse(c,tok, res, cpt);
				Operand::deref(res, cpt);
				Expression::rvalue(res, cpt);

				if (!Operand::is_numeric_value(res.type)) {
					throw_specific_error(err, "Operation requires number operand");
				}

				if (cpt == CompileType::compile) {
					ILBuilder::build_negative(Compiler::current()->scope(), res.type->rvalue());
				}
				else {
					ILBuilder::eval_negative(Compiler::current()->evaluator(), res.type->rvalue());
				}
			}return;

			case RecognizedToken::ExclamationMark: {
				c.move(tok);
				Cursor err = c;
				Expression::parse(c,tok, res, cpt);
				Operand::deref(res, cpt);
				Expression::rvalue(res, cpt);
				Operand::cast(err, res, Compiler::current()->types()->t_bool, cpt, true);

				if (cpt == CompileType::compile) {
					ILBuilder::build_negate(Compiler::current()->scope());
				}
				else {
					ILBuilder::eval_negate(Compiler::current()->evaluator());
				}
			}return;

			case RecognizedToken::Number:
			case RecognizedToken::UnsignedNumber: {
				Operand::parse_number(res, c,tok, cpt);
			}break;

			case RecognizedToken::LongNumber:
			case RecognizedToken::UnsignedLongNumber: {
				Operand::parse_long_number(res, c,tok, cpt);
			}break;

			case RecognizedToken::FloatNumber:
			case RecognizedToken::DoubleNumber: {
				Operand::parse_float_number(res, c,tok, cpt);
			}break;

			case RecognizedToken::String: {
				Operand::parse_string_literal(res, c,tok, cpt);
			}break;


			default: {
				throw_specific_error(c, "Expected to parse operand");
			} break;
		}

		while (true) {
			switch (tok) {
				case RecognizedToken::Symbol: {
					if (c.buffer() == "cast") {
						//Expression::rvalue(ret, cpt);

						c.move(tok);
						if (tok != RecognizedToken::OpenParenthesis) {
							throw_wrong_token_error(c, "'('");
						}
						c.move(tok);



						Cursor err = c;

						CompileValue t_res;
						{
							auto state = ScopeState().context(ILContext::compile);
							Expression::parse(c, tok, t_res, CompileType::eval);
							Operand::deref(t_res, cpt);
							Expression::rvalue(t_res, CompileType::eval);
						}

						if (t_res.type != Compiler::current()->types()->t_type) {
							throw_specific_error(err, "Expected type");
						}

						auto to = Compiler::current()->evaluator()->pop_register_value<Type*>();
						to->compile();

						if (tok != RecognizedToken::CloseParenthesis) {
							throw_wrong_token_error(c, "')'");

						}
						c.move(tok);


						Operand::deref(res, cpt);
						Operand::cast(err, res, to, cpt, false);

						res.reflock = true;
					}
					else if (c.buffer() == "bitcast") {
						c.move(tok);
						if (tok != RecognizedToken::OpenParenthesis) {
							throw_wrong_token_error(c, "'('");

						}
						c.move(tok);

						Cursor err = c;
						CompileValue t_res;

						{
							auto state = ScopeState().context(ILContext::compile);

							Expression::parse(c, tok, t_res, CompileType::eval);
							Operand::deref(t_res, cpt);
							Expression::rvalue(t_res, CompileType::eval);
							if (t_res.type != Compiler::current()->types()->t_type) {
								throw_specific_error(err, "Expected type");
							}
						}

						auto to = Compiler::current()->evaluator()->pop_register_value<Type*>();
						to->compile();

						if (tok != RecognizedToken::CloseParenthesis) {
							throw_wrong_token_error(c, "')'");
						}
						c.move(tok);

						err = c;
						Operand::deref(res, cpt);
						Expression::rvalue(res, cpt);

						if (res.type->rvalue_stacked() || to->rvalue_stacked()) {
							throw_specific_error(err, "This type cannot be bitcasted (yet)");
						}

						if (cpt == CompileType::compile) {
							ILBuilder::build_bitcast(Compiler::current()->scope(), res.type->rvalue(), to->rvalue());
						}
						else {
							ILBuilder::eval_bitcast(Compiler::current()->evaluator(), res.type->rvalue(), to->rvalue());
						}

						res.type = to;
						res.lvalue = false;
						res.reflock = true;
					}
					else goto break_while;
				}break;
				case RecognizedToken::OpenParenthesis: {
					parse_call_operator(res, c, tok, cpt,targets_defer);
				}break;
				case RecognizedToken::OpenBracket: {
					parse_array_operator(res, c, tok, cpt);
				}break;
				case RecognizedToken::Dot: {
					parse_dot_operator(res, c, tok, cpt,targets_defer);
				}break;
				case RecognizedToken::DoubleColon: {
					parse_double_colon_operator(res, c, tok, cpt,targets_defer);
				}break;
				default: goto break_while;
			}

			continue;
		break_while:
			break;
		}
	}

	void Operand::parse_const_type_function(Cursor& c, RecognizedToken& tok, CompileValue& res) {

		auto state = ScopeState().context(ILContext::compile);
		Cursor err = c;
		FindNameResult found;

		StackItem sitm;
		if (Compiler::current()->compiler_stack()->find(c.buffer(), sitm)) {
			if (sitm.type == Compiler::current()->types()->t_type) {
				Compiler::current()->eval_local(sitm.id);
				ILBuilder::eval_load(Compiler::current()->evaluator(), ILDataType::word);
				Type* rec_type = Compiler::current()->evaluator()->pop_register_value<Type*>();


				if (rec_type->type() == TypeInstanceType::type_structure_instance) {
					c.move(tok);
					if (tok != RecognizedToken::DoubleColon) {
						ILBuilder::build_const_type(Compiler::current()->scope(), rec_type);
						res.type = Compiler::current()->types()->t_type;
						res.lvalue = false;
						return;
					}
					c.move(tok);

					found = ((TypeStructureInstance*)rec_type)->owner->find_name(c.buffer());
				}
				else if (rec_type->type() == TypeInstanceType::type_structure_template) {
					found = ((TypeStructureTemplate*)rec_type)->owner;
				}
				else if (rec_type->type() == TypeInstanceType::type_function_template) {
					found = ((TypeFunctionTemplate*)rec_type)->owner;
				}
				else {
					throw_specific_error(c, "Expected structure type");
				}

				c.move(tok);
			}
			else {
				uint8_t* source = Compiler::current()->stack_ptr(sitm.id);
				sitm.type->constantize(c,nullptr, source);

				res.type = sitm.type; res.lvalue = false;
				c.move(tok);
				return;
			}
		}
		else {
			found = Compiler::current()->workspace()->find_name(c.buffer());

			if (found.type() == FindNameResultType::None) {
				throw_specific_error(c, "Path start point not found");
			}


			c.move(tok);
			while (tok == RecognizedToken::DoubleColon && found.type() == FindNameResultType::Namespace) {
				c.move(tok);
				if (tok != RecognizedToken::Symbol)
				{
					throw_not_a_name_error(c);
				}
				err = c;

				found = found.get_namespace()->find_name(c.buffer());
				c.move(tok);
			}
		}


		while (found.type() == FindNameResultType::Structure) {
			auto struct_inst = found.get_structure();
			struct_inst->compile();

			StructureInstance* inst;
			if (struct_inst->ast_node->is_generic) {
				if (tok != RecognizedToken::OpenParenthesis) {
					ILBuilder::build_const_type(Compiler::current()->scope(), struct_inst->type.get());
					res.type = Compiler::current()->types()->t_type;
					res.lvalue = false;
					return;
				}
				c.move(tok);

				Operand::parse_generate_template(c, tok, struct_inst, inst);
			}
			else {
				struct_inst->generate(nullptr, inst);
			}



			if (tok != RecognizedToken::DoubleColon) {
				ILBuilder::build_const_type(Compiler::current()->scope(), inst->type.get());
				res.type = Compiler::current()->types()->t_type;
				res.lvalue = false;
				return;
			}
			c.move(tok);

			found = inst->find_name(c.buffer());
			c.move(tok);
		}


		if (found.type() == FindNameResultType::Function) {
			auto func_inst = found.get_function();
			func_inst->compile();

			if (!(func_inst->ast_node->has_body() && ((AstFunctionNode*)func_inst->ast_node)->is_generic)) {
				FunctionInstance* inst;
				func_inst->generate(nullptr, inst);
				inst->compile();


				ILBuilder::build_fnptr(Compiler::current()->scope(), inst->func);
				res.type = inst->type;
				res.lvalue = false;

				return;
			}
			else {
				FunctionInstance* inst;
				if (tok != RecognizedToken::OpenParenthesis) {
					ILBuilder::build_const_type(Compiler::current()->scope(), func_inst->type.get());
					res.type = Compiler::current()->types()->t_type;
					res.lvalue = false;
					return;
				}
				c.move(tok);

				Operand::parse_generate_template(c, tok, func_inst, inst);
				inst->compile();

				ILBuilder::build_fnptr(Compiler::current()->scope(), inst->func);
				res.type = inst->type;
				res.lvalue = false;
				return;
			}

		}
		else if (found.type() == FindNameResultType::Static) {
			auto static_inst = found.get_static();
			static_inst->compile();

			ILBuilder::build_staticref(Compiler::current()->scope(), static_inst->sid);
			res.type = static_inst->type;
			res.lvalue = true;
		}
		else {
			throw_specific_error(err, "Only function or static variable may be brought in the runtime context");
		}
	}



	void Operand::parse_expression(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType cpt) {
		c.move(tok);
		Expression::parse(c, tok, ret, cpt);
		if (tok != RecognizedToken::CloseParenthesis) {
			throw_wrong_token_error(c, "')'");

		}
		c.move(tok);
	}



	void Operand::parse_symbol(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType cpt, bool targets_defer) {
		auto buf = c.buffer();

		if (buf == "true") {
			c.move(tok);
			ret.lvalue = false;
			ret.type = Compiler::current()->types()->t_bool;
			if (cpt == CompileType::compile) {
				ILBuilder::build_const_i8(Compiler::current()->scope(), true);
			}
			else if (cpt == CompileType::eval) {
				ILBuilder::eval_const_i8(Compiler::current()->evaluator(), true);
			}
		}
		else if (buf == "false") {
			c.move(tok);
			ret.lvalue = false;
			ret.type = Compiler::current()->types()->t_bool;
			if (cpt == CompileType::compile) {
				ILBuilder::build_const_i8(Compiler::current()->scope(), false);
			}
			else if (cpt == CompileType::eval) {
				ILBuilder::eval_const_i8(Compiler::current()->evaluator(), false);
			}
		}
		else if (buf == "self") {
			ret.lvalue = false;
			ret.type = Compiler::current()->types()->t_type;

			Namespace* nspc = Compiler::current()->workspace();
			StructureInstance* s = dynamic_cast<StructureInstance*>(nspc);

			if (!s) {
				throw_specific_error(c, "Self must be used inside structure");
			}
			else {

				if (Compiler::current()->scope_context() != ILContext::compile) {
					throw_specific_error(c, "Self type can be used only in compile context");
				}

				if (cpt == CompileType::compile) {
					ILBuilder::build_const_type(Compiler::current()->scope(), s->type.get());
				}
				else if (cpt == CompileType::eval) {
					ILBuilder::eval_const_type(Compiler::current()->evaluator(), s->type.get());
				}
			}

			c.move(tok);
		}
		else if (buf == "null") {
			c.move(tok);
			ret.lvalue = false;
			ret.type = Compiler::current()->types()->t_ptr;

			if (cpt == CompileType::compile) {
				ILBuilder::build_null(Compiler::current()->scope());
			}
			else if (cpt == CompileType::eval) {
				ILBuilder::eval_null(Compiler::current()->evaluator());
			}
		}
		else if (buf == "typesize") {
			c.move(tok);
			auto state = ScopeState().context(ILContext::compile);

			CompileValue type_val;
			Cursor err = c;
			Operand::parse(c,tok, type_val, CompileType::eval,targets_defer);
			Expression::rvalue(type_val, CompileType::eval);

			if (type_val.type != Compiler::current()->types()->t_type) {
				throw_specific_error(err, "Expected type");
			}

			auto tp = Compiler::current()->evaluator()->pop_register_value<Type*>();
			tp->compile();

			if (cpt == CompileType::compile) {
				ILBuilder::build_const_size(Compiler::current()->scope(), tp->size());
			}
			else {
				ILBuilder::eval_const_size(Compiler::current()->evaluator(), tp->size().eval(Compiler::current()->global_module(), compiler_arch));
			}

			ret.lvalue = false;
			ret.type = Compiler::current()->types()->t_size;
		}
		else if (buf == "type") {

			c.move(tok);
			if (tok != RecognizedToken::OpenParenthesis) {
				Compiler::current()->evaluator()->write_register_value(Compiler::current()->types()->t_type);
			}
			else {
				c.move(tok);
				std::vector<Type*> arg_types;

				if (tok != RecognizedToken::CloseParenthesis) {
					while (true) {
						Cursor err = c;
						CompileValue arg;
						Expression::parse(c,tok, arg, CompileType::eval);
						Operand::deref(arg, cpt);
						Expression::rvalue(arg, CompileType::eval);

						if (arg.type != Compiler::current()->types()->t_type) {
							throw_specific_error(err, "Expected type");

						}

						auto type = Compiler::current()->evaluator()->pop_register_value<Type*>();
						arg_types.push_back(type);

						if (tok == RecognizedToken::Comma) {
							c.move(tok);
						}
						else if (tok == RecognizedToken::CloseParenthesis) {
							break;
						}
						else {
							throw_wrong_token_error(c, "',' or ')'");
						}
					}
				}
				c.move(tok);


				Type* ftype = Compiler::current()->types()->load_or_register_template_type(std::move(arg_types));

				Compiler::current()->evaluator()->write_register_value(ftype);
			}

			ret.lvalue = false;
			ret.type = Compiler::current()->types()->t_type;
		}
		else if (buf == "fn") {
			if (cpt == CompileType::compile) {
				if (Compiler::current()->scope_context() != ILContext::compile) {
					throw_specific_error(c, "Function type cannot be created in runtime context");

				}
			}

			c.move(tok);
			ILCallingConvention call_conv = ILCallingConvention::bytecode;
			ILContext t_context = ILContext::both;

			while (true)
			{
				auto buf = c.buffer();
				if (tok == RecognizedToken::Symbol && buf == "compile") {
					t_context = ILContext::compile;
					c.move(tok);
				}
				else if (tok == RecognizedToken::Symbol && buf == "runtime") {
					t_context = ILContext::runtime;
					c.move(tok);
				}
				else if (tok == RecognizedToken::Symbol && buf == "native") {
					call_conv = ILCallingConvention::native;
					c.move(tok);
				}
				else if (tok == RecognizedToken::Symbol && buf == "stdcall") {
					call_conv = ILCallingConvention::stdcall;
					c.move(tok);
				}
				else {
					break;
				}
			}

			if (tok != RecognizedToken::OpenParenthesis) {
				throw_wrong_token_error(c, "(");
			}
			c.move(tok);

			std::vector<Type*> arg_types;

			if (tok != RecognizedToken::CloseParenthesis) {
				while (true) {
					Cursor err = c;
					CompileValue arg;
					Expression::parse(c,tok, arg, CompileType::eval);
					Operand::deref(arg, cpt);
					Expression::rvalue(arg, CompileType::eval);

					if (arg.type != Compiler::current()->types()->t_type) {
						throw_specific_error(err, "Expected type");

					}

					auto type = Compiler::current()->evaluator()->pop_register_value<Type*>();
					arg_types.push_back(type);

					if (tok == RecognizedToken::Comma) {
						c.move(tok);
					}
					else if (tok == RecognizedToken::CloseParenthesis) {
						break;
					}
					else {
						throw_wrong_token_error(c, "',' or ')'");
					}
				}
			}
			c.move(tok);
			Type* ret_type = Compiler::current()->types()->t_void;

			if (tok == RecognizedToken::Symbol || tok == RecognizedToken::And || tok == RecognizedToken::DoubleAnd) {
				Cursor err = c;
				CompileValue rt;
				Expression::parse(c,tok, rt, CompileType::eval);
				Operand::deref(rt, cpt);
				Expression::rvalue(rt, CompileType::eval);

				if (rt.type != Compiler::current()->types()->t_type) {
					throw_specific_error(err, "Expected type");

				}
				ret_type = Compiler::current()->evaluator()->pop_register_value<Type*>();
			}

			Type* ftype = Compiler::current()->types()->load_or_register_function_type(call_conv, std::move(arg_types), ret_type, t_context);

			Compiler::current()->evaluator()->write_register_value(ftype);
			ret.lvalue = false;
			ret.type = Compiler::current()->types()->t_type;
		}
		else {

			StackItem sitm;

			if (cpt == CompileType::compile && Compiler::current()->stack()->find(c.buffer(), sitm)) {
				ILBuilder::build_local(Compiler::current()->scope(), sitm.id);
				ret.type = sitm.type;
				ret.lvalue = true;
				c.move(tok);
			}
			else if (cpt != CompileType::compile && Compiler::current()->compiler_stack()->find(c.buffer(), sitm)) {
				Compiler::current()->eval_local(sitm.id);
				ret.type = sitm.type;
				ret.lvalue = true;
				c.move(tok);
			}
			else if (cpt == CompileType::compile) {
				
				Cursor err = c;
				parse_const_type_function(c,tok, ret);
			}
			else {


				Cursor err = c;

				auto res = Compiler::current()->workspace()->find_name(c.buffer());

				if (res.type() == FindNameResultType::None) {
					throw_specific_error(c, "Path start point not found");
				}


				Cursor nm_err = c;
				c.move(tok);
				while (tok == RecognizedToken::DoubleColon && res.type() == FindNameResultType::Namespace) {
					c.move(tok);
					if (tok != RecognizedToken::Symbol)
					{
						throw_not_a_name_error(c);
					}
					nm_err = c;

					res = res.get_namespace()->find_name(c.buffer());
					c.move(tok);
				}


				if (auto struct_inst = res.get_structure()) {
					struct_inst->compile();

					if (struct_inst->ast_node->is_generic) {
						if (cpt == CompileType::eval) {

							ILBuilder::eval_const_type(Compiler::current()->evaluator(), struct_inst->type.get());
						}
						else if (cpt == CompileType::compile) {
							if (Compiler::current()->scope_context() != ILContext::compile) {
								throw_specific_error(err, "Use of a type in runtime context");

							}

							ILBuilder::build_const_type(Compiler::current()->scope(), struct_inst->type.get());
						}
					}
					else {
						StructureInstance* inst;
						struct_inst->generate(nullptr, inst);

						if (cpt == CompileType::eval) {
							ILBuilder::eval_const_type(Compiler::current()->evaluator(), inst->type.get());
						}
						else if (cpt == CompileType::compile) {
							if (Compiler::current()->scope_context() != ILContext::compile) {
								throw_specific_error(err, "Use of a type in runtime context");

							}
							ILBuilder::build_const_type(Compiler::current()->scope(), inst->type.get());
						}
					}

					ret.lvalue = false;
					ret.type = Compiler::current()->types()->t_type;
				}
				else if (auto func_inst = res.get_function()) {

					func_inst->compile();

					if (!(func_inst->ast_node->has_body() && ((AstFunctionNode*)func_inst->ast_node)->is_generic)) {
						FunctionInstance* inst;
						func_inst->generate(nullptr, inst);
						inst->compile();

						if (cpt == CompileType::eval) {
							ILBuilder::eval_fnptr(Compiler::current()->evaluator(), inst->func);
							//ILBuilder::eval_const_type(ctx.eval, inst->type);
						}
						else if (cpt == CompileType::compile) {
							ILBuilder::build_fnptr(Compiler::current()->scope(), inst->func);
						}

						ret.lvalue = false;
						ret.type = inst->type;
					}
					else {
						if (cpt == CompileType::eval) {
							ILBuilder::eval_const_type(Compiler::current()->evaluator(), func_inst->type.get());
						}
						else if (cpt == CompileType::compile) {
							if (Compiler::current()->scope_context() != ILContext::compile) {
								throw_specific_error(err, "Use of a type in runtime context");

							}

							ILBuilder::build_const_type(Compiler::current()->scope(), func_inst->type.get());
						}

						ret.lvalue = false;
						ret.type = Compiler::current()->types()->t_type;
					}
				}
				else if (auto trait_inst = res.get_trait()) {
					trait_inst->compile();

					if (trait_inst->ast_node->is_generic) {
						if (cpt == CompileType::eval) {
							ILBuilder::eval_const_type(Compiler::current()->evaluator(), trait_inst->type.get());
						}
						else if (cpt == CompileType::compile) {
							if (Compiler::current()->scope_context() != ILContext::compile) {
								throw_specific_error(err, "Use of a type in runtime context");

							}

							ILBuilder::build_const_type(Compiler::current()->scope(), trait_inst->type.get());
						}
					}
					else {
						TraitInstance* inst;
						trait_inst->generate(nullptr, inst);

						if (cpt == CompileType::eval) {
							ILBuilder::eval_const_type(Compiler::current()->evaluator(), inst->type.get());
						}
						else if (cpt == CompileType::compile) {
							if (Compiler::current()->scope_context() != ILContext::compile) {
								throw_specific_error(err, "Use of a type in runtime context");

							}
							ILBuilder::build_const_type(Compiler::current()->scope(), inst->type.get());
						}
					}



					ret.lvalue = false;
					ret.type = Compiler::current()->types()->t_type;
				}
				else if (auto static_inst = res.get_static()) {
					static_inst->compile();

					if (static_inst->context != ILContext::both && static_inst->context != Compiler::current()->scope_context()) {
						throw_specific_error(nm_err, "Static declaration is not targeted for this context");
					}

					if (cpt == CompileType::compile) {
						ILBuilder::build_staticref(Compiler::current()->scope(), static_inst->sid);
					}
					else {
						ILBuilder::eval_staticref(Compiler::current()->evaluator(), static_inst->sid);
					}
					ret.lvalue = true;
					ret.type = static_inst->type;
				}
				else {
					throw_specific_error(nm_err, "Path is pointing to a namespace");
				}

			}

		}


	}




	void Operand::parse_number(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType cpt) {
		bool usg = tok == RecognizedToken::UnsignedNumber;

		std::string_view ndata;
		if (usg)
			ndata = c.buffer().substr(0, c.length - 1);
		else
			ndata = c.buffer();

		unsigned long long d = svtoi(ndata);
		c.move(tok);

		if (cpt == CompileType::compile) {
			if (usg)
				ILBuilder::build_const_u32(Compiler::current()->scope(), (uint32_t)d);
			else
				ILBuilder::build_const_i32(Compiler::current()->scope(), (int32_t)d);
		}
		else if (cpt == CompileType::eval) {
			if (usg) {
				ILBuilder::eval_const_u32(Compiler::current()->evaluator(), (uint32_t)d);
			}
			else {
				ILBuilder::eval_const_i32(Compiler::current()->evaluator(), (int32_t)d);
			}

		}

		ret.type = usg ? Compiler::current()->types()->t_u32 : Compiler::current()->types()->t_i32;
		ret.lvalue = false;
	}





	void Operand::parse_long_number(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType cpt) {

		bool usg = tok == RecognizedToken::UnsignedLongNumber;

		std::string_view ndata;
		if (usg)
			ndata = c.buffer().substr(0, c.length - 2);
		else
			ndata = c.buffer().substr(0, c.length - 1);

		unsigned long long d = svtoi(ndata);
		c.move(tok);

		if (cpt == CompileType::compile) {
			if (usg)
				ILBuilder::build_const_u64(Compiler::current()->scope(), d);
			else
				ILBuilder::build_const_i64(Compiler::current()->scope(), d);
		}
		else if (cpt == CompileType::eval) {
			if (usg) {
				ILBuilder::eval_const_u64(Compiler::current()->evaluator(), d);
			}
			else {
				ILBuilder::eval_const_i64(Compiler::current()->evaluator(), d);
			}

		}

		ret.type = usg ? Compiler::current()->types()->t_u64 : Compiler::current()->types()->t_i64;
		ret.lvalue = false;
	}





	void Operand::parse_float_number(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType cpt) {

		bool dbl = tok == RecognizedToken::DoubleNumber;

		std::string_view ndata;
		if (dbl)
			ndata = c.buffer().substr(0, c.length - 1);
		else
			ndata = c.buffer();

		double d = svtod(ndata);
		c.move(tok);

		if (cpt == CompileType::compile) {
			if (dbl)
				ILBuilder::build_const_f64(Compiler::current()->scope(), d);
			else
				ILBuilder::build_const_f32(Compiler::current()->scope(), (float)d);
		}
		else if (cpt == CompileType::eval) {
			if (dbl) {
				ILBuilder::eval_const_f64(Compiler::current()->evaluator(), d);
			}
			else {
				ILBuilder::eval_const_f32(Compiler::current()->evaluator(), (float)d);
			}

		}

		ret.type = dbl ? Compiler::current()->types()->t_f64 : Compiler::current()->types()->t_f32;
		ret.lvalue = false;
	}


	template<typename T, typename S>
	void Operand::parse_generate_template(Cursor& c, RecognizedToken& tok, T* generating, S*& out) {
		auto layout = generating->generic_ctx.generic_layout.begin();

		if (tok != RecognizedToken::CloseParenthesis) {
			while (true) {
				if (layout == generating->generic_ctx.generic_layout.end()) {
					throw_specific_error(c, "Too much arguments");
				}

				CompileValue res;
				Cursor err = c;
				Expression::parse(c,tok, res, CompileType::eval);
				Operand::deref(res, CompileType::eval);
				Operand::cast(err, res, std::get<1>(*layout), CompileType::eval, false);
				Expression::rvalue(res, CompileType::eval);

				layout++;

				if (tok == RecognizedToken::Comma) {
					c.move(tok);
				}
				else if (tok == RecognizedToken::CloseParenthesis) {
					break;
				}
				else {
					throw_wrong_token_error(c, "')' or ','");
				}
			}
		}

		if (layout != generating->generic_ctx.generic_layout.end()) {
			throw_specific_error(c, "Not enough arguments");

		}

		c.move(tok);



		auto state = ScopeState().compiler_stack();


		std::vector<std::tuple<Cursor, Type*>>::reverse_iterator act_layout;
		size_t act_layout_size = 0;

		//if (generating->generic_ctx.generator != nullptr) {
		//	generating->generic_ctx.generator->insert_key_on_stack(Compiler::current()->evaluator());
		//}

		act_layout = generating->generic_ctx.generic_layout.rbegin();
		act_layout_size = generating->generic_ctx.generic_layout.size();

		unsigned char* key_base = Compiler::current()->local_stack_base.back();


		for (size_t arg_i = act_layout_size - 1; arg_i >= 0 && arg_i < act_layout_size; arg_i--) {

			Type* type = std::get<1>(*act_layout);
			type->compile();

			stackid_t local_id = Compiler::current()->push_local(type->size());
			Compiler::current()->compiler_stack()->push_item(std::get<0>(*act_layout).buffer(), type, local_id);

			Compiler::current()->eval_local(local_id);
			Expression::copy_from_rvalue(type, CompileType::eval);

			act_layout++;
		}

		generating->generate(key_base, out);

		StackItem sitm;
		while (Compiler::current()->compiler_stack()->pop_item(sitm)) {}

	}

	void Operand::priv_build_push_template(ILEvaluator* eval) {
		Type* t = eval->pop_register_value<Type*>();
		template_stack[template_sp] = t;
		template_sp++;

	}

	void Operand::priv_build_build_template(ILEvaluator* eval) {
		Type* gen_type = template_stack[template_sp - 1];
		gen_type->compile();

		auto state = ScopeState().compiler_stack();

		std::vector<std::tuple<Cursor, Type*>>::reverse_iterator act_layout;
		std::vector<std::tuple<Cursor, Type*>>::reverse_iterator act_layout_it;
		size_t gen_types = 0;

		if (gen_type->type() == TypeInstanceType::type_structure_template) {
			act_layout = ((TypeStructureTemplate*)gen_type)->owner->generic_ctx.generic_layout.rbegin();
			gen_types = ((TypeStructureTemplate*)gen_type)->owner->generic_ctx.generic_layout.size();
		}
		else if (gen_type->type() == TypeInstanceType::type_trait_template) {
			act_layout = ((TypeTraitTemplate*)gen_type)->owner->generic_ctx.generic_layout.rbegin();
			gen_types = ((TypeTraitTemplate*)gen_type)->owner->generic_ctx.generic_layout.size();
		}




		unsigned char* key_base = Compiler::current()->local_stack_base.back() + Compiler::current()->local_stack_size.back();


		act_layout_it = act_layout;
		for (size_t arg_i = gen_types - 1; arg_i >= 0 && arg_i < gen_types; arg_i--) {

			Type* type = std::get<1>((*act_layout_it));

			stackid_t local_id = Compiler::current()->push_local(type->size());
			unsigned char* data_place = Compiler::current()->stack_ptr(local_id);
			Compiler::current()->compiler_stack()->push_item(std::get<0>(*act_layout_it).buffer(), type, local_id);


			eval->write_register_value(data_place);
			Expression::copy_from_rvalue(type, CompileType::eval);

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


		StackItem sitm;
		while (Compiler::current()->compiler_stack()->pop_item(sitm)) {}
	}

	Type* Operand::template_stack[1024];
	uint16_t Operand::template_sp = 0;


	void Operand::read_arguments(Cursor& c, RecognizedToken& tok, unsigned int& argi, TypeFunction* ft, CompileType cpt) {
		if (tok != RecognizedToken::CloseParenthesis) {
			while (true) {
				if (argi >= Compiler::current()->types()->argument_array_storage.get(ft->argument_array_id).size()) {
					throw_specific_error(c, "Wrong number of arguments");
				}

				CompileValue arg;
				Cursor err = c;
				Expression::parse(c,tok, arg, cpt);
				Operand::deref(arg, cpt);
				Operand::cast(err, arg, Compiler::current()->types()->argument_array_storage.get(ft->argument_array_id)[argi], cpt, true);
				Expression::rvalue(arg, cpt);
				argi++;

				if (tok == RecognizedToken::Comma) {
					c.move(tok);
				}
				else if (tok == RecognizedToken::CloseParenthesis) {
					break;
				}
				else {
					throw_wrong_token_error(c, "',' or ')'");

				}
			}
		}


	}

	void Operand::parse_call_operator(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType cpt, bool targets_defer) {

		if (ret.type == Compiler::current()->types()->t_type || ret.type->type() == TypeInstanceType::type_template) {
			Cursor nm_err = c;
			Expression::rvalue(ret, cpt);

			if (cpt != CompileType::compile) {
				Type* dt = nullptr;

				dt = Compiler::current()->evaluator()->pop_register_value<Type*>();

				if (dt->type() != TypeInstanceType::type_structure_template && dt->type() != TypeInstanceType::type_trait_template && dt->type() != TypeInstanceType::type_function_template) {
					throw_specific_error(c, "this type is not a generic type");

				}
				c.move(tok);

				if (dt->type() == TypeInstanceType::type_structure_template) {
					StructureInstance* inst;
					parse_generate_template(c,tok, ((TypeStructureTemplate*)dt)->owner, inst);
					ILBuilder::eval_const_type(Compiler::current()->evaluator(), inst->type.get());

					ret.lvalue = false;
					ret.type = Compiler::current()->types()->t_type;

				}
				else if (dt->type() == TypeInstanceType::type_trait_template) {
					TraitInstance* inst;
					parse_generate_template(c,tok, ((TypeTraitTemplate*)dt)->owner, inst);
					ILBuilder::eval_const_type(Compiler::current()->evaluator(), inst->type.get());

					ret.lvalue = false;
					ret.type = Compiler::current()->types()->t_type;
				}
				else if (dt->type() == TypeInstanceType::type_function_template) {
					FunctionInstance* inst;
					parse_generate_template(c,tok, ((TypeFunctionTemplate*)dt)->owner, inst);
					inst->compile();

					ILBuilder::eval_fnptr(Compiler::current()->evaluator(), inst->func);

					ret.lvalue = false;
					ret.type = inst->type;
				}



			}
			else if (ret.type->type() == TypeInstanceType::type_template) {
				Expression::rvalue(ret, cpt);

				if (cpt == CompileType::compile) {
					ILBuilder::build_const_type(Compiler::current()->scope(), ret.type);

					ILBuilder::build_insintric(Compiler::current()->scope(), ILInsintric::type_dynamic_cast);
					ILBuilder::build_insintric(Compiler::current()->scope(), ILInsintric::push_template);
				}
				else {
					ILBuilder::eval_const_type(Compiler::current()->evaluator(), ret.type);
					ILBuilder::eval_insintric(Compiler::current()->evaluator(), ILInsintric::type_dynamic_cast);
					ILBuilder::eval_insintric(Compiler::current()->evaluator(), ILInsintric::push_template);
				}

				TypeTemplate* tt = (TypeTemplate*)ret.type;
				auto layout = Compiler::current()->types()->argument_array_storage.get(tt->argument_array_id).begin();

				if (tok != RecognizedToken::OpenParenthesis) {
					throw_wrong_token_error(c, "'('");
				}
				c.move(tok);

				std::vector<CompileValue> results;
				if (tok != RecognizedToken::CloseParenthesis) {
					while (true) {
						CompileValue res;
						Cursor err = c;
						Expression::parse(c,tok, res, cpt);
						Operand::deref(res, cpt);
						Expression::rvalue(res, cpt);
						Operand::cast(err, res, *layout, cpt, true);

						results.push_back(res);
						layout++;

						if (tok == RecognizedToken::Comma) {
							c.move(tok);
						}
						else if (tok == RecognizedToken::CloseParenthesis) {
							break;
						}
						else {
							throw_wrong_token_error(c, "')' or ','");
						}
					}
				}

				c.move(tok);

				if (cpt == CompileType::compile) {
					ILBuilder::build_insintric(Compiler::current()->scope(), ILInsintric::build_template);
				}
				else {
					ILBuilder::eval_insintric(Compiler::current()->evaluator(), ILInsintric::build_template);
				}
			}
			else {
				throw_specific_error(c, "Operation not supported on plain type, please cast to generic type");
			}

		}
		else if (ret.type->type() == TypeInstanceType::type_function) {
			function_call(ret, c,tok, cpt, 0,targets_defer);
		}
		else {
			throw_specific_error(c, "not implemented yet");
		}


	}

	Type* Operand::priv_build_reference(Type* t) {
		return t->generate_reference();
	}

	Type* Operand::priv_build_subtype(Type* t, dword_t slice) {
		std::string_view slice_str((char*)slice.p1, (size_t)slice.p2);

		Type* str = Compiler::current()->types()->t_u8->generate_slice();

		if (t->type() == TypeInstanceType::type_structure_instance) {
			TypeStructureInstance* tsi = (TypeStructureInstance*)t;

			auto res = tsi->owner->find_name(slice_str);

			if (auto struct_template = res.get_structure()) {
				struct_template->compile();
				if (!struct_template->ast_node->is_generic) {
					StructureInstance* sinst;
					struct_template->generate(nullptr, sinst);
					sinst->compile();
					return sinst->type.get();
				}
				else {
					return struct_template->type.get();
				}
			}
			else if (auto function_temaplte = res.get_function()) {
				function_temaplte->compile();
				return function_temaplte->type.get();
			}
			else if (auto trait_template = res.get_trait()) {
				trait_template->compile();
				if (!trait_template->ast_node->is_generic) {
					TraitInstance* trait_instance;
					trait_template->generate(nullptr, trait_instance);
					return trait_instance->type.get();
				}
				else {
					return trait_template->type.get();
				}
			}

		}

		return nullptr;
	}

	Type* Operand::priv_build_slice(Type* t) {
		return t->generate_slice();
	}

	void Operand::parse_reference(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType cpt, bool targets_defer) {
		Cursor operr = c;

		unsigned int type_ref_count = 0;
		while (tok == RecognizedToken::And || tok == RecognizedToken::DoubleAnd) {
			type_ref_count++;
			if (tok == RecognizedToken::DoubleAnd)
				type_ref_count++;
			c.move(tok);
		}

		Cursor err = c;
		Operand::parse(c,tok, ret, cpt, targets_defer);

		if (ret.type == Compiler::current()->types()->t_type) {
			Expression::rvalue(ret, cpt);

			if (cpt == CompileType::eval) {
				Type* t = Compiler::current()->evaluator()->pop_register_value<Type*>();
				for (unsigned int i = 0; i < type_ref_count; i++)
					t = t->generate_reference();

				Compiler::current()->evaluator()->write_register_value(t);
			}
			else if (cpt == CompileType::compile) {
				for (unsigned int i = 0; i < type_ref_count; i++) {
					ILBuilder::build_fncall(Compiler::current()->scope(), Compiler::current()->types()->f_build_reference->func);
				}
			}

			ret.type = Compiler::current()->types()->t_type;
			ret.lvalue = false;
		}
		else if (ret.type->type() == TypeInstanceType::type_reference) {
			ret.reflock = true;
		}
		else {
			throw_specific_error(err, "operator expected to recieve type or reference");
		}
	}

	void Operand::parse_array_operator(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType cpt) {

		while (ret.type->type() != TypeInstanceType::type_slice && (ret.type->type() != TypeInstanceType::type_reference || ((TypeReference*)ret.type)->owner->type() != TypeInstanceType::type_slice)) {

			if (ret.type->type() == TypeInstanceType::type_reference) {
				if (ret.lvalue) {
					if (cpt == CompileType::compile) {
						ILBuilder::build_load(Compiler::current()->scope(), ret.type->rvalue());
					}
					else if (cpt == CompileType::eval) {
						ILBuilder::eval_load(Compiler::current()->evaluator(), ret.type->rvalue());
					}
				}

				ret.type = ((TypeReference*)ret.type)->owner;
				ret.lvalue = true;
			}

			if (ret.type->type() != TypeInstanceType::type_structure_instance) {
				throw_specific_error(c, "Offset can be applied only on slices or passed to slice by alias");
			}


			TypeStructureInstance* ti = (TypeStructureInstance*)ret.type;
			ti->compile();
			StructureInstance* si = ti->owner;

			if (!si->pass_array_operator) {
				throw_specific_error(c, "Offset can be applied only on slices or passed to slice by alias");
			}

			Operand::structure_element_offset(ret, si->pass_array_id, cpt);
		}

		if (ret.type->type() == TypeInstanceType::type_reference && ((TypeReference*)ret.type)->owner->type() == TypeInstanceType::type_slice) {
			if (ret.lvalue) {
				if (cpt == CompileType::compile) {
					ILBuilder::build_load(Compiler::current()->scope(), ret.type->rvalue());
				}
				else if (cpt == CompileType::eval) {
					ILBuilder::eval_load(Compiler::current()->evaluator(), ret.type->rvalue());
				}
			}

			ret.type = ((TypeReference*)ret.type)->owner;
			ret.lvalue = true;

		}

		if (ret.type->type() != TypeInstanceType::type_slice) {
			throw_specific_error(c, "Offset can be applied only on slices or passed to slice by alias");
		}
		c.move(tok);

		TypeSlice* slice = (TypeSlice*)ret.type;
		Type* base_slice = slice->owner;


		if (cpt == CompileType::compile) {
			ILBuilder::build_load(Compiler::current()->scope(), ILDataType::word);
		}
		else {
			ILBuilder::eval_load(Compiler::current()->evaluator(), ILDataType::word);
		}

		Cursor err = c;
		CompileValue index;
		Expression::parse(c,tok, index, cpt);
		Operand::deref(index, cpt);
		Expression::rvalue(index, cpt);
		Operand::cast(err, index, Compiler::current()->types()->t_size, cpt, true);



		if (cpt == CompileType::compile) {
			ILBuilder::build_const_size(Compiler::current()->scope(), base_slice->size());
			ILBuilder::build_mul(Compiler::current()->scope(), ILDataType::word, ILDataType::word);
			ILBuilder::build_rtoffset(Compiler::current()->scope());
		}
		else {
			ILBuilder::eval_const_size(Compiler::current()->evaluator(), base_slice->size().eval(Compiler::current()->global_module(), compiler_arch));
			ILBuilder::eval_mul(Compiler::current()->evaluator(), ILDataType::word, ILDataType::word);
			ILBuilder::eval_rtoffset(Compiler::current()->evaluator());
		}

		if (tok != RecognizedToken::CloseBracket) {
			throw_wrong_token_error(c, "']'");
		}

		c.move(tok);

		ret.lvalue = true;
		ret.type = base_slice;


	}

	Type* Operand::priv_build_array(uint32_t size, Type* t) {
		return t->generate_array(size);
	}

	void Operand::parse_array_type(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType cpt, bool targets_defer) {
		Cursor err = c;
		CompileValue res;
		c.move(tok);

		if (tok == RecognizedToken::CloseBracket) {
			c.move(tok);

			Cursor t_err = c;

			Operand::parse(c,tok, res, cpt, targets_defer);
			Expression::rvalue(res, cpt);

			if (res.type != Compiler::current()->types()->t_type) {
				throw_specific_error(t_err, "Expected type");

			}

			if (cpt == CompileType::eval) {
				Type* t = Compiler::current()->evaluator()->pop_register_value<Type*>();
				Type* nt = t->generate_slice();
				ILBuilder::eval_const_type(Compiler::current()->evaluator(), nt);
			}
			else {
				ILBuilder::build_fncall(Compiler::current()->scope(), Compiler::current()->types()->f_build_slice->func);
			}
		}
		else {
			Expression::parse(c,tok, res, cpt);
			Operand::deref(res, cpt);
			Expression::rvalue(res, cpt);
			Operand::cast(err, res, Compiler::current()->types()->t_u32, cpt, true);


			if (tok != RecognizedToken::CloseBracket) {
				throw_wrong_token_error(c, "']'");
			}

			c.move(tok);

			Cursor t_err = c;

			Operand::parse(c,tok, res, cpt, targets_defer);
			Expression::rvalue(res, cpt);

			if (res.type != Compiler::current()->types()->t_type) {
				throw_specific_error(t_err, "Expected type");

			}

			if (cpt == CompileType::eval) {
				Type* t = Compiler::current()->evaluator()->pop_register_value<Type*>();
				uint32_t val = Compiler::current()->evaluator()->pop_register_value<uint32_t>();
				TypeArray* nt = t->generate_array(val);
				ILBuilder::eval_const_type(Compiler::current()->evaluator(), nt);
			}
			else {
				ILBuilder::build_fncall(Compiler::current()->scope(), Compiler::current()->types()->f_build_array->func);
			}
		}

		ret.type = Compiler::current()->types()->t_type;
		ret.lvalue = false;

	}


	void Operand::parse_double_colon_operator(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType cpt, bool targets_defer) {
		if (ret.type->type() != TypeInstanceType::type_structure_instance && ret.type != Compiler::current()->types()->t_type) {
			throw_specific_error(c, "Operator :: is only supported on structure instances");
		}

		if (cpt == CompileType::compile) {
			throw_specific_error(c, "Operator :: is not supported in runtime context");
		}

		if (ret.type != Compiler::current()->types()->t_type) {
			throw_specific_error(c, "left operator is not a type instance");
		}

		c.move(tok);

		if (cpt == CompileType::eval) {
			Expression::rvalue(ret, cpt);
			TypeStructureInstance* ti = Compiler::current()->evaluator()->pop_register_value<TypeStructureInstance*>();
			if (ti->type() != TypeInstanceType::type_structure_instance) {
				throw_specific_error(c, "Type is not structure instance");
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
							tplt->generic_ctx.generator->insert_key_on_stack(Compiler::current()->evaluator());*/


						tplt->compile();

						ret.lvalue = false;
						ret.type = Compiler::current()->types()->t_type;

						if (tplt->ast_node->is_generic) {
							ILBuilder::eval_const_type(Compiler::current()->evaluator(), tplt->type.get());
						}
						else {
							StructureInstance* inst = nullptr;

							tplt->generate(Compiler::current()->local_stack_base.back(), inst);
							ILBuilder::eval_const_type(Compiler::current()->evaluator(), inst->type.get());
						}


						c.move(tok);
					}break;
					case 2: {

						c.move(tok);
						if (tok != RecognizedToken::OpenParenthesis) {
							throw_wrong_token_error(c, "'('");
						}

						FunctionInstance* finst;
						struct_inst->subfunctions[f->second.second]->generate(nullptr, finst);
						finst->compile();

						ILBuilder::eval_fnptr(Compiler::current()->evaluator(), finst->func);

						ret.type = finst->type;
						ret.lvalue = false;
						function_call(ret, c, tok, cpt, 0, targets_defer);
					}break;

					case 4: {
						c.move(tok);
						StaticInstance* sinst = struct_inst->substatics[f->second.second].get();
						sinst->compile();
						ret.lvalue = true;
						ret.type = sinst->type;
						ILBuilder::eval_staticref(Compiler::current()->evaluator(), sinst->sid);
					} break;

					default:
						throw_specific_error(c, "Target is not a structure or static");
						break;
				}

				
			}
			else {
				throw_specific_error(c, "Structure instance does not contain a structure with this name");
			}
		}
	}

	void Operand::function_call(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType cpt, unsigned int argi, bool targets_defer) {
		Expression::rvalue(ret, cpt);

		TypeFunction* ft = (TypeFunction*)ret.type;

		if (ft->context() != ILContext::both && Compiler::current()->scope_context() != ft->context()) {
			throw_specific_error(c, "Cannot call function with different context specifier");
		}

		c.move(tok);

		CompileValue retval;
		retval.type = ft->return_type;
		retval.lvalue = true;
		ft->return_type->compile();

		if (cpt == CompileType::compile) {

			stackid_t local_return_id = 0;


			ILBuilder::build_callstart(Compiler::current()->scope());
			if (ft->return_type->rvalue_stacked()) {
				local_return_id = Compiler::current()->target()->local_stack_lifetime.append(retval.type->size());
				Compiler::current()->temp_stack()->push_item("$tmp", retval.type, local_return_id);

				ILBuilder::build_local(Compiler::current()->scope(), local_return_id);
			}

			Operand::read_arguments(c,tok, argi, ft, cpt);

			if (argi != Compiler::current()->types()->argument_array_storage.get(ft->argument_array_id).size()) {
				throw_specific_error(c, "Wrong number of arguments");
			}
			c.move(tok);
			ft->compile();

			if (!targets_defer || tok != RecognizedToken::Semicolon) {
				ILBuilder::build_call(Compiler::current()->scope(), ft->il_function_decl);

				if (ft->return_type->rvalue_stacked()) {
					ILBuilder::build_local(Compiler::current()->scope(), local_return_id);
				}


				ret.type = ft->return_type;
				ret.lvalue = false;
			}
			else {
				Compiler::current()->defer_scope().push_back(ft);

				ret.type = Compiler::current()->types()->t_void;
				ret.lvalue = true;
			}

		}
		else {

			stackid_t local_stack_item;

			ILBuilder::eval_callstart(Compiler::current()->evaluator());

			if (ft->return_type->rvalue_stacked()) {
				local_stack_item = Compiler::current()->push_local(retval.type->size());
				unsigned char* memory_place = Compiler::current()->stack_ptr(local_stack_item);

				Compiler::current()->compiler_stack()->push_item("$tmp", retval.type, local_stack_item);
				Compiler::current()->eval_local(local_stack_item);
			}

			Operand::read_arguments(c,tok, argi, ft, cpt);

			if (argi != Compiler::current()->types()->argument_array_storage.get(ft->argument_array_id).size()) {
				throw_specific_error(c, "Wrong number of arguments");

			}

			c.move(tok);
			ft->compile();

			if (!targets_defer || tok != RecognizedToken::Semicolon) {

				ILBuilder::eval_call(Compiler::current()->evaluator(), ft->il_function_decl);

				if (ft->return_type->rvalue_stacked()) {
					Compiler::current()->eval_local(local_stack_item);
				}


				ret.type = ft->return_type;
				ret.lvalue = false;
			}
			else {
				Compiler::current()->compile_defer_scope().push_back(ft);
				ret.type = Compiler::current()->types()->t_void;
				ret.lvalue = true;
			}
		}
	}

	void Operand::structure_element_offset(CompileValue& ret, tableelement_t id, CompileType cpt) {

		TypeStructureInstance* ti = (TypeStructureInstance*)ret.type;
		ti->compile();

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

		if (ret.lvalue)
		{
			if (cpt == CompileType::compile) {
				if (!si->wrapper) {
					if (elem_size.type == ILSizeType::table)
						ILBuilder::build_tableoffset(Compiler::current()->scope(), elem_size.value, elem_id);
					else if (elem_size.type == ILSizeType::absolute)
						ILBuilder::build_aoffset(Compiler::current()->scope(), (uint32_t)elem_size.value);
					else if (elem_size.type == ILSizeType::word)
						ILBuilder::build_woffset(Compiler::current()->scope(), (uint32_t)elem_size.value);
				}
			}
			else if (cpt == CompileType::eval) {
				if (!si->wrapper) {
					if (elem_size.type == ILSizeType::table)
						ILBuilder::eval_tableoffset(Compiler::current()->evaluator(), elem_size.value, elem_id);
					else if (elem_size.type == ILSizeType::absolute)
						ILBuilder::eval_aoffset(Compiler::current()->evaluator(), (uint32_t)elem_size.value);
					else if (elem_size.type == ILSizeType::word)
						ILBuilder::eval_woffset(Compiler::current()->evaluator(), (uint32_t)elem_size.value);
				}
			}
		}
		else if (ret.type->rvalue_stacked()) {
			if (cpt == CompileType::compile) {
				if (!si->wrapper) {
					if (elem_size.type == ILSizeType::table)
						ILBuilder::build_tableoffset(Compiler::current()->scope(), elem_size.value, elem_id);
					else if (elem_size.type == ILSizeType::absolute)
						ILBuilder::build_aoffset(Compiler::current()->scope(), (uint32_t)elem_size.value);
					else if (elem_size.type == ILSizeType::word)
						ILBuilder::build_woffset(Compiler::current()->scope(), (uint32_t)elem_size.value);
				}

				if (!mem_type->rvalue_stacked()) {
					ILBuilder::build_load(Compiler::current()->scope(), mem_type->rvalue());
				}
			}
			else if (cpt == CompileType::eval) {

				if (!si->wrapper) {
					if (elem_size.type == ILSizeType::table)
						ILBuilder::eval_tableoffset(Compiler::current()->evaluator(), elem_size.value, elem_id);
					else if (elem_size.type == ILSizeType::absolute)
						ILBuilder::eval_aoffset(Compiler::current()->evaluator(), (uint32_t)elem_size.value);
					else if (elem_size.type == ILSizeType::word)
						ILBuilder::eval_woffset(Compiler::current()->evaluator(), (uint32_t)elem_size.value);
				}

				if (!mem_type->rvalue_stacked()) {
					ILBuilder::eval_load(Compiler::current()->evaluator(), mem_type->rvalue());
				}
			}
		}
		else {
			if (cpt == CompileType::compile) {
				if (!si->wrapper) {
					if (elem_size.type == ILSizeType::table)
						ILBuilder::build_tableroffset(Compiler::current()->scope(), ret.type->rvalue(), mem_type->rvalue(), elem_size.value, elem_id);
					else if (elem_size.type == ILSizeType::absolute)
						ILBuilder::build_aroffset(Compiler::current()->scope(), ret.type->rvalue(), mem_type->rvalue(), elem_size.value);
					else if (elem_size.type == ILSizeType::word)
						ILBuilder::build_wroffset(Compiler::current()->scope(), ret.type->rvalue(), mem_type->rvalue(), elem_size.value);
				}

			}
			else if (cpt == CompileType::eval) {

				if (!si->wrapper) {
					if (elem_size.type == ILSizeType::table)
						ILBuilder::eval_tableroffset(Compiler::current()->evaluator(), ret.type->rvalue(), mem_type->rvalue(), elem_size.value, elem_id);
					else if (elem_size.type == ILSizeType::absolute)
						ILBuilder::eval_aroffset(Compiler::current()->evaluator(), ret.type->rvalue(), mem_type->rvalue(), elem_size.value);
					else if (elem_size.type == ILSizeType::word)
						ILBuilder::eval_wroffset(Compiler::current()->evaluator(), ret.type->rvalue(), mem_type->rvalue(), elem_size.value);
				}

			}

		}

		// ret.lvalue stays the same
		ret.type = mem_type;
	}


	void Operand::parse_dot_operator(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType cpt, bool targets_defer) {
		c.move(tok);

		Operand::deref(ret, cpt);

		if (ret.type->type() == TypeInstanceType::type_slice) {
			TypeSlice* ts = (TypeSlice*)ret.type;

			auto buf = c.buffer();
			if (buf == "count") {
				if (cpt == CompileType::compile) {
					if (ret.lvalue || ret.type->rvalue_stacked()) {
						ILBuilder::build_woffset(Compiler::current()->scope(), 1);
						ILBuilder::build_load(Compiler::current()->scope(), ILDataType::word);
					}
					else {
						ILBuilder::build_high_word(Compiler::current()->scope());
					}
				}
				else {
					if (ret.lvalue || ret.type->rvalue_stacked()) {
						ILBuilder::eval_woffset(Compiler::current()->evaluator(), 1);
						ILBuilder::eval_load(Compiler::current()->evaluator(), ILDataType::word);
					}
					else {
						ILBuilder::eval_high_word(Compiler::current()->evaluator());
					}
				}

				if (ts->owner->size().type != ILSizeType::absolute || ts->owner->size().value > 1) {
					if (cpt == CompileType::compile) {
						ILBuilder::build_const_size(Compiler::current()->scope(), ts->owner->size());
						ILBuilder::build_div(Compiler::current()->scope(), ILDataType::word, ILDataType::word);
					}
					else {
						ILBuilder::eval_const_size(Compiler::current()->evaluator(), ts->owner->size().eval(Compiler::current()->global_module(), compiler_arch));
						ILBuilder::eval_div(Compiler::current()->evaluator(), ILDataType::word, ILDataType::word);
					}
				}

				c.move(tok);

				ret.lvalue = false;
				ret.type = Compiler::current()->types()->t_size;

			}
			else if (buf == "size") {

				if (cpt == CompileType::compile) {

					if (ret.lvalue) {
						ILBuilder::build_woffset(Compiler::current()->scope(), 1);
					}
					else if (ret.type->rvalue_stacked()) {
						ILBuilder::build_woffset(Compiler::current()->scope(), 1);
						ILBuilder::build_load(Compiler::current()->scope(), ILDataType::word);
					}
					else {

						ILBuilder::build_high_word(Compiler::current()->scope());
					}
				}
				else {

					if (ret.lvalue) {
						ILBuilder::eval_woffset(Compiler::current()->evaluator(), 1);
					}
					else if (ret.type->rvalue_stacked()) {
						ILBuilder::eval_woffset(Compiler::current()->evaluator(), 1);
						ILBuilder::eval_load(Compiler::current()->evaluator(), ILDataType::word);
					}
					else {
						ILBuilder::eval_high_word(Compiler::current()->evaluator());
					}
				}


				c.move(tok);

				ret.type = Compiler::current()->types()->t_size;

			}
			else if (buf == "ptr") {
				if (cpt == CompileType::compile) {
					if (ret.lvalue) {

					}
					else if (ret.type->rvalue_stacked()) {
						ILBuilder::build_load(Compiler::current()->scope(), ILDataType::word);
					}
					else {
						ILBuilder::build_bitcast(Compiler::current()->scope(), ret.type->rvalue(), ILDataType::word);
					}
				}
				else {
					if (ret.lvalue) {

					}
					else if (ret.type->rvalue_stacked()) {
						ILBuilder::eval_load(Compiler::current()->evaluator(), ILDataType::word);
					}
					else {

						ILBuilder::eval_bitcast(Compiler::current()->evaluator(), ret.type->rvalue(), ILDataType::word);
					}
				}

				c.move(tok);

				ret.type = Compiler::current()->types()->t_ptr;

			}
			else {
				throw_specific_error(c, "Indentifier not recognized as a value of slice");

			}
		}
		else if (ret.type->type() == TypeInstanceType::type_trait) {

			TypeTraitInstance* tti = (TypeTraitInstance*)ret.type;
			TraitInstance* ti = tti->owner;


			auto off_f = ti->member_table.find(c.buffer());
			if (off_f == ti->member_table.end()) {
				throw_specific_error(c, "Trait function not found");

			}
			uint32_t off = (uint32_t)off_f->second;
			auto& mf = ti->member_declarations[off];

			if (mf->ptr_context != ILContext::both && Compiler::current()->scope_context() != mf->ptr_context) {
				throw_specific_error(c, "Cannot access trait function with different context");
			}

			Expression::rvalue(ret, cpt);

			c.move(tok);
			if (tok == RecognizedToken::OpenParenthesis) {
				if (cpt == CompileType::compile) {
					ILBuilder::build_split_dword(Compiler::current()->scope());
					ILBuilder::build_woffset(Compiler::current()->scope(), (uint32_t)off);
					ILBuilder::build_load(Compiler::current()->scope(), ILDataType::word);
					ILBuilder::build_callstart(Compiler::current()->scope());
				}
				else {
					ILBuilder::eval_split_dword(Compiler::current()->evaluator());
					ILBuilder::eval_woffset(Compiler::current()->evaluator(), (uint32_t)off);
					ILBuilder::eval_load(Compiler::current()->evaluator(), ILDataType::word);
					ILBuilder::eval_callstart(Compiler::current()->evaluator());
				}

				stackid_t local_return_id = 0;

				if (cpt == CompileType::compile) {
					if (mf->return_type->rvalue_stacked()) {
						local_return_id = Compiler::current()->target()->local_stack_lifetime.append(mf->return_type->size());
						Compiler::current()->temp_stack()->push_item("$tmp", mf->return_type, local_return_id);
						ILBuilder::build_local(Compiler::current()->scope(), local_return_id);
					}
				}
				else {
					if (mf->return_type->rvalue_stacked()) {
						local_return_id = Compiler::current()->push_local(mf->return_type->size());
						Compiler::current()->compiler_stack()->push_item("$tmp", mf->return_type, local_return_id);
						Compiler::current()->eval_local(local_return_id);
					}
				}

				c.move(tok);
				unsigned int argi = 1;
				Operand::read_arguments(c,tok, argi, mf, cpt);

				mf->compile();

				c.move(tok);

				if (!targets_defer || tok!=RecognizedToken::Semicolon) {
					if (cpt == CompileType::compile) {
						ILBuilder::build_call(Compiler::current()->scope(), mf->il_function_decl);

						if (mf->return_type->rvalue_stacked()) {
							ILBuilder::build_local(Compiler::current()->scope(), local_return_id);
						}
					}
					else {
						ILBuilder::eval_call(Compiler::current()->evaluator(), mf->il_function_decl);

						if (mf->return_type->rvalue_stacked()) {
							Compiler::current()->eval_local(local_return_id);
						}
					}

					ret.lvalue = false;
					ret.type = mf->return_type;
				}
				else {
					if (cpt == CompileType::compile) {
						Compiler::current()->defer_scope().push_back(mf);
					}
					else {
						Compiler::current()->compile_defer_scope().push_back(mf);

					}

					ret.lvalue = true;
					ret.type = Compiler::current()->types()->t_void;
				}


			}
			else {
				if (cpt == CompileType::compile) {
					ILBuilder::build_high_word(Compiler::current()->scope());
					ILBuilder::build_woffset(Compiler::current()->scope(), (uint32_t)off);
					ILBuilder::build_load(Compiler::current()->scope(), ILDataType::word);
				}
				else {
					ILBuilder::eval_high_word(Compiler::current()->evaluator());
					ILBuilder::eval_woffset(Compiler::current()->evaluator(), (uint32_t)off);
					ILBuilder::eval_load(Compiler::current()->evaluator(), ILDataType::word);
				}

				ret.lvalue = false;
				ret.type = mf;
			}
		}
		else {

			Cursor err = c;
			c.move(tok);

			bool continue_deeper = true;

			while (continue_deeper) {
				Operand::deref(ret, cpt);
				if (ret.type->type() != TypeInstanceType::type_structure_instance) {
					throw_specific_error(err, "Operator cannot be used on this type");
				}


				TypeStructureInstance* ti = (TypeStructureInstance*)ret.type;
				ti->compile();
				StructureInstance* si = ti->owner;

				auto table_element = si->member_table.find(err.buffer());
				if (table_element != si->member_table.end()) {
					switch (table_element->second.second)
					{
						case MemberTableEntryType::alias:
							continue_deeper = true;
							structure_element_offset(ret, table_element->second.first, cpt);
							break;
						case MemberTableEntryType::var:
							continue_deeper = false;
							structure_element_offset(ret, table_element->second.first, cpt);
							break;
						case MemberTableEntryType::func:

							// rvalues will be stored in temporary memory location
							if (!ret.lvalue && !ret.type->rvalue_stacked()) {
								if (cpt == CompileType::compile) {
									stackid_t local_id = Compiler::current()->target()->local_stack_lifetime.append(ret.type->size());
									Compiler::current()->temp_stack()->push_item("$tmp", ret.type, local_id);
									
									ILBuilder::build_store(Compiler::current()->scope(), ret.type->rvalue());
									ILBuilder::build_local(Compiler::current()->scope(), local_id);
									ret.lvalue = true;
								}
								else {
									stackid_t local_id = Compiler::current()->push_local(ret.type->size());
									Compiler::current()->compiler_stack()->push_item("$tmp", ret.type, local_id);
									ILBuilder::eval_store(Compiler::current()->evaluator(), ret.type->rvalue());
									Compiler::current()->eval_local(local_id);
									ret.lvalue = true;
								}
							}

							if (!ret.lvalue && !ret.type->rvalue_stacked()) {
								throw_wrong_token_error(c, "This function can be called only from lvalue object or reference");
							}

							if (tok != RecognizedToken::OpenParenthesis) {
								throw_wrong_token_error(c, "'('");
							}

							FunctionInstance* finst;
							si->subfunctions[table_element->second.first]->generate(nullptr, finst);
							finst->compile();

							if (cpt == CompileType::compile) {
								ILBuilder::build_fnptr(Compiler::current()->scope(), finst->func);
							}
							else {
								ILBuilder::eval_fnptr(Compiler::current()->evaluator(), finst->func);
							}

							ret.type = finst->type;
							ret.lvalue = false;
							function_call(ret, c,tok, cpt, 1,targets_defer);
							return;
					}
				}
				else {
					throw_specific_error(err, "Instance does not contain member with this name");
				}
			}

		}


	}


	void Operand::parse_string_literal(CompileValue& ret, Cursor& c, RecognizedToken& tok, CompileType cpt) {
		auto lit = Compiler::current()->constant_manager()->register_string_literal(c);

		Type* slice = Compiler::current()->types()->t_u8->generate_slice();
		slice->compile();

		if (cpt == CompileType::compile) {
			ILBuilder::build_const_slice(Compiler::current()->scope(), lit.second, (uint64_t)lit.first.length());
		}
		else {
			ILBuilder::eval_const_slice(Compiler::current()->evaluator(), lit.second, (uint64_t)lit.first.length());
		}

		c.move(tok);
		ret.type = slice;
		ret.lvalue = false;
	}
}