#include "Declaration.h"
#include "Error.h"
#include <iostream>
#include "Contents.h"
#include <string>
#include "PredefinedTypes.h"


namespace Corrosive {

	void StructDeclaration::PreCompile(CompileContext& ctx) {
		if (llvm_type != nullptr) return;
		Contents::StaticStructures.push_back(this);

		if (DeclType() == StructDeclarationType::t_i64 || DeclType() == StructDeclarationType::t_u64) {
			llvm_type = LLVMInt64Type();
		}
		else if(DeclType() == StructDeclarationType::t_ptr) {
			llvm_type = LLVMPointerType(LLVMVoidType(),0);
		}
		else {

			std::string llvm_name;
			if (Class())
				llvm_name.append("t.");
			else
				llvm_name.append("s.");

			if (!package.empty()) {
				llvm_name.append(package);
				llvm_name.append(".");
			}
			llvm_name.append(name.Data());

			if (GenID() != 0) {
				llvm_name.append(".");
				llvm_name.append(std::to_string(GenID()));
			}

			llvm_type = LLVMStructCreateNamed(LLVMGetGlobalContext(), llvm_name.c_str());


			for (int i = 0; i < Extends().size(); i++) {
				const Corrosive::Type*& ext = Extends()[i].second;

				CompileContext nctx = ctx;
				nctx.parent_struct = Extends()[i].first;
				nctx.parent_namespace = Extends()[i].first->ParentPack();
				Type::ResolvePackageInPlace(ext, nctx);

				if (auto exttype = dynamic_cast<const PrimitiveType*>(ext)) {
					if (exttype->Templates() == nullptr) {
						if (exttype->Ref())
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
							if (!fs->Generic()) {
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
				if (!(*it)->isClass) {
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
						thistype.Name(isClass ? ptrc : name);
						thistype.Pack(isClass ? "corrosive" : package);
						thistype.Ref(isClass?false:true);
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
					if (!Class())
						mem_types.push_back(vdecl->Type()->LLVMType());
					else
						ThrowSpecificError(vdecl->Name(), "variable found in trait type");
				}
				else if (fdecl != nullptr) {
					if (Class())
						mem_types.push_back(LLVMPointerType(fdecl->Type()->LLVMType(), 0));
				}
			}

			if (DeclType() == StructDeclarationType::Declared)
				LLVMStructSetBody(llvm_type, mem_types.data(), (unsigned int)mem_types.size(), false);


			TestInterfaceComplete();
			BuildLookupTable();
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
					if (vdecl->Type()->Ref() == 0)
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


	void StructDeclaration::TestInterfaceComplete() {
		std::map<std::tuple<bool, std::string_view>, FunctionDeclaration*> iface_list;

		for (auto it = extends_structures.begin(); it != extends_structures.end(); it++) {
			for (auto mit = (*it)->Members.begin(); mit != (*it)->Members.end(); mit++) {
				if (auto f = dynamic_cast<FunctionDeclaration*>(mit->get())) {
					std::tuple<bool, std::string_view>  key = std::make_tuple(f->Static(),f->Name().Data());

					auto nifc = iface_list.find(key);
					if (nifc == iface_list.end()) {
						iface_list[key] = f;
					}
					else {
						if (!((const FunctionType*)nifc->second->Type())->CanPrimCastIntoIgnoreThis(f->Type())) {
							ThrowSpecificError(Name(), "Structure has two interfaces with incompatible function");
						}
					}
				}
			}
		}

		for (auto it = Members.begin(); it != Members.end(); it++) {
			if (auto f = dynamic_cast<FunctionDeclaration*>(it->get())) {
				std::tuple<bool, std::string_view> key = std::make_tuple(f->Static(), f->Name().Data());
				auto nifc = iface_list.find(key);
				if (nifc != iface_list.end()) {
					if (!((const FunctionType*)f->Type())->CanPrimCastIntoIgnoreThis(nifc->second->Type())) {
						ThrowSpecificError(f->Name(), "Function is not compatible with interface declaration");
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


	void StructDeclaration::BuildLookupTable() {
		unsigned int lookup_id = 0;

		for (auto it = Members.begin(); it != Members.end(); it++) {
			if (auto f = dynamic_cast<FunctionDeclaration*>(it->get())) {
				std::tuple<Declaration*, unsigned int, std::string_view> val = std::make_tuple(it->get(), 0,"");
				
				auto i = LookupTable.emplace(f->Name().Data(), val);
				if (!i.second) {
					ThrowSpecificError(f->Name(), "Member with the same name already existed in the structure");
				}
			}
			else if (auto v = dynamic_cast<VariableDeclaration*>(it->get())) {
				std::tuple<Declaration*, unsigned int, std::string_view> val = std::make_tuple(it->get(), lookup_id++, "");
				
				auto i = LookupTable.emplace(v->Name().Data(), val);
				if (!i.second) {
					ThrowSpecificError(v->Name(), "Member with the same name already existed in the structure");
				}
			}
		}

		for (auto it = Aliases.begin(); it != Aliases.end(); it++) {
			Cursor a_nm = it->first;
			Cursor a_fm = it->second;
			auto look = LookupTable.find(a_fm.Data());
			if (look == LookupTable.end()) {
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

					if (a_nm.Data().empty()) {
						for (auto m_it = alias_struct->LookupTable.begin(); m_it != alias_struct->LookupTable.end(); m_it++) {
							std::tuple<Declaration*, unsigned int, std::string_view> val = std::make_tuple(alias_struct, std::get<1>(look->second), m_it->first);
							LookupTable.emplace((std::string_view)m_it->first,val);
						}
					}
					else {
						auto m_it = alias_struct->LookupTable.find(a_nm.Data());
						if (m_it == alias_struct->LookupTable.end()) {
							ThrowSpecificError(a_nm, "Member with this name does not exist in the aliased structure");
						}
						else {
							std::tuple<Declaration*, unsigned int, std::string_view> val = std::make_tuple(alias_struct, std::get<1>(look->second), m_it->first);
							auto emp = LookupTable.emplace((std::string_view)m_it->first, val);
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