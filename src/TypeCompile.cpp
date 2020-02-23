#include "Type.h"
#include "Declaration.h"
#include "Error.h"
#include "Contents.h"
#include "PredefinedTypes.h"
#include "svtoi.h"
#include <iostream>
#include "Expression.h"

namespace Corrosive {
	
	void Type::Compile(CompileContext& ctx) const {
		if (compiled) return;
	}

	void Type::PreCompile(CompileContext& ctx) const {
		if (llvm_type != nullptr) return;
	}

	void ArrayType::PreCompile(CompileContext& ctx) const {
		if (llvm_type != nullptr) return;

		ArrayType* self = (ArrayType*)this;

		if (actual_size == 0) {
			ThrowSpecificError(size, "Size of array type has not been evaluated (compiler error)");
		}

		base->PreCompile(ctx);

		self->heavy_type = true;

		self->llvm_type = LLVMArrayType(base->LLVMType(), self->actual_size);
		self->llvm_lvalue = self->llvm_rvalue = LLVMPointerType(self->llvm_type, 0);
	}

	void ArrayType::Compile(CompileContext& ctx) const {
		if (compiled) return;
		PreCompile(ctx);

		ArrayType* self = (ArrayType*)this;

		base->Compile(ctx);

		self->compiled = true;
	}

	void FunctionType::PreCompile(CompileContext& ctx) const {
		if (llvm_type != nullptr) return;
		FunctionType* self = (FunctionType*)this; 
			
		LLVMTypeRef ret;
		std::vector<LLVMTypeRef> argtps;

		Returns()->PreCompile(ctx);
		if (!Returns()->HeavyType()) {
			ret = Returns()->LLVMTypeRValue();
		}
		else {
			argtps.push_back(Returns()->LLVMTypeRValue());
			ret = LLVMVoidType();
		}

		for (auto it = Args()->begin(); it != Args()->end(); it++) {
			(*it)->PreCompile(ctx);
			argtps.push_back((*it)->LLVMTypeRValue());
		}

		self->llvm_type = LLVMFunctionType(ret, argtps.data(), (unsigned int)argtps.size(), false);
	}

	void FunctionType::Compile(CompileContext& ctx) const {
		if (compiled) return;

		PreCompile(ctx);
		FunctionType* self = (FunctionType*)this;

		Returns()->Compile(ctx);

		for (auto it = Args()->begin(); it != Args()->end(); it++) {
			(*it)->Compile(ctx);
		}

		self->compiled = true;
	}

	void InterfaceType::PreCompile(CompileContext& ctx) const {
		if (llvm_type != nullptr) return;

		InterfaceType* self = (InterfaceType*)this;
		for (auto it = Types()->begin(); it != Types()->end(); it++) {
			(*it)->PreCompile(ctx);
		}
	}

	void InterfaceType::Compile(CompileContext& ctx) const {
		if (compiled) return;

		PreCompile(ctx);
		InterfaceType* self = (InterfaceType*)this;
		for (auto it = Types()->begin(); it != Types()->end(); it++) {
			(*it)->Compile(ctx);
		}

		self->compiled = true;
	}


	void TupleType::PreCompile(CompileContext& ctx) const {
		if (llvm_type != nullptr) return;

		TupleType* self = (TupleType*)this;
		for (auto it = Types()->begin(); it != Types()->end(); it++) {
			(*it)->PreCompile(ctx);
		}
	}


	void TupleType::Compile(CompileContext& ctx) const {
		if (compiled) return;
		PreCompile(ctx);

		TupleType* self = (TupleType*)this;
		for (auto it = Types()->begin(); it != Types()->end(); it++) {
			(*it)->Compile(ctx);
		}

		self->compiled = true;
	}



	void PrimitiveType::PreCompile(CompileContext& ctx) const {
		if (llvm_type != nullptr) return;


		PrimitiveType* self = (PrimitiveType*)this;

		std::string_view nm = name.Data();

		if (package == PredefinedNamespace && name.Data() == "void") {
			self->llvm_type = self->llvm_lvalue = self->llvm_rvalue = LLVMVoidType();
			return;
		}
		else {
			StructDeclaration* sd = Contents::FindStruct(package, nm);
			if (sd == nullptr) {
				ThrowSpecificError(name, "Compiler is searching for structure type, but it was not found (compiler error)");
			}
			else {

				if (sd->DeclType() == StructDeclarationType::t_array || sd->DeclType() == StructDeclarationType::t_tuple) {

					if (Templates() == nullptr || Templates()->size() != 1)
						ThrowSpecificError(name, "Wrong parameters given to predefined type");


					CompileContext nctx = ctx;
					nctx.template_ctx = Templates();

					StructDeclaration* gsd = ((GenericStructDeclaration*)sd)->CreateTemplate(nctx);
					self->structure_cache = gsd;

					if (ref) {
						(*Templates())[0]->Compile(nctx);
					}
					else {
						(*Templates())[0]->PreCompile(nctx);
					}

					self->llvm_type = (*Templates())[0]->LLVMType();
					self->llvm_lvalue = (*Templates())[0]->LLVMTypeLValue();
					self->llvm_rvalue = (*Templates())[0]->LLVMTypeRValue();
				}
				else {

					CompileContext nctx = ctx;
					nctx.template_ctx = Templates();

					if (sd->Generic()) {
						if (Templates() == nullptr) {
							ThrowSpecificError(name, "Primitive type points to generic structure and was not given generic arguments");
						}
						else {
							StructDeclaration* gsd = ((GenericStructDeclaration*)sd)->CreateTemplate(nctx);
							sd = gsd;
						}
					}


					self->structure_cache = sd;

					if (ref && sd->DeclType() == StructDeclarationType::Declared) {
						self->heavy_type = true;
					}
					
					sd->PreCompile(nctx);

					self->llvm_type = sd->LLVMType();
					if (ref)
						self->llvm_type = LLVMPointerType(self->llvm_type, 0);


					if (HeavyType())
						self->llvm_lvalue = self->llvm_rvalue = LLVMPointerType(self->llvm_type, 0);
					else {
						self->llvm_lvalue = LLVMPointerType(self->llvm_type, 0);
						self->llvm_rvalue = self->llvm_type;
					}

				}
				
			}
		}		
	}


	void PrimitiveType::Compile(CompileContext& ctx) const {
		if (compiled) return;
		PreCompile(ctx);
		PrimitiveType* self = (PrimitiveType*)this;


		if (package == PredefinedNamespace && name.Data() == "void") {
			
		}
		else {
			CompileContext nctx = ctx;
			nctx.template_ctx = Templates();
			structure_cache->Compile(nctx);
		}

		self->compiled = true;
	}
}