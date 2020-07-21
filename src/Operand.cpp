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
		eval->write_register_value(t->size().eval(t->compiler()->global_module(), compiler_arch));
	}

	void Operand::priv_type_template_cast_crsr(ILEvaluator* eval, Cursor& err) {

		Type* template_cast = eval->pop_register_value<Type*>();
		Type* template_type = eval->read_register_value<Type*>();

		if (template_type->type() == TypeInstanceType::type_structure_template) {
			TypeStructureTemplate* st = (TypeStructureTemplate*)template_type;
			TypeTemplate* tt = (TypeTemplate*)template_cast;

			if (st->owner->generic_ctx.generic_layout.size() != st->owner->compiler->types()->argument_array_storage.get(tt->argument_array_id).size()) {
				throw_cannot_cast_error(err, template_type, template_cast);
			}
			else {
				for (size_t i = 0; i < st->owner->generic_ctx.generic_layout.size(); i++) {
					if (std::get<1>(st->owner->generic_ctx.generic_layout[i]) != st->owner->compiler->types()->argument_array_storage.get(tt->argument_array_id)[i]) {
						throw_cannot_cast_error(err, template_type, template_cast);
					}
				}
			}
		}
		else if (template_type->type() == TypeInstanceType::type_trait_template) {
			TypeTraitTemplate* st = (TypeTraitTemplate*)template_type;
			TypeTemplate* tt = (TypeTemplate*)template_cast;

			if (st->owner->generic_ctx.generic_layout.size() != st->owner->compiler->types()->argument_array_storage.get(tt->argument_array_id).size()) {
				throw_cannot_cast_error(err, template_type, template_cast);
			}
			else {
				for (size_t i = 0; i < st->owner->generic_ctx.generic_layout.size(); i++) {
					if (std::get<1>(st->owner->generic_ctx.generic_layout[i]) != st->owner->compiler->types()->argument_array_storage.get(tt->argument_array_id)[i]) {
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

			if (st->owner->generic_ctx.generic_layout.size() != st->owner->compiler->types()->argument_array_storage.get(tt->argument_array_id).size()) {
				throw_runtime_exception(eval, "Template cannot be casted to this generic type");
			}
			else {
				for (size_t i = 0; i < st->owner->generic_ctx.generic_layout.size(); i++) {
					if (std::get<1>(st->owner->generic_ctx.generic_layout[i]) != st->owner->compiler->types()->argument_array_storage.get(tt->argument_array_id)[i]) {
						throw_runtime_exception(eval, "Template cannot be casted to this generic type");
					}
				}
			}
		}
		else if (template_type->type() == TypeInstanceType::type_trait_template) {
			TypeTraitTemplate* st = (TypeTraitTemplate*)template_type;
			TypeTemplate* tt = (TypeTemplate*)template_cast;

			if (st->owner->generic_ctx.generic_layout.size() != st->owner->compiler->types()->argument_array_storage.get(tt->argument_array_id).size()) {
				throw_runtime_exception(eval, "Template cannot be casted to this generic type");
			}
			else {
				for (size_t i = 0; i < st->owner->generic_ctx.generic_layout.size(); i++) {
					if (std::get<1>(st->owner->generic_ctx.generic_layout[i]) != st->owner->compiler->types()->argument_array_storage.get(tt->argument_array_id)[i]) {
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
		return !t->rvalue_stacked() && t->rvalue() < ILDataType::none;
	}

	void Operand::cast(Compiler& compiler, Cursor& err, CompileValue& res, Type*& to, CompileType cpt, bool implicit) {

		if (res.type == compiler.types()->t_type && to->type() == TypeInstanceType::type_template) {
			if (cpt == CompileType::eval) {
				ILBuilder::eval_const_type(compiler.evaluator(), to);
				priv_type_template_cast_crsr(compiler.evaluator(), err);

				res.type = to;

			}
			else {
				ILBuilder::build_const_type(compiler.scope(), to);
				ILBuilder::build_insintric(compiler.scope(), ILInsintric::type_dynamic_cast);
				res.type = to;
			}
		}
		else if (to == compiler.types()->t_type && res.type->type() == TypeInstanceType::type_template) {
			res.type = to;

		}
		else if ((res.type == compiler.types()->t_ptr || res.type->type() == TypeInstanceType::type_reference || (Operand::is_numeric_value(res.type) && res.type != compiler.types()->t_bool)) && to == compiler.types()->t_bool) {
			Expression::rvalue(compiler, res, cpt);
			if (cpt == CompileType::eval) {
				ILBuilder::eval_isnotzero(compiler.evaluator(), res.type->rvalue());
				res.type = to;
				res.lvalue = false;

			}
			else {
				ILBuilder::build_isnotzero(compiler.scope(), res.type->rvalue());
				res.type = to;
				res.lvalue = false;

			}
		}
		else if ((res.type->type() == TypeInstanceType::type_reference || (res.type->type() == TypeInstanceType::type_structure_instance && res.lvalue)) && to->type() == TypeInstanceType::type_trait) {

			TypeTraitInstance* tt = (TypeTraitInstance*)to;

			TypeStructureInstance* ts = nullptr;
			if (res.type->type() == TypeInstanceType::type_reference) {
				TypeReference* tr = (TypeReference*)res.type;
				if (tr->owner->type() == TypeInstanceType::type_structure_instance) {
					ts = (TypeStructureInstance*)tr->owner;
					Expression::rvalue(compiler, res, cpt);
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


				if (to->rvalue_stacked()) {
					uint16_t sid = compiler.evaluator()->push_local(to->size());
					unsigned char* memory_place = compiler.evaluator()->stack_ptr(sid);
					compiler.compiler_stack()->push_item("$tmp", to, sid, StackItemTag::regular);

					ILBuilder::eval_const_ptr(compiler.evaluator(), memory_place);
					ILBuilder::eval_store(compiler.evaluator(), ILDataType::word);
					ILBuilder::eval_vtable(compiler.evaluator(), vtableid);
					ILBuilder::eval_const_ptr(compiler.evaluator(), memory_place + sizeof(void*));
					ILBuilder::eval_store(compiler.evaluator(), ILDataType::word);
					ILBuilder::eval_const_ptr(compiler.evaluator(), memory_place);
				}
				else {
					ILBuilder::eval_vtable(compiler.evaluator(), vtableid); // there is two pointers on register stack, if i read 2x pointer size register it will read the right value
				}

			}
			else {
				if (to->rvalue_stacked()) {
					to->compile();
					uint32_t local_id = compiler.target()->local_stack_lifetime.append(to->size());
					compiler.temp_stack()->push_item("$tmp", to, local_id, StackItemTag::regular);

					ILBuilder::build_local(compiler.scope(), local_id);
					ILBuilder::build_store(compiler.scope(), ILDataType::word);
					ILBuilder::build_vtable(compiler.scope(), vtableid);
					ILBuilder::build_local(compiler.scope(), local_id);
					ILBuilder::build_woffset(compiler.scope(), 1);
					ILBuilder::build_store(compiler.scope(), ILDataType::word);
					ILBuilder::build_local(compiler.scope(), local_id);
				}
				else {
					std::cout << "???"; // two pointers require 128 int on x64, not yet implemented
				}
			}

			res.type = to;


		}
		else if ((res.type == compiler.types()->t_ptr && to->type() == TypeInstanceType::type_reference) || (to == compiler.types()->t_ptr && res.type->type() == TypeInstanceType::type_reference)) {
			Expression::rvalue(compiler, res, cpt);
			res.lvalue = false;
			res.type = to;

		}
		else if (res.type->type() == TypeInstanceType::type_reference && to->type() == TypeInstanceType::type_reference) {
			if (res.type != to && implicit) {
				throw_cannot_implicit_cast_error(err, res.type, to);

			}

			Expression::rvalue(compiler, res, cpt);
			res.lvalue = false;
			res.type = to;

		}
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
		else if (Operand::is_numeric_value(res.type) && Operand::is_numeric_value(to)) {
			Expression::rvalue(compiler, res, cpt);
			if (cpt == CompileType::eval) {
				if (res.type->rvalue() != to->rvalue()) {
					ILBuilder::eval_cast(compiler.evaluator(), res.type->rvalue(), to->rvalue());
				}

				res.type = to;
				res.lvalue = false;

			}
			else {
				if (res.type->rvalue() != to->rvalue())
					ILBuilder::build_cast(compiler.scope(), res.type->rvalue(), to->rvalue());

				res.type = to;
				res.lvalue = false;

			}
		}
		else if (res.type != to) {
			throw_cannot_cast_error(err, res.type, to);
		}


		res.type = to;

	}

	void Operand::parse(Compiler& compiler, Cursor& c, CompileValue& res, CompileType cpt) {
		CompileValue ret;

		ret.lvalue = false;
		ret.type = nullptr;

		switch (c.tok) {
			case RecognizedToken::And:
			case RecognizedToken::DoubleAnd: {
				Operand::parse_reference(compiler,res, c, cpt);
				return;
			}
			case RecognizedToken::OpenBracket: {
				Operand::parse_array_type(compiler,res, c, cpt);
				return;
			}
			case RecognizedToken::Star: {
				c.move();
				Cursor err = c;
				CompileValue value;
				Operand::parse(compiler,c, value, cpt);
				Expression::rvalue(compiler, value, cpt);
				if (value.type->type() != TypeInstanceType::type_reference) {
					throw_specific_error(err, "Target is not reference, and cannot be dereferenced");

				}

				res.lvalue = true;
				res.type = ((TypeReference*)value.type)->owner;
				return;
			}

			case RecognizedToken::OpenParenthesis: {
				Operand::parse_expression(compiler,ret, c, cpt);
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

					compiler.push_scope_context(ILContext::compile);

					Expression::parse(compiler, c, t_res, CompileType::eval);
					Expression::rvalue(compiler, t_res, CompileType::eval);

					if (t_res.type != compiler.types()->t_type) {
						throw_specific_error(err, "Expected type");

					}

					auto to = compiler.evaluator()->pop_register_value<Type*>();
					to->compile();

					if (c.tok != RecognizedToken::CloseParenthesis) {
						throw_wrong_token_error(c, "')'");

					}
					c.move();

					compiler.pop_scope_context();

					err = c;
					CompileValue value;
					Operand::parse(compiler,c, value, cpt);
					Operand::cast(compiler,err, value, to, cpt, false);

					res = value;
					return;
				}
				else {
					Operand::parse_symbol(compiler,ret, c, cpt);
				}
			}break;

			case RecognizedToken::Minus: {
				Cursor err = c;
				c.move();
				Expression::parse(compiler, c, res, cpt);
				Expression::rvalue(compiler, res, cpt);

				if (!Operand::is_numeric_value(res.type)) {
					throw_specific_error(err, "Operation requires number operand");
				}

				if (cpt == CompileType::compile) {
					ILBuilder::build_negative(compiler.scope(), res.type->rvalue());
				}
				else {
					ILBuilder::eval_negative(compiler.evaluator(), res.type->rvalue());
				}
			}return;

			case RecognizedToken::ExclamationMark: {
				c.move();
				Cursor err = c;
				Expression::parse(compiler, c, res, cpt);
				Expression::rvalue(compiler, res, cpt);
				Operand::cast(compiler,err, res, compiler.types()->t_bool, cpt, true);

				if (cpt == CompileType::compile) {
					ILBuilder::build_negate(compiler.scope());
				}
				else {
					ILBuilder::eval_negate(compiler.evaluator());
				}
			}return;

			case RecognizedToken::Number:
			case RecognizedToken::UnsignedNumber: {
				Operand::parse_number(compiler,ret, c, cpt);
			}break;

			case RecognizedToken::LongNumber:
			case RecognizedToken::UnsignedLongNumber: {
				Operand::parse_long_number(compiler,ret, c, cpt);
			}break;

			case RecognizedToken::FloatNumber:
			case RecognizedToken::DoubleNumber: {
				Operand::parse_float_number(compiler,ret, c, cpt);
			}break;

			case RecognizedToken::String: {
				Operand::parse_string_literal(compiler,ret, c, cpt);
			}break;


			default: {
				throw_specific_error(c, "Expected to parse operand");
			} break;
		}

		while (true) {
			switch (c.tok) {
				case RecognizedToken::Symbol: {
					if (c.buffer == "cast") {
						//Expression::rvalue(compiler, ret, cpt);

						c.move();
						if (c.tok != RecognizedToken::OpenParenthesis) {
							throw_wrong_token_error(c, "'('");

						}
						c.move();


						compiler.push_scope_context(ILContext::compile);

						Cursor err = c;

						CompileValue t_res;
						Expression::parse(compiler, c, t_res, CompileType::eval);
						Expression::rvalue(compiler, t_res, CompileType::eval);

						if (t_res.type != compiler.types()->t_type) {
							throw_specific_error(err, "Expected type");

						}

						auto to = compiler.evaluator()->pop_register_value<Type*>();
						to->compile();

						if (c.tok != RecognizedToken::CloseParenthesis) {
							throw_wrong_token_error(c, "')'");

						}
						c.move();

						compiler.pop_scope_context();

						Operand::cast(compiler,err, ret, to, cpt, false);
					}
					else goto break_while;
				}break;
				case RecognizedToken::OpenParenthesis: {
					parse_call_operator(compiler,ret, c, cpt);
				}break;
				case RecognizedToken::OpenBracket: {
					parse_array_operator(compiler,ret, c, cpt);
				}break;
				case RecognizedToken::Dot: {
					parse_dot_operator(compiler,ret, c, cpt);
				}break;
				case RecognizedToken::DoubleColon: {
					parse_double_colon_operator(compiler,ret, c, cpt);
				}break;
				default: goto break_while;
			}

			continue;
		break_while:
			break;
		}

		res = ret;

	}




	void Operand::parse_const_type_function(Compiler& compiler, Cursor& c, FunctionInstance*& func, Type*& type, ILSize& type_size) {
		Namespace* namespace_inst = nullptr;
		StructureTemplate* struct_inst = nullptr;
		FunctionTemplate* func_inst = nullptr;
		TraitTemplate* trait_inst = nullptr;


		compiler.push_scope_context(ILContext::compile);



		Cursor err = c;
		compiler.workspace()->find_name(c.buffer, namespace_inst, struct_inst, func_inst, trait_inst);

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
					compiler.pop_scope_context();
					return;
				}
				c.move();

				Operand::parse_generate_template(compiler,c, struct_inst, inst);
			}
			else {
				struct_inst->generate(nullptr, inst);
			}



			if (c.tok != RecognizedToken::DoubleColon) {
				compiler.pop_scope_context();
				type = inst->type.get();
				return;
			}
			c.move();

			nm_err = c;

			c.move();

			Namespace* f_nspc;
			TraitTemplate* f_ttemp;

			inst->find_name(nm_err.buffer, f_nspc, struct_inst, func_inst, f_ttemp);

			if (f_nspc || f_ttemp) {
				throw_specific_error(nm_err, "Only function may be brought in the runtime context");
			}
		}


		if (func_inst != nullptr) {

			func_inst->compile();

			if (!func_inst->is_generic) {
				FunctionInstance* inst;
				func_inst->generate(nullptr, inst);
				inst->compile();

				func = inst;

				compiler.pop_scope_context();
				return;
			}
			else {
				FunctionInstance* inst;
				if (c.tok != RecognizedToken::OpenParenthesis) {
					type = func_inst->type.get();

					compiler.pop_scope_context();
					return;
				}
				c.move();

				Operand::parse_generate_template(compiler,c, func_inst, inst);
				inst->compile();

				compiler.pop_scope_context();
				func = inst;
				return;
			}

		}
		else {
			throw_specific_error(nm_err, "Only function may be brought in the runtime context");

		}

		compiler.pop_scope_context();

	}



	void Operand::parse_expression(Compiler& compiler, CompileValue& ret, Cursor& c, CompileType cpt) {
		c.move();
		Expression::parse(compiler, c, ret, cpt);

		if (c.tok != RecognizedToken::CloseParenthesis) {
			throw_wrong_token_error(c, "')'");

		}
		c.move();


	}



	void Operand::parse_symbol(Compiler& compiler, CompileValue& ret, Cursor& c, CompileType cpt) {
		if (c.buffer == "true") {
			c.move();
			ret.lvalue = false;
			ret.type = compiler.types()->t_bool;
			if (cpt == CompileType::compile) {
				ILBuilder::build_const_i8(compiler.scope(), true);
			}
			else if (cpt == CompileType::eval) {
				ILBuilder::eval_const_i8(compiler.evaluator(), true);
			}
		}
		else if (c.buffer == "false") {
			c.move();
			ret.lvalue = false;
			ret.type = compiler.types()->t_bool;
			if (cpt == CompileType::compile) {
				ILBuilder::build_const_i8(compiler.scope(), false);
			}
			else if (cpt == CompileType::eval) {
				ILBuilder::eval_const_i8(compiler.evaluator(), false);
			}
		}
		else if (c.buffer == "self") {
			ret.lvalue = false;
			ret.type = compiler.types()->t_type;

			Namespace* nspc = compiler.workspace();
			StructureInstance* s = dynamic_cast<StructureInstance*>(nspc);

			if (!s) {
				throw_specific_error(c, "Self must be used inside structure");
			}
			else {

				if (compiler.scope_context() != ILContext::compile) {
					throw_specific_error(c, "Self type can be used only in compile context");
				}

				if (cpt == CompileType::compile) {
					ILBuilder::build_const_type(compiler.scope(), s->type.get());
				}
				else if (cpt == CompileType::eval) {
					ILBuilder::eval_const_type(compiler.evaluator(), s->type.get());
				}
			}

			c.move();
		}
		else if (c.buffer == "null") {
			c.move();
			ret.lvalue = false;
			ret.type = compiler.types()->t_ptr;

			if (cpt == CompileType::compile) {
				ILBuilder::build_null(compiler.scope());
			}
			else if (cpt == CompileType::eval) {
				ILBuilder::eval_null(compiler.evaluator());
			}
		}
		else if (c.buffer == "typesize") {
			c.move();
			compiler.push_scope_context(ILContext::compile);

			CompileValue type_val;
			Cursor err = c;
			Operand::parse(compiler,c, type_val, CompileType::eval);
			Expression::rvalue(compiler, type_val, CompileType::eval);

			if (type_val.type != compiler.types()->t_type) {
				throw_specific_error(err, "Expected type");
			}

			auto tp = compiler.evaluator()->pop_register_value<Type*>();
			tp->compile();
			compiler.pop_scope_context();

			if (cpt == CompileType::compile) {
				ILBuilder::build_const_size(compiler.scope(), tp->size());
			}
			else {
				ILBuilder::eval_const_size(compiler.evaluator(), tp->size().eval(compiler.global_module(), compiler_arch));
			}

			ret.lvalue = false;
			ret.type = compiler.types()->t_size;
		}
		else if (c.buffer == "default") {
			c.move();

			CompileValue type_val;
			Cursor err = c;
			Operand::parse(compiler,c, type_val, cpt);

			if (type_val.type->type() == TypeInstanceType::type_structure_instance && type_val.lvalue) {
				type_val.type->compile();

				if (cpt == CompileType::compile) {
					//ILBuilder::build_duplicate(compiler.scope(), ILDataType::ptr);
					type_val.type->build_construct();
				}
				else {
					type_val.type->construct(compiler.evaluator()->pop_register_value<unsigned char*>());
				}
			}
			else if (type_val.type->type() == TypeInstanceType::type_reference) {
				TypeReference* tr = (TypeReference*)type_val.type;
				tr->owner->compile();
				Expression::rvalue(compiler, type_val, cpt);

				if (cpt == CompileType::compile) {
					//ILBuilder::build_duplicate(compiler.scope(), ILDataType::ptr);
					tr->owner->build_construct();
				}
				else {
					tr->owner->construct(compiler.evaluator()->pop_register_value<unsigned char*>());
				}

			}
			else {
				throw_specific_error(err, "Expected lvalue of structure or equivalent reference");
			}

			ret.type = compiler.types()->t_void;
			ret.lvalue = false;
		}
		else if (c.buffer == "drop") {
			c.move();

			CompileValue type_val;
			Cursor err = c;
			Operand::parse(compiler,c, type_val, cpt);

			if (type_val.type->type() == TypeInstanceType::type_structure_instance && type_val.lvalue) {

				type_val.type->compile();

				if (cpt == CompileType::compile) {
					type_val.type->build_drop();
				}
				else {
					type_val.type->drop(compiler.evaluator()->pop_register_value<unsigned char*>());
				}

			}
			else if (type_val.type->type() == TypeInstanceType::type_reference) {
				TypeReference* tr = (TypeReference*)type_val.type;
				tr->owner->compile();
				tr->compile();
				Expression::rvalue(compiler, type_val, cpt);

				if (cpt == CompileType::compile) {
					tr->owner->build_drop();
				}
				else {
					tr->owner->drop(compiler.evaluator()->pop_register_value<unsigned char*>());
				}
			}
			else {
				throw_specific_error(err, "Expected lvalue of structure or equivalent reference");
			}

			ret.type = compiler.types()->t_void;
			ret.lvalue = false;
		}
		else if (c.buffer == "type") {

			c.move();
			if (c.tok != RecognizedToken::OpenParenthesis) {
				compiler.evaluator()->write_register_value(compiler.types()->t_type);
			}
			else {
				c.move();
				std::vector<Type*> arg_types;

				if (c.tok != RecognizedToken::CloseParenthesis) {
					while (true) {
						Cursor err = c;
						CompileValue arg;
						Expression::parse(compiler, c, arg, CompileType::eval);
						Expression::rvalue(compiler, arg, CompileType::eval);

						if (arg.type != compiler.types()->t_type) {
							throw_specific_error(err, "Expected type");

						}

						auto type = compiler.evaluator()->pop_register_value<Type*>();
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


				Type* ftype = compiler.types()->load_or_register_template_type(std::move(arg_types));

				compiler.evaluator()->write_register_value(ftype);
			}

			ret.lvalue = false;
			ret.type = compiler.types()->t_type;
		}
		else if (c.buffer == "fn") {
			if (cpt == CompileType::compile) {
				if (compiler.scope_context() != ILContext::compile) {
					throw_specific_error(c, "Function type cannot be created in runtime context");

				}
			}

			c.move();
			ILCallingConvention call_conv = ILCallingConvention::bytecode;
			ILContext t_context = ILContext::both;

			while (true)
			{
				if (c.tok == RecognizedToken::Symbol && c.buffer == "compile") {
					t_context = ILContext::compile;
					c.move();
				}
				else if (c.tok == RecognizedToken::Symbol && c.buffer == "runtime") {
					t_context = ILContext::runtime;
					c.move();
				}
				else if (c.tok == RecognizedToken::Symbol && c.buffer == "native") {
					call_conv = ILCallingConvention::native;
					c.move();
				}
				else if (c.tok == RecognizedToken::Symbol && c.buffer == "stdcall") {
					call_conv = ILCallingConvention::stdcall;
					c.move();
				}
				else {
					break;
				}
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
					Expression::parse(compiler, c, arg, CompileType::eval);
					Expression::rvalue(compiler, arg, CompileType::eval);

					if (arg.type != compiler.types()->t_type) {
						throw_specific_error(err, "Expected type");

					}

					auto type = compiler.evaluator()->pop_register_value<Type*>();
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
			Type* ret_type = compiler.types()->t_void;

			if (c.tok == RecognizedToken::Symbol || c.tok == RecognizedToken::And || c.tok == RecognizedToken::DoubleAnd) {
				Cursor err = c;
				CompileValue rt;
				Expression::parse(compiler, c, rt, CompileType::eval);
				Expression::rvalue(compiler, rt, CompileType::eval);

				if (rt.type != compiler.types()->t_type) {
					throw_specific_error(err, "Expected type");

				}
				ret_type = compiler.evaluator()->pop_register_value<Type*>();
			}

			Type* ftype = compiler.types()->load_or_register_function_type(call_conv,std::move(arg_types), ret_type, t_context);

			compiler.evaluator()->write_register_value(ftype);
			ret.lvalue = false;
			ret.type = compiler.types()->t_type;
		}
		else {

			StackItem sitm;

			if (cpt == CompileType::compile && compiler.stack()->find(c.buffer, sitm)) {
				ILBuilder::build_local(compiler.scope(), sitm.id);
				ret.type = sitm.type;
				ret.lvalue = true;
				c.move();
			}
			else if (cpt != CompileType::compile && compiler.compiler_stack()->find(c.buffer, sitm)) {
				ILBuilder::eval_local(compiler.evaluator(), sitm.id);
				ret.type = sitm.type;
				ret.lvalue = true;
				c.move();
			}
			else if (cpt == CompileType::compile) {
				FunctionInstance* f = nullptr;
				Type* t = nullptr;
				ILSize t_s;
				parse_const_type_function(compiler,c, f, t, t_s);

				if (f) {
					ILBuilder::build_fnptr(compiler.scope(), f->func);
					ret.type = f->type;
				}
				else if (t != nullptr) {

					ILBuilder::build_const_type(compiler.scope(), t);
					ret.type = compiler.types()->t_type;
				}
				else {
					ILBuilder::build_const_size(compiler.scope(), t_s);
					ret.type = compiler.types()->t_size;
				}

				ret.lvalue = false;
			}
			else {
				Namespace* namespace_inst = nullptr;
				StructureTemplate* struct_inst = nullptr;
				FunctionTemplate* func_inst = nullptr;
				TraitTemplate* trait_inst = nullptr;


				Cursor err = c;

				compiler.workspace()->find_name(c.buffer, namespace_inst, struct_inst, func_inst, trait_inst);

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

							ILBuilder::eval_const_type(compiler.evaluator(), struct_inst->type.get());
						}
						else if (cpt == CompileType::compile) {
							if (compiler.scope_context() != ILContext::compile) {
								throw_specific_error(err, "Use of a type in runtime context");

							}

							ILBuilder::build_const_type(compiler.scope(), struct_inst->type.get());
						}
					}
					else {
						StructureInstance* inst;
						struct_inst->generate(nullptr, inst);

						if (cpt == CompileType::eval) {
							ILBuilder::eval_const_type(compiler.evaluator(), inst->type.get());
						}
						else if (cpt == CompileType::compile) {
							if (compiler.scope_context() != ILContext::compile) {
								throw_specific_error(err, "Use of a type in runtime context");

							}
							ILBuilder::build_const_type(compiler.scope(), inst->type.get());
						}
					}



					ret.lvalue = false;
					ret.type = compiler.types()->t_type;
				}
				else if (func_inst != nullptr) {

					func_inst->compile();

					if (!func_inst->is_generic) {
						FunctionInstance* inst;
						func_inst->generate(nullptr, inst);
						inst->compile();

						if (cpt == CompileType::eval) {
							ILBuilder::eval_fnptr(compiler.evaluator(), inst->func);
							//ILBuilder::eval_const_type(ctx.eval, inst->type);
						}
						else if (cpt == CompileType::compile) {
							ILBuilder::build_fnptr(compiler.scope(), inst->func);
						}

						ret.lvalue = false;
						ret.type = inst->type;
					}
					else {
						if (cpt == CompileType::eval) {
							ILBuilder::eval_const_type(compiler.evaluator(), func_inst->type.get());
						}
						else if (cpt == CompileType::compile) {
							if (compiler.scope_context() != ILContext::compile) {
								throw_specific_error(err, "Use of a type in runtime context");

							}

							ILBuilder::build_const_type(compiler.scope(), func_inst->type.get());
						}

						ret.lvalue = false;
						ret.type = compiler.types()->t_type;
					}
				}
				else if (trait_inst != nullptr) {
					trait_inst->compile();

					if (trait_inst->is_generic) {
						if (cpt == CompileType::eval) {
							ILBuilder::eval_const_type(compiler.evaluator(), trait_inst->type.get());
						}
						else if (cpt == CompileType::compile) {
							if (compiler.scope_context() != ILContext::compile) {
								throw_specific_error(err, "Use of a type in runtime context");

							}

							ILBuilder::build_const_type(compiler.scope(), trait_inst->type.get());
						}
					}
					else {
						TraitInstance* inst;
						trait_inst->generate(nullptr, inst);

						if (cpt == CompileType::eval) {
							ILBuilder::eval_const_type(compiler.evaluator(), inst->type.get());
						}
						else if (cpt == CompileType::compile) {
							if (compiler.scope_context() != ILContext::compile) {
								throw_specific_error(err, "Use of a type in runtime context");

							}
							ILBuilder::build_const_type(compiler.scope(), inst->type.get());
						}
					}



					ret.lvalue = false;
					ret.type = compiler.types()->t_type;
				}
				else {
					throw_specific_error(nm_err, "Path is pointing to a namespace");

				}

			}

		}


	}




	void Operand::parse_number(Compiler& compiler, CompileValue& ret, Cursor& c, CompileType cpt) {
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
				ILBuilder::build_const_u32(compiler.scope(), (uint32_t)d);
			else
				ILBuilder::build_const_i32(compiler.scope(), (int32_t)d);
		}
		else if (cpt == CompileType::eval) {
			if (usg) {
				ILBuilder::eval_const_u32(compiler.evaluator(), (uint32_t)d);
			}
			else {
				ILBuilder::eval_const_i32(compiler.evaluator(), (int32_t)d);
			}

		}

		ret.type = usg ? compiler.types()->t_u32 : compiler.types()->t_i32;
		ret.lvalue = false;

	}





	void Operand::parse_long_number(Compiler& compiler, CompileValue& ret, Cursor& c, CompileType cpt) {

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
				ILBuilder::build_const_u64(compiler.scope(), d);
			else
				ILBuilder::build_const_i64(compiler.scope(), d);
		}
		else if (cpt == CompileType::eval) {
			if (usg) {
				ILBuilder::eval_const_u64(compiler.evaluator(), d);
			}
			else {
				ILBuilder::eval_const_i64(compiler.evaluator(), d);
			}

		}

		ret.type = usg ? compiler.types()->t_u64 : compiler.types()->t_i64;
		ret.lvalue = false;

	}





	void Operand::parse_float_number(Compiler& compiler, CompileValue& ret, Cursor& c, CompileType cpt) {

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
				ILBuilder::build_const_f64(compiler.scope(), d);
			else
				ILBuilder::build_const_f32(compiler.scope(), (float)d);
		}
		else if (cpt == CompileType::eval) {
			if (dbl) {
				ILBuilder::eval_const_f64(compiler.evaluator(), d);
			}
			else {
				ILBuilder::eval_const_f32(compiler.evaluator(), (float)d);
			}

		}

		ret.type = dbl ? compiler.types()->t_f64 : compiler.types()->t_f32;
		ret.lvalue = false;

	}


	template<typename T, typename S>
	void Operand::parse_generate_template(Compiler& compiler, Cursor& c, T* generating, S*& out) {

		auto layout = generating->generic_ctx.generic_layout.begin();

		if (c.tok != RecognizedToken::CloseParenthesis) {
			while (true) {
				if (layout == generating->generic_ctx.generic_layout.end()) {
					throw_specific_error(c, "Too much arguments");

				}

				CompileValue res;
				Cursor err = c;
				Expression::parse(compiler, c, res, CompileType::eval);
				Operand::cast(compiler,err, res, std::get<1>(*layout), CompileType::eval, false);
				Expression::rvalue(compiler, res, CompileType::eval);

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


		compiler.compiler_stack()->push();
		compiler.evaluator()->stack_push();


		std::vector<std::tuple<Cursor, Type*>>::reverse_iterator act_layout;
		size_t act_layout_size = 0;

		//if (generating->generic_ctx.generator != nullptr) {
		//	generating->generic_ctx.generator->insert_key_on_stack(compiler.evaluator());
		//}

		act_layout = generating->generic_ctx.generic_layout.rbegin();
		act_layout_size = generating->generic_ctx.generic_layout.size();

		unsigned char* key_base = compiler.evaluator()->local_stack_base.back();


		for (size_t arg_i = act_layout_size - 1; arg_i >= 0 && arg_i < act_layout_size; arg_i--) {

			Type* type = std::get<1>(*act_layout);

			uint16_t sid = compiler.evaluator()->push_local(type->size());
			unsigned char* data_place = compiler.evaluator()->stack_ptr(sid);
			compiler.compiler_stack()->push_item(std::get<0>(*act_layout).buffer, type, sid, StackItemTag::regular);


			compiler.evaluator()->write_register_value(data_place);
			Expression::copy_from_rvalue(type, CompileType::eval, false);

			act_layout++;
		}

		generating->generate(key_base, out);

		// ?????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
		// DROP WHOLE STACK

		StackItem sitm;
		while (compiler.compiler_stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) {
			if (sitm.type->has_special_destructor()) {
				sitm.type->drop(compiler.evaluator()->stack_ptr(sitm.id));
			}
		}

		// ?????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????

		compiler.evaluator()->stack_pop();
		compiler.compiler_stack()->pop();

	}

	void Operand::priv_build_push_template(ILEvaluator* eval) {
		Type* t = eval->pop_register_value<Type*>();
		template_stack[template_sp] = t;
		template_sp++;

	}

	void Operand::priv_build_build_template(ILEvaluator* eval) {
		Type* gen_type = template_stack[template_sp - 1];
		gen_type->compile();
		Compiler* compiler = gen_type->compiler();
		compiler->compiler_stack()->push();
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
			compiler->compiler_stack()->push_item(std::get<0>(*act_layout_it).buffer, type, local_id, StackItemTag::regular);


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
		while (compiler->compiler_stack()->pop_item(sitm) && sitm.tag != StackItemTag::alias) {
			if (sitm.type->has_special_destructor()) {
				sitm.type->drop(eval->stack_ptr(sitm.id));
			}
		}
		// ?????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????

		eval->stack_pop();
		compiler->compiler_stack()->pop();

	}

	Type* Operand::template_stack[1024];
	uint16_t Operand::template_sp = 0;


	void Operand::read_arguments(Compiler& compiler, Cursor& c, unsigned int& argi, TypeFunction* ft, CompileType cpt) {
		if (c.tok != RecognizedToken::CloseParenthesis) {
			while (true) {
				if (argi >= compiler.types()->argument_array_storage.get(ft->argument_array_id).size()) {
					throw_specific_error(c, "Wrong number of arguments");
				}

				CompileValue arg;
				Cursor err = c;
				Expression::parse(compiler, c, arg, cpt);
				Operand::cast(compiler,err, arg, compiler.types()->argument_array_storage.get(ft->argument_array_id)[argi], cpt, true);
				Expression::rvalue(compiler, arg, cpt);
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

	void Operand::parse_call_operator(Compiler& compiler, CompileValue& ret, Cursor& c, CompileType cpt) {

		if (ret.type == compiler.types()->t_type || ret.type->type() == TypeInstanceType::type_template) {
			Cursor nm_err = c;
			Expression::rvalue(compiler, ret, cpt);

			if (cpt != CompileType::compile) {
				Type* dt = nullptr;

				dt = compiler.evaluator()->pop_register_value<Type*>();

				if (dt->type() != TypeInstanceType::type_structure_template && dt->type() != TypeInstanceType::type_trait_template && dt->type() != TypeInstanceType::type_function_template) {
					throw_specific_error(c, "this type is not a generic type");

				}
				c.move();

				if (dt->type() == TypeInstanceType::type_structure_template) {
					StructureInstance* inst;
					parse_generate_template(compiler,c, ((TypeStructureTemplate*)dt)->owner, inst);
					ILBuilder::eval_const_type(compiler.evaluator(), inst->type.get());

					ret.lvalue = false;
					ret.type = compiler.types()->t_type;

				}
				else if (dt->type() == TypeInstanceType::type_trait_template) {
					TraitInstance* inst;
					parse_generate_template(compiler,c, ((TypeTraitTemplate*)dt)->owner, inst);
					ILBuilder::eval_const_type(compiler.evaluator(), inst->type.get());

					ret.lvalue = false;
					ret.type = compiler.types()->t_type;
				}
				else if (dt->type() == TypeInstanceType::type_function_template) {
					FunctionInstance* inst;
					parse_generate_template(compiler,c, ((TypeFunctionTemplate*)dt)->owner, inst);
					inst->compile();

					ILBuilder::eval_fnptr(compiler.evaluator(), inst->func);

					ret.lvalue = false;
					ret.type = inst->type;
				}



			}
			else if (ret.type->type() == TypeInstanceType::type_template) {
				Expression::rvalue(compiler, ret, cpt);

				if (cpt == CompileType::compile) {
					ILBuilder::build_const_type(compiler.scope(), ret.type);

					ILBuilder::build_insintric(compiler.scope(), ILInsintric::type_dynamic_cast);
					ILBuilder::build_insintric(compiler.scope(), ILInsintric::push_template);
				}
				else {
					ILBuilder::eval_const_type(compiler.evaluator(), ret.type);
					ILBuilder::eval_insintric(compiler.evaluator(), ILInsintric::type_dynamic_cast);
					ILBuilder::eval_insintric(compiler.evaluator(), ILInsintric::push_template);
				}

				TypeTemplate* tt = (TypeTemplate*)ret.type;
				auto layout = compiler.types()->argument_array_storage.get(tt->argument_array_id).begin();

				if (c.tok != RecognizedToken::OpenParenthesis) {
					throw_wrong_token_error(c, "'('");

				}
				c.move();

				std::vector<CompileValue> results;
				if (c.tok != RecognizedToken::CloseParenthesis) {
					while (true) {
						CompileValue res;
						Cursor err = c;
						Expression::parse(compiler, c, res, cpt);
						Expression::rvalue(compiler, res, cpt);
						Operand::cast(compiler,err, res, *layout, cpt, true);

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
					ILBuilder::build_insintric(compiler.scope(), ILInsintric::build_template);
				}
				else {
					ILBuilder::eval_insintric(compiler.evaluator(), ILInsintric::build_template);
				}
			}
			else {
				throw_specific_error(c, "Operation not supported on plain type, please cast to generic type");
			}

		}
		else if (ret.type->type() == TypeInstanceType::type_function) {
			function_call(compiler,ret, c, cpt, 0);
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

		auto slice_ptr = eval->pop_register_value<size_t*>();
		char* slice_data = (char*)slice_ptr[0];
		size_t slice_size = slice_ptr[1];

		std::string_view slice_str(slice_data, slice_size);

		Type* t = eval->pop_register_value<Type*>();
		Compiler* compiler = t->compiler();
		Type* str = compiler->types()->t_u8->generate_slice();

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

	void Operand::parse_reference(Compiler& compiler,CompileValue& ret, Cursor& c, CompileType cpt) {
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
		Operand::parse(compiler,c, value, cpt);
		Expression::rvalue(compiler, value, cpt);

		if (value.type != compiler.types()->t_type) {
			throw_specific_error(err, "operator expected to recieve type");

		}

		if (cpt == CompileType::eval) {
			for (unsigned int i = 0; i < type_ref_count; i++)
				priv_build_reference(compiler.evaluator());
		}
		else if (cpt == CompileType::compile) {
			throw_specific_error(operr, "Operation not supported in runtime context, please use .reference(...) function");

			/*for (unsigned int i = 0; i < type_ref_count; i++)
				ILBuilder::build_priv(compiler.scope(),2,0);*/
		}

		ret.type = compiler.types()->t_type;
		ret.lvalue = false;


	}

	void Operand::parse_array_operator(Compiler& compiler,CompileValue& ret, Cursor& c, CompileType cpt) {

		while (ret.type->type() != TypeInstanceType::type_slice && (ret.type->type() != TypeInstanceType::type_reference || ((TypeReference*)ret.type)->owner->type() != TypeInstanceType::type_slice)) {

			if (ret.type->type() == TypeInstanceType::type_reference) {
				if (ret.lvalue) {
					if (cpt == CompileType::compile) {
						ILBuilder::build_load(compiler.scope(), ret.type->rvalue());
					}
					else if (cpt == CompileType::eval) {
						ILBuilder::eval_load(compiler.evaluator(), ret.type->rvalue());
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

			Operand::structure_element_offset(compiler,ret, si->pass_array_id, cpt);
		}

		if (ret.type->type() == TypeInstanceType::type_reference && ((TypeReference*)ret.type)->owner->type() == TypeInstanceType::type_slice) {
			if (ret.lvalue) {
				if (cpt == CompileType::compile) {
					ILBuilder::build_load(compiler.scope(), ret.type->rvalue());
				}
				else if (cpt == CompileType::eval) {
					ILBuilder::eval_load(compiler.evaluator(), ret.type->rvalue());
				}
			}

			ret.type = ((TypeReference*)ret.type)->owner;
			ret.lvalue = true;

		}

		if (ret.type->type() != TypeInstanceType::type_slice) {
			throw_specific_error(c, "Offset can be applied only on slices or passed to slice by alias");
		}
		c.move();

		TypeSlice* slice = (TypeSlice*)ret.type;
		Type* base_slice = slice->owner;


		if (cpt == CompileType::compile) {
			ILBuilder::build_load(compiler.scope(), ILDataType::word);
		}
		else {
			ILBuilder::eval_load(compiler.evaluator(), ILDataType::word);
		}

		Cursor err = c;
		CompileValue index;
		Expression::parse(compiler, c, index, cpt);
		Expression::rvalue(compiler, index, cpt);
		Operand::cast(compiler,err, index, compiler.types()->t_size, cpt, true);



		if (cpt == CompileType::compile) {
			ILBuilder::build_const_size(compiler.scope(), base_slice->size());
			ILBuilder::build_mul(compiler.scope(), ILDataType::word, ILDataType::word);
			ILBuilder::build_rtoffset(compiler.scope());
		}
		else {
			ILBuilder::eval_const_size(compiler.evaluator(), base_slice->size().eval(compiler.global_module(), compiler_arch));
			ILBuilder::eval_mul(compiler.evaluator(), ILDataType::word, ILDataType::word);
			ILBuilder::eval_rtoffset(compiler.evaluator());
		}

		if (c.tok != RecognizedToken::CloseBracket) {
			throw_wrong_token_error(c, "']'");

		}
		c.move();

		ret.lvalue = true;
		ret.type = base_slice;


	}

	void Operand::priv_build_array(ILEvaluator* eval) {
		uint32_t val = eval->pop_register_value<uint32_t>();
		Type* t = eval->pop_register_value<Type*>();
		TypeArray* nt = t->generate_array(val);
		ILBuilder::eval_const_type(eval, nt);

	}

	void Operand::parse_array_type(Compiler& compiler,CompileValue& ret, Cursor& c, CompileType cpt) {
		Cursor err = c;
		CompileValue res;
		c.move();

		if (c.tok == RecognizedToken::CloseBracket) {
			c.move();

			Cursor t_err = c;

			Operand::parse(compiler,c, res, cpt);
			Expression::rvalue(compiler, res, cpt);

			if (res.type != compiler.types()->t_type) {
				throw_specific_error(t_err, "Expected type");

			}

			if (cpt == CompileType::eval) {
				Type* t = compiler.evaluator()->pop_register_value<Type*>();
				Type* nt = t->generate_slice();
				ILBuilder::eval_const_type(compiler.evaluator(), nt);
			}
			else {
				throw_specific_error(err, "Operation not supported in runtime context, please use .slice function");

			}
		}
		else {
			Expression::parse(compiler, c, res, cpt);
			Expression::rvalue(compiler, res, cpt);

			Operand::cast(compiler,err, res, compiler.types()->t_u32, cpt, true);


			if (c.tok != RecognizedToken::CloseBracket) {
				throw_wrong_token_error(c, "']'");

			}
			c.move();

			Cursor t_err = c;

			Operand::parse(compiler,c, res, cpt);
			Expression::rvalue(compiler, res, cpt);

			if (res.type != compiler.types()->t_type) {
				throw_specific_error(t_err, "Expected type");

			}

			if (cpt == CompileType::eval) {
				Type* t = compiler.evaluator()->pop_register_value<Type*>();
				uint32_t val = compiler.evaluator()->pop_register_value<uint32_t>();
				TypeArray* nt = t->generate_array(val);
				ILBuilder::eval_const_type(compiler.evaluator(), nt);
			}
			else {
				throw_specific_error(err, "Operation not supported in runtime context, please use .array(...) function");

			}
		}

		ret.type = compiler.types()->t_type;
		ret.lvalue = false;

	}


	void Operand::parse_double_colon_operator(Compiler& compiler,CompileValue& ret, Cursor& c, CompileType cpt) {
		c.move();

		if (cpt == CompileType::compile) {
			throw_specific_error(c, "Operation :: is not supported in runtime context");

		}

		if (ret.type != compiler.types()->t_type) {
			throw_specific_error(c, "left operator is not a type instance");

		}

		if (cpt == CompileType::eval) {
			Expression::rvalue(compiler, ret, cpt);
			TypeStructureInstance* ti = compiler.evaluator()->pop_register_value<TypeStructureInstance*>();
			if (ti->type() != TypeInstanceType::type_structure_instance) {
				throw_specific_error(c, "Type is not structure instance");
			}

			StructureInstance* struct_inst = ti->owner;

			auto f = struct_inst->name_table.find(c.buffer);
			if (f != struct_inst->name_table.end()) {

				if (f->second.first != 1) {
					throw_specific_error(c, "Target is not a structure");
				}

				StructureTemplate* tplt = struct_inst->subtemplates[f->second.second].get();

				compiler.compiler_stack()->push();
				compiler.evaluator()->stack_push();

				/*if (tplt->generic_ctx.generator)
					tplt->generic_ctx.generator->insert_key_on_stack(compiler.evaluator());*/


				tplt->compile();

				ret.lvalue = false;
				if (tplt->is_generic) {
					ILBuilder::eval_const_type(compiler.evaluator(), tplt->type.get());
				}
				else {
					StructureInstance* inst = nullptr;

					tplt->generate(compiler.evaluator()->local_stack_base.back(), inst);
					ILBuilder::eval_const_type(compiler.evaluator(), inst->type.get());
				}

				compiler.evaluator()->stack_pop();
				compiler.compiler_stack()->pop();
			}
			else {
				throw_specific_error(c, "Structure instance does not contain a structure with this name");
			}
		}

		ret.lvalue = false;
		ret.type = compiler.types()->t_type;

		c.move();
		

	}

	void Operand::function_call(Compiler& compiler, CompileValue& ret, Cursor& c, CompileType cpt, unsigned int argi) {
		Expression::rvalue(compiler, ret, cpt);

		c.move();
		TypeFunction* ft = (TypeFunction*)ret.type;

		if (ft->context() != ILContext::both && compiler.scope_context() != ft->context()) {
			throw_specific_error(c, "Cannot call function with different context specifier");
		}

		CompileValue retval;
		retval.type = ft->return_type;
		retval.lvalue = true;
		ft->return_type->compile();

		if (cpt == CompileType::compile) {

			uint16_t local_return_id = 0;


			ILBuilder::build_callstart(compiler.scope());
			if (ft->return_type->rvalue_stacked()) {
				local_return_id = compiler.target()->local_stack_lifetime.append(retval.type->size());
				compiler.temp_stack()->push_item("$tmp", retval.type, local_return_id, StackItemTag::regular);

				if (retval.type->has_special_constructor()) {
					ILBuilder::build_local(compiler.scope(), local_return_id);
					retval.type->build_construct();
				}

				ILBuilder::build_local(compiler.scope(), local_return_id);
			}

			Operand::read_arguments(compiler,c, argi, ft, cpt);

			if (argi != compiler.types()->argument_array_storage.get(ft->argument_array_id).size()) {
				throw_specific_error(c, "Wrong number of arguments");
			}
			c.move();
			ft->compile();

			ILBuilder::build_call(compiler.scope(), ft->il_function_decl);

			if (ft->return_type->rvalue_stacked()) {
				ILBuilder::build_local(compiler.scope(), local_return_id);
			}

		}
		else if (cpt == CompileType::eval) {

			StackItem local_stack_item;

			ILBuilder::eval_callstart(compiler.evaluator());

			if (ft->return_type->rvalue_stacked()) {
				uint16_t sid = compiler.evaluator()->push_local(retval.type->size());
				unsigned char* memory_place = compiler.evaluator()->stack_ptr(sid);

				compiler.compiler_stack()->push_item("$tmp", retval.type, sid, StackItemTag::regular);

				if (retval.type->has_special_constructor()) {
					retval.type->construct(memory_place);
				}

				ILBuilder::eval_local(compiler.evaluator(), sid);
			}

			Operand::read_arguments(compiler,c, argi, ft, cpt);

			if (argi != compiler.types()->argument_array_storage.get(ft->argument_array_id).size()) {
				throw_specific_error(c, "Wrong number of arguments");

			}

			c.move();
			ft->compile();

			ILBuilder::eval_call(compiler.evaluator(), ft->il_function_decl);

			if (ft->return_type->rvalue_stacked()) {
				ILBuilder::eval_local(compiler.evaluator(), local_stack_item.id);
			}

		}

		ret.type = ft->return_type;
		ret.lvalue = false;
	}

	void Operand::structure_element_offset(Compiler& compiler, CompileValue& ret, uint16_t id, CompileType cpt) {
		
		TypeStructureInstance* ti = (TypeStructureInstance*)ret.type;
		ti->compile();

		uint16_t elem_id = 0;
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
						ILBuilder::build_tableoffset(compiler.scope(), elem_size.value, elem_id);
					else if (elem_size.type == ILSizeType::absolute)
						ILBuilder::build_aoffset(compiler.scope(), (uint32_t)elem_size.value);
					else if (elem_size.type == ILSizeType::word)
						ILBuilder::build_woffset(compiler.scope(), (uint32_t)elem_size.value);
				}
			}
			else if (cpt == CompileType::eval) {
				if (!si->wrapper) {
					if (elem_size.type == ILSizeType::table)
						ILBuilder::eval_tableoffset(compiler.evaluator(), elem_size.value, elem_id);
					else if (elem_size.type == ILSizeType::absolute)
						ILBuilder::eval_aoffset(compiler.evaluator(), (uint32_t)elem_size.value);
					else if (elem_size.type == ILSizeType::word)
						ILBuilder::eval_woffset(compiler.evaluator(), (uint32_t)elem_size.value);
				}
			}
		}
		else if (ret.type->rvalue_stacked()) {
			if (cpt == CompileType::compile) {
				if (!si->wrapper) {
					if (elem_size.type == ILSizeType::table)
						ILBuilder::build_tableoffset(compiler.scope(), elem_size.value, elem_id);
					else if (elem_size.type == ILSizeType::absolute)
						ILBuilder::build_aoffset(compiler.scope(), (uint32_t)elem_size.value);
					else if (elem_size.type == ILSizeType::word)
						ILBuilder::build_woffset(compiler.scope(), (uint32_t)elem_size.value);
				}

				if (!mem_type->rvalue_stacked()) {
					ILBuilder::build_load(compiler.scope(), mem_type->rvalue());
				}
			}
			else if (cpt == CompileType::eval) {

				if (!si->wrapper) {
					if (elem_size.type == ILSizeType::table)
						ILBuilder::eval_tableoffset(compiler.evaluator(), elem_size.value, elem_id);
					else if (elem_size.type == ILSizeType::absolute)
						ILBuilder::eval_aoffset(compiler.evaluator(), (uint32_t)elem_size.value);
					else if (elem_size.type == ILSizeType::word)
						ILBuilder::eval_woffset(compiler.evaluator(), (uint32_t)elem_size.value);
				}

				if (!mem_type->rvalue_stacked()) {
					ILBuilder::eval_load(compiler.evaluator(), mem_type->rvalue());
				}
			}
		}
		else {
			if (cpt == CompileType::compile) {
				if (!si->wrapper) {
					if (elem_size.type == ILSizeType::table)
						ILBuilder::build_tableroffset(compiler.scope(), ret.type->rvalue(), mem_type->rvalue(), elem_size.value, elem_id);
					else if (elem_size.type == ILSizeType::absolute)
						ILBuilder::build_aroffset(compiler.scope(), ret.type->rvalue(), mem_type->rvalue(), (uint8_t)elem_size.value);
					else if (elem_size.type == ILSizeType::word)
						ILBuilder::build_wroffset(compiler.scope(), ret.type->rvalue(), mem_type->rvalue(), (uint8_t)elem_size.value);
				}

			}
			else if (cpt == CompileType::eval) {

				if (!si->wrapper) {
					if (elem_size.type == ILSizeType::table)
						ILBuilder::eval_tableroffset(compiler.evaluator(), ret.type->rvalue(), mem_type->rvalue(), elem_size.value, elem_id);
					else if (elem_size.type == ILSizeType::absolute)
						ILBuilder::eval_aroffset(compiler.evaluator(), ret.type->rvalue(), mem_type->rvalue(), (uint8_t)elem_size.value);
					else if (elem_size.type == ILSizeType::word)
						ILBuilder::eval_wroffset(compiler.evaluator(), ret.type->rvalue(), mem_type->rvalue(), (uint8_t)elem_size.value);
				}

			}

		}

		// ret.lvalue stays the same
		ret.type = mem_type;
	}


	void Operand::parse_dot_operator(Compiler& compiler,CompileValue& ret, Cursor& c, CompileType cpt) {
		if (ret.type->type() == TypeInstanceType::type_slice) {
			TypeSlice* ts = (TypeSlice*)ret.type;
			c.move();
			if (c.buffer == "count") {
				if (cpt == CompileType::compile) {
					if (ret.lvalue || ret.type->rvalue_stacked()) {
						ILBuilder::build_woffset(compiler.scope(), 1);
						ILBuilder::build_load(compiler.scope(), ILDataType::word);
					}
					else {
						ILBuilder::build_wroffset(compiler.scope(), ret.type->rvalue(), ILDataType::word, 1);
					}
				}
				else {
					if (ret.lvalue || ret.type->rvalue_stacked()) {
						ILBuilder::eval_woffset(compiler.evaluator(), 1);
						ILBuilder::eval_load(compiler.evaluator(), ILDataType::word);
					}
					else {
						ILBuilder::eval_wroffset(compiler.evaluator(), ret.type->rvalue(), ILDataType::word, 1);
					}
				}

				if (ts->owner->size().type != ILSizeType::absolute || ts->owner->size().value > 1) {
					if (cpt == CompileType::compile) {
						ILBuilder::build_const_size(compiler.scope(), ts->owner->size());
						ILBuilder::build_div(compiler.scope(), ILDataType::word, ILDataType::word);
					}
					else {
						ILBuilder::eval_const_size(compiler.evaluator(), ts->owner->size().eval(compiler.global_module(), compiler_arch));
						ILBuilder::eval_div(compiler.evaluator(), ILDataType::word, ILDataType::word);
					}
				}

				c.move();

				ret.lvalue = false;
				ret.type = compiler.types()->t_size;

			}
			else if (c.buffer == "size") {

				if (cpt == CompileType::compile) {

					if (ret.lvalue) {
						ILBuilder::build_woffset(compiler.scope(), 1);
					}
					else if (ret.type->rvalue_stacked()) {
						ILBuilder::build_woffset(compiler.scope(), 1);
						ILBuilder::build_load(compiler.scope(), ILDataType::word);
					}
					else {
						ILBuilder::build_wroffset(compiler.scope(), ret.type->rvalue(), ILDataType::word, 1);
					}
				}
				else {

					if (ret.lvalue) {
						ILBuilder::eval_woffset(compiler.evaluator(), 1);
					}
					else if (ret.type->rvalue_stacked()) {
						ILBuilder::eval_woffset(compiler.evaluator(), 1);
						ILBuilder::eval_load(compiler.evaluator(), ILDataType::word);
					}
					else {
						ILBuilder::eval_wroffset(compiler.evaluator(), ret.type->rvalue(), ILDataType::word, 1);
					}
				}


				c.move();

				//ret.lvalue is the original: lvalue.x will be lvalue, rvalue.x will be rvalue
				ret.type = compiler.types()->t_size;

			}
			else if (c.buffer == "ptr") {
				Expression::rvalue(compiler, ret, cpt);

				if (cpt == CompileType::compile) {
					if (ret.lvalue) {

					}
					else if (ret.type->rvalue_stacked()) {
						ILBuilder::build_load(compiler.scope(), ILDataType::word);
					}
					else {
						ILBuilder::build_cast(compiler.scope(), ret.type->rvalue(), ILDataType::word);
					}
				}
				else {
					if (ret.lvalue) {

					}
					else if (ret.type->rvalue_stacked()) {
						ILBuilder::eval_load(compiler.evaluator(), ILDataType::word);
					}
					else {
						ILBuilder::build_cast(compiler.scope(), ret.type->rvalue(), ILDataType::word);
					}
				}

				c.move();
				//ret.lvalue is the original: lvalue.x will be lvalue, rvalue.x will be rvalue
				ret.type = compiler.types()->t_ptr;

			}
			else {
				throw_specific_error(c, "Indentifier not recognized as a value of slice");

			}
		}
		else if (ret.type->type() == TypeInstanceType::type_trait) {
			c.move();

			TypeTraitInstance* tti = (TypeTraitInstance*)ret.type;
			TraitInstance* ti = tti->owner;


			auto off_f = ti->member_table.find(c.buffer);
			if (off_f == ti->member_table.end()) {
				throw_specific_error(c, "Trait function not found");

			}
			uint32_t off = (uint32_t)off_f->second;
			auto& mf = ti->member_funcs[off];

			if (mf.ctx != ILContext::both && compiler.scope_context() != mf.ctx) {
				throw_specific_error(c, "Cannot access trait function with different context");

			}

			Expression::rvalue(compiler, ret, cpt);
			c.move();
			if (c.tok == RecognizedToken::OpenParenthesis) {
				if (cpt == CompileType::compile) {
					if (ret.type->rvalue_stacked()) {

						ILBuilder::build_duplicate(compiler.scope(), ILDataType::word);

						ILBuilder::build_woffset(compiler.scope(), 1);
						ILBuilder::build_load(compiler.scope(), ILDataType::word);
						ILBuilder::build_woffset(compiler.scope(), (uint32_t)off);
						ILBuilder::build_load(compiler.scope(), ILDataType::word);
						ILBuilder::build_callstart(compiler.scope());

						ILBuilder::build_load(compiler.scope(), ILDataType::word);
					}
					else {
						throw_specific_error(c, "Compiler error, trait object was placed inside register, but register trait call was not implemented");
					}
				}
				else {
					if (ret.type->rvalue_stacked()) {

						ILBuilder::eval_duplicate(compiler.evaluator(), ILDataType::word);

						ILBuilder::eval_woffset(compiler.evaluator(), 1);
						ILBuilder::eval_load(compiler.evaluator(), ILDataType::word);
						ILBuilder::eval_woffset(compiler.evaluator(), (uint32_t)off);
						ILBuilder::eval_load(compiler.evaluator(), ILDataType::word);
						ILBuilder::eval_callstart(compiler.evaluator());

						ILBuilder::eval_load(compiler.evaluator(), ILDataType::word);
					}
					else {
						throw_specific_error(c, "Compiler error, trait object was placed inside register, but register trait call was not implemented");
					}
				}

				c.move();
				unsigned int argi = 1;
				Operand::read_arguments(compiler,c, argi, mf.type, cpt);
				c.move();

				mf.type->compile();

				if (cpt == CompileType::compile) {
					ILBuilder::build_call(compiler.scope(), mf.type->il_function_decl);
				}
				else {
					ILBuilder::eval_call(compiler.evaluator(), mf.type->il_function_decl);
				}

				ret.lvalue = false;
				ret.type = mf.type->return_type;
			}
			else {
				if (cpt == CompileType::compile) {
					if (ret.type->rvalue_stacked()) {
						ILBuilder::build_woffset(compiler.scope(), 1);
						ILBuilder::build_load(compiler.scope(), ILDataType::word);
						ILBuilder::build_woffset(compiler.scope(), (uint32_t)off);
					}
					else {
						std::cout << "???"; // not invented yet
					}
				}
				else {
					if (ret.type->rvalue_stacked()) {
						ILBuilder::eval_woffset(compiler.evaluator(), 1);
						ILBuilder::eval_load(compiler.evaluator(), ILDataType::word);
						ILBuilder::eval_woffset(compiler.evaluator(), (uint32_t)off);
					}
					else {
						std::cout << "???"; // not invented yet
					}
				}

				ret.lvalue = true;
				ret.type = mf.type;
			}
		}
		else {

			c.move();
			Cursor tok = c;
			c.move();

			bool continue_deeper = true;

			while (continue_deeper) {

				if (ret.type->type() == TypeInstanceType::type_reference) {
					if (ret.lvalue) {
						if (cpt == CompileType::compile) {
							ILBuilder::build_load(compiler.scope(), ret.type->rvalue());
						}
						else if (cpt == CompileType::eval) {
							ILBuilder::eval_load(compiler.evaluator(), ret.type->rvalue());
						}
					}

					ret.type = ((TypeReference*)ret.type)->owner;
					ret.lvalue = true;
				}

				if (ret.type->type() != TypeInstanceType::type_structure_instance) {
					throw_specific_error(tok, "Operator cannot be used on this type");
				}


				TypeStructureInstance* ti = (TypeStructureInstance*)ret.type;
				ti->compile();
				StructureInstance* si = ti->owner;

				auto table_element = si->member_table.find(tok.buffer);
				if (table_element != si->member_table.end()) {
					switch (table_element->second.second)
					{
						case MemberTableEntryType::alias:
							continue_deeper = true;
							structure_element_offset(compiler,ret, table_element->second.first, cpt);
							break;
						case MemberTableEntryType::var:
							continue_deeper = false;
							structure_element_offset(compiler,ret, table_element->second.first, cpt);
							break;
						case MemberTableEntryType::func:

							// rvalues will be stored in temporary memory location
							if (!ret.lvalue && !ret.type->rvalue_stacked()) {
								if (cpt == CompileType::compile) {
									uint32_t local_id = compiler.target()->local_stack_lifetime.append(ret.type->size());
									compiler.temp_stack()->push_item("$tmp", ret.type, local_id, StackItemTag::regular);
									ILBuilder::build_store(compiler.scope(), ret.type->rvalue());
									ILBuilder::build_local(compiler.scope(), local_id);
									ret.lvalue = true;
								}
								else {
									uint32_t local_id = compiler.evaluator()->push_local(ret.type->size());
									compiler.compiler_stack()->push_item("$tmp", ret.type, local_id, StackItemTag::regular);
									ILBuilder::eval_store(compiler.evaluator(), ret.type->rvalue());
									ILBuilder::eval_local(compiler.evaluator(), local_id);
									ret.lvalue = true;
								}
							}

							if (!ret.lvalue && !ret.type->rvalue_stacked()) {
								throw_wrong_token_error(c, "This function can be called only from lvalue object or reference");
							}

							if (c.tok != RecognizedToken::OpenParenthesis) {
								throw_wrong_token_error(c, "'('");
							}

							FunctionInstance* finst;
							si->subfunctions[table_element->second.first]->generate(nullptr, finst);
							finst->compile();

							if (cpt == CompileType::compile) {
								ILBuilder::build_fnptr(compiler.scope(), finst->func);
							}
							else {
								ILBuilder::eval_fnptr(compiler.evaluator(), finst->func);
							}

							ret.type = finst->type;
							ret.lvalue = false;
							function_call(compiler,ret, c, cpt, 1);
							return;
					}
				}
				else {
					throw_specific_error(tok, "Instance does not contain member with this name");
				}
			}
			
		}


	}


	void Operand::parse_string_literal(Compiler& compiler, CompileValue& ret, Cursor& c, CompileType cpt) {
		auto lit = compiler.constant_manager()->register_string_literal(c);

		Type* slice = compiler.types()->t_u8->generate_slice();

		if (cpt == CompileType::compile) {
			uint32_t local_id = compiler.target()->local_stack_lifetime.append(slice->size());
			compiler.temp_stack()->push_item("$tmp", slice, local_id, StackItemTag::regular);
			ILBuilder::build_constref(compiler.scope(), lit.second);
			ILBuilder::build_local(compiler.scope(), local_id);
			ILBuilder::build_store(compiler.scope(), ILDataType::word);
			ILBuilder::build_const_size(compiler.scope(), ILSize(ILSizeType::absolute, (uint32_t)lit.first.length()));
			ILBuilder::build_local(compiler.scope(), local_id);
			ILBuilder::build_woffset(compiler.scope(), 1);
			ILBuilder::build_store(compiler.scope(), ILDataType::word);
			ILBuilder::build_local(compiler.scope(), local_id);
		}
		else {
			uint32_t local_id = compiler.evaluator()->push_local(slice->size());
			compiler.compiler_stack()->push_item("$tmp", slice, local_id, StackItemTag::regular);
			ILBuilder::eval_constref(compiler.evaluator(), lit.second);
			ILBuilder::eval_local(compiler.evaluator(), local_id);
			ILBuilder::eval_store(compiler.evaluator(), ILDataType::word);
			ILBuilder::eval_const_size(compiler.evaluator(), lit.first.length());
			ILBuilder::eval_local(compiler.evaluator(), local_id);
			ILBuilder::eval_woffset(compiler.evaluator(), 1);
			ILBuilder::eval_store(compiler.evaluator(), ILDataType::word);
			ILBuilder::eval_local(compiler.evaluator(), local_id);
		}

		c.move();
		ret.type = slice;
		ret.lvalue = false;
	}
}