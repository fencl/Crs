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

	void Type::pre_compile(CompileContext& ctx) const {
		if (rvalue != IRDataType::undefined) return;
	}

	void ArrayType::pre_compile(CompileContext& ctx) const {
		if (rvalue != IRDataType::undefined) return;

		ArrayType* self = (ArrayType*)this;
		base->pre_compile(ctx);
		self->is_heavy = true;
		self->rvalue = IRDataType::ptr;
	}


	void FunctionType::pre_compile(CompileContext& ctx) const {
		if (rvalue != IRDataType::undefined) return;
		FunctionType* self = (FunctionType*)this;

		
		returns->pre_compile(ctx);

		for (auto it = arguments->begin(); it != arguments->end(); it++) {
			(*it)->pre_compile(ctx);
		}

		self->is_heavy = true;
		self->rvalue = IRDataType::ptr;
	}

	void InterfaceType::pre_compile(CompileContext& ctx) const {
		if (rvalue != IRDataType::undefined) return;

		InterfaceType* self = (InterfaceType*)this;
		for (auto it = types->begin(); it != types->end(); it++) {
			(*it)->pre_compile(ctx);
		}

		self->rvalue = IRDataType::ptr;
	}


	void TupleType::pre_compile(CompileContext& ctx) const {
		if (rvalue != IRDataType::undefined) return;

		TupleType* self = (TupleType*)this;
		for (auto it = types->begin(); it != types->end(); it++) {
			(*it)->pre_compile(ctx);
		}
		self->is_heavy = true;
		self->rvalue = IRDataType::ptr;
	}


	void PrimitiveType::pre_compile(CompileContext& ctx) const {
		if (rvalue != IRDataType::undefined) return;


		PrimitiveType* self = (PrimitiveType*)this;

		std::string_view nm = name.buffer;

		if (package == PredefinedNamespace && name.buffer == "void") {
			self->rvalue = IRDataType::none;
			self->is_heavy = false;
			return;
		}
		else {
			StructDeclaration* sd = Contents::find_struct(package, nm);
			if (sd == nullptr) {
				throw_specific_error(name, "Compiler is searching for structure type, but it was not found (compiler error)");
			}
			else {

				if (sd->decl_type == StructDeclarationType::t_array || sd->decl_type == StructDeclarationType::t_tuple) {

					if (templates == nullptr || templates->size() != 1)
						throw_specific_error(name, "Wrong parameters given to predefined type");


					CompileContext nctx = ctx;
					nctx.template_ctx = templates;

					StructDeclaration* gsd = ((GenericStructDeclaration*)sd)->create_template(nctx);
					self->structure = gsd;

					if (ref) {
						(*templates)[0]->compile(nctx);
					}
					else {
						(*templates)[0]->pre_compile(nctx);
					}

					self->rvalue = (*templates)[0]->rvalue;
				}
				else {

					CompileContext nctx = ctx;
					nctx.template_ctx = templates;

					if (sd->is_generic()) {
						if (templates == nullptr) {
							throw_specific_error(name, "Primitive type points to generic structure and was not given generic arguments");
						}
						else {
							StructDeclaration* gsd = ((GenericStructDeclaration*)sd)->create_template(nctx);
							sd = gsd;
						}
					}


					self->structure = sd;

					if (!ref && sd->decl_type == StructDeclarationType::Declared) {
						self->is_heavy = true;
					}

					sd->pre_compile(nctx);


					if (ref) {
						self->rvalue = IRDataType::ptr;
					}
					else {
						self->rvalue = sd->rvalue;
					}

				}

			}
		}
	}




	/*
	 *	After compile we need to have the target structure compiled
	 *
	 */


	void Type::compile(CompileContext& ctx) const {
		if (compiled) return;
	}

	void ArrayType::compile(CompileContext& ctx) const {
		if (compiled) return;
		pre_compile(ctx);

		ArrayType* self = (ArrayType*)this;

		base->compile(ctx);

		self->compiled = true;
	}

	void FunctionType::compile(CompileContext& ctx) const {
		if (compiled) return;

		pre_compile(ctx);
		FunctionType* self = (FunctionType*)this;

		returns->compile(ctx);

		for (auto it = arguments->begin(); it != arguments->end(); it++) {
			(*it)->compile(ctx);
		}

		self->compiled = true;
	}


	void InterfaceType::compile(CompileContext& ctx) const {
		if (compiled) return;

		pre_compile(ctx);
		InterfaceType* self = (InterfaceType*)this;
		for (auto it = types->begin(); it != types->end(); it++) {
			(*it)->compile(ctx);
		}

		self->compiled = true;
	}

	void TupleType::compile(CompileContext& ctx) const {
		if (compiled) return;
		pre_compile(ctx);

		TupleType* self = (TupleType*)this;
		for (auto it = types->begin(); it != types->end(); it++) {
			(*it)->compile(ctx);
		}

		self->compiled = true;
	}

	void PrimitiveType::compile(CompileContext& ctx) const {
		if (compiled) return;
		pre_compile(ctx);
		PrimitiveType* self = (PrimitiveType*)this;

		if (package == PredefinedNamespace && name.buffer == "void") {
			
		}
		else {
			CompileContext nctx = ctx;
			nctx.template_ctx = templates;
			structure->compile(nctx);
		}

		self->compiled = true;
	}
}