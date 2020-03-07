#include "Type.h"
#include "Declaration.h"
#include "Error.h"
#include "Contents.h"
#include "PredefinedTypes.h"
#include "Utilities.h"
#include <iostream>
#include "Expression.h"

namespace Corrosive {

	/*
	 *	After precompile we need to be sure of the type size
	 *	this preccess is used to eliminate type cycles
	 *
	 */

	bool Type::pre_compile(CompileContext& ctx) const {
		return true;
	}

	bool ArrayType::pre_compile(CompileContext& ctx) const {
		if (iltype != nullptr) return true;

		ArrayType* self = (ArrayType*)this;
		if (!base->pre_compile(ctx)) return false;
		self->is_heavy = true;
		self->iltype = ctx.module->t_i32;//REAPIR!

		return true;
	}


	bool FunctionType::pre_compile(CompileContext& ctx) const {
		if (iltype != nullptr) return true;
		FunctionType* self = (FunctionType*)this;

		
		if (!returns->pre_compile(ctx)) return false;

		for (auto it = arguments->begin(); it != arguments->end(); it++) {
			(*it)->pre_compile(ctx);
		}

		self->is_heavy = true;
		self->iltype = ctx.module->t_ptr; // maybe we can create special function type, as far as ir code is concerned we don't need to care. LLVM on the other hand...
		return true;
	}

	bool InterfaceType::pre_compile(CompileContext& ctx) const {
		if (iltype != nullptr) return true;

		InterfaceType* self = (InterfaceType*)this;
		for (auto it = types->begin(); it != types->end(); it++) {
			if (!(*it)->pre_compile(ctx)) return false;
		}

		self->iltype = ctx.module->t_i32;//REAPIR!
		return true;
	}


	bool TupleType::pre_compile(CompileContext& ctx) const {
		if (iltype != nullptr) return true;

		TupleType* self = (TupleType*)this;

		self->iltype = ctx.module->create_struct_type();

		for (auto it = types->begin(); it != types->end(); it++) {
			if (!(*it)->pre_compile(ctx)) return false;
			((ILStruct*)self->iltype)->add_member((*it)->iltype);
		}

		((ILStruct*)iltype)->align_size();

		self->is_heavy = true;
		return true;
	}


	bool PrimitiveType::pre_compile(CompileContext& ctx) const {
		if (iltype != nullptr) return true;


		PrimitiveType* self = (PrimitiveType*)this;

		std::string_view nm = name.buffer;

		if (package == PredefinedNamespace && name.buffer == "void") {
			self->iltype = ctx.module->t_void;
			self->is_heavy = false;
			return true;
		}
		else {
			StructDeclaration* sd = Contents::find_struct(package, nm);
			if (sd == nullptr) {
				throw_specific_error(name, "Compiler is searching for structure type, but it was not found (compiler error)");
				return false;
			}
			else {

				if (sd->decl_type == StructDeclarationType::t_array || sd->decl_type == StructDeclarationType::t_tuple) {

					if (templates == nullptr || templates->size() != 1) {
						throw_specific_error(name, "Wrong parameters given to predefined type");
						return false;
					}


					CompileContext nctx = ctx;
					nctx.template_ctx = templates;

					StructDeclaration* gsd;
					if (!((GenericStructDeclaration*)sd)->create_template(nctx,gsd)) return false;
					self->structure = gsd;

					if (ref) {
						(*templates)[0]->compile(nctx);
					}
					else {
						(*templates)[0]->pre_compile(nctx);
					}

					self->iltype = (*templates)[0]->iltype;

					return true;
				}
				else {

					CompileContext nctx = ctx;
					nctx.template_ctx = templates;

					if (sd->is_generic()) {
						if (templates == nullptr) {
							throw_specific_error(name, "Primitive type points to generic structure and was not given generic arguments");
							return false;
						}
						else {
							StructDeclaration* gsd;
							if (!((GenericStructDeclaration*)sd)->create_template(nctx,gsd)) return false;
							sd = gsd;
						}
					}


					self->structure = sd;

					if (!ref && sd->decl_type == StructDeclarationType::Declared) {
						self->is_heavy = true;
					}

					if (!sd->pre_compile(nctx)) return false;


					if (ref) {
						self->iltype = ctx.module->t_ptr;
					}
					else {
						self->iltype = sd->iltype;
					}

					return true;

				}

			}
		}
	}




	/*
	 *	After compile we need to have the target structure compiled
	 *
	 */


	bool Type::compile(CompileContext& ctx) const {
		return true;
	}

	bool ArrayType::compile(CompileContext& ctx) const {
		if (compiled) return true;
		if (!pre_compile(ctx)) return false;

		ArrayType* self = (ArrayType*)this;

		if (!base->compile(ctx)) return false;

		self->compiled = true;

		return true;
	}

	bool FunctionType::compile(CompileContext& ctx) const {
		if (compiled) return true;

		if (!pre_compile(ctx)) return false;
		FunctionType* self = (FunctionType*)this;

		if (!returns->compile(ctx)) return false;

		for (auto it = arguments->begin(); it != arguments->end(); it++) {
			if (!(*it)->compile(ctx)) return false;
		}

		self->compiled = true;
		return true;
	}


	bool InterfaceType::compile(CompileContext& ctx) const {
		if (compiled) return true;

		if (!pre_compile(ctx)) return false;
		InterfaceType* self = (InterfaceType*)this;
		for (auto it = types->begin(); it != types->end(); it++) {
			if (!(*it)->compile(ctx)) return false;
		}

		self->compiled = true;

		return true;
	}

	bool TupleType::compile(CompileContext& ctx) const {
		if (compiled) return true;
		if (!pre_compile(ctx)) return false;

		TupleType* self = (TupleType*)this;
		for (auto it = types->begin(); it != types->end(); it++) {
			if (!(*it)->compile(ctx)) return false;
		}

		self->compiled = true;

		return true;
	}

	bool PrimitiveType::compile(CompileContext& ctx) const {
		if (compiled) return true;
		if (!pre_compile(ctx)) return false;
		PrimitiveType* self = (PrimitiveType*)this;

		if (package == PredefinedNamespace && name.buffer == "void") {
			
		}
		else {
			CompileContext nctx = ctx;
			nctx.template_ctx = templates;
			if (!structure->compile(nctx)) return false;
		}

		self->compiled = true;
		return true;
	}
}