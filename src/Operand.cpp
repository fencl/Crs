#include "Operand.h"
#include "Error.h"
#include "Utilities.h"
#include "PredefinedTypes.h"
#include "Expression.h"
#include "StackManager.h"
#include "ConstantManager.h"
#include <iostream>

namespace Corrosive {

	void Operand::priv_type_size(ILEvaluator* eval) {
		auto t = eval->pop_register_value<Type*>();
		eval->write_register_value(t->size().eval(Ctx::global_module(), compiler_arch));
	}

	void Operand::priv_type_template_cast_crsr(ILEvaluator* eval, Cursor& err) {

		Type* template_cast = eval->pop_register_value<Type*>();
		Type* template_type = eval->read_register_value<Type*>();

		if (template_type->type() == TypeInstanceType::type_structure_template) {
			TypeStructureTemplate* st = (TypeStructureTemplate*)template_type;
			TypeTemplate* tt = (TypeTemplate*)template_cast;

			if (st->owner->generic_ctx.generic_layout.size() != Ctx::types()->argument_array_storage.get(tt->argument_array_id).size()) {
				throw_cannot_cast_error(err, template_type, template_cast);
			}
			else {
				for (size_t i = 0; i < st->owner->generic_ctx.generic_layout.size(); i++) {
					if (std::get<1>(st->owner->generic_ctx.generic_layout[i]) != Ctx::types()->argument_array_storage.get(tt->argument_array_id)[i]) {
						throw_cannot_cast_error(err, template_type, template_cast);
					}
				}
			}
		}
		else if (template_type->type() == TypeInstanceType::type_trait_template) {
			TypeTraitTemplate* st = (TypeTraitTemplate*)template_type;
			TypeTemplate* tt = (TypeTemplate*)template_cast;

			if (st->owner->generic_ctx.generic_layout.size() != Ctx::types()->argument_array_storage.get(tt->argument_array_id).size()) {
				throw_cannot_cast_error(err, template_type, template_cast);
			}
			else {
				for (size_t i = 0; i < st->owner->generic_ctx.generic_layout.size(); i++) {
					if (std::get<1>(st->owner->generic_ctx.generic_layout[i]) != Ctx::types()->argument_array_storage.get(tt->argument_array_id)[i]) {
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

			if (st->owner->generic_ctx.generic_layout.size() != Ctx::types()->argument_array_storage.get(tt->argument_array_id).size()) {
				throw_runtime_exception(Ctx::eval(), "Template cannot be casted to this generic type");
			}
			else {
				for (size_t i = 0; i < st->owner->generic_ctx.generic_layout.size(); i++) {
					if (std::get<1>(st->owner->generic_ctx.generic_layout[i]) != Ctx::types()->argument_array_storage.get(tt->argument_array_id)[i]) {
						throw_runtime_exception(Ctx::eval(), "Template cannot be casted to this generic type");
					}
				}
			}
		}
		else if (template_type->type() == TypeInstanceType::type_trait_template) {
			TypeTraitTemplate* st = (TypeTraitTemplate*)template_type;
			TypeTemplate* tt = (TypeTemplate*)template_cast;

			if (st->owner->generic_ctx.generic_layout.size() != Ctx::types()->argument_array_storage.get(tt->argument_array_id).size()) {
				throw_runtime_exception(Ctx::eval(), "Template cannot be casted to this generic type");
			}
			else {
				for (size_t i = 0; i < st->owner->generic_ctx.generic_layout.size(); i++) {
					if (std::get<1>(st->owner->generic_ctx.generic_layout[i]) != Ctx::types()->argument_array_storage.get(tt->argument_array_id)[i]) {
						throw_runtime_exception(Ctx::eval(), "Template cannot be casted to this generic type");
					}
				}
			}
		}
		else {
			throw_runtime_exception(Ctx::eval(), "Template cannot be casted to this generic type");
		}
	}

	bool Operand::is_numeric_value(Type* t) {
		return !t->rvalue_stacked() && t->rvalue() < ILDataType::none;
	}

	void Operand::cast(Cursor& err, CompileValue& res, Type*& to, CompileType cpt, bool implicit) {

		if (res.t == Ctx::types()->t_type && to->type() == TypeInstanceType::type_template) {
			if (cpt == CompileType::eval) {
				ILBuilder::eval_const_type(Ctx::eval(), to);
				priv_type_template_cast_crsr(Ctx::eval(), err);

				res.t = to;

			}
			else {
				ILBuilder::build_const_type(Ctx::scope(), to);
				ILBuilder::build_insintric(Ctx::scope(), ILInsintric::template_cast);
				res.t = to;
			}
		}
		else if (to == Ctx::types()->t_type && res.t->type() == TypeInstanceType::type_template) {
			res.t = to;

		}
		else if ((res.t == Ctx::types()->t_ptr || res.t->type() == TypeInstanceType::type_reference || (Operand::is_numeric_value(res.t) && res.t != Ctx::types()->t_bool)) && to == Ctx::types()->t_bool) {
			Expression::rvalue(res, cpt);
			if (cpt == CompileType::eval) {
				ILBuilder::eval_isnotzero(Ctx::eval(), res.t->rvalue());
				res.t = to;
				res.lvalue = false;

			}
			else {
				ILBuilder::build_isnotzero(Ctx::scope(), res.t->rvalue());
				res.t = to;
				res.lvalue = false;

			}
		}
		else if ((res.t->type() == TypeInstanceType::type_reference || (res.t->type() == TypeInstanceType::type_structure_instance && res.lvalue)) && to->type() == TypeInstanceType::type_trait) {

			TypeTraitInstance* tt = (TypeTraitInstance*)to;

			TypeStructureInstance* ts = nullptr;
			if (res.t->type() == TypeInstanceType::type_reference) {
				TypeReference* tr = (TypeReference*)res.t;
				if (tr->owner->type() == TypeInstanceType::type_structure_instance) {
					ts = (TypeStructureInstance*)tr->owner;
					Expression::rvalue(res, cpt);
				}
				else {
					throw_cannot_cast_error(err, res.t, to);

				}
			}
			else {
				ts = (TypeStructureInstance*)res.t;
			}


			ts->compile();

			auto tti = ts->owner->traitfunctions.find(tt->owner);
			if (tti == ts->owner->traitfunctions.end()) {
				throw_cannot_cast_error(err, res.t, to);
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


				if (to->rvalue_stacked()) {
					uint16_t sid = Ctx::eval()->push_local(to->size());
					unsigned char* memory_place = Ctx::eval()->stack_ptr(sid);
					Ctx::eval_stack()->push_item("$tmp", to, sid, StackItemTag::regular);

					ILBuilder::eval_const_ptr(Ctx::eval(), memory_place);
					ILBuilder::eval_store(Ctx::eval(), ILDataType::word);
					ILBuilder::eval_vtable(Ctx::eval(), vtableid);
					ILBuilder::eval_const_ptr(Ctx::eval(), memory_place + sizeof(void*));
					ILBuilder::eval_store(Ctx::eval(), ILDataType::word);
					ILBuilder::eval_const_ptr(Ctx::eval(), memory_place);
				}
				else {
					ILBuilder::eval_vtable(Ctx::eval(), vtableid); // there is two pointers on register stack, if i read 2x pointer size register it will read the right value
				}

			}
			else {
				if (to->rvalue_stacked()) {
					to->compile();
					uint32_t local_id = Ctx::workspace_function()->local_stack_lifetime.append(to->size());
					Ctx::temp_stack()->push_item("$tmp", to, local_id, StackItemTag::regular);

					ILBuilder::build_local(Ctx::scope(), local_id);
					ILBuilder::build_store(Ctx::scope(), ILDataType::word);
					ILBuilder::build_vtable(Ctx::scope(), vtableid);
					ILBuilder::build_local(Ctx::scope(), local_id);
					ILBuilder::build_woffset(Ctx::scope(), 1);
					ILBuilder::build_store(Ctx::scope(), ILDataType::word);
					ILBuilder::build_local(Ctx::scope(), local_id);
				}
				else {
					std::cout << "???"; // two pointers require 128 int on x64, not yet implemented
				}
			}

			res.t = to;


		}
		else if ((res.t == Ctx::types()->t_ptr && to->type() == TypeInstanceType::type_reference) || (to == Ctx::types()->t_ptr && res.t->type() == TypeInstanceType::type_reference)) {
			Expression::rvalue(res, cpt);
			res.lvalue = false;
			res.t = to;

		}
		else if (res.t->type() == TypeInstanceType::type_reference && to->type() == TypeInstanceType::type_reference) {
			if (res.t != to && implicit) {
				throw_cannot_implicit_cast_error(err, res.t, to);

			}

			Expression::rvalue(res, cpt);
			res.lvalue = false;
			res.t = to;

		}
		else if (res.t->type() == TypeInstanceType::type_structure_instance && to->type() == TypeInstanceType::type_reference) {
			TypeReference* tr = (TypeReference*)to;
			if (tr->owner != res.t) {
				throw_cannot_cast_error(err, res.t, to);

			}

			if (!res.lvalue) {
				throw_cannot_cast_error(err, res.t, to);
				std::cerr << " |\tType was not lvalue\n";

			}

			res.t = to;
			res.lvalue = false;

		}
		else if (res.t->type() == TypeInstanceType::type_slice && to == Ctx::types()->t_ptr) {
			if (res.lvalue || res.t->rvalue_stacked()) {
				if (cpt == CompileType::compile) {
					ILBuilder::build_load(Ctx::scope(), ILDataType::word);
				}
				else {
					ILBuilder::eval_load(Ctx::eval(), ILDataType::word);
				}
			}
			else {
				if (cpt == CompileType::compile) {
					ILBuilder::build_cast(Ctx::scope(), res.t->rvalue(), ILDataType::word);
					//ILBuilder::build_roffset(Ctx::scope(), res.t->rvalue(), ILDataType::ptr,ILSize(ILSizeType::absolute,0));
				}
				else {
					ILBuilder::build_cast(Ctx::scope(), res.t->rvalue(), ILDataType::word);
					//ILBuilder::eval_roffset(Ctx::eval(), res.t->rvalue(), ILDataType::ptr, 0);
				}
			}
			res.t = to;
			res.lvalue = false;
		}
		else if (Operand::is_numeric_value(res.t) && Operand::is_numeric_value(to)) {
			Expression::rvalue(res, cpt);
			if (cpt == CompileType::eval) {
				if (res.t->rvalue() != to->rvalue()) {
					ILBuilder::eval_cast(Ctx::eval(), res.t->rvalue(), to->rvalue());
				}

				res.t = to;
				res.lvalue = false;

			}
			else {
				if (res.t->rvalue() != to->rvalue())
					ILBuilder::build_cast(Ctx::scope(), res.t->rvalue(), to->rvalue());

				res.t = to;
				res.lvalue = false;

			}
		}
		else if (res.t != to) {
			throw_cannot_cast_error(err, res.t, to);
		}


		res.t = to;

	}

	void Operand::parse(Cursor& c, CompileValue& res, CompileType cpt) {
		CompileValue ret;

		ret.lvalue = false;
		ret.t = nullptr;

		switch (c.tok) {
			case RecognizedToken::And:
			case RecognizedToken::DoubleAnd: {
				Operand::parse_reference(res, c, cpt);
				return;
			}
			case RecognizedToken::OpenBracket: {
				Operand::parse_array_type(res, c, cpt);
				return;
			}
			case RecognizedToken::Star: {
				c.move();
				Cursor err = c;
				CompileValue value;
				Operand::parse(c, value, cpt);
				Expression::rvalue(value, cpt);
				if (value.t->type() != TypeInstanceType::type_reference) {
					throw_specific_error(err, "Target is not reference, and cannot be dereferenced");

				}

				res.lvalue = true;
				res.t = ((TypeReference*)value.t)->owner;
				return;
			}

			case RecognizedToken::OpenParenthesis: {
				Operand::parse_expression(ret, c, cpt);
			}break;

			case RecognizedToken::Symbol: {
				if (c.buffer == "cast") {
					c.move();
					if (c.tok != RecognizedToken::OpenParenthesis) {
						throw_wrong_token_error(c, "'('");

					}
					c.move();

					Cursor err = c;
					CompileValue t_res;

					Ctx::push_scope_context(ILContext::compile);

					Expression::parse(c, t_res, CompileType::eval);
					Expression::rvalue(t_res, CompileType::eval);

					if (t_res.t != Ctx::types()->t_type) {
						throw_specific_error(err, "Expected type");

					}

					auto to = Ctx::eval()->pop_register_value<Type*>();
					to->compile();

					if (c.tok != RecognizedToken::CloseParenthesis) {
						throw_wrong_token_error(c, "')'");

					}
					c.move();

					Ctx::pop_scope_context();

					err = c;
					CompileValue value;
					Operand::parse(c, value, cpt);
					Operand::cast(err, value, to, cpt, false);

					res = value;
					return;
				}
				else {
					Operand::parse_symbol(ret, c, cpt);
				}
			}break;

			case RecognizedToken::Minus: {
				Cursor err = c;
				c.move();
				Expression::parse(c, res, cpt);
				Expression::rvalue(res, cpt);

				if (!Operand::is_numeric_value(res.t)) {
					throw_specific_error(err, "Operation requires number operand");
				}

				if (cpt == CompileType::compile) {
					ILBuilder::build_negative(Ctx::scope(), res.t->rvalue());
				}
				else {
					ILBuilder::eval_negative(Ctx::eval(), res.t->rvalue());
				}
			}return;

			case RecognizedToken::ExclamationMark: {
				c.move();
				Cursor err = c;
				Expression::parse(c, res, cpt);
				Expression::rvalue(res, cpt);
				Operand::cast(err, res, Ctx::types()->t_bool, cpt, true);

				if (cpt == CompileType::compile) {
					ILBuilder::build_negate(Ctx::scope());
				}
				else {
					ILBuilder::eval_negate(Ctx::eval());
				}
			}return;

			case RecognizedToken::Number:
			case RecognizedToken::UnsignedNumber: {
				Operand::parse_number(ret, c, cpt);
			}break;

			case RecognizedToken::LongNumber:
			case RecognizedToken::UnsignedLongNumber: {
				Operand::parse_long_number(ret, c, cpt);
			}break;

			case RecognizedToken::FloatNumber:
			case RecognizedToken::DoubleNumber: {
				Operand::parse_float_number(ret, c, cpt);
			}break;

			case RecognizedToken::String: {
				Operand::parse_string_literal(ret, c, cpt);
			}break;


			default: {
				throw_specific_error(c, "Expected to parse operand");
			} break;
		}

		while (true) {
			switch (c.tok) {
				case RecognizedToken::Symbol: {
					if (c.buffer == "cast") {
						//Expression::rvalue(ret, cpt);

						c.move();
						if (c.tok != RecognizedToken::OpenParenthesis) {
							throw_wrong_token_error(c, "'('");

						}
						c.move();


						Ctx::push_scope_context(ILContext::compile);

						Cursor err = c;

						CompileValue t_res;
						Expression::parse(c, t_res, CompileType::eval);
						Expression::rvalue(t_res, CompileType::eval);

						if (t_res.t != Ctx::types()->t_type) {
							throw_specific_error(err, "Expected type");

						}

						auto to = Ctx::eval()->pop_register_value<Type*>();
						to->compile();

						if (c.tok != RecognizedToken::CloseParenthesis) {
							throw_wrong_token_error(c, "')'");

						}
						c.move();

						Ctx::pop_scope_context();

						Operand::cast(err, ret, to, cpt, false);
					}
					else goto break_while;
				}break;
				case RecognizedToken::OpenParenthesis: {
					parse_call_operator(ret, c, cpt);
				}break;
				case RecognizedToken::OpenBracket: {
					parse_array_operator(ret, c, cpt);
				}break;
				case RecognizedToken::Dot: {
					parse_dot_operator(ret, c, cpt);
				}break;
				case RecognizedToken::DoubleColon: {
					parse_double_colon_operator(ret, c, cpt);
				}break;
				default: goto break_while;
			}

			continue;
		break_while:
			break;
		}

		res = ret;

	}




	void Operand::parse_const_type_function(Cursor& c, FunctionInstance*& func, Type*& type, ILSize& type_size) {
		Namespace* namespace_inst = nullptr;
		StructureTemplate* struct_inst = nullptr;
		FunctionTemplate* func_inst = nullptr;
		TraitTemplate* trait_inst = nullptr;


		Ctx::push_scope_context(ILContext::compile);



		Cursor err = c;
		Ctx::workspace()->find_name(c.buffer, namespace_inst, struct_inst, func_inst, trait_inst);

		if (namespace_inst == nullptr && struct_inst == nullptr && func_inst == nullptr && trait_inst == nullptr) {
			throw_specific_error(c, "Path start point not found");
		}

		Cursor nm_err = c;
		c.move();
		while (c.tok == RecognizedToken::DoubleColon && namespace_inst != nullptr) {
			c.move();
			if (c.tok != RecognizedToken::Symbol)
			{
				throw_not_a_name_error(c);
			}
			nm_err = c;

			namespace_inst->find_name(c.buffer, namespace_inst, struct_inst, func_inst, trait_inst);
			c.move();
		}


		while (struct_inst) {
			struct_inst->compile();

			StructureInstance* inst;
			if (struct_inst->is_generic) {
				if (c.tok != RecognizedToken::OpenParenthesis) {
					type = struct_inst->type.get();
					Ctx::pop_scope_context();
					return;
				}
				c.move();

				parse_generate_template(c, struct_inst, inst);
			}
			else {
				struct_inst->generate(nullptr, inst);
			}



			if (c.tok != RecognizedToken::DoubleColon) {
				Ctx::pop_scope_context();
				type = inst->type.get();
				return;
			}
			c.move();

			nm_err = c;

			c.move();

			auto f = inst->subfunctions.find(nm_err.buffer);
			if (f != inst->subfunctions.end()) {
				func_inst = f->second.get();
				break;
			}
			else {
				auto f2 = inst->subtemplates.find(nm_err.buffer);

				if (f2 != inst->subtemplates.end()) {
					struct_inst = f2->second.get();
					continue;
				}
				else {
					throw_specific_error(nm_err, "Only function may be brought in the runtime context");

				}
			}
		}


		if (func_inst != nullptr) {

			func_inst->compile();

			if (!func_inst->is_generic) {
				FunctionInstance* inst;
				func_inst->generate(nullptr, inst);
				inst->compile();

				func = inst;

				Ctx::pop_scope_context();
				return;
			}
			else {
				FunctionInstance* inst;
				if (c.tok != RecognizedToken::OpenParenthesis) {
					type = func_inst->type.get();

					Ctx::pop_scope_context();
					return;
				}
				c.move();

				parse_generate_template(c, func_inst, inst);
				inst->compile();

				Ctx::pop_scope_context();
				func = inst;
				return;
			}

		}
		else {
			throw_specific_error(nm_err, "Only function may be brought in the runtime context");

		}

		Ctx::pop_scope_context();

	}



	void Operand::parse_expression(CompileValue& ret, Cursor& c, CompileType cpt) {
		c.move();
		Expression::parse(c, ret, cpt);

		if (c.tok != RecognizedToken::CloseParenthesis) {
			throw_wrong_token_error(c, "')'");

		}
		c.move();


	}



	void Operand::parse_symbol(CompileValue& ret, Cursor& c, CompileType cpt) {
		if (c.buffer == "true") {
			c.move();
			ret.lvalue = false;
			ret.t = Ctx::types()->t_bool;
			if (cpt == CompileType::compile) {
				ILBuilder::build_const_i8(Ctx::scope(), true);
			}
			else if (cpt == CompileType::eval) {
				ILBuilder::eval_const_i8(Ctx::eval(), true);
			}
		}
		else if (c.buffer == "false") {
			c.move();
			ret.lvalue = false;
			ret.t = Ctx::types()->t_bool;
			if (cpt == CompileType::compile) {
				ILBuilder::build_const_i8(Ctx::scope(), false);
			}
			else if (cpt == CompileType::eval) {
				ILBuilder::eval_const_i8(Ctx::eval(), false);
			}
		}
		else if (c.buffer == "null") {
			c.move();
			ret.lvalue = false;
			ret.t = Ctx::types()->t_ptr;

			if (cpt == CompileType::compile) {
				ILBuilder::build_null(Ctx::scope());
			}
			else if (cpt == CompileType::eval) {
				ILBuilder::eval_null(Ctx::eval());
			}
		}
		else if (c.buffer == "typesize") {
			c.move();
			Ctx::push_scope_context(ILContext::compile);

			CompileValue type_val;
			Cursor err = c;
			Operand::parse(c, type_val, CompileType::eval);
			Expression::rvalue(type_val, CompileType::eval);

			if (type_val.t != Ctx::types()->t_type) {
				throw_specific_error(err, "Expected type");

			}

			auto tp = Ctx::eval()->pop_register_value<Type*>();
			tp->compile();
			Ctx::pop_scope_context();

			if (cpt == CompileType::compile) {
				ILBuilder::build_const_size(Ctx::scope(), tp->size());
			}
			else {
				ILBuilder::eval_const_size(Ctx::eval(), tp->size().eval(Ctx::global_module(), compiler_arch));
			}

			ret.lvalue = false;
			ret.t = Ctx::types()->t_size;
		}
		else if (c.buffer == "default") {
			c.move();

			CompileValue type_val;
			Cursor err = c;
			Operand::parse(c, type_val, cpt);

			if (type_val.t->type() == TypeInstanceType::type_structure_instance && type_val.lvalue) {
				type_val.t->compile();

				if (cpt == CompileType::compile) {
					//ILBuilder::build_duplicate(Ctx::scope(), ILDataType::ptr);
					type_val.t->build_construct();
				}
				else {
					type_val.t->construct(Ctx::eval()->pop_register_value<unsigned char*>());
				}
			}
			else if (type_val.t->type() == TypeInstanceType::type_reference) {
				TypeReference* tr = (TypeReference*)type_val.t;
				tr->owner->compile();
				Expression::rvalue(type_val, cpt);

				if (cpt == CompileType::compile) {
					//ILBuilder::build_duplicate(Ctx::scope(), ILDataType::ptr);
					tr->owner->build_construct();
				}
				else {
					tr->owner->construct(Ctx::eval()->pop_register_value<unsigned char*>());
				}

			}
			else {
				throw_specific_error(err, "Expected lvalue of structure or equivalent reference");
			}

			ret.t = Ctx::types()->t_void;
			ret.lvalue = false;
		}
		else if (c.buffer == "drop") {
			c.move();

			CompileValue type_val;
			Cursor err = c;
			Operand::parse(c, type_val, cpt);

			if (type_val.t->type() == TypeInstanceType::type_structure_instance && type_val.lvalue) {

				type_val.t->compile();

				if (cpt == CompileType::compile) {
					type_val.t->build_drop();
				}
				else {
					type_val.t->drop(Ctx::eval()->pop_register_value<unsigned char*>());
				}

			}
			else if (type_val.t->type() == TypeInstanceType::type_reference) {
				TypeReference* tr = (TypeReference*)type_val.t;
				tr->owner->compile();
				tr->compile();
				Expression::rvalue(type_val, cpt);

				if (cpt == CompileType::compile) {
					tr->owner->build_drop();
				}
				else {
					tr->owner->drop(Ctx::eval()->pop_register_value<unsigned char*>());
				}
			}
			else {
				throw_specific_error(err, "Expected lvalue of structure or equivalent reference");
			}

			ret.t = Ctx::types()->t_void;
			ret.lvalue = false;
		}
		else if (c.buffer == "type") {

			c.move();
			if (c.tok != RecognizedToken::OpenParenthesis) {
				Ctx::eval()->write_register_value(Ctx::types()->t_type);
			}
			else {
				c.move();
				std::vector<Type*> arg_types;

				if (c.tok != RecognizedToken::CloseParenthesis) {
					while (true) {
						Cursor err = c;
						CompileValue arg;
						Expression::parse(c, arg, CompileType::eval);
						Expression::rvalue(arg, CompileType::eval);

						if (arg.t != Ctx::types()->t_type) {
							throw_specific_error(err, "Expected type");

						}

						auto type = Ctx::eval()->pop_register_value<Type*>();
						arg_types.push_back(type);

						if (c.tok == RecognizedToken::Comma) {
							c.move();
						}
						else if (c.tok == RecognizedToken::CloseParenthesis) {
							break;
						}
						else {
							throw_wrong_token_error(c, "',' or ')'");
						}
					}
				}
				c.move();


				Type* ftype = Ctx::types()->load_or_register_template_type(std::move(arg_types));

				Ctx::eval()->write_register_value(ftype);
			}

			ret.lvalue = false;
			ret.t = Ctx::types()->t_type;
		}
		else if (c.buffer == "fn") {
			if (cpt == CompileType::compile) {
				if (Ctx::scope_context() != ILContext::compile) {
					throw_specific_error(c, "Function type cannot be created in runtime context");

				}
			}

			c.move();
			ILContext t_context = ILContext::both;
			if (c.tok == RecognizedToken::Symbol && c.buffer == "compile") {
				t_context = ILContext::compile;
				c.move();
			}
			else if (c.tok == RecognizedToken::Symbol && c.buffer == "runtime") {
				t_context = ILContext::runtime;
				c.move();
			}

			if (c.tok != RecognizedToken::OpenParenthesis) {
				throw_wrong_token_error(c, "(");

			}
			c.move();
			std::vector<Type*> arg_types;

			if (c.tok != RecognizedToken::CloseParenthesis) {
				while (true) {
					Cursor err = c;
					CompileValue arg;
					Expression::parse(c, arg, CompileType::eval);
					Expression::rvalue(arg, CompileType::eval);

					if (arg.t != Ctx::types()->t_type) {
						throw_specific_error(err, "Expected type");

					}

					auto type = Ctx::eval()->pop_register_value<Type*>();
					arg_types.push_back(type);

					if (c.tok == RecognizedToken::Comma) {
						c.move();
					}
					else if (c.tok == RecognizedToken::CloseParenthesis) {
						break;
					}
					else {
						throw_wrong_token_error(c, "',' or ')'");
					}
				}
			}
			c.move();
			Type* ret_type = Ctx::types()->t_void;

			if (c.tok == RecognizedToken::Symbol || c.tok == RecognizedToken::And || c.tok == RecognizedToken::DoubleAnd) {
				Cursor err = c;
				CompileValue rt;
				Expression::parse(c, rt, CompileType::eval);
				Expression::rvalue(rt, CompileType::eval);

				if (rt.t != Ctx::types()->t_type) {
					throw_specific_error(err, "Expected type");

				}
				ret_type = Ctx::eval()->pop_register_value<Type*>();
			}

			Type* ftype = Ctx::types()->load_or_register_function_type(std::move(arg_types), ret_type, t_context);

			Ctx::eval()->write_register_value(ftype);
			ret.lvalue = false;
			ret.t = Ctx::types()->t_type;
		}
		else {

			StackItem sitm;

			if (cpt == CompileType::compile && Ctx::stack()->find(c.buffer, sitm)) {
				ILBuilder::build_local(Ctx::scope(), sitm.id);
				ret.t = sitm.type;
				ret.lvalue = true;
				c.move();
			}
			else if (cpt != CompileType::compile && Ctx::eval_stack()->find(c.buffer, sitm)) {

				//std::cout <<sitm.id<<": 0x"<<std::hex << (size_t)Ctx::eval()->stack_ptr(sitm.id) << "\n";

				ILBuilder::eval_local(Ctx::eval(), sitm.id);
				ret.t = sitm.type;
				ret.lvalue = true;
				c.move();
			}
			else if (cpt == CompileType::compile) {
				FunctionInstance* f = nullptr;
				Type* t = nullptr;
				ILSize t_s;
				parse_const_type_function(c, f, t, t_s);

				if (f) {
					ILBuilder::build_fnptr(Ctx::scope(), f->func);
					ret.t = f->type;
				}
				else if (t != nullptr) {

					ILBuilder::build_const_type(Ctx::scope(), t);
					ret.t = Ctx::types()->t_type;
				}
				else {
					ILBuilder::build_const_size(Ctx::scope(), t_s);
					ret.t = Ctx::types()->t_size;
				}

				ret.lvalue = false;
			}
			else {
				Namespace* namespace_inst = nullptr;
				StructureTemplate* struct_inst = nullptr;
				FunctionTemplate* func_inst = nullptr;
				TraitTemplate* trait_inst = nullptr;


				Cursor err = c;

				Ctx::workspace()->find_name(c.buffer, namespace_inst, struct_inst, func_inst, trait_inst);

				if (namespace_inst == nullptr && struct_inst == nullptr && func_inst == nullptr && trait_inst == nullptr) {
					throw_specific_error(c, "Path start point not found");

				}


				Cursor nm_err = c;
				c.move();
				while (c.tok == RecognizedToken::DoubleColon && namespace_inst != nullptr) {
					c.move();
					if (c.tok != RecognizedToken::Symbol)
					{
						throw_not_a_name_error(c);

					}
					nm_err = c;

					namespace_inst->find_name(c.buffer, namespace_inst, struct_inst, func_inst, trait_inst);
					c.move();
				}


				if (struct_inst) {
					struct_inst->compile();

					if (struct_inst->is_generic) {
						if (cpt == CompileType::eval) {

							ILBuilder::eval_const_type(Ctx::eval(), struct_inst->type.get());
						}
						else if (cpt == CompileType::compile) {
							if (Ctx::scope_context() != ILContext::compile) {
								throw_specific_error(err, "Use of a type in runtime context");

							}

							ILBuilder::build_const_type(Ctx::scope(), struct_inst->type.get());
						}
					}
					else {
						StructureInstance* inst;
						struct_inst->generate(nullptr, inst);

						if (cpt == CompileType::eval) {
							ILBuilder::eval_const_type(Ctx::eval(), inst->type.get());
						}
						else if (cpt == CompileType::compile) {
							if (Ctx::scope_context() != ILContext::compile) {
								throw_specific_error(err, "Use of a type in runtime context");

							}
							ILBuilder::build_const_type(Ctx::scope(), inst->type.get());
						}
					}



					ret.lvalue = false;
					ret.t = Ctx::types()->t_type;
				}
				else if (func_inst != nullptr) {

					func_inst->compile();

					if (!func_inst->is_generic) {
						FunctionInstance* inst;
						func_inst->generate(nullptr, inst);
						inst->compile();

						if (cpt == CompileType::eval) {
							ILBuilder::eval_fnptr(Ctx::eval(), inst->func);
							//ILBuilder::eval_const_type(ctx.eval, inst->type);
						}
						else if (cpt == CompileType::compile) {
							ILBuilder::build_fnptr(Ctx::scope(), inst->func);
						}

						ret.lvalue = false;
						ret.t = inst->type;
					}
					else {
						if (cpt == CompileType::eval) {
							ILBuilder::eval_const_type(Ctx::eval(), func_inst->type.get());
						}
						else if (cpt == CompileType::compile) {
							if (Ctx::scope_context() != ILContext::compile) {
								throw_specific_error(err, "Use of a type in runtime context");

							}

							ILBuilder::build_const_type(Ctx::scope(), func_inst->type.get());
						}

						ret.lvalue = false;
						ret.t = Ctx::types()->t_type;
					}
				}
				else if (trait_inst != nullptr) {
					trait_inst->compile();

					if (trait_inst->is_generic) {
						if (cpt == CompileType::eval) {
							ILBuilder::eval_const_type(Ctx::eval(), trait_inst->type.get());
						}
						else if (cpt == CompileType::compile) {
							if (Ctx::scope_context() != ILContext::compile) {
								throw_specific_error(err, "Use of a type in runtime context");

							}

							ILBuilder::build_const_type(Ctx::scope(), trait_inst->type.get());
						}
					}
					else {
						TraitInstance* inst;
						trait_inst->generate(nullptr, inst);

						if (cpt == CompileType::eval) {
							ILBuilder::eval_const_type(Ctx::eval(), inst->type.get());
						}
						else if (cpt == CompileType::compile) {
							if (Ctx::scope_context() != ILContext::compile) {
								throw_specific_error(err, "Use of a type in runtime context");

							}
							ILBuilder::build_const_type(Ctx::scope(), inst->type.get());
						}
					}



					ret.lvalue = false;
					ret.t = Ctx::types()->t_type;
				}
				else {
					throw_specific_error(nm_err, "Path is pointing to a namespace");

				}

			}

		}


	}




	void Operand::parse_number(CompileValue& ret, Cursor& c, CompileType cpt) {
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
				ILBuilder::build_const_u32(Ctx::scope(), (uint32_t)d);
			else
				ILBuilder::build_const_i32(Ctx::scope(), (int32_t)d);
		}
		else if (cpt == CompileType::eval) {
			if (usg) {
				ILBuilder::eval_const_u32(Ctx::eval(), (uint32_t)d);
			}
			else {
				ILBuilder::eval_const_i32(Ctx::eval(), (int32_t)d);
			}

		}

		ret.t = usg ? Ctx::types()->t_u32 : Ctx::types()->t_i32;
		ret.lvalue = false;

	}





	void Operand::parse_long_number(CompileValue& ret, Cursor& c, CompileType cpt) {

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
				ILBuilder::build_const_u64(Ctx::scope(), d);
			else
				ILBuilder::build_const_i64(Ctx::scope(), d);
		}
		else if (cpt == CompileType::eval) {
			if (usg) {
				ILBuilder::eval_const_u64(Ctx::eval(), d);
			}
			else {
				ILBuilder::eval_const_i64(Ctx::eval(), d);
			}

		}

		ret.t = usg ? Ctx::types()->t_u64 : Ctx::types()->t_i64;
		ret.lvalue = false;

	}





	void Operand::parse_float_number(CompileValue& ret, Cursor& c, CompileType cpt) {

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
				ILBuilder::build_const_f64(Ctx::scope(), d);
			else
				ILBuilder::build_const_f32(Ctx::scope(), (float)d);
		}
		else if (cpt == CompileType::eval) {
			if (dbl) {
				ILBuilder::eval_const_f64(Ctx::eval(), d);
			}
			else {
				ILBuilder::eval_const_f32(Ctx::eval(), (float)d);
			}

		}

		ret.t = dbl ? Ctx::types()->t_f64 : Ctx::types()->t_f32;
		ret.lvalue = false;

	}


	template<typename T, typename S>
	void Operand::parse_generate_template(Cursor& c, T* generating, S*& out) {

		auto layout = generating->generic_ctx.generic_layout.begin();

		if (c.tok != RecognizedToken::CloseParenthesis) {
			while (true) {
				if (layout == generating->generic_ctx.generic_layout.end()) {
					throw_specific_error(c, "Too much arguments");

				}

				CompileValue res;
				Cursor err = c;
				Expression::parse(c, res, CompileType::eval);
				Operand::cast(err, res, std::get<1>(*layout), CompileType::eval, false);
				Expression::rvalue(res, CompileType::eval);

				layout++;

				if (c.tok == RecognizedToken::Comma) {
					c.move();
				}
				else if (c.tok == RecognizedToken::CloseParenthesis) {
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

		c.move();


		Ctx::eval_stack()->push();
		Ctx::eval()->stack_push();


		std::vector<std::tuple<Cursor, Type*>>::reverse_iterator act_layout;
		size_t act_layout_size = 0;

		//if (generating->generic_ctx.generator != nullptr) {
		//	generating->generic_ctx.generator->insert_key_on_stack(Ctx::eval());
		//}

		act_layout = generating->generic_ctx.generic_layout.rbegin();
		act_layout_size = generating->generic_ctx.generic_layout.size();

		unsigned char* key_base = Ctx::eval()->local_stack_base.back();


		for (size_t arg_i = act_layout_size - 1; arg_i >= 0 && arg_i < act_layout_size; arg_i--) {

			Type* type = std::get<1>(*act_layout);

			uint16_t sid = Ctx::eval()->push_local(type->size());
			unsigned char* data_place = Ctx::eval()->stack_ptr(sid);
			Ctx::eval_stack()->push_item(std::get<0>(*act_layout).buffer, type, sid, StackItemTag::regular);


			Ctx::eval()->write_register_value(data_place);
			Expression::copy_from_rvalue(type, CompileType::eval, false);

			act_layout++;
		}

		generating->generate(key_base, out);

		// ?????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
		// DROP WHOLE STACK

		StackItem sitm;
		while (Ctx::eval_stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) {
			if (sitm.type->has_special_destructor()) {
				sitm.type->drop(Ctx::eval()->stack_ptr(sitm.id));
			}
		}

		// ?????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????

		Ctx::eval()->stack_pop();
		Ctx::eval_stack()->pop();

	}

	void Operand::priv_build_push_template(ILEvaluator* eval) {
		Type* t = eval->pop_register_value<Type*>();
		template_stack[template_sp] = t;
		template_sp++;

	}

	void Operand::priv_build_build_template(ILEvaluator* eval) {
		Type* gen_type = template_stack[template_sp - 1];
		gen_type->compile();

		Ctx::eval_stack()->push();
		eval->stack_push();


		std::vector<std::tuple<Cursor, Type*>>::reverse_iterator act_layout;
		std::vector<std::tuple<Cursor, Type*>>::reverse_iterator act_layout_it;
		size_t gen_types = 0;

		if (gen_type->type() == TypeInstanceType::type_structure_template) {
			/*if (((TypeStructureTemplate*)gen_type)->owner->generic_ctx.generator != nullptr) {
				((TypeStructureTemplate*)gen_type)->owner->generic_ctx.generator->insert_key_on_stack(eval);
			}*/
			act_layout = ((TypeStructureTemplate*)gen_type)->owner->generic_ctx.generic_layout.rbegin();
			gen_types = ((TypeStructureTemplate*)gen_type)->owner->generic_ctx.generic_layout.size();
		}
		else if (gen_type->type() == TypeInstanceType::type_trait_template) {
			/*if (((TypeTraitTemplate*)gen_type)->owner->generic_ctx.generator != nullptr) {
				((TypeTraitTemplate*)gen_type)->owner->generic_ctx.generator->insert_key_on_stack(eval);
			}*/
			act_layout = ((TypeTraitTemplate*)gen_type)->owner->generic_ctx.generic_layout.rbegin();
			gen_types = ((TypeTraitTemplate*)gen_type)->owner->generic_ctx.generic_layout.size();
		}




		unsigned char* key_base = eval->local_stack_base.back() + eval->local_stack_size.back();


		act_layout_it = act_layout;
		for (size_t arg_i = gen_types - 1; arg_i >= 0 && arg_i < gen_types; arg_i--) {

			Type* type = std::get<1>((*act_layout_it));

			uint16_t local_id = eval->push_local(type->size());
			unsigned char* data_place = eval->stack_ptr(local_id);
			Ctx::eval_stack()->push_item(std::get<0>(*act_layout_it).buffer, type, local_id, StackItemTag::regular);


			eval->write_register_value(data_place);
			Expression::copy_from_rvalue(type, CompileType::eval, false);

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


		// ?????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
		StackItem sitm;
		while (Ctx::eval_stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) {
			if (sitm.type->has_special_destructor()) {
				sitm.type->drop(eval->stack_ptr(sitm.id));
			}
		}
		// ?????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????

		eval->stack_pop();
		Ctx::eval_stack()->pop();

	}

	Type* Operand::template_stack[1024];
	uint16_t Operand::template_sp = 0;


	void _crs_read_arguments(Cursor& c, unsigned int& argi, TypeFunction* ft, CompileType cpt) {
		if (c.tok != RecognizedToken::CloseParenthesis) {
			while (true) {
				if (argi >= Ctx::types()->argument_array_storage.get(ft->argument_array_id).size()) {
					throw_specific_error(c, "Wrong number of arguments");
				}

				CompileValue arg;
				Cursor err = c;
				Expression::parse(c, arg, cpt);
				Operand::cast(err, arg, Ctx::types()->argument_array_storage.get(ft->argument_array_id)[argi], cpt, true);
				Expression::rvalue(arg, cpt);
				argi++;

				if (c.tok == RecognizedToken::Comma) {
					c.move();
				}
				else if (c.tok == RecognizedToken::CloseParenthesis) {
					break;
				}
				else {
					throw_wrong_token_error(c, "',' or ')'");

				}
			}
		}


	}

	void Operand::parse_call_operator(CompileValue& ret, Cursor& c, CompileType cpt) {

		if (ret.t == Ctx::types()->t_type || ret.t->type() == TypeInstanceType::type_template) {
			Cursor nm_err = c;
			Expression::rvalue(ret, cpt);

			if (cpt != CompileType::compile) {
				Type* dt = nullptr;

				dt = Ctx::eval()->pop_register_value<Type*>();

				if (dt->type() != TypeInstanceType::type_structure_template && dt->type() != TypeInstanceType::type_trait_template && dt->type() != TypeInstanceType::type_function_template) {
					throw_specific_error(c, "this type is not a generic type");

				}
				c.move();

				if (dt->type() == TypeInstanceType::type_structure_template) {
					StructureInstance* inst;
					parse_generate_template(c, ((TypeStructureTemplate*)dt)->owner, inst);
					ILBuilder::eval_const_type(Ctx::eval(), inst->type.get());

					ret.lvalue = false;
					ret.t = Ctx::types()->t_type;

				}
				else if (dt->type() == TypeInstanceType::type_trait_template) {
					TraitInstance* inst;
					parse_generate_template(c, ((TypeTraitTemplate*)dt)->owner, inst);
					ILBuilder::eval_const_type(Ctx::eval(), inst->type.get());

					ret.lvalue = false;
					ret.t = Ctx::types()->t_type;
				}
				else if (dt->type() == TypeInstanceType::type_function_template) {
					FunctionInstance* inst;
					parse_generate_template(c, ((TypeFunctionTemplate*)dt)->owner, inst);
					inst->compile();

					ILBuilder::eval_fnptr(Ctx::eval(), inst->func);

					ret.lvalue = false;
					ret.t = inst->type;
				}



			}
			else {
				throw_specific_error(c, "Template generator not supported as a runtime operator, please use .generate(...) on generic template type");

			}

		}
		else if (ret.t->type() == TypeInstanceType::type_function) {
			Expression::rvalue(ret, cpt);

			c.move();
			TypeFunction* ft = (TypeFunction*)ret.t;

			if (ft->context() != ILContext::both && Ctx::scope_context() != ft->context()) {
				throw_specific_error(c, "Cannot call function with different context specifier");

			}

			unsigned int argi = 0;

			CompileValue retval;
			retval.t = ft->return_type;
			retval.lvalue = true;
			ft->return_type->compile();

			if (cpt == CompileType::compile) {

				uint16_t local_return_id = 0;


				ILBuilder::build_callstart(Ctx::scope());
				if (ft->return_type->rvalue_stacked()) {
					local_return_id = Ctx::workspace_function()->local_stack_lifetime.append(retval.t->size());
					Ctx::temp_stack()->push_item("$tmp", retval.t, local_return_id, StackItemTag::regular);

					if (retval.t->has_special_constructor()) {
						ILBuilder::build_local(Ctx::scope(), local_return_id);
						retval.t->build_construct();
					}

					ILBuilder::build_local(Ctx::scope(), local_return_id);
				}

				_crs_read_arguments(c, argi, ft, cpt);

				if (argi != Ctx::types()->argument_array_storage.get(ft->argument_array_id).size()) {
					throw_specific_error(c, "Wrong number of arguments");
				}

				c.move();

				if (!ft->return_type->rvalue_stacked()) {
					ILBuilder::build_call(Ctx::scope(), ft->return_type->rvalue(), (uint16_t)argi);
				}
				else {
					ILBuilder::build_call(Ctx::scope(), ILDataType::none, (uint16_t)argi + 1);
				}

				if (ft->return_type->rvalue_stacked()) {
					ILBuilder::build_local(Ctx::scope(), local_return_id);
				}

			}
			else if (cpt == CompileType::eval) {

				StackItem local_stack_item;

				ILBuilder::eval_callstart(Ctx::eval());

				if (ft->return_type->rvalue_stacked()) {
					uint16_t sid = Ctx::eval()->push_local(retval.t->size());
					unsigned char* memory_place = Ctx::eval()->stack_ptr(sid);

					Ctx::eval_stack()->push_item("$tmp", retval.t, sid, StackItemTag::regular);

					if (retval.t->has_special_constructor()) {
						retval.t->construct(memory_place);
					}

					ILBuilder::eval_local(Ctx::eval(), sid);
				}

				_crs_read_arguments(c, argi, ft, cpt);

				if (argi != Ctx::types()->argument_array_storage.get(ft->argument_array_id).size()) {
					throw_specific_error(c, "Wrong number of arguments");

				}

				c.move();

				if (!ft->return_type->rvalue_stacked()) {
					ILBuilder::eval_call(Ctx::eval(), ft->return_type->rvalue(), (uint16_t)argi);
				}
				else {
					ILBuilder::eval_call(Ctx::eval(), ILDataType::none, (uint16_t)argi + 1);
				}


				if (ft->return_type->rvalue_stacked()) {
					ILBuilder::eval_local(Ctx::eval(), local_stack_item.id);
				}

			}

			ret.t = ft->return_type;
			ret.lvalue = false;
		}
		else {
			throw_specific_error(c, "not implemented yet");

		}


	}

	void Operand::priv_build_reference(ILEvaluator* eval) {
		Type* t = eval->pop_register_value<Type*>();
		t = t->generate_reference();
		ILBuilder::eval_const_type(eval, t);
	}

	void Operand::priv_build_subtype(ILEvaluator* eval) {
		Type* str = Ctx::types()->t_u8->generate_slice();

		auto slice_ptr = eval->pop_register_value<size_t*>();
		char* slice_data = (char*)slice_ptr[0];
		size_t slice_size = slice_ptr[1];

		std::string_view slice_str(slice_data, slice_size);

		Type* t = eval->pop_register_value<Type*>();
		if (t->type() == TypeInstanceType::type_structure_instance) {
			TypeStructureInstance* tsi = (TypeStructureInstance*)t;

			Corrosive::Namespace* nspc;
			Corrosive::StructureTemplate* templ;
			Corrosive::FunctionTemplate* ftempl;
			Corrosive::TraitTemplate* trait;

			tsi->owner->find_name(slice_str, nspc, templ, ftempl, trait);

			if (templ) {
				templ->compile();
				if (!templ->is_generic) {
					StructureInstance* sinst;
					templ->generate(nullptr, sinst);
					sinst->compile();
					eval->write_register_value<void*>(sinst->type.get());
				}
				else {
					eval->write_register_value<void*>(templ->type.get());
				}
			}
			else if (ftempl) {
				ftempl->compile();
				eval->write_register_value<void*>(ftempl->type.get());
			}
			else if (trait) {
				trait->compile();
				if (!trait->is_generic) {
					TraitInstance* tinst;
					trait->generate(nullptr, tinst);
					eval->write_register_value<void*>(tinst->type.get());
				}
				else {
					eval->write_register_value<void*>(trait->type.get());
				}
			}
			else {
				eval->write_register_value<void*>(nullptr);
			}

		}
		else {
			eval->write_register_value<void*>(nullptr);
		}
	}

	void Operand::priv_build_slice(ILEvaluator* eval) {
		Type* t = eval->pop_register_value<Type*>();
		t = t->generate_slice();
		ILBuilder::eval_const_type(eval, t);
	}

	void Operand::parse_reference(CompileValue& ret, Cursor& c, CompileType cpt) {
		Cursor operr = c;

		unsigned int type_ref_count = 0;
		while (c.tok == RecognizedToken::And || c.tok == RecognizedToken::DoubleAnd) {
			type_ref_count++;
			if (c.tok == RecognizedToken::DoubleAnd)
				type_ref_count++;
			c.move();
		}

		Cursor err = c;
		CompileValue value;
		Operand::parse(c, value, cpt);
		Expression::rvalue(value, cpt);

		if (value.t != Ctx::types()->t_type) {
			throw_specific_error(err, "operator expected to recieve type");

		}

		if (cpt == CompileType::eval) {
			for (unsigned int i = 0; i < type_ref_count; i++)
				priv_build_reference(Ctx::eval());
		}
		else if (cpt == CompileType::compile) {
			throw_specific_error(operr, "Operation not supported in runtime context, please use .reference(...) function");

			/*for (unsigned int i = 0; i < type_ref_count; i++)
				ILBuilder::build_priv(Ctx::scope(),2,0);*/
		}

		ret.t = Ctx::types()->t_type;
		ret.lvalue = false;


	}

	void Operand::parse_array_operator(CompileValue& ret, Cursor& c, CompileType cpt) {
		if (ret.t->type() != TypeInstanceType::type_slice) {
			throw_specific_error(c, "Offset can be applied only on slices");

		}
		c.move();

		TypeSlice* slice = (TypeSlice*)ret.t;
		Type* base_slice = slice->owner;


		if (cpt == CompileType::compile) {
			ILBuilder::build_load(Ctx::scope(), ILDataType::word);
		}
		else {
			ILBuilder::eval_load(Ctx::eval(), ILDataType::word);
		}

		Cursor err = c;
		CompileValue index;
		Expression::parse(c, index, cpt);
		Expression::rvalue(index, cpt);
		Operand::cast(err, index, Ctx::types()->t_size, cpt, true);



		if (cpt == CompileType::compile) {
			ILBuilder::build_const_size(Ctx::scope(), base_slice->size());
			ILBuilder::build_mul(Ctx::scope(), ILDataType::word, ILDataType::word);
			ILBuilder::build_rtoffset(Ctx::scope());
		}
		else {
			ILBuilder::eval_const_size(Ctx::eval(), base_slice->size().eval(Ctx::global_module(), compiler_arch));
			ILBuilder::eval_mul(Ctx::eval(), ILDataType::word, ILDataType::word);
			ILBuilder::eval_rtoffset(Ctx::eval());
		}

		if (c.tok != RecognizedToken::CloseBracket) {
			throw_wrong_token_error(c, "']'");

		}
		c.move();

		ret.lvalue = true;
		ret.t = base_slice;


	}

	void Operand::priv_build_array(ILEvaluator* eval) {
		uint32_t val = eval->pop_register_value<uint32_t>();
		Type* t = eval->pop_register_value<Type*>();
		TypeArray* nt = t->generate_array(val);
		ILBuilder::eval_const_type(eval, nt);

	}

	void Operand::parse_array_type(CompileValue& ret, Cursor& c, CompileType cpt) {
		Cursor err = c;
		CompileValue res;
		c.move();

		if (c.tok == RecognizedToken::CloseBracket) {
			c.move();

			Cursor t_err = c;

			Operand::parse(c, res, cpt);
			Expression::rvalue(res, cpt);

			if (res.t != Ctx::types()->t_type) {
				throw_specific_error(t_err, "Expected type");

			}

			if (cpt == CompileType::eval) {
				Type* t = Ctx::eval()->pop_register_value<Type*>();
				Type* nt = t->generate_slice();
				ILBuilder::eval_const_type(Ctx::eval(), nt);
			}
			else {
				throw_specific_error(err, "Operation not supported in runtime context, please use .slice function");

			}
		}
		else {
			Expression::parse(c, res, cpt);
			Expression::rvalue(res, cpt);

			Operand::cast(err, res, Ctx::types()->t_u32, cpt, true);


			if (c.tok != RecognizedToken::CloseBracket) {
				throw_wrong_token_error(c, "']'");

			}
			c.move();

			Cursor t_err = c;

			Operand::parse(c, res, cpt);
			Expression::rvalue(res, cpt);

			if (res.t != Ctx::types()->t_type) {
				throw_specific_error(t_err, "Expected type");

			}

			if (cpt == CompileType::eval) {
				Type* t = Ctx::eval()->pop_register_value<Type*>();
				uint32_t val = Ctx::eval()->pop_register_value<uint32_t>();
				TypeArray* nt = t->generate_array(val);
				ILBuilder::eval_const_type(Ctx::eval(), nt);
			}
			else {
				throw_specific_error(err, "Operation not supported in runtime context, please use .array(...) function");

			}
		}

		ret.t = Ctx::types()->t_type;
		ret.lvalue = false;

	}


	void Operand::parse_double_colon_operator(CompileValue& ret, Cursor& c, CompileType cpt) {
		c.move();

		if (cpt == CompileType::compile) {
			throw_specific_error(c, "Operation :: is not supported in runtime context");

		}

		if (ret.t != Ctx::types()->t_type) {
			throw_specific_error(c, "left operator is not a type instance");

		}

		if (cpt == CompileType::eval) {
			Expression::rvalue(ret, cpt);
			TypeStructureInstance* ti = Ctx::eval()->pop_register_value<TypeStructureInstance*>();
			if (ti->type() != TypeInstanceType::type_structure_instance) {
				throw_specific_error(c, "Type is not structure instance");
			}

			StructureInstance* struct_inst = ti->owner;

			auto f = struct_inst->subtemplates.find(c.buffer);
			if (f != struct_inst->subtemplates.end()) {
				StructureTemplate* tplt = f->second.get();

				Ctx::eval_stack()->push();
				Ctx::eval()->stack_push();

				/*if (tplt->generic_ctx.generator)
					tplt->generic_ctx.generator->insert_key_on_stack(Ctx::eval());*/


				tplt->compile();

				ret.lvalue = false;
				if (tplt->is_generic) {
					ILBuilder::eval_const_type(Ctx::eval(), tplt->type.get());
				}
				else {
					StructureInstance* inst = nullptr;

					tplt->generate(Ctx::eval()->local_stack_base.back(), inst);
					ILBuilder::eval_const_type(Ctx::eval(), inst->type.get());
				}

				Ctx::eval()->stack_pop();
				Ctx::eval_stack()->pop();
			}
			else {
				throw_specific_error(c, "Structure instance does not contain a structure with this name");

			}
		}

		ret.lvalue = false;
		ret.t = Ctx::types()->t_type;

		c.move();


	}


	void Operand::parse_dot_operator(CompileValue& ret, Cursor& c, CompileType cpt) {
		if (ret.t->type() == TypeInstanceType::type_slice) {
			TypeSlice* ts = (TypeSlice*)ret.t;
			c.move();
			if (c.buffer == "count") {
				if (cpt == CompileType::compile) {
					if (ret.lvalue || ret.t->rvalue_stacked()) {
						ILBuilder::build_woffset(Ctx::scope(), 1);
						ILBuilder::build_load(Ctx::scope(), ILDataType::word);
					}
					else {
						ILBuilder::build_wroffset(Ctx::scope(), ret.t->rvalue(), ILDataType::word, 1);
					}
				}
				else {
					if (ret.lvalue || ret.t->rvalue_stacked()) {
						ILBuilder::eval_woffset(Ctx::eval(), 1);
						ILBuilder::eval_load(Ctx::eval(), ILDataType::word);
					}
					else {
						ILBuilder::eval_wroffset(Ctx::eval(), ret.t->rvalue(), ILDataType::word, 1);
					}
				}

				if (ts->owner->size().type != ILSizeType::absolute || ts->owner->size().value > 1) {
					if (cpt == CompileType::compile) {
						ILBuilder::build_const_size(Ctx::scope(), ts->owner->size());
						ILBuilder::build_div(Ctx::scope(), ILDataType::word, ILDataType::word);
					}
					else {
						ILBuilder::eval_const_size(Ctx::eval(), ts->owner->size().eval(Ctx::global_module(), compiler_arch));
						ILBuilder::eval_div(Ctx::eval(), ILDataType::word, ILDataType::word);
					}
				}

				c.move();

				ret.lvalue = false;
				ret.t = Ctx::types()->t_size;

			}
			else if (c.buffer == "size") {

				if (cpt == CompileType::compile) {

					if (ret.lvalue) {
						ILBuilder::build_woffset(Ctx::scope(), 1);
					}
					else if (ret.t->rvalue_stacked()) {
						ILBuilder::build_woffset(Ctx::scope(), 1);
						ILBuilder::build_load(Ctx::scope(), ILDataType::word);
					}
					else {
						ILBuilder::build_wroffset(Ctx::scope(), ret.t->rvalue(), ILDataType::word, 1);
					}
				}
				else {

					if (ret.lvalue) {
						ILBuilder::eval_woffset(Ctx::eval(), 1);
					}
					else if (ret.t->rvalue_stacked()) {
						ILBuilder::eval_woffset(Ctx::eval(), 1);
						ILBuilder::eval_load(Ctx::eval(), ILDataType::word);
					}
					else {
						ILBuilder::eval_wroffset(Ctx::eval(), ret.t->rvalue(), ILDataType::word, 1);
					}
				}


				c.move();

				//ret.lvalue is the original: lvalue.x will be lvalue, rvalue.x will be rvalue
				ret.t = Ctx::types()->t_size;

			}
			else if (c.buffer == "ptr") {
				Expression::rvalue(ret, cpt);

				if (cpt == CompileType::compile) {
					if (ret.lvalue) {

					}
					else if (ret.t->rvalue_stacked()) {
						ILBuilder::build_load(Ctx::scope(), ILDataType::word);
					}
					else {
						ILBuilder::build_cast(Ctx::scope(), ret.t->rvalue(), ILDataType::word);
					}
				}
				else {
					if (ret.lvalue) {

					}
					else if (ret.t->rvalue_stacked()) {
						ILBuilder::eval_load(Ctx::eval(), ILDataType::word);
					}
					else {
						ILBuilder::build_cast(Ctx::scope(), ret.t->rvalue(), ILDataType::word);
					}
				}

				c.move();
				//ret.lvalue is the original: lvalue.x will be lvalue, rvalue.x will be rvalue
				ret.t = Ctx::types()->t_ptr;

			}
			else {
				throw_specific_error(c, "Indentifier not recognized as a value of slice");

			}
		}
		else if (ret.t->type() == TypeInstanceType::type_template || ret.t == Ctx::types()->t_type) {
			c.move();

			if (c.buffer == "generate") {

				if (ret.t == Ctx::types()->t_type) {
					throw_specific_error(c, "Operation not supported on generic type, please use generic template type");

				}

				c.move();


				Expression::rvalue(ret, cpt);

				if (cpt == CompileType::compile) {
					ILBuilder::build_const_type(Ctx::scope(), ret.t);
					ILBuilder::build_insintric(Ctx::scope(), ILInsintric::template_cast);
					ILBuilder::build_insintric(Ctx::scope(), ILInsintric::push_template);
				}
				else {
					ILBuilder::eval_const_type(Ctx::eval(), ret.t);
					ILBuilder::eval_insintric(Ctx::eval(), ILInsintric::template_cast);
					ILBuilder::eval_insintric(Ctx::eval(), ILInsintric::push_template);
				}

				TypeTemplate* tt = (TypeTemplate*)ret.t;
				auto layout = Ctx::types()->argument_array_storage.get(tt->argument_array_id).begin();

				if (c.tok != RecognizedToken::OpenParenthesis) {
					throw_wrong_token_error(c, "'('");

				}
				c.move();

				std::vector<CompileValue> results;
				if (c.tok != RecognizedToken::CloseParenthesis) {
					while (true) {
						CompileValue res;
						Cursor err = c;
						Expression::parse(c, res, cpt);
						Expression::rvalue(res, cpt);
						Operand::cast(err, res, *layout, cpt, true);

						results.push_back(res);
						layout++;

						if (c.tok == RecognizedToken::Comma) {
							c.move();
						}
						else if (c.tok == RecognizedToken::CloseParenthesis) {
							break;
						}
						else {
							throw_wrong_token_error(c, "')' or ','");

						}
					}
				}

				c.move();

				if (cpt == CompileType::compile) {
					ILBuilder::build_insintric(Ctx::scope(), ILInsintric::build_template);
				}
				else {
					ILBuilder::eval_insintric(Ctx::eval(), ILInsintric::build_template);
				}
			}
			else if (c.buffer == "array") {
				c.move();
				if (c.tok != RecognizedToken::OpenParenthesis) {
					throw_wrong_token_error(c, "'('");

				}
				c.move();

				CompileValue res;
				Cursor err = c;
				Expression::rvalue(ret, cpt);

				Expression::parse(c, res, cpt);
				Operand::cast(err, res, Ctx::types()->t_u32, cpt, true);
				Expression::rvalue(res, cpt);


				if (c.tok != RecognizedToken::CloseParenthesis) {
					throw_wrong_token_error(c, "')'");
				}
				c.move();

				if (cpt == CompileType::compile) {
					ILBuilder::build_insintric(Ctx::scope(), ILInsintric::build_array);
				}
				else {
					ILBuilder::eval_insintric(Ctx::eval(), ILInsintric::build_array);
				}
			}
			else if (c.buffer == "reference") {
				c.move();
				Expression::rvalue(ret, cpt);
				if (cpt == CompileType::compile) {
					ILBuilder::build_insintric(Ctx::scope(), ILInsintric::build_reference);
				}
				else {
					ILBuilder::eval_insintric(Ctx::eval(), ILInsintric::build_reference);
				}
			}
			else if (c.buffer == "subtype") {
				c.move();

				if (c.tok != RecognizedToken::OpenParenthesis) {
					throw_wrong_token_error(c, "'('");
				}
				c.move();

				Expression::rvalue(ret, cpt);

				Cursor err = c;
				CompileValue res;
				Expression::parse(c, res, cpt);
				Type* to = Ctx::types()->t_u8->generate_slice();
				Operand::cast(err, res, to, cpt, true);
				Expression::rvalue(res, cpt);


				if (c.tok != RecognizedToken::CloseParenthesis) {
					throw_wrong_token_error(c, "')'");
				}
				c.move();


				if (cpt == CompileType::compile) {
					ILBuilder::build_insintric(Ctx::scope(), ILInsintric::build_subtype);
				}
				else {
					ILBuilder::eval_insintric(Ctx::eval(), ILInsintric::build_subtype);
				}
			}
			else if (c.buffer == "slice") {
				c.move();
				Expression::rvalue(ret, cpt);
				if (cpt == CompileType::compile) {
					ILBuilder::build_insintric(Ctx::scope(), ILInsintric::build_slice);
				}
				else {
					ILBuilder::eval_insintric(Ctx::eval(), ILInsintric::build_slice);
				}
			}
			else if (c.buffer == "size") {
				Expression::rvalue(ret, cpt);

				if (cpt == CompileType::compile) {
					ILBuilder::build_insintric(Ctx::scope(), ILInsintric::type_size);
				}
				else {
					ILBuilder::eval_insintric(Ctx::eval(), ILInsintric::type_size);
				}

				ret.lvalue = false;
				ret.t = Ctx::types()->t_size;

			}
			else {
				throw_specific_error(c, "Unknown type functional operator");

			}

			ret.lvalue = false;
			ret.t = Ctx::types()->t_type;
		}
		else if (ret.t->type() == TypeInstanceType::type_trait) {
			c.move();

			TypeTraitInstance* tti = (TypeTraitInstance*)ret.t;
			TraitInstance* ti = tti->owner;


			auto off_f = ti->member_table.find(c.buffer);
			if (off_f == ti->member_table.end()) {
				throw_specific_error(c, "Trait function not found");

			}
			uint32_t off = (uint32_t)off_f->second;
			auto& mf = ti->member_funcs[off];

			if (mf.ctx != ILContext::both && Ctx::scope_context() != mf.ctx) {
				throw_specific_error(c, "Cannot access trait function with different context");

			}

			Expression::rvalue(ret, cpt);
			c.move();
			if (c.tok == RecognizedToken::OpenParenthesis) {
				if (cpt == CompileType::compile) {
					if (ret.t->rvalue_stacked()) {

						ILBuilder::build_duplicate(Ctx::scope(), ILDataType::word);

						ILBuilder::build_woffset(Ctx::scope(), 1);
						ILBuilder::build_load(Ctx::scope(), ILDataType::word);
						ILBuilder::build_woffset(Ctx::scope(), (uint32_t)off);
						ILBuilder::build_load(Ctx::scope(), ILDataType::word);
						ILBuilder::build_callstart(Ctx::scope());

						ILBuilder::build_load(Ctx::scope(), ILDataType::word);
					}
					else {
						throw_specific_error(c, "Compiler error, trait object was placed inside register, but register trait call was not implemented");
					}
				}
				else {
					if (ret.t->rvalue_stacked()) {

						ILBuilder::eval_duplicate(Ctx::eval(), ILDataType::word);

						ILBuilder::eval_woffset(Ctx::eval(), 1);
						ILBuilder::eval_load(Ctx::eval(), ILDataType::word);
						ILBuilder::eval_woffset(Ctx::eval(), (uint32_t)off);
						ILBuilder::eval_load(Ctx::eval(), ILDataType::word);
						ILBuilder::eval_callstart(Ctx::eval());

						ILBuilder::eval_load(Ctx::eval(), ILDataType::word);
					}
					else {
						throw_specific_error(c, "Compiler error, trait object was placed inside register, but register trait call was not implemented");
					}
				}

				c.move();
				unsigned int argi = 1;
				_crs_read_arguments(c, argi, mf.type, cpt);
				c.move();

				if (cpt == CompileType::compile) {
					ILBuilder::build_call(Ctx::scope(), mf.type->return_type->rvalue(), argi);
				}
				else {
					ILBuilder::eval_call(Ctx::eval(), mf.type->return_type->rvalue(), argi);
				}

				ret.lvalue = false;
				ret.t = mf.type->return_type;
			}
			else {
				if (cpt == CompileType::compile) {
					if (ret.t->rvalue_stacked()) {
						ILBuilder::build_woffset(Ctx::scope(), 1);
						ILBuilder::build_load(Ctx::scope(), ILDataType::word);
						ILBuilder::build_woffset(Ctx::scope(), (uint32_t)off);
					}
					else {
						std::cout << "???"; // not invented yet
					}
				}
				else {
					if (ret.t->rvalue_stacked()) {
						ILBuilder::eval_woffset(Ctx::eval(), 1);
						ILBuilder::eval_load(Ctx::eval(), ILDataType::word);
						ILBuilder::eval_woffset(Ctx::eval(), (uint32_t)off);
					}
					else {
						std::cout << "???"; // not invented yet
					}
				}

				ret.lvalue = true;
				ret.t = mf.type;
			}
		}
		else {

			c.move();
			Cursor tok = c;
			c.move();

			bool continue_deeper = true;

			while (continue_deeper) {

				if (ret.t->type() == TypeInstanceType::type_reference) {
					ret.t = ((TypeReference*)ret.t)->owner;
					ret.lvalue = true;

					if (cpt == CompileType::compile) {
						ILBuilder::build_load(Ctx::scope(), ret.t->rvalue());
					}
					else if (cpt == CompileType::eval) {
						ILBuilder::eval_load(Ctx::eval(), ret.t->rvalue());
					}
				}

				if (ret.t->type() != TypeInstanceType::type_structure_instance) {
					throw_specific_error(c, "Operator cannot be used on this type");
				}


				TypeStructureInstance* ti = (TypeStructureInstance*)ret.t;
				if (cpt == CompileType::compile) {
					ti->compile();
				}

				uint16_t elem_id = 0;

				Type* mem_type = nullptr;

				StructureInstance* si = ti->owner;


				ILSize elem_size(si->size.type, 0);

				auto table_element = si->member_table.find(tok.buffer);
				if (table_element != si->member_table.end()) {

					auto& member = si->member_vars[table_element->second.first];
					continue_deeper = table_element->second.second;

					if (si->size.type == ILSizeType::table) {
						elem_id = table_element->second.first;
						elem_size.value = si->size.value;
					}
					else {
						elem_size.value = member.second;
					}

					mem_type = member.first;
				}
				else {
					throw_specific_error(tok, "Instance does not contain member with this name");
				}

				if (ret.lvalue)
				{
					if (cpt == CompileType::compile) {
						if (!si->wrapper) {
							if (elem_size.type == ILSizeType::table)
								ILBuilder::build_tableoffset(Ctx::scope(), elem_size.value, elem_id);
							else if (elem_size.type == ILSizeType::absolute)
								ILBuilder::build_aoffset(Ctx::scope(), (uint32_t)elem_size.value);
							else if (elem_size.type == ILSizeType::word)
								ILBuilder::build_woffset(Ctx::scope(), (uint32_t)elem_size.value);
						}
					}
					else if (cpt == CompileType::eval) {
						if (!si->wrapper) {
							if (elem_size.type == ILSizeType::table)
								ILBuilder::eval_tableoffset(Ctx::eval(), elem_size.value, elem_id);
							else if (elem_size.type == ILSizeType::absolute)
								ILBuilder::eval_aoffset(Ctx::eval(), (uint32_t)elem_size.value);
							else if (elem_size.type == ILSizeType::word)
								ILBuilder::eval_woffset(Ctx::eval(), (uint32_t)elem_size.value);
						}
					}
				}
				else if (ret.t->rvalue_stacked()) {
					if (cpt == CompileType::compile) {
						if (!si->wrapper) {
							if (elem_size.type == ILSizeType::table)
								ILBuilder::build_tableoffset(Ctx::scope(), elem_size.value, elem_id);
							else if (elem_size.type == ILSizeType::absolute)
								ILBuilder::build_aoffset(Ctx::scope(), (uint32_t)elem_size.value);
							else if (elem_size.type == ILSizeType::word)
								ILBuilder::build_woffset(Ctx::scope(), (uint32_t)elem_size.value);
						}

						if (!mem_type->rvalue_stacked()) {
							ILBuilder::build_load(Ctx::scope(), mem_type->rvalue());
						}
					}
					else if (cpt == CompileType::eval) {

						if (!si->wrapper) {
							if (elem_size.type == ILSizeType::table)
								ILBuilder::eval_tableoffset(Ctx::eval(), elem_size.value, elem_id);
							else if (elem_size.type == ILSizeType::absolute)
								ILBuilder::eval_aoffset(Ctx::eval(), (uint32_t)elem_size.value);
							else if (elem_size.type == ILSizeType::word)
								ILBuilder::eval_woffset(Ctx::eval(), (uint32_t)elem_size.value);
						}

						if (!mem_type->rvalue_stacked()) {
							ILBuilder::eval_load(Ctx::eval(), mem_type->rvalue());
						}
					}
				}
				else {
					if (cpt == CompileType::compile) {
						if (!si->wrapper) {
							if (elem_size.type == ILSizeType::table)
								ILBuilder::build_tableroffset(Ctx::scope(), ret.t->rvalue(), mem_type->rvalue(), elem_size.value, elem_id);
							else if (elem_size.type == ILSizeType::absolute)
								ILBuilder::build_aroffset(Ctx::scope(), ret.t->rvalue(), mem_type->rvalue(), (uint8_t)elem_size.value);
							else if (elem_size.type == ILSizeType::word)
								ILBuilder::build_wroffset(Ctx::scope(), ret.t->rvalue(), mem_type->rvalue(), (uint8_t)elem_size.value);
						}

					}
					else if (cpt == CompileType::eval) {

						if (!si->wrapper) {
							if (elem_size.type == ILSizeType::table)
								ILBuilder::eval_tableroffset(Ctx::eval(), ret.t->rvalue(), mem_type->rvalue(), elem_size.value, elem_id);
							else if (elem_size.type == ILSizeType::absolute)
								ILBuilder::eval_aroffset(Ctx::eval(), ret.t->rvalue(), mem_type->rvalue(), (uint8_t)elem_size.value);
							else if (elem_size.type == ILSizeType::word)
								ILBuilder::eval_wroffset(Ctx::eval(), ret.t->rvalue(), mem_type->rvalue(), (uint8_t)elem_size.value);
						}

					}

				}

				// ret.lvalue stays the same
				ret.t = mem_type;
			}
			
		}


	}


	void Operand::parse_string_literal(CompileValue& ret, Cursor& c, CompileType cpt) {
		auto lit = Ctx::const_mgr()->register_string_literal(c);

		Type* slice = Ctx::types()->t_u8->generate_slice();

		if (cpt == CompileType::compile) {
			uint32_t local_id = Ctx::workspace_function()->local_stack_lifetime.append(slice->size());
			Ctx::temp_stack()->push_item("$tmp", slice, local_id, StackItemTag::regular);
			ILBuilder::build_constref(Ctx::scope(), lit.second);
			ILBuilder::build_local(Ctx::scope(), local_id);
			ILBuilder::build_store(Ctx::scope(), ILDataType::word);
			ILBuilder::build_const_size(Ctx::scope(), ILSize(ILSizeType::absolute, (uint32_t)lit.first.length()));
			ILBuilder::build_local(Ctx::scope(), local_id);
			ILBuilder::build_woffset(Ctx::scope(), 1);
			ILBuilder::build_store(Ctx::scope(), ILDataType::word);
			ILBuilder::build_local(Ctx::scope(), local_id);
		}
		else {
			uint32_t local_id = Ctx::eval()->push_local(slice->size());
			Ctx::eval_stack()->push_item("$tmp", slice, local_id, StackItemTag::regular);
			ILBuilder::eval_constref(Ctx::eval(), lit.second);
			ILBuilder::eval_local(Ctx::eval(), local_id);
			ILBuilder::eval_store(Ctx::eval(), ILDataType::word);
			ILBuilder::eval_const_size(Ctx::eval(), lit.first.length());
			ILBuilder::eval_local(Ctx::eval(), local_id);
			ILBuilder::eval_woffset(Ctx::eval(), 1);
			ILBuilder::eval_store(Ctx::eval(), ILDataType::word);
			ILBuilder::eval_local(Ctx::eval(), local_id);
		}

		c.move();
		ret.t = slice;
		ret.lvalue = false;
	}
}