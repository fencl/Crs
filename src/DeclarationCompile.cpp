#include "Declaration.h"
#include "Error.h"
#include <iostream>
#include "Contents.h"
#include <string>
#include "PredefinedTypes.h"
#include "Expression.h"
#include "StackManager.h"

namespace Corrosive {

	void StructDeclaration::PreCompile(CompileContext& ctx) {
		if (llvm_type != nullptr) return;
		Contents::StaticStructures.push_back(this);

		if (decl_type == StructDeclarationType::t_i64 || decl_type == StructDeclarationType::t_u64) {
			llvm_type = LLVMInt64Type();
		}else if (decl_type == StructDeclarationType::t_i32 || decl_type == StructDeclarationType::t_u32) {
			llvm_type = LLVMInt32Type();
		}
		else if (decl_type == StructDeclarationType::t_i16 || decl_type == StructDeclarationType::t_u16) {
			llvm_type = LLVMInt16Type();
		}
		else if (decl_type == StructDeclarationType::t_i8 || decl_type == StructDeclarationType::t_u8) {
			llvm_type = LLVMInt8Type();
		}
		else if (decl_type == StructDeclarationType::t_f32) {
			llvm_type = LLVMFloatType();
		}
		else if (decl_type == StructDeclarationType::t_f64) {
			llvm_type = LLVMDoubleType();
		}
		else if (decl_type == StructDeclarationType::t_bool) {
			llvm_type = LLVMInt1Type();
		}
		else if(decl_type == StructDeclarationType::t_ptr) {
			llvm_type = LLVMPointerType(LLVMVoidType(),0);
		}
		else {

			std::string llvm_name;
			if (is_trait)
				llvm_name.append("t.");
			else
				llvm_name.append("s.");

			if (!package.empty()) {
				llvm_name.append(package);
				llvm_name.append(".");
			}
			llvm_name.append(name.Data());

			if (gen_id != 0) {
				llvm_name.append(".");
				llvm_name.append(std::to_string(gen_id));
			}

			llvm_type = LLVMStructCreateNamed(LLVMGetGlobalContext(), llvm_name.c_str());


			for (int i = 0; i < implements.size(); i++) {
				const Corrosive::Type*& ext = implements[i].second;

				CompileContext nctx = ctx;
				nctx.parent_struct = implements[i].first;
				nctx.parent_namespace = implements[i].first->ParentPack();
				Type::ResolvePackageInPlace(ext, nctx);

				if (auto exttype = dynamic_cast<const PrimitiveType*>(ext)) {
					if (exttype->Templates() == nullptr) {
						if (exttype->ref)
							ThrowSpecificError(Name(), "Structure cannot extend references");

						std::pair<std::string_view, std::string_view> key = std::make_pair(exttype->Pack(), exttype->Name().Data());

						if (auto fs = exttype->Structure()) {
							fs->PreCompile(ctx);
							extends_structures.push_back(fs);
						}
						else {
							ThrowSpecificError(Name(), "Extended structure was not found in any package from the lookup queue");
						}
					}
					else {
						if (auto fs = exttype->Structure()) {
							if (!fs->is_generic()) {
								ThrowSpecificError(exttype->Name(), "Target structure is not generic");
							}
							GenericStructDeclaration* gsd = (GenericStructDeclaration*)fs;
							CompileContext nctx = ctx;
							nctx.template_ctx = exttype->Templates();

							auto gen = gsd->CreateTemplate(nctx);

							gen->PreCompile(nctx);
							extends_structures.push_back(gen);

						}
						else {
							ThrowSpecificError(Name(), "Extended structure was not found in any package from the lookup queue");
						}
					}

				}
				else {
					ThrowSpecificError(Name(), "Structure cannot extend non-structural type");
				}
			}

			for (auto it = extends_structures.begin(); it != extends_structures.end(); it++) {
				if (!(*it)->is_trait) {
					ThrowSpecificError(Name(), "All extended types needs to be classes");
				}
				(*it)->PreCompile(ctx);
			}

			std::vector<LLVMTypeRef> mem_types;

			for (int i = 0; i < Members.size(); i++) {
				std::unique_ptr<Declaration>& decl = Members[i];
				VariableDeclaration* vdecl;
				FunctionDeclaration* fdecl;

				if (fdecl = dynamic_cast<FunctionDeclaration*>(decl.get())) {

					const FunctionType*& fdt = (const FunctionType*&)fdecl->Type();


					if (!fdecl->Static()) {
						PrimitiveType thistype;
						Cursor ptrc;
						ptrc.Data("ptr");
						thistype.Name(is_trait ? ptrc : name);
						thistype.Pack(is_trait ? "corrosive" : package);
						thistype.ref = is_trait ?false:true;
						thistype.Templates() = ctx.template_ctx;
						FunctionType nfd = *fdt;
						std::vector<const Type*> nargs = *nfd.Args();
						nargs.insert(nargs.begin(), Contents::EmplaceType(thistype));
						nfd.Args() = Contents::RegisterTypeArray(std::move(nargs));
						fdt = (const FunctionType*)Contents::EmplaceType(nfd);
					}

					CompileContext nctx = ctx;
					nctx.parent_struct = fdecl->ParentStruct();
					nctx.parent_namespace = fdecl->ParentPack();
					Type::ResolvePackageInPlace(fdecl->Type(), nctx);

					Cursor thisc; thisc.Data("this");
					fdecl->Argnames()->insert(fdecl->Argnames()->begin(), thisc);
				}

				decl->PreCompile(ctx);

				if (vdecl = dynamic_cast<VariableDeclaration*>(decl.get())) {
					if (!is_trait)
						mem_types.push_back(vdecl->Type()->LLVMType());
					else
						ThrowSpecificError(vdecl->Name(), "variable found in trait type");
				}
				else if (fdecl != nullptr) {
					if (is_trait)
						mem_types.push_back(LLVMPointerType(fdecl->Type()->LLVMType(), 0));
				}
			}

			if (decl_type == StructDeclarationType::Declared)
				LLVMStructSetBody(llvm_type, mem_types.data(), (unsigned int)mem_types.size(), false);


			build_lookup_table();
			test_interface_complete();
		}
	}

	void StructDeclaration::Compile(CompileContext& ctx) {
		if (llvm_compile_progress == 0) {
			PreCompile(ctx);

			llvm_compile_progress = 1;

			for (auto it = extends_structures.begin(); it != extends_structures.end(); it++) {
				(*it)->Compile(ctx);
			}

			for (int i = 0; i < Members.size(); i++) {
				std::unique_ptr<Declaration>& decl = Members[i];

				if (auto vdecl = dynamic_cast<VariableDeclaration*>(decl.get())) {
					if (!vdecl->Type()->ref)
						vdecl->Compile(ctx);
				}
			}

			llvm_compile_progress = 2;
		}
		else if (llvm_compile_progress == 2) {
			return;
		}
		else {
			ThrowSpecificError(Name(), "This structure caused build cycle");
		}

	}


	void StructDeclaration::test_interface_complete() {
		std::map<std::string_view, FunctionDeclaration*> iface_list;

		for (auto it = extends_structures.begin(); it != extends_structures.end(); it++) {
			for (auto mit = (*it)->Members.begin(); mit != (*it)->Members.end(); mit++) {
				if (auto f = dynamic_cast<FunctionDeclaration*>(mit->get())) {
					std::string_view  key = f->Name().Data();

					auto nifc = iface_list.find(key);
					if (nifc == iface_list.end()) {
						iface_list[key] = f;
					}
					else {
						if (!((const FunctionType*)nifc->second->Type())->CanPrimCastIntoIgnoreThis(f->Type())) {
							ThrowSpecificError(Name(), "Structure has two interfaces with trait function");
						}
						if (nifc->second->Static() != f->Static()) {
							ThrowSpecificError(Name(), "Structure has two interfaces with trait function");
						}
					}
				}
			}
		}

		for (auto it = lookup_table.begin(); it != lookup_table.end(); it++) {
			Declaration* actual_decl = FindDeclarationOfMember(it->first);

			if (auto f = dynamic_cast<FunctionDeclaration*>(actual_decl)) {
				std::string_view key = f->Name().Data();
				auto nifc = iface_list.find(key);
				if (nifc != iface_list.end()) {
					if (!((const FunctionType*)f->Type())->CanPrimCastIntoIgnoreThis(nifc->second->Type())) {
						ThrowSpecificError(f->Name(), "Declaration is not compatible with trait declaration");
					}
					iface_list.erase(nifc);
				}
			}
		}

		for (auto it = iface_list.begin(); it != iface_list.end(); it++) {
			if (!it->second->HasBlock())
				ThrowSpecificError(Name(), "Structure lacks some functions from its interfaces");
		}
	}


	void StructDeclaration::build_lookup_table() {
		if (has_lookup_table) return;
		has_lookup_table = true;

		unsigned int lookup_id = 0;

		for (auto it = Members.begin(); it != Members.end(); it++) {
			if (auto f = dynamic_cast<FunctionDeclaration*>(it->get())) {
				std::tuple<Declaration*, unsigned int, std::string_view> val = std::make_tuple(it->get(), 0,"");
				
				auto i = lookup_table.emplace(f->Name().Data(), val);
				if (!i.second) {
					ThrowSpecificError(f->Name(), "Member with the same name already existed in the structure");
				}
			}
			else if (auto v = dynamic_cast<VariableDeclaration*>(it->get())) {
				std::tuple<Declaration*, unsigned int, std::string_view> val = std::make_tuple(it->get(), lookup_id++, "");
				
				auto i = lookup_table.emplace(v->Name().Data(), val);
				if (!i.second) {
					ThrowSpecificError(v->Name(), "Member with the same name already existed in the structure");
				}
			}
		}

		for (auto it = aliases.begin(); it != aliases.end(); it++) {
			Cursor a_nm = it->first;
			Cursor a_fm = it->second;
			auto look = lookup_table.find(a_fm.Data());
			if (look == lookup_table.end()) {
				ThrowSpecificError(a_fm, "Member with this name was not declared in this structure");
			}
			else {
				Declaration* alias_var = std::get<0>(look->second);
				if (auto v = dynamic_cast<VariableDeclaration*>(alias_var)) {
					const Type* alias_var_type = v->Type();
					StructDeclaration* alias_struct = nullptr;
					
					if (auto pt = dynamic_cast<const PrimitiveType*>(alias_var_type)) {
						alias_struct = pt->Structure();
					}
					else {
						ThrowSpecificError(a_fm, "Alias points to variable with type that cannot be aliased");
					}

					alias_struct->build_lookup_table();

					if (a_nm.Data().empty()) {
						for (auto m_it = alias_struct->lookup_table.begin(); m_it != alias_struct->lookup_table.end(); m_it++) {
							std::tuple<Declaration*, unsigned int, std::string_view> val = std::make_tuple(alias_struct, std::get<1>(look->second), m_it->first);
							lookup_table.emplace((std::string_view)m_it->first,val);
						}
					}
					else {
						auto m_it = alias_struct->lookup_table.find(a_nm.Data());
						if (m_it == alias_struct->lookup_table.end()) {
							ThrowSpecificError(a_nm, "Member with this name does not exist in the aliased structure");
						}
						else {
							std::tuple<Declaration*, unsigned int, std::string_view> val = std::make_tuple(alias_struct, std::get<1>(look->second), m_it->first);
							auto emp = lookup_table.emplace((std::string_view)m_it->first, val);
							if (!emp.second) {
								ThrowSpecificError(a_nm, "Member with the same name already exists in the structure");
							}
						}
					}
				}
				else {
					ThrowSpecificError(a_fm, "Alias points to function");
				}
			}
		}
	}

	void FunctionDeclaration::PreCompile(CompileContext& ctx) {
		if (Type()->LLVMType() != nullptr) return;

		CompileContext nctx = ctx;
		nctx.parent_struct = ParentStruct();
		nctx.parent_namespace = ParentPack();

		Type::ResolvePackageInPlace(Type(), nctx);
		Type()->PreCompile(nctx);
	}

	void FunctionDeclaration::Compile(CompileContext& ctx) {
		if (llvm_compile_progress == 0) {
			PreCompile(ctx);

			llvm_compile_progress = 1;

			CompileContext nctx = ctx;
			nctx.parent_struct = ParentStruct();
			nctx.parent_namespace = ParentPack();

			Type()->Compile(nctx);


			std::string name = "f.";
			if (!Pack().empty()) {
				name.append(Pack());
				name.append(".");
			}
			name.append(Name().Data());

			unsigned long stack = StackManager::StackState();

			function = LLVMAddFunction(ctx.module, name.c_str(), type->LLVMType());
			LLVMBasicBlockRef block = LLVMAppendBasicBlock(function, "entry");
			LLVMBuilderRef builder = LLVMCreateBuilder();

			LLVMPositionBuilderAtEnd(builder, block);

			const FunctionType* ft = (const FunctionType*)type;
			bool heavy_return = ft->Returns()->HeavyType();

			for (int i = 0; i < ft->Args()->size(); i++) {
				CompileValue cv;
				cv.lvalue = true;
				cv.t = (*ft->Args())[i];
				cv.v = LLVMGetParam(function, i+heavy_return?1:0);

				if (!cv.t->HeavyType()) {
					LLVMValueRef vr = cv.v;
					cv.v = LLVMBuildAlloca(builder, cv.t->LLVMType(), "");
					LLVMBuildStore(builder, vr,cv.v);
				}
				StackManager::StackPush(argnames[i].Data(),cv);
			}


			Cursor c = Block();
			Corrosive::CompileContextExt cctx;
			cctx.function = function;
			cctx.unit = this;
			cctx.basic.parent_namespace = ParentPack();
			cctx.basic.parent_struct = nullptr;
			cctx.basic.template_ctx = nullptr;
			cctx.builder = builder;
			cctx.fallback_and = nullptr;
			cctx.fallback_or = nullptr;
			cctx.block = block;

			Corrosive::CompileValue cv = Expression::Parse(c, cctx, Corrosive::CompileType::Compile);
			LLVMBuildRet(cctx.builder, cv.v);

			LLVMDisposeBuilder(builder);


			StackManager::StackStateRestore(stack);
			llvm_compile_progress = 2;
			return;
		}
		else if (llvm_compile_progress == 2) {
			return;
		}
		else {
			ThrowSpecificError(Name(), "This function caused build cycle");
		}

		return;
	}

	void VariableDeclaration::PreCompile(CompileContext& ctx) {
		if (Type()->LLVMType() != nullptr) return;

		CompileContext nctx = ctx;
		nctx.parent_struct = ParentStruct();
		nctx.parent_namespace = ParentPack();
		Type::ResolvePackageInPlace(Type(), nctx);
		Type()->PreCompile(nctx);
	}

	void VariableDeclaration::Compile(CompileContext& ctx) {
		if (llvm_compile_progress == 0) {
			PreCompile(ctx);

			llvm_compile_progress = 1;

			CompileContext nctx = ctx;
			nctx.parent_struct = ParentStruct();
			nctx.parent_namespace = ParentPack();
			Type()->Compile(nctx);

			llvm_compile_progress = 2;
			return;
		}
		else if (llvm_compile_progress == 2) {
			return;
		}
		else {
			ThrowSpecificError(Name(), "This variable caused build cycle");
		}

		return;
	}

	void Declaration::PreCompile(CompileContext& ctx) {
		return;
	}
	void Declaration::Compile(CompileContext& ctx) {
		return;
	}

}