#include "Operand.h"
#include "Error.h"
#include "Utilities.h"
#include "PredefinedTypes.h"
#include "Expression.h"
#include "StackManager.h"

#include <iostream>

namespace Corrosive {

	bool Operand::priv_memcpy(ILEvaluator* eval) {
		size_t sz = eval->pop_register_value<size_t>();
		void* dst = eval->pop_register_value<void*>();
		void* src = eval->pop_register_value<void*>();
		memcpy(dst, src, sz);
		return true;
	}
	bool Operand::priv_memcpy2(ILEvaluator* eval) {
		size_t sz = eval->pop_register_value<size_t>();
		void* src = eval->pop_register_value<void*>();
		void* dst = eval->pop_register_value<void*>();
		memcpy(dst, src, sz);
		return true;
	}

	bool Operand::priv_malloc(ILEvaluator* eval) {
		size_t sz = eval->pop_register_value<size_t>();
		size_t* dst = eval->pop_register_value<size_t*>();
		size_t ptr = (size_t)malloc(sz);
		*dst++ = ptr;
		*dst = sz;
		return true;
	}

	bool Operand::priv_type_template_cast_crsr(ILEvaluator* eval, Cursor& err) {
		CompileContext& nctx = CompileContext::get();
		Type* template_cast = eval->pop_register_value<Type*>();
		Type* template_type = eval->read_register_value<Type*>();

		if (template_type->type() == TypeInstanceType::type_structure_template) {
			TypeStructureTemplate* st = (TypeStructureTemplate*)template_type;
			TypeTemplate* tt = (TypeTemplate*)template_cast;

			if (st->owner->generic_layout.size() != nctx.default_types->argument_array_storage.get(tt->argument_array_id).size()) {
				throw_cannot_cast_error(err, template_type, template_cast);
				return false;
			}
			else {
				for (size_t i = 0; i < st->owner->generic_layout.size(); i++) {
					if (std::get<1>(st->owner->generic_layout[i]) != nctx.default_types->argument_array_storage.get(tt->argument_array_id)[i]) {
						throw_cannot_cast_error(err, template_type, template_cast);
						return false;
					}
				}
			}
		}
		else if (template_type->type() == TypeInstanceType::type_trait_template) {
			TypeTraitTemplate* st = (TypeTraitTemplate*)template_type;
			TypeTemplate* tt = (TypeTemplate*)template_cast;

			if (st->owner->generic_layout.size() != nctx.default_types->argument_array_storage.get(tt->argument_array_id).size()) {
				throw_cannot_cast_error(err, template_type, template_cast);
				return false;
			}
			else {
				for (size_t i = 0; i < st->owner->generic_layout.size(); i++) {
					if (std::get<1>(st->owner->generic_layout[i]) != nctx.default_types->argument_array_storage.get(tt->argument_array_id)[i]) {
						throw_cannot_cast_error(err, template_type, template_cast);
						return false;
					}
				}
			}
		}
		else {
			throw_cannot_cast_error(err, template_type, template_cast);
			return false;
		}

		return true;
	}

	bool Operand::priv_type_template_cast(ILEvaluator* eval) {
		CompileContext& nctx = CompileContext::get();
		return priv_type_template_cast_crsr(eval, nctx.default_types->debug_info);
	}

	bool _crs_is_numeric_value(Type* t) {
		CompileContext& nctx = CompileContext::get();
		return !t->rvalue_stacked() && t->rvalue() <= ILDataType::f64;
	}

	bool Operand::cast(Cursor& err, CompileValue& res, Type*& to, CompileType cpt, bool implicit) {
		CompileContext& nctx = CompileContext::get();

		if (res.t == nctx.default_types->t_type && to->type() == TypeInstanceType::type_template) {
			if (cpt == CompileType::eval) {
				ILBuilder::eval_const_type(nctx.eval, to);
				priv_type_template_cast_crsr(nctx.eval, err);

				res.t = to;
				return true;
			}
			else {
				ILBuilder::build_const_type(nctx.scope, to);

				uint64_t dbg_c = nctx.default_types->load_or_register_debug_cursor(err);
				ILBuilder::build_const_u64(nctx.scope, dbg_c);
				ILBuilder::build_insintric(nctx.scope, ILInsintric::debug_cursor);
				ILBuilder::build_insintric(nctx.scope, ILInsintric::template_cast);

				res.t = to;
				return true;
			}
		}
		else if (to == nctx.default_types->t_type && res.t->type() == TypeInstanceType::type_template) {

			res.t = to;
			return true;
		}
		else if (_crs_is_numeric_value(res.t) && _crs_is_numeric_value(to)) {
			if (cpt == CompileType::eval) {
				if (res.t->rvalue() != to->rvalue())
					ILBuilder::eval_cast(nctx.eval, res.t->rvalue(), to->rvalue());

				res.t = to;
				return true;
			}
			else {
				if (res.t->rvalue() != to->rvalue())
					ILBuilder::build_cast(nctx.scope, res.t->rvalue(), to->rvalue());

				res.t = to;
				return true;
			}
		}
		else if ((res.t->type() == TypeInstanceType::type_reference || (res.t->type() == TypeInstanceType::type_structure_instance && res.lvalue)) && to->type() == TypeInstanceType::type_trait) {
			
			TypeTraitInstance* tt = (TypeTraitInstance*)to;

			TypeStructureInstance* ts = nullptr;
			if (res.t->type() == TypeInstanceType::type_reference) {
				TypeReference* tr = (TypeReference*)res.t;
				if (tr->owner->type() == TypeInstanceType::type_structure_instance) {
					ts = (TypeStructureInstance*)tr->owner;
				}
				else {
					throw_cannot_cast_error(err, res.t, to);
					return false;
				}
			}
			else {
				ts = (TypeStructureInstance*)res.t;
			}

			
			if (!ts->compile()) return false;

			auto tti = ts->owner->traitfunctions.find(tt->owner);
			if (tti == ts->owner->traitfunctions.end()) {
				throw_cannot_cast_error(err, res.t, to);
				std::cerr << " |\trequested trait is not implemented in the structure\n";
				return false;
			}

			if (!tt->compile()) return false;

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
					CompileValue val;
					val.t = to;
					val.lvalue = false;
					unsigned char* memory_place = nctx.eval->stack_reserve(to->size().eval(compiler_arch));
					StackItem local_stack_item = StackManager::stack_push<1>(nctx.eval, "$tmp", val, 0);

					ILBuilder::eval_local(nctx.eval, local_stack_item.local_offset.eval(compiler_arch));
					ILBuilder::eval_store(nctx.eval, ILDataType::ptr);

					ILBuilder::eval_vtable(nctx.eval, vtableid);
					ILBuilder::eval_local(nctx.eval, (local_stack_item.local_offset + ILSize::single_ptr).eval(compiler_arch));
					ILBuilder::eval_store(nctx.eval, ILDataType::ptr);

					ILBuilder::eval_local(nctx.eval, local_stack_item.local_offset.eval(compiler_arch));
				}
				else {
					ILBuilder::eval_vtable(nctx.eval, vtableid); // there is two pointers on register stack, if i read 2x pointer size register it will read the right value
				}

			}
			else {
				if (to->rvalue_stacked()) {
					CompileValue val;
					val.t = to;
					val.lvalue = false;
					uint32_t local_id = nctx.function->register_local(to->size());
					StackItem local_stack_item = StackManager::stack_push<0>(nctx.eval, "$tmp", val, local_id);
					
					ILBuilder::build_local(nctx.scope, local_id);
					ILBuilder::build_store(nctx.scope, ILDataType::ptr);

					ILBuilder::build_vtable(nctx.scope, vtableid);
					ILBuilder::build_local(nctx.scope, local_id);
					ILBuilder::build_offset(nctx.scope, ILSize::single_ptr);
					ILBuilder::build_store(nctx.scope, ILDataType::ptr);

					ILBuilder::build_local(nctx.scope, local_id);
				}
				else {
					std::cout << "???"; // two pointers require 128 int on x64, not yet implemented
				}
			}

			res.t = to;
			return true;
			
		}
		else if (res.t->type() == TypeInstanceType::type_reference && to->type() == TypeInstanceType::type_reference) {
			if (res.t != to && implicit) {
				throw_cannot_implicit_cast_error(err, res.t, to);
				return false;
			}

			res.t = to;
			return true;
		}
		else if (res.t->type() == TypeInstanceType::type_structure_instance && to->type() == TypeInstanceType::type_reference) {
			TypeReference* tr = (TypeReference*)to;
			if (tr->owner != res.t) {
				throw_cannot_cast_error(err, res.t, to);
				return false;
			}

			if (!res.lvalue) {
				throw_cannot_cast_error(err, res.t, to);
				std::cerr << " |\tType was not lvalue\n";
				return false;
			}

			res.t = to;
			res.lvalue = false;
			return true;
		}
		else if (res.t != to) {
			throw_cannot_cast_error(err, res.t, to);
			return false;
		}


		res.t = to;
		return true;
	}

	bool Operand::parse(Cursor& c, CompileValue& res, CompileType cpt) {
		CompileValue ret;

		CompileContext& nctx = CompileContext::get();

		ret.lvalue = false;
		ret.t = nullptr;

		switch (c.tok) {
			case RecognizedToken::And:
			case RecognizedToken::DoubleAnd: {
				return parse_reference(res, c, cpt);
			}
			case RecognizedToken::OpenBracket: {
				return parse_array_type(res, c, cpt);
			}
			case RecognizedToken::Star: {
				c.move();
				Cursor err = c;
				CompileValue value;
				if (!Operand::parse(c, value, cpt)) return false;
				if (!Expression::rvalue(value, cpt)) return false;
				if (value.t->type() != TypeInstanceType::type_reference) {
					throw_specific_error(err, "Target is not reference, and cannot be dereferenced");
					return false;
				}

				res.lvalue = true;
				res.t = ((TypeReference*)value.t)->owner;
				return true;
			}
			case RecognizedToken::OpenParenthesis: {
				if (!parse_expression(ret, c, cpt)) return false;
			}break;

			case RecognizedToken::Symbol: {
				if (c.buffer == "cast") {
					c.move();
					if (c.tok != RecognizedToken::OpenParenthesis) {
						throw_wrong_token_error(c, "'('");
						return false;
					}
					c.move();

					Cursor err = c;

					CompileValue t_res;
					if (!Expression::parse(c, t_res, CompileType::eval)) return false;
					if (!Expression::rvalue(t_res, CompileType::eval)) return false;

					if (t_res.t != nctx.default_types->t_type) {
						throw_specific_error(err, "Expected type");
						return false;
					}
					Type* to = nctx.eval->pop_register_value<Type*>();

					if (c.tok != RecognizedToken::CloseParenthesis) {
						throw_wrong_token_error(c, "')'");
						return false;
					}
					c.move();

					err = c;
					CompileValue value;
					if (!Operand::parse(c, value, cpt)) return false;
					if (!Expression::rvalue(value, cpt)) return false;

					if (!Operand::cast(err, value, to, cpt, false)) return false;

					res = value;
					return true;
				}
				else {
					if (!parse_symbol(ret, c, cpt)) return false;
				}
			}break;

			case RecognizedToken::Number:
			case RecognizedToken::UnsignedNumber: {
				if (!parse_number(ret, c, cpt)) return false;
			}break;

			case RecognizedToken::LongNumber:
			case RecognizedToken::UnsignedLongNumber: {
				if (!parse_long_number(ret, c, cpt)) return false;
			}break;

			case RecognizedToken::FloatNumber:
			case RecognizedToken::DoubleNumber: {
				if (!parse_float_number(ret, c, cpt)) return false;
			}break;

			default: {
				throw_specific_error(c, "Expected to parse operand");
				return false;
			} break;
		}

		while (true) {
			switch (c.tok) {
				case RecognizedToken::Symbol: {
					if (c.buffer == "cast") {
						if (!Expression::rvalue(ret, cpt)) return false;

						c.move();
						if (c.tok != RecognizedToken::OpenParenthesis) {
							throw_wrong_token_error(c, "'('");
							return false;
						}
						c.move();

						Cursor err = c;

						CompileValue t_res;
						if (!Expression::parse(c, t_res, CompileType::eval)) return false;
						if (!Expression::rvalue(t_res, CompileType::eval)) return false;

						if (t_res.t != nctx.default_types->t_type) {
							throw_specific_error(err, "Expected type");
							return false;
						}
						Type* to = nctx.eval->pop_register_value<Type*>();

						if (c.tok != RecognizedToken::CloseParenthesis) {
							throw_wrong_token_error(c, "')'");
							return false;
						}
						c.move();

						if (!Operand::cast(err, ret, to, cpt, false)) return false;
					}
					else goto break_while;
				}break;
				case RecognizedToken::OpenParenthesis: {
					if (!parse_call_operator(ret, c, cpt)) return false;
				}break;
				case RecognizedToken::OpenBracket: {
					if (!parse_array_operator(ret, c, cpt)) return false;
				}break;
				case RecognizedToken::Dot: {
					if (!parse_dot_operator(ret, c, cpt)) return false;
				}break;
				case RecognizedToken::DoubleColon: {
					if (!parse_double_colon_operator(ret, c, cpt)) return false;
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




	bool Operand::parse_const_type_function(Cursor& c, FunctionInstance*& func, Type*& type) {
		Namespace* namespace_inst = nullptr;
		StructureTemplate* struct_inst = nullptr;
		FunctionTemplate* func_inst = nullptr;
		TraitTemplate* trait_inst = nullptr;
		
		CompileContext new_ctx = CompileContext::get();
		new_ctx.scope_context = ILContext::compile;
		CompileContext::push(new_ctx);
		CompileContext& nctx = CompileContext::get();

		


		Cursor err = c;
		nctx.inside->find_name(c.buffer, namespace_inst, struct_inst, func_inst, trait_inst);

		if (namespace_inst == nullptr && struct_inst == nullptr && func_inst == nullptr && trait_inst == nullptr) {
			throw_specific_error(c, "Path start point not found");
			return false;
		}

		Cursor nm_err = c;
		c.move();
		while (c.tok == RecognizedToken::DoubleColon && namespace_inst != nullptr) {
			c.move();
			if (c.tok != RecognizedToken::Symbol)
			{
				throw_not_a_name_error(c);
				return false;
			}
			nm_err = c;

			namespace_inst->find_name(c.buffer, namespace_inst, struct_inst, func_inst, trait_inst);
			c.move();
		}


		while (struct_inst) {
			if (!struct_inst->compile()) return false;

			StructureInstance* inst;
			if (struct_inst->is_generic) {
				if (c.tok != RecognizedToken::OpenParenthesis) {
					type = struct_inst->type.get();

					CompileContext::pop();
					return true;
				}
				c.move();

				if (!parse_generate_template(c, struct_inst, inst)) return false;
			}
			else {
				if (!struct_inst->generate(nullptr, inst)) return false;
			}

			if (c.tok != RecognizedToken::DoubleColon) {

				CompileContext::pop();
				type = inst->type.get();
				return true;
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
					return false;
				}
			}
		}


		if (func_inst != nullptr) {

			if (!func_inst->compile()) return false;

			if (!func_inst->is_generic) {
				FunctionInstance* inst;
				if (!func_inst->generate(nullptr, inst)) return false;
				if (!inst->compile()) return false;

				func = inst;

				CompileContext::pop();
				return true;
			}
			else {
				FunctionInstance* inst;
				if (c.tok != RecognizedToken::OpenParenthesis) {
					type = func_inst->type.get();

					CompileContext::pop();
					return true;
				}
				c.move();

				if (!parse_generate_template(c, func_inst, inst)) return false;
				if (!inst->compile()) return false;

				CompileContext::pop();
				func = inst;
				return true;
			}

		}
		else {
			throw_specific_error(nm_err, "Only function may be brought in the runtime context");
			return false;
		}

		CompileContext::pop();
		return true;
	}



	bool Operand::parse_expression(CompileValue& ret, Cursor& c, CompileType cpt) {
		c.move();
		if (!Expression::parse(c, ret, cpt)) return false;
		if (!Expression::rvalue(ret, cpt)) return false;

		if (c.tok != RecognizedToken::CloseParenthesis) {
			throw_wrong_token_error(c, "')'");
			return false;
		}
		c.move();

		return true;
	}



	bool Operand::parse_symbol(CompileValue& ret, Cursor& c, CompileType cpt) {
		CompileContext& nctx = CompileContext::get();
		if (c.buffer == "true") {
			c.move();
			ret.lvalue = false;
			ret.t = nctx.default_types->t_bool;
			if (cpt == CompileType::compile) {
				if (!ILBuilder::build_const_ibool(nctx.scope, true)) return false;
			}
			else if (cpt == CompileType::eval) {
				if (!ILBuilder::eval_const_ibool(nctx.eval, true)) return false;
			}
		}
		else if (c.buffer == "false") {
			c.move();
			ret.lvalue = false;
			ret.t = nctx.default_types->t_bool;
			if (cpt == CompileType::compile) {
				if (!ILBuilder::build_const_ibool(nctx.scope, false)) return false;
			}
			else if (cpt == CompileType::eval) {
				if (!ILBuilder::eval_const_ibool(nctx.eval, false)) return false;
			}
		}
		else if (c.buffer == "type") {

			c.move();
			if (c.tok != RecognizedToken::OpenParenthesis) {
				nctx.eval->write_register_value(nctx.default_types->t_type);
			}
			else {
				c.move();
				std::vector<Type*> arg_types;

				if (c.tok != RecognizedToken::CloseParenthesis) {
					while (true) {
						Cursor err = c;
						CompileValue arg;
						if (!Expression::parse(c, arg, CompileType::eval)) return false;
						if (!Expression::rvalue(arg, CompileType::eval)) return false;

						if (arg.t != nctx.default_types->t_type) {
							throw_specific_error(err, "Expected type");
							return false;
						}
						auto type = nctx.eval->pop_register_value<Type*>();
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


				Type* ftype = nctx.default_types->load_or_register_template_type(std::move(arg_types));

				nctx.eval->write_register_value(ftype);
			}

			ret.lvalue = false;
			ret.t = nctx.default_types->t_type;
		}
		else if (c.buffer == "fn") {
			if (cpt == CompileType::compile) {
				if (nctx.scope_context != ILContext::compile) {
					throw_specific_error(c, "Function type cannot be created in runtime context");
					return false;
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
				return false;
			}
			c.move();
			std::vector<Type*> arg_types;

			if (c.tok != RecognizedToken::CloseParenthesis) {
				while (true) {
					Cursor err = c;
					CompileValue arg;
					if (!Expression::parse(c, arg, CompileType::eval)) return false;
					if (!Expression::rvalue(arg, CompileType::eval)) return false;

					if (arg.t != nctx.default_types->t_type) {
						throw_specific_error(err, "Expected type");
						return false;
					}
					auto type = nctx.eval->pop_register_value<Type*>();
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
			Type* ret_type = nctx.default_types->t_void;

			if (c.tok == RecognizedToken::Symbol || c.tok == RecognizedToken::And || c.tok == RecognizedToken::DoubleAnd) {
				Cursor err = c;
				CompileValue rt;
				if (!Expression::parse(c, rt, CompileType::eval)) return false;
				if (!Expression::rvalue(rt, CompileType::eval)) return false;

				if (rt.t != nctx.default_types->t_type) {
					throw_specific_error(err, "Expected type");
					return false;
				}
				ret_type = nctx.eval->pop_register_value<Type*>();
			}

			Type* ftype = nctx.default_types->load_or_register_function_type(std::move(arg_types), ret_type, t_context);

			nctx.eval->write_register_value(ftype);
			ret.lvalue = false;
			ret.t = nctx.default_types->t_type;
		}
		else {

			StackItem* sitm;

			if (cpt == CompileType::compile && (sitm = StackManager::stack_find<0>(c.buffer))) {
				ILBuilder::build_local(nctx.scope, sitm->id);

				ret = sitm->value;
				ret.lvalue = true;

				c.move();
			}
			else if (cpt != CompileType::compile && (sitm = StackManager::stack_find<1>(c.buffer))) {

				ILBuilder::eval_local(nctx.eval, sitm->local_offset.eval(compiler_arch));

				ret = sitm->value;
				ret.lvalue = true;
				c.move();
			}
			else if (cpt == CompileType::compile) {
				FunctionInstance* f = nullptr;
				Type* t = nullptr;
				if (!parse_const_type_function(c, f, t)) return false;
				if (f) {
					ILBuilder::build_fnptr(nctx.scope, f->func);
					ret.t = f->type;
				}
				else {
					ILBuilder::build_const_type(nctx.scope, t);
					ret.t = nctx.default_types->t_type;
				}

				ret.lvalue = false;
			}
			else {
				Namespace* namespace_inst = nullptr;
				StructureTemplate* struct_inst = nullptr;
				FunctionTemplate* func_inst = nullptr;
				TraitTemplate* trait_inst = nullptr;


				Cursor err = c;

				nctx.inside->find_name(c.buffer, namespace_inst, struct_inst, func_inst, trait_inst);

				if (namespace_inst == nullptr && struct_inst == nullptr && func_inst == nullptr && trait_inst == nullptr) {
					throw_specific_error(c, "Path start point not found");
					return false;
				}


				Cursor nm_err = c;
				c.move();
				while (c.tok == RecognizedToken::DoubleColon && namespace_inst != nullptr) {
					c.move();
					if (c.tok != RecognizedToken::Symbol)
					{
						throw_not_a_name_error(c);
						return false;
					}
					nm_err = c;

					namespace_inst->find_name(c.buffer, namespace_inst, struct_inst, func_inst, trait_inst);
					c.move();
				}


				if (struct_inst) {
					if (!struct_inst->compile()) return false;

					if (struct_inst->is_generic) {
						if (cpt == CompileType::eval) {

							if (!ILBuilder::eval_const_type(nctx.eval, struct_inst->type.get())) return false;
						}
						else if (cpt == CompileType::compile) {
							if (nctx.scope_context != ILContext::compile) {
								throw_specific_error(err, "Use of a type in runtime context");
								return false;
							}

							if (!ILBuilder::build_const_type(nctx.scope, struct_inst->type.get())) return false;
						}
					}
					else {
						StructureInstance* inst;
						if (!struct_inst->generate(nullptr, inst)) return false;

						if (cpt == CompileType::eval) {
							if (!ILBuilder::eval_const_type(nctx.eval, inst->type.get())) return false;
						}
						else if (cpt == CompileType::compile) {
							if (nctx.scope_context != ILContext::compile) {
								throw_specific_error(err, "Use of a type in runtime context");
								return false;
							}
							if (!ILBuilder::build_const_type(nctx.scope, inst->type.get())) return false;
						}
					}



					ret.lvalue = false;
					ret.t = nctx.default_types->t_type;
				}
				else if (func_inst != nullptr) {

					if (!func_inst->compile()) return false;

					if (!func_inst->is_generic) {
						FunctionInstance* inst;
						if (!func_inst->generate(nullptr, inst)) return false;
						if (!inst->compile()) return false;

						if (cpt == CompileType::eval) {
							if (!ILBuilder::eval_fnptr(nctx.eval, inst->func)) return false;
							//if (!ILBuilder::eval_const_type(ctx.eval, inst->type)) return false;
						}
						else if (cpt == CompileType::compile) {
							if (!ILBuilder::build_fnptr(nctx.scope, inst->func)) return false;
						}

						ret.lvalue = false;
						ret.t = inst->type;
					}
					else {
						if (cpt == CompileType::eval) {
							if (!ILBuilder::eval_const_type(nctx.eval, func_inst->type.get())) return false;
						}
						else if (cpt == CompileType::compile) {
							if (nctx.scope_context != ILContext::compile) {
								throw_specific_error(err, "Use of a type in runtime context");
								return false;
							}

							if (!ILBuilder::build_const_type(nctx.scope, func_inst->type.get())) return false;
						}

						ret.lvalue = false;
						ret.t = nctx.default_types->t_type;
					}
				}
				else if (trait_inst != nullptr) {
					if (!trait_inst->compile()) return false;

					if (trait_inst->is_generic) {
						if (cpt == CompileType::eval) {
							if (!ILBuilder::eval_const_type(nctx.eval, trait_inst->type.get())) return false;
						}
						else if (cpt == CompileType::compile) {
							if (nctx.scope_context != ILContext::compile) {
								throw_specific_error(err, "Use of a type in runtime context");
								return false;
							}

							if (!ILBuilder::build_const_type(nctx.scope, trait_inst->type.get())) return false;
						}
					}
					else {
						TraitInstance* inst;
						if (!trait_inst->generate(nullptr, inst)) return false;

						if (cpt == CompileType::eval) {
							if (!ILBuilder::eval_const_type(nctx.eval, inst->type.get())) return false;
						}
						else if (cpt == CompileType::compile) {
							if (nctx.scope_context != ILContext::compile) {
								throw_specific_error(err, "Use of a type in runtime context");
								return false;
							}
							if (!ILBuilder::build_const_type(nctx.scope, inst->type.get())) return false;
						}
					}



					ret.lvalue = false;
					ret.t = nctx.default_types->t_type;
				}
				else {
					throw_specific_error(nm_err, "Path is pointing to a namespace");
					return false;
				}

			}

		}

		return true;
	}




	bool Operand::parse_number(CompileValue& ret, Cursor& c, CompileType cpt) {
		bool usg = c.tok == RecognizedToken::UnsignedNumber;

		CompileContext& nctx = CompileContext::get();

		std::string_view ndata;
		if (usg)
			ndata = c.buffer.substr(0, c.buffer.size() - 1);
		else
			ndata = c.buffer;

		unsigned long long d = svtoi(ndata);
		c.move();

		if (cpt == CompileType::compile) {
			if (usg)
				ILBuilder::build_const_u32(nctx.scope, (uint32_t)d);
			else
				ILBuilder::build_const_i32(nctx.scope, (int32_t)d);
		}
		else if (cpt == CompileType::eval) {
			if (usg)
				ILBuilder::eval_const_u32(nctx.eval, (uint32_t)d);
			else
				ILBuilder::eval_const_i32(nctx.eval, (int32_t)d);

		}

		ret.t = usg ? nctx.default_types->t_u32 : nctx.default_types->t_i32;
		ret.lvalue = false;
		return true;
	}





	bool Operand::parse_long_number(CompileValue& ret, Cursor& c, CompileType cpt) {

		CompileContext& nctx = CompileContext::get();
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
				ILBuilder::build_const_u64(nctx.scope, d);
			else
				ILBuilder::build_const_i64(nctx.scope, d);
		}
		else if (cpt == CompileType::eval) {
			if (usg)
				ILBuilder::eval_const_u64(nctx.eval, d);
			else
				ILBuilder::eval_const_i64(nctx.eval, d);

		}

		ret.t = usg ? nctx.default_types->t_u64 : nctx.default_types->t_i64;
		ret.lvalue = false;
		return true;
	}





	bool Operand::parse_float_number(CompileValue& ret, Cursor& c, CompileType cpt) {

		CompileContext& nctx = CompileContext::get();
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
				ILBuilder::build_const_f64(nctx.scope, d);
			else
				ILBuilder::build_const_f32(nctx.scope, (float)d);
		}
		else if (cpt == CompileType::eval) {
			if (dbl)
				ILBuilder::eval_const_f64(nctx.eval, d);
			else
				ILBuilder::eval_const_f32(nctx.eval, (float)d);

		}

		ret.t = dbl ? nctx.default_types->t_f64 : nctx.default_types->t_f32;
		ret.lvalue = false;
		return true;
	}


	template<typename T, typename S>
	bool Operand::parse_generate_template(Cursor& c, T* generating, S*& out) {

		CompileContext& nctx = CompileContext::get();

		auto layout = generating->generic_layout.begin();

		if (c.tok != RecognizedToken::CloseParenthesis) {
			while (true) {
				if (layout == generating->generic_layout.end()) {
					throw_specific_error(c, "Too much arguments");
					return false;
				}

				CompileValue res;
				Cursor err = c;
				if (!Expression::parse(c, res, CompileType::eval)) return false;
				if (!Expression::rvalue(res, CompileType::eval)) return false;

				if (!Operand::cast(err, res, std::get<1>(*layout), CompileType::eval,false)) return false;

				layout++;

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

		if (layout != generating->generic_layout.end()) {
			throw_specific_error(c, "Not enough arguments");
			return false;
		}

		c.move();


		auto ss = std::move(StackManager::move_stack_out<1>());
		auto sp = nctx.eval->stack_push();


		std::vector<std::tuple<Cursor, Type*>>::reverse_iterator act_layout;
		size_t act_layout_size = 0;

		if (generating->template_parent != nullptr) {
			generating->template_parent->insert_key_on_stack(nctx.eval);
		}

		act_layout = generating->generic_layout.rbegin();
		act_layout_size = generating->generic_layout.size();

		unsigned char* key_base = nctx.eval->memory_stack_pointer;


		for (size_t arg_i = act_layout_size - 1; arg_i >= 0 && arg_i < act_layout_size; arg_i--) {

			CompileValue res;
			res.t = std::get<1>(*act_layout);
			res.lvalue = true;

			unsigned char* data_place = nctx.eval->stack_reserve(res.t->size().eval(compiler_arch));
			StackManager::stack_push<1>(nctx.eval, std::get<0>(*act_layout).buffer, res, 0);

			bool stacked = res.t->rvalue_stacked();

			if (stacked) {
				unsigned char* src = nctx.eval->pop_register_value<unsigned char*>();
				if (res.t->has_special_constructor()) {
					res.t->construct(nctx.eval, data_place);
				}
				res.t->copy(nctx.eval, data_place, src);
			}
			else {
				void* src = nctx.eval->read_last_register_value_indirect(res.t->rvalue());
				memcpy(data_place, src, res.t->size().eval(compiler_arch));
				nctx.eval->discard_last_register_type(res.t->rvalue());
			}


			act_layout++;
		}

		if (!generating->generate(key_base, out)) return false;

		// ?????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
		// DROP WHOLE STACK

		while (StackManager::stack_state<1>() > 0) {
			StackItem sitm = StackManager::stack_pop<1>(nctx.eval);
			CompileValue res = sitm.value;

			if (res.t->has_special_destructor()) {
				res.t->drop(nctx.eval, nctx.eval->memory_stack_base_pointer + sitm.local_offset.eval(compiler_arch));
			}
		}

		// ?????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????

		nctx.eval->stack_pop(sp);
		StackManager::move_stack_in<1>(std::move(ss));
		return true;
	}

	bool Operand::priv_build_push_template(ILEvaluator* eval) {
		Type* t = eval->pop_register_value<Type*>();
		template_stack[template_sp] = t;
		template_sp++;
		return true;
	}

	bool Operand::priv_build_build_template(ILEvaluator* eval) {
		Type* gen_type = template_stack[template_sp - 1];

		auto ss = std::move(StackManager::move_stack_out<1>());
		auto sp = eval->stack_push();


		std::vector<std::tuple<Cursor, Type*>>::reverse_iterator act_layout;
		std::vector<std::tuple<Cursor, Type*>>::reverse_iterator act_layout_it;
		size_t gen_types = 0;

		if (gen_type->type() == TypeInstanceType::type_structure_template) {
			if (((TypeStructureTemplate*)gen_type)->owner->template_parent != nullptr) {
				((TypeStructureTemplate*)gen_type)->owner->template_parent->insert_key_on_stack(eval);
			}
			act_layout = ((TypeStructureTemplate*)gen_type)->owner->generic_layout.rbegin();
			gen_types = ((TypeStructureTemplate*)gen_type)->owner->generic_layout.size();
		}
		else if (gen_type->type() == TypeInstanceType::type_trait_template) {
			if (((TypeTraitTemplate*)gen_type)->owner->template_parent != nullptr) {
				((TypeTraitTemplate*)gen_type)->owner->template_parent->insert_key_on_stack(eval);
			}
			act_layout = ((TypeTraitTemplate*)gen_type)->owner->generic_layout.rbegin();
			gen_types = ((TypeTraitTemplate*)gen_type)->owner->generic_layout.size();
		}


		unsigned char* key_base = eval->memory_stack_pointer;


		act_layout_it = act_layout;
		for (size_t arg_i = gen_types - 1; arg_i >= 0 && arg_i < gen_types; arg_i--) {

			CompileValue res;
			res.t = std::get<1>((*act_layout_it));
			res.lvalue = true;

			unsigned char* data_place = eval->stack_reserve(res.t->size().eval(compiler_arch));
			StackManager::stack_push<1>(eval, std::get<0>(*act_layout_it).buffer, res, 0);

			bool stacked = res.t->rvalue_stacked();
			if (stacked) {
				unsigned char* src = eval->pop_register_value<unsigned char*>();
				if (res.t->has_special_constructor())
					res.t->construct(eval, data_place);
				res.t->copy(eval, data_place, src);
			}
			else {
				void* src = eval->read_last_register_value_indirect(res.t->rvalue());
				memcpy(data_place, src, res.t->size().eval(compiler_arch));
				eval->discard_last_register_type(res.t->rvalue());
			}


			act_layout_it++;
		}


		if (gen_type->type() == TypeInstanceType::type_structure_template) {
			StructureInstance* out = nullptr;
			if (!((TypeStructureTemplate*)gen_type)->owner->generate(key_base, out)) return false;
			eval->write_register_value(out->type.get());
		}
		else if (gen_type->type() == TypeInstanceType::type_trait_template) {
			TraitInstance* out = nullptr;
			if (!((TypeTraitTemplate*)gen_type)->owner->generate(key_base, out)) return false;
			eval->write_register_value(out->type.get());
		}



		// ?????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????
		while (StackManager::stack_state<1>() > 0) {
			StackItem sitm = StackManager::stack_pop<1>(eval);
			CompileValue res = sitm.value;

			if (res.t->has_special_destructor()) {
				res.t->drop(eval, eval->memory_stack_base_pointer + sitm.local_offset.eval(compiler_arch));
			}
		}
		// ?????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????????

		eval->stack_pop(sp);
		StackManager::move_stack_in<1>(std::move(ss));
		return true;
	}

	Type* Operand::template_stack[1024];
	uint16_t Operand::template_sp = 0;


	bool _crs_read_arguments(Cursor& c, unsigned int& argi, TypeFunction* ft, CompileType cpt) {
		CompileContext& nctx = CompileContext::get();
		if (c.tok != RecognizedToken::CloseParenthesis) {
			while (true) {
				if (argi >= nctx.default_types->argument_array_storage.get(ft->argument_array_id).size()) {
					throw_specific_error(c, "Wrong number of arguments");
					return false;
				}

				CompileValue arg;
				Cursor err = c;
				if (!Expression::parse(c, arg, cpt)) return false;
				if (!Expression::rvalue(arg, cpt)) return false;

				if (!Operand::cast(err, arg, nctx.default_types->argument_array_storage.get(ft->argument_array_id)[argi], cpt, true)) return false;
				argi++;

				if (c.tok == RecognizedToken::Comma) {
					c.move();
				}
				else if (c.tok == RecognizedToken::CloseParenthesis) {
					break;
				}
				else {
					throw_wrong_token_error(c, "',' or ')'");
					return false;
				}
			}
		}

		return true;
	}

	bool Operand::parse_call_operator(CompileValue& ret, Cursor& c, CompileType cpt) {
		CompileContext& nctx = CompileContext::get();

		if (ret.t == nctx.default_types->t_type || ret.t->type() == TypeInstanceType::type_template) {
			Cursor nm_err = c;
			if (!Expression::rvalue(ret, cpt)) return false;

			if (cpt != CompileType::compile) {
				Type* dt = nullptr;

				dt = nctx.eval->pop_register_value<Type*>();

				if (dt->type() != TypeInstanceType::type_structure_template && dt->type() != TypeInstanceType::type_trait_template && dt->type() != TypeInstanceType::type_function_template) {
					throw_specific_error(c, "this type is not a generic type");
					return false;
				}
				c.move();

				if (dt->type() == TypeInstanceType::type_structure_template) {
					StructureInstance* inst;
					if (!parse_generate_template(c, ((TypeStructureTemplate*)dt)->owner, inst)) return false;
					ILBuilder::eval_const_type(nctx.eval, inst->type.get());

					ret.lvalue = false;
					ret.t = nctx.default_types->t_type;

				}
				else if (dt->type() == TypeInstanceType::type_trait_template) {
					TraitInstance* inst;
					if (!parse_generate_template(c, ((TypeTraitTemplate*)dt)->owner, inst)) return false;
					ILBuilder::eval_const_type(nctx.eval, inst->type.get());

					ret.lvalue = false;
					ret.t = nctx.default_types->t_type;
				}
				else if (dt->type() == TypeInstanceType::type_function_template) {
					FunctionInstance* inst;
					if (!parse_generate_template(c, ((TypeFunctionTemplate*)dt)->owner, inst)) return false;
					if (!inst->compile()) return false;

					ILBuilder::eval_fnptr(nctx.eval, inst->func);

					ret.lvalue = false;
					ret.t = inst->type;
				}



			}
			else {
				throw_specific_error(c, "Template generator not supported as a runtime operator, please use .generate(...) on generic template type");
				return false;
			}

		}
		else if (ret.t->type() == TypeInstanceType::type_function) {
			if (!Expression::rvalue(ret, cpt)) return false;

			c.move();
			TypeFunction* ft = (TypeFunction*)ret.t;

			if (ft->context()!=ILContext::both && nctx.scope_context != ft->context()) {
				throw_specific_error(c, "Cannot call function with different context specifier");
				return false;
			}

			unsigned int argi = 0;

			CompileValue retval;
			retval.t = ft->return_type;
			retval.lvalue = true;


			if (cpt == CompileType::compile) {

				uint16_t local_return_id = 0;


				ILBuilder::build_callstart(nctx.scope);

				if (ft->return_type->rvalue_stacked()) {
					local_return_id = nctx.function->register_local(retval.t->size());
					StackManager::stack_push<0>(nctx.eval, "$tmp", retval, local_return_id);
					if (retval.t->has_special_constructor()) {
						ILBuilder::build_local(nctx.scope, local_return_id);
						retval.t->build_construct();
					}

					ILBuilder::build_local(nctx.scope, local_return_id);
				}

				if (!_crs_read_arguments(c, argi, ft, cpt)) return false;

				if (argi != nctx.default_types->argument_array_storage.get(ft->argument_array_id).size()) {
					throw_specific_error(c, "Wrong number of arguments");
					return false;
				}

				c.move();

				if (!ft->return_type->rvalue_stacked()) {
					if (!ILBuilder::build_call(nctx.scope, ft->return_type->rvalue(), (uint16_t)argi)) return false;
				}
				else {
					if (!ILBuilder::build_call(nctx.scope, ILDataType::none, (uint16_t)argi + 1)) return false;
				}

				if (ft->return_type->rvalue_stacked()) {
					ILBuilder::build_local(nctx.scope, local_return_id);
				}

			}
			else if (cpt == CompileType::eval) {

				StackItem local_stack_item;

				ILBuilder::eval_callstart(nctx.eval);

				if (ft->return_type->rvalue_stacked()) {
					unsigned char* memory_place = nctx.eval->stack_reserve(retval.t->size().eval(compiler_arch));
					local_stack_item = StackManager::stack_push<1>(nctx.eval, "$tmp", retval, 0);

					if (retval.t->has_special_constructor()) {
						retval.t->construct(nctx.eval, memory_place);
					}

					ILBuilder::eval_local(nctx.eval, local_stack_item.local_offset.eval(compiler_arch));
				}

				if (!_crs_read_arguments(c, argi, ft, cpt)) return false;

				if (argi != nctx.default_types->argument_array_storage.get(ft->argument_array_id).size()) {
					throw_specific_error(c, "Wrong number of arguments");
					return false;
				}

				c.move();

				if (!ft->return_type->rvalue_stacked()) {
					if (!ILBuilder::eval_call(nctx.eval, ft->return_type->rvalue(), (uint16_t)argi)) return false;
				}
				else {
					if (!ILBuilder::eval_call(nctx.eval, ILDataType::none, (uint16_t)argi + 1)) return false;
				}


				if (ft->return_type->rvalue_stacked()) {
					ILBuilder::eval_local(nctx.eval, local_stack_item.local_offset.eval(compiler_arch));
				}

			}

			ret.t = ft->return_type;
			ret.lvalue = false;
		}
		else {
			throw_specific_error(c, "not implemented yet");
			return false;
		}

		return true;
	}

	bool Operand::priv_build_reference(ILEvaluator* eval) {

		Type* t = eval->pop_register_value<Type*>();
		t = t->generate_reference();
		ILBuilder::eval_const_type(eval, t);

		return true;
	}

	bool Operand::priv_build_slice(ILEvaluator* eval) {

		Type* t = eval->pop_register_value<Type*>();
		t = t->generate_slice();
		ILBuilder::eval_const_type(eval, t);

		return true;
	}

	bool Operand::parse_reference(CompileValue& ret, Cursor& c, CompileType cpt) {
		CompileContext& nctx = CompileContext::get();
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
		if (!Operand::parse(c, value, cpt)) return false;
		if (!Expression::rvalue(value, cpt)) return false;

		if (value.t != nctx.default_types->t_type) {
			throw_specific_error(err, "operator expected to recieve type");
			return false;
		}

		if (cpt == CompileType::eval) {
			for (unsigned int i = 0; i < type_ref_count; i++)
				priv_build_reference(nctx.eval);
		}
		else if (cpt == CompileType::compile) {
			throw_specific_error(operr, "Operation not supported in runtime context, please use .reference(...) function");
			return false;
			/*for (unsigned int i = 0; i < type_ref_count; i++)
				ILBuilder::build_priv(nctx.scope,2,0);*/
		}

		ret.t = nctx.default_types->t_type;
		ret.lvalue = false;
		return true;

	}

	bool Operand::parse_array_operator(CompileValue& ret, Cursor& c, CompileType cpt) {
		if (ret.t->type() != TypeInstanceType::type_slice) {
			throw_specific_error(c, "Offset can be applied only on slices");
			return false;
		}
		CompileContext& nctx = CompileContext::get();
		c.move();

		TypeSlice* slice = (TypeSlice*)ret.t;
		Type* base_slice = slice->owner;

		Cursor err = c;
		CompileValue index;
		if (!Expression::parse(c, index, cpt)) return false;
		if (!Expression::rvalue(index, cpt)) return false;

		if (!Operand::cast(err, index, nctx.default_types->t_size,cpt,true)) return false;

		if (c.tok != RecognizedToken::CloseBracket) {
			throw_wrong_token_error(c, "']'");
			return false;
		}
		c.move();
		
		uint16_t base_compile_size = base_slice->size().eval(compiler_arch);


		ret.lvalue = false;
		ret.t = base_slice->generate_reference();

		return true;
	}

	bool Operand::priv_build_array(ILEvaluator* eval) {
		uint32_t val = eval->pop_register_value<uint32_t>();
		Type* t = eval->pop_register_value<Type*>();
		TypeArray* nt = t->generate_array(val);
		ILBuilder::eval_const_type(eval, nt);
		return true;
	}

	bool Operand::parse_array_type(CompileValue& ret, Cursor& c, CompileType cpt) {
		CompileContext& nctx = CompileContext::get();
		Cursor err = c;
		CompileValue res;
		c.move();

		if (c.tok == RecognizedToken::CloseBracket) {
			c.move();

			Cursor t_err = c;

			if (!Operand::parse(c, res, cpt)) return false;
			if (!Expression::rvalue(res, cpt)) return false;

			if (res.t != nctx.default_types->t_type) {
				throw_specific_error(t_err, "Expected type");
				return false;
			}

			if (cpt == CompileType::eval) {
				Type* t = nctx.eval->pop_register_value<Type*>();
				Type* nt = t->generate_slice();
				ILBuilder::eval_const_type(nctx.eval, nt);
			}
			else {
				throw_specific_error(err, "Operation not supported in runtime context, please use .slice function");
				return false;
			}
		}
		else {
			if (!Expression::parse(c, res, cpt)) return false;
			if (!Expression::rvalue(res, cpt)) return false;

			if (!Operand::cast(err, res, nctx.default_types->t_u32, cpt, true)) return false;


			if (c.tok != RecognizedToken::CloseBracket) {
				throw_wrong_token_error(c, "']'");
				return false;
			}
			c.move();

			Cursor t_err = c;

			if (!Operand::parse(c, res, cpt)) return false;
			if (!Expression::rvalue(res, cpt)) return false;

			if (res.t != nctx.default_types->t_type) {
				throw_specific_error(t_err, "Expected type");
				return false;
			}

			if (cpt == CompileType::eval) {
				Type* t = nctx.eval->pop_register_value<Type*>();
				uint32_t val = nctx.eval->pop_register_value<uint32_t>();
				TypeArray* nt = t->generate_array(val);
				ILBuilder::eval_const_type(nctx.eval, nt);
			}
			else {
				throw_specific_error(err, "Operation not supported in runtime context, please use .array(...) function");
				return false;
			}
		}

		ret.t = nctx.default_types->t_type;
		ret.lvalue = false;
		return true;
	}


	bool Operand::parse_double_colon_operator(CompileValue& ret, Cursor& c, CompileType cpt) {
		CompileContext& nctx = CompileContext::get();
		c.move();

		if (cpt == CompileType::compile) {
			throw_specific_error(c, "Operation :: is not supported in runtime context");
			return false;
		}

		if (ret.t != nctx.default_types->t_type) {
			throw_specific_error(c, "left operator is not a type instance");
			return false;
		}

		if (cpt == CompileType::eval) {
			Expression::rvalue(ret, cpt);
			TypeStructureInstance* ti = nctx.eval->pop_register_value<TypeStructureInstance*>();
			if (ti->type() != TypeInstanceType::type_structure_instance) {
				throw_specific_error(c, "Type is not structure instance");
				return false;
			}

			StructureInstance* struct_inst = ti->owner;

			auto f = struct_inst->subtemplates.find(c.buffer);
			if (f != struct_inst->subtemplates.end()) {
				StructureTemplate* tplt = f->second.get();

				auto ss = std::move(StackManager::move_stack_out<1>());
				auto sp = nctx.eval->stack_push();
				tplt->template_parent->insert_key_on_stack(nctx.eval);


				if (!tplt->compile()) return false;

				ret.lvalue = false;
				if (tplt->is_generic) {
					ILBuilder::eval_const_type(nctx.eval, tplt->type.get());
				}
				else {
					StructureInstance* inst = nullptr;

					if (!tplt->generate(sp.first, inst)) return false;
					ILBuilder::eval_const_type(nctx.eval, inst->type.get());
				}

				nctx.eval->stack_pop(sp);
				StackManager::move_stack_in<1>(std::move(ss));
			}
			else {
				throw_specific_error(c, "Structure instance does not contain a structure with this name");
				return false;
			}
		}

		ret.lvalue = false;
		ret.t = nctx.default_types->t_type;

		c.move();

		return true;
	}


	bool Operand::parse_dot_operator(CompileValue& ret, Cursor& c, CompileType cpt) {
		CompileContext& nctx = CompileContext::get();
		if (ret.t->type() == TypeInstanceType::type_slice) {
			c.move();
			if (c.buffer == "count" || c.buffer == "size") {
				if (!Expression::rvalue(ret, cpt)) return false;

				uint32_t compile_pointer = ILSize::single_ptr.eval(compiler_arch);

				if (cpt == CompileType::compile) {

					if (ret.t->rvalue_stacked()) {
						ILBuilder::build_offset(nctx.scope, ILSize::single_ptr);
						ILBuilder::build_load(nctx.scope, ILDataType::size);
					}
					else {
						ILBuilder::build_roffset(nctx.scope, ret.t->rvalue(), ILDataType::size, ILSmallSize(0,1));
					}

					if (c.buffer == "size") {
						TypeSlice* st = (TypeSlice*)ret.t;
						Type* bt = st->owner;

						ILBuilder::build_const_size(nctx.scope, bt->size());
						ILBuilder::build_mul(nctx.scope, ILDataType::size, ILDataType::size);
					}
				}
				else {
					if (ret.t->rvalue_stacked()) {
						ILBuilder::eval_offset(nctx.eval, compile_pointer);
						ILBuilder::eval_load(nctx.eval, ILDataType::size);
					}
					else {
						ILBuilder::eval_roffset(nctx.eval, ret.t->rvalue(), ILDataType::size, (uint8_t)compile_pointer);
					}

					if (c.buffer == "size") {
						TypeSlice* st = (TypeSlice*)ret.t;
						Type* bt = st->owner;

						ILBuilder::eval_const_size(nctx.eval, bt->size().eval(compiler_arch));
						ILBuilder::eval_mul(nctx.eval, ILDataType::size, ILDataType::size);
					}
				}


				c.move();
				ret.lvalue = false;
				ret.t = nctx.default_types->t_size;
				return true;
			}
			/*else if(c.buffer=="slice") {
				c.move();
				if (c.tok != RecognizedToken::OpenParenthesis) {
					throw_wrong_token_error(c, "'('");
					return false;
				}
				c.move();

				uint16_t local_tmp_id = 0;
				uint32_t compile_pointer = nctx.eval->get_compile_pointer_size();
				uint32_t runtime_poniter = nctx.eval->get_pointer_size();

				if (cpt == CompileType::compile) {
					CompileValue tmp_res = ret;
					tmp_res.lvalue = false;
					local_tmp_id = nctx.function->register_local(compile_pointer * 2, runtime_poniter * 2);
					StackManager::stack_push<0>(nctx.eval, "$tmp", tmp_res, local_tmp_id);
					ILBuilder::build_local(nctx.scope, local_tmp_id);
					ILBuilder::build_const_size(nctx.scope, compile_pointer*2, runtime_poniter*2);
					ILBuilder::build_insintric(nctx.scope, ILInsintric::memcpy);
				}

				CompileValue arg;
				Cursor err = c;
				if (!Expression::parse(c, arg, cpt)) return false;
				if (!Operand::cast(err, arg, nctx.default_types->t_size, cpt, true)) return false;

				if (c.tok != RecognizedToken::Comma) {
					throw_wrong_token_error(c, "','");
					return false;
				}
				c.move();

				err = c;
				if (!Expression::parse(c, arg, cpt)) return false;
				if (!Operand::cast(err, arg, nctx.default_types->t_size, cpt, true)) return false;

				if (c.tok != RecognizedToken::CloseParenthesis) {
					throw_wrong_token_error(c, "')'");
					return false;
				}
				c.move();

				TypeSlice* st = (TypeSlice*)ret.t;
				Type* bt = st->owner;


				if (cpt == CompileType::compile) {

					ILBuilder::build_local(nctx.scope, local_tmp_id);
					if (compile_pointer != runtime_poniter)
						ILBuilder::build_member2(nctx.scope, compile_pointer, runtime_poniter);
					else
						ILBuilder::build_member(nctx.scope, compile_pointer);
					ILBuilder::build_store(nctx.scope, ILDataType::size);

					ILBuilder::build_local(nctx.scope, local_tmp_id);
					ILBuilder::build_load(nctx.scope, ILDataType::size);
					ILBuilder::build_const_size(nctx.scope, bt->compile_size(nctx.eval), bt->size(nctx.eval));
					ILBuilder::build_mul(nctx.scope, ILDataType::size, ILDataType::size);
					ILBuilder::build_add(nctx.scope, ILDataType::size, ILDataType::size);
					ILBuilder::build_local(nctx.scope, local_tmp_id);
					ILBuilder::build_store(nctx.scope, ILDataType::size);

					ILBuilder::build_local(nctx.scope, local_tmp_id);
				}
				else {
					CompileValue tmp_res = ret;
					tmp_res.lvalue = false;

					size_t* memory_place = (size_t*)nctx.eval->stack_reserve(compile_pointer*2);
					StackItem local_stack_item = StackManager::stack_push<1>(nctx.eval, "$tmp", tmp_res, 0);


					size_t cnt = nctx.eval->pop_register_value<size_t>();
					size_t from = nctx.eval->pop_register_value<size_t>();
					size_t* old_slice = nctx.eval->pop_register_value<size_t*>();

					memory_place[0] = old_slice[0] + from*bt->size(nctx.eval);
					memory_place[1] = cnt;

					ILBuilder::eval_const_ptr(nctx.eval, memory_place);
				}

				ret.lvalue = false;
				return true;
			}*/
			else {
				throw_specific_error(c, "Indentifier not recognized as a value of slice");
				return false;
			}
		}
		else if (ret.t->type() == TypeInstanceType::type_template || ret.t == nctx.default_types->t_type) {
			c.move();
			if (cpt != CompileType::compile) {
				throw_specific_error(c, "Please use compile time operators for compile time type manipulation");
				return false;
			}

			if (c.buffer == "generate") {

				if (ret.t == nctx.default_types->t_type) {
					throw_specific_error(c, "Operation not supported on generic type, please use generic template type");
					return false;
				}

				c.move();

				if (!Expression::rvalue(ret, cpt)) return false;
				ILBuilder::build_const_type(nctx.scope, ret.t);

				uint64_t dbg_c = nctx.default_types->load_or_register_debug_cursor(c);
				ILBuilder::build_const_u64(nctx.scope, dbg_c);
				ILBuilder::build_insintric(nctx.scope, ILInsintric::debug_cursor);
				ILBuilder::build_insintric(nctx.scope, ILInsintric::template_cast);
				ILBuilder::build_insintric(nctx.scope, ILInsintric::push_template);

				TypeTemplate* tt = (TypeTemplate*)ret.t;
				auto layout = nctx.default_types->argument_array_storage.get(tt->argument_array_id).begin();

				if (c.tok != RecognizedToken::OpenParenthesis) {
					throw_wrong_token_error(c, "'('");
					return false;
				}
				c.move();

				std::vector<CompileValue> results;
				if (c.tok != RecognizedToken::CloseParenthesis) {
					while (true) {
						CompileValue res;
						Cursor err = c;
						if (!Expression::parse(c, res, CompileType::compile)) return false;
						if (!Expression::rvalue(res, CompileType::compile)) return false;
						if (!Operand::cast(err, res, *layout, CompileType::compile, true)) return false;

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
							return false;
						}
					}
				}

				c.move();

				ILBuilder::build_insintric(nctx.scope, ILInsintric::build_template);
			}
			else if (c.buffer == "array") {
				c.move();
				if (c.tok != RecognizedToken::OpenParenthesis) {
					throw_wrong_token_error(c, "'('");
					return false;
				}
				c.move();

				CompileValue res;
				Cursor err = c;
				if (!Expression::rvalue(ret, cpt)) return false;

				if (!Expression::parse(c, res, cpt)) return false;
				if (!Expression::rvalue(res, cpt)) return false;

				if (!Operand::cast(err, res, nctx.default_types->t_u32, cpt, true)) return false;


				if (c.tok != RecognizedToken::CloseParenthesis) {
					throw_wrong_token_error(c, "')'");
					return false;
				}
				c.move();

				ILBuilder::build_insintric(nctx.scope, ILInsintric::build_array);
			}
			else if (c.buffer == "reference") {
				c.move();
				if (!Expression::rvalue(ret, cpt)) return false;
				ILBuilder::build_insintric(nctx.scope, ILInsintric::build_reference);
			}
			else if (c.buffer == "slice") {
				c.move();
				if (!Expression::rvalue(ret, cpt)) return false;
				ILBuilder::build_insintric(nctx.scope, ILInsintric::build_slice);
			}
			else {
				throw_specific_error(c, "Unknown type functional operator");
				return false;
			}

			ret.lvalue = false;
			ret.t = nctx.default_types->t_type;
		}
		else if (ret.t->type() == TypeInstanceType::type_trait) {
			c.move(); 
			
			TypeTraitInstance* tti = (TypeTraitInstance*)ret.t;
			TraitInstance* ti = tti->owner;


			auto off_f = ti->member_table.find(c.buffer);
			if (off_f == ti->member_table.end()) {
				throw_specific_error(c, "Trait function not found");
				return false;
			}
			uint32_t off = (uint32_t)off_f->second;
			auto& mf = ti->member_funcs[off];

			if (!Expression::rvalue(ret, cpt)) return false;
			c.move();
			if (c.tok == RecognizedToken::OpenParenthesis) {
				if (cpt == CompileType::compile) {
					if (ret.t->rvalue_stacked()) {
						ILBuilder::build_duplicate(nctx.scope, ILDataType::ptr);
						ILBuilder::build_offset(nctx.scope, {0,1});
						ILBuilder::build_load(nctx.scope, ILDataType::ptr);
						ILBuilder::build_offset(nctx.scope, ILSize::single_ptr*off);
						ILBuilder::build_load(nctx.scope, ILDataType::ptr);
						ILBuilder::build_callstart(nctx.scope);

						ILBuilder::build_load(nctx.scope, ILDataType::ptr);
						// todo run the function
					}
					else {
						std::cout << "???"; // not invented yet
					}
				}
				else {
					if (ret.t->rvalue_stacked()) {	
						ILBuilder::eval_duplicate(nctx.eval, ILDataType::ptr);
						ILBuilder::eval_load(nctx.eval, ILDataType::ptr);
						ILBuilder::eval_offset(nctx.eval, ILSize::single_ptr.eval(compiler_arch));
						ILBuilder::eval_load(nctx.eval, ILDataType::ptr);
						ILBuilder::eval_offset(nctx.eval, off* ILSize::single_ptr.eval(compiler_arch));
						ILBuilder::eval_load(nctx.eval, ILDataType::ptr);
						ILBuilder::eval_callstart(nctx.eval);
						ILBuilder::eval_load(nctx.eval, ILDataType::ptr);
					}
					else {
						std::cout << "???"; // not invented yet
					}
				}

				c.move();
				unsigned int argi = 1;
				_crs_read_arguments(c, argi, mf.type, cpt);
				c.move();

				if (cpt == CompileType::compile) {
					ILBuilder::build_call(nctx.scope, mf.type->return_type->rvalue(), argi);
				}
				else {
					ILBuilder::eval_call(nctx.eval, mf.type->return_type->rvalue(), argi);
				}

				ret.lvalue = false;
				ret.t = mf.type->return_type;
			}
			else {
				if (cpt == CompileType::compile) {
					if (ret.t->rvalue_stacked()) {
						ILBuilder::build_offset(nctx.scope, ILSize::single_ptr);
						ILBuilder::build_load(nctx.scope, ILDataType::ptr);
						ILBuilder::build_offset(nctx.scope, ILSize::single_ptr*off);
					}
					else {
						std::cout << "???"; // not invented yet
					}
				}
				else {
					if (ret.t->rvalue_stacked()) {
						ILBuilder::eval_offset(nctx.eval, ILSize::single_ptr.eval(compiler_arch));
						ILBuilder::eval_load(nctx.eval, ILDataType::ptr);
						ILBuilder::eval_offset(nctx.eval, off * ILSize::single_ptr.eval(compiler_arch));
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

			if (ret.t->type() == TypeInstanceType::type_reference) {

				ret.t = ((TypeReference*)ret.t)->owner;
				ret.lvalue = true;

				if (cpt == CompileType::compile) {
					ILBuilder::build_load(nctx.scope, ret.t->rvalue());
				}
				else if (cpt == CompileType::eval) {
					ILBuilder::eval_load(nctx.eval, ret.t->rvalue());
				}
			}

			if (ret.t->type() == TypeInstanceType::type_structure_instance) {
				

				c.move();

				TypeStructureInstance* ti = (TypeStructureInstance*)ret.t;
				if (cpt == CompileType::compile) {
					if (!ti->compile()) return false;
				}

				ILSize offset;

				Type* mem_type = nullptr;

				StructureInstance* si = ti->owner;
				auto table_element = si->member_table.find(c.buffer);
				if (table_element != si->member_table.end()) {
					size_t id = table_element->second;
					auto& member = si->member_vars[id];
					
					offset = member.second;
					mem_type = member.first;

					
					c.move();
				}
				else {
					throw_specific_error(c, "Instance does not contain member with this name");
					return false;
				}

				if (!ret.lvalue || ret.t->rvalue_stacked()) {
					if (cpt == CompileType::compile) {
						ILBuilder::build_offset(nctx.scope, offset);
					}
					else if (cpt == CompileType::eval) {
						ILBuilder::eval_offset(nctx.eval, offset.eval(compiler_arch));
					}
					ret.lvalue = true;
				}
				else {
					if (cpt == CompileType::compile) {
						ILBuilder::build_roffset(nctx.scope, ret.t->rvalue(), mem_type->rvalue(),ILSmallSize(offset.absolute,offset.pointers));
					}
					else if (cpt == CompileType::eval) {
						ILBuilder::eval_roffset(nctx.eval, ret.t->rvalue(), mem_type->rvalue(), (uint8_t)offset.eval(compiler_arch));
					}
					ret.lvalue = false;
				}

				ret.t = mem_type;
			}
			else {
				throw_specific_error(c, "Operator cannot be used on this type");
				return false;
			}
		}


		return true;
	}
}