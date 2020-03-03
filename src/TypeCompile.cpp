#include "Type.h"
#include "Declaration.h"
#include "Error.h"
#include "Contents.h"
#include "PredefinedTypes.h"
#include "Utilities.h"
#include <iostream>
#include "Expression.h"

namespace Corrosive {
	
	void Type::compile(CompileContext& ctx) const {
		if (compiled) return;
	}

	void Type::pre_compile(CompileContext& ctx) const {
		if (llvm_type != nullptr) return;
	}

	void ArrayType::pre_compile(CompileContext& ctx) const {
		if (llvm_type != nullptr) return;

		ArrayType* self = (ArrayType*)this;

		if (actual_size == 0) {
			ThrowSpecificError(size, "Size of array type has not been evaluated (compiler error)");
		}

		base->pre_compile(ctx);

		self->is_heavy = true;

		self->llvm_type = LLVMArrayType(base->LLVMType(), self->actual_size);
		self->llvm_lvalue = self->llvm_rvalue = LLVMPointerType(self->llvm_type, 0);
	}

	void ArrayType::compile(CompileContext& ctx) const {
		if (compiled) return;
		pre_compile(ctx);

		ArrayType* self = (ArrayType*)this;

		base->compile(ctx);

		self->compiled = true;
	}

	void FunctionType::pre_compile(CompileContext& ctx) const {
		if (llvm_type != nullptr) return;
		FunctionType* self = (FunctionType*)this; 
			
		LLVMTypeRef ret;
		std::vector<LLVMTypeRef> argtps;

		returns->pre_compile(ctx);
		if (!returns->is_heavy) {
			ret = returns->LLVMTypeRValue();
		}
		else {
			argtps.push_back(returns->LLVMTypeRValue());
			ret = LLVMVoidType();
		}

		for (auto it = arguments->begin(); it != arguments->end(); it++) {
			(*it)->pre_compile(ctx);
			argtps.push_back((*it)->LLVMTypeRValue());
		}
		self->is_heavy = true;
		self->llvm_type = LLVMFunctionType(ret, argtps.data(), (unsigned int)argtps.size(), false);
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

	void InterfaceType::pre_compile(CompileContext& ctx) const {
		if (llvm_type != nullptr) return;

		InterfaceType* self = (InterfaceType*)this;
		for (auto it = types->begin(); it != types->end(); it++) {
			(*it)->pre_compile(ctx);
		}
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


	void TupleType::pre_compile(CompileContext& ctx) const {
		if (llvm_type != nullptr) return;

		TupleType* self = (TupleType*)this;
		for (auto it = types->begin(); it != types->end(); it++) {
			(*it)->pre_compile(ctx);
		}
		self->is_heavy = true;
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



	void PrimitiveType::pre_compile(CompileContext& ctx) const {
		if (llvm_type != nullptr) return;


		PrimitiveType* self = (PrimitiveType*)this;

		std::string_view nm = name.Data();

		if (package == PredefinedNamespace && name.Data() == "void") {
			self->llvm_type = self->llvm_lvalue = self->llvm_rvalue = LLVMVoidType();
			self->is_heavy = false;
			return;
		}
		else {
			StructDeclaration* sd = Contents::find_struct(package, nm);
			if (sd == nullptr) {
				ThrowSpecificError(name, "Compiler is searching for structure type, but it was not found (compiler error)");
			}
			else {

				if (sd->decl_type == StructDeclarationType::t_array || sd->decl_type == StructDeclarationType::t_tuple) {

					if (templates == nullptr || templates->size() != 1)
						ThrowSpecificError(name, "Wrong parameters given to predefined type");


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

					self->llvm_type = (*templates)[0]->LLVMType();
					self->llvm_lvalue = (*templates)[0]->LLVMTypeLValue();
					self->llvm_rvalue = (*templates)[0]->LLVMTypeRValue();
				}
				else {

					CompileContext nctx = ctx;
					nctx.template_ctx = templates;

					if (sd->is_generic()) {
						if (templates == nullptr) {
							ThrowSpecificError(name, "Primitive type points to generic structure and was not given generic arguments");
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

					self->llvm_type = sd->LLVMType();
					if (ref)
						self->llvm_type = LLVMPointerType(self->llvm_type, 0);


					if (is_heavy)
						self->llvm_lvalue = self->llvm_rvalue = LLVMPointerType(self->llvm_type, 0);
					else {
						self->llvm_lvalue = LLVMPointerType(self->llvm_type, 0);
						self->llvm_rvalue = self->llvm_type;
					}

				}
				
			}
		}		
	}


	void PrimitiveType::compile(CompileContext& ctx) const {
		if (compiled) return;
		pre_compile(ctx);
		PrimitiveType* self = (PrimitiveType*)this;


		if (package == PredefinedNamespace && name.Data() == "void") {
			
		}
		else {
			CompileContext nctx = ctx;
			nctx.template_ctx = templates;
			structure->compile(nctx);
		}

		self->compiled = true;
	}
}