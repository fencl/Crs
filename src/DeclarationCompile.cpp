#include "Declaration.h"
#include "Error.h"
#include <iostream>
#include "Contents.h"
#include <string>
#include "PredefinedTypes.h"
#include "Expression.h"
#include "StackManager.h"
#include <llvm/Core.h>

namespace Corrosive {

	void StructDeclaration::pre_compile(CompileContext& ctx) {
		if (irtype != nullptr) return;
		Contents::StaticStructures.push_back(this);

		if (decl_type == StructDeclarationType::t_i64)
			irtype = ctx.module->t_i64;
		else if (decl_type == StructDeclarationType::t_u64)
			irtype = ctx.module->t_u64;
		else if (decl_type == StructDeclarationType::t_i32)
			irtype = ctx.module->t_i32;
		else if (decl_type == StructDeclarationType::t_u32)
			irtype = ctx.module->t_u32;
		else if (decl_type == StructDeclarationType::t_i16)
			irtype = ctx.module->t_i16;
		else if (decl_type == StructDeclarationType::t_u16)
			irtype = ctx.module->t_u16;
		else if (decl_type == StructDeclarationType::t_i8)
			irtype = ctx.module->t_i8;
		else if (decl_type == StructDeclarationType::t_u8)
			irtype = ctx.module->t_u8;
		else if (decl_type == StructDeclarationType::t_bool)
			irtype = ctx.module->t_bool;
		else if (decl_type == StructDeclarationType::t_ptr)
			irtype = ctx.module->t_ptr;
		else if (decl_type == StructDeclarationType::t_f32)
			irtype = ctx.module->t_f32;
		else if (decl_type == StructDeclarationType::t_f64)
			irtype = ctx.module->t_f64;
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
			llvm_name.append(name.buffer);

			if (gen_id != 0) {
				llvm_name.append(".");
				llvm_name.append(std::to_string(gen_id));
			}



			for (int i = 0; i < implements.size(); i++) {
				const Corrosive::Type*& ext = implements[i].second;

				CompileContext nctx = ctx;
				nctx.parent_struct = implements[i].first;
				nctx.parent_namespace = implements[i].first->parent_pack;
				Type::resolve_package_in_place(ext, nctx);

				if (auto exttype = dynamic_cast<const PrimitiveType*>(ext)) {
					if (exttype->templates == nullptr) {
						if (exttype->ref)
							throw_specific_error(name, "Structure cannot extend references");

						std::pair<std::string_view, std::string_view> key = std::make_pair(exttype->package, exttype->name.buffer);

						if (auto fs = exttype->structure) {
							fs->pre_compile(ctx);
							implements_structures.push_back(fs);
						}
						else {
							throw_specific_error(name, "Extended structure was not found in any package from the lookup queue");
						}
					}
					else {
						if (auto fs = exttype->structure) {
							if (!fs->is_generic()) {
								throw_specific_error(exttype->name, "Target structure is not generic");
							}
							GenericStructDeclaration* gsd = (GenericStructDeclaration*)fs;
							CompileContext nctx = ctx;
							nctx.template_ctx = exttype->templates;

							auto gen = gsd->create_template(nctx);

							gen->pre_compile(nctx);
							implements_structures.push_back(gen);

						}
						else {
							throw_specific_error(name, "Extended structure was not found in any package from the lookup queue");
						}
					}

				}
				else {
					throw_specific_error(name, "Structure cannot extend non-structural type");
				}
			}

			for (auto it = implements_structures.begin(); it != implements_structures.end(); it++) {
				if (!(*it)->is_trait) {
					throw_specific_error(name, "All extended types needs to be classes");
				}
				(*it)->pre_compile(ctx);
			}

			IRStruct* s_type = ctx.module->create_struct_type();

			//std::vector<LLVMTypeRef> mem_types;

			for (int i = 0; i < members.size(); i++) {
				std::unique_ptr<Declaration>& decl = members[i];
				VariableDeclaration* vdecl;
				FunctionDeclaration* fdecl;

				if (fdecl = dynamic_cast<FunctionDeclaration*>(decl.get())) {

					const FunctionType*& fdt = (const FunctionType*&)fdecl->type;


					if (!fdecl->is_static) {
						PrimitiveType thistype;
						Cursor ptrc;
						ptrc.buffer = "ptr";
						thistype.name = is_trait ? ptrc : name;
						thistype.package = is_trait ? "corrosive" : package;
						thistype.ref = is_trait ? false : true;
						thistype.templates = ctx.template_ctx;
						FunctionType nfd = *fdt;
						std::vector<const Type*> nargs = *nfd.arguments;
						nargs.insert(nargs.begin(), Contents::emplace_type(thistype));
						nfd.arguments = Contents::register_type_array(std::move(nargs));
						fdt = (const FunctionType*)Contents::emplace_type(nfd);
					}

					CompileContext nctx = ctx;
					nctx.parent_struct = fdecl->parent_struct();
					nctx.parent_namespace = fdecl->parent_pack;
					Type::resolve_package_in_place(fdecl->type, nctx);

					Cursor thisc; thisc.buffer = "this";
					fdecl->argnames.insert(fdecl->argnames.begin(), thisc);
				}

				decl->pre_compile(ctx);

				if (vdecl = dynamic_cast<VariableDeclaration*>(decl.get())) {
					if (!is_trait)
						s_type->add_member(vdecl->type->irtype);
					else
						throw_specific_error(vdecl->name, "variable found in trait type");
				}
				else if (fdecl != nullptr) {
					if (is_trait)
						s_type->add_member(ctx.module->t_ptr);
				}
			}

			irtype = s_type;

			build_lookup_table();
			test_interface_complete();
		}
	}

	void StructDeclaration::compile(CompileContext& ctx) {
		if (compile_progress == 0) {
			pre_compile(ctx);

			compile_progress = 1;

			for (auto it = implements_structures.begin(); it != implements_structures.end(); it++) {
				(*it)->compile(ctx);
			}

			for (int i = 0; i < members.size(); i++) {
				std::unique_ptr<Declaration>& decl = members[i];

				if (auto vdecl = dynamic_cast<VariableDeclaration*>(decl.get())) {
					if (!vdecl->type->ref)
						vdecl->compile(ctx);
				}
			}

			compile_progress = 2;
		}
		else if (compile_progress == 2) {
			return;
		}
		else {
			throw_specific_error(name, "This structure caused build cycle");
		}

	}


	void StructDeclaration::test_interface_complete() {
		std::map<std::string_view, FunctionDeclaration*> iface_list;

		for (auto it = implements_structures.begin(); it != implements_structures.end(); it++) {
			for (auto mit = (*it)->members.begin(); mit != (*it)->members.end(); mit++) {
				if (auto f = dynamic_cast<FunctionDeclaration*>(mit->get())) {
					std::string_view  key = f->name.buffer;

					auto nifc = iface_list.find(key);
					if (nifc == iface_list.end()) {
						iface_list[key] = f;
					}
					else {
						if (!((const FunctionType*)nifc->second->type)->can_simple_cast_into_ignore_this(f->type)) {
							throw_specific_error(name, "Structure has two interfaces with trait function");
						}
						if (nifc->second->is_static != f->is_static) {
							throw_specific_error(name, "Structure has two interfaces with trait function");
						}
					}
				}
			}
		}

		for (auto it = lookup_table.begin(); it != lookup_table.end(); it++) {
			Declaration* actual_decl = FindDeclarationOfMember(it->first);

			if (auto f = dynamic_cast<FunctionDeclaration*>(actual_decl)) {
				std::string_view key = f->name.buffer;
				auto nifc = iface_list.find(key);
				if (nifc != iface_list.end()) {
					if (!((const FunctionType*)f->type)->can_simple_cast_into_ignore_this(nifc->second->type)) {
						throw_specific_error(f->name, "Declaration is not compatible with trait declaration");
					}
					iface_list.erase(nifc);
				}
			}
		}

		for (auto it = iface_list.begin(); it != iface_list.end(); it++) {
			if (!it->second->has_block)
				throw_specific_error(name, "Structure lacks some functions from its interfaces");
		}
	}


	void StructDeclaration::build_lookup_table() {
		if (has_lookup_table) return;
		has_lookup_table = true;

		unsigned int lookup_id = 0;

		for (auto it = members.begin(); it != members.end(); it++) {
			if (auto f = dynamic_cast<FunctionDeclaration*>(it->get())) {
				std::tuple<Declaration*, unsigned int, std::string_view> val = std::make_tuple(it->get(), 0,"");
				
				auto i = lookup_table.emplace(f->name.buffer, val);
				if (!i.second) {
					throw_specific_error(f->name, "Member with the same name already existed in the structure");
				}
			}
			else if (auto v = dynamic_cast<VariableDeclaration*>(it->get())) {
				std::tuple<Declaration*, unsigned int, std::string_view> val = std::make_tuple(it->get(), lookup_id++, "");
				
				auto i = lookup_table.emplace(v->name.buffer, val);
				if (!i.second) {
					throw_specific_error(v->name, "Member with the same name already existed in the structure");
				}
			}
		}

		for (auto it = aliases.begin(); it != aliases.end(); it++) {
			Cursor a_nm = it->first;
			Cursor a_fm = it->second;
			auto look = lookup_table.find(a_fm.buffer);
			if (look == lookup_table.end()) {
				throw_specific_error(a_fm, "Member with this name was not declared in this structure");
			}
			else {
				Declaration* alias_var = std::get<0>(look->second);
				if (auto v = dynamic_cast<VariableDeclaration*>(alias_var)) {
					const Type* alias_var_type = v->type;
					StructDeclaration* alias_struct = nullptr;
					
					if (auto pt = dynamic_cast<const PrimitiveType*>(alias_var_type)) {
						alias_struct = pt->structure;
					}
					else {
						throw_specific_error(a_fm, "Alias points to variable with type that cannot be aliased");
					}

					alias_struct->build_lookup_table();

					if (a_nm.buffer.empty()) {
						for (auto m_it = alias_struct->lookup_table.begin(); m_it != alias_struct->lookup_table.end(); m_it++) {
							std::tuple<Declaration*, unsigned int, std::string_view> val = std::make_tuple(alias_struct, std::get<1>(look->second), m_it->first);
							lookup_table.emplace((std::string_view)m_it->first,val);
						}
					}
					else {
						auto m_it = alias_struct->lookup_table.find(a_nm.buffer);
						if (m_it == alias_struct->lookup_table.end()) {
							throw_specific_error(a_nm, "Member with this name does not exist in the aliased structure");
						}
						else {
							std::tuple<Declaration*, unsigned int, std::string_view> val = std::make_tuple(alias_struct, std::get<1>(look->second), m_it->first);
							auto emp = lookup_table.emplace((std::string_view)m_it->first, val);
							if (!emp.second) {
								throw_specific_error(a_nm, "Member with the same name already exists in the structure");
							}
						}
					}
				}
				else {
					throw_specific_error(a_fm, "Alias points to function");
				}
			}
		}
	}

	void FunctionDeclaration::pre_compile(CompileContext& ctx) {
		if (type->irtype!=nullptr) return;

		CompileContext nctx = ctx;
		nctx.parent_struct = parent_struct();
		nctx.parent_namespace = parent_pack;

		Type::resolve_package_in_place(type, nctx);
		type->pre_compile(nctx);
	}

	void FunctionDeclaration::compile(CompileContext& ctx) {
		if (compile_progress == 0) {
			pre_compile(ctx);

			compile_progress = 1;

			CompileContext nctx = ctx;
			nctx.parent_struct = parent_struct();
			nctx.parent_namespace = parent_pack;

			type->compile(nctx);


			std::string f_name = "f.";
			if (!package.empty()) {
				f_name.append(package);
				f_name.append(".");
			}
			f_name.append(name.buffer);

			unsigned long stack = StackManager::stack_state();

			function = ctx.module->create_function(((const FunctionType*)type)->returns->irtype);
			IRBlock* entry = function->create_block(IRDataType::none);
			function->append_block(entry);
			IRBuilder::build_discard(entry);

			const FunctionType* ft = (const FunctionType*)type;
			bool heavy_return = ft->returns->is_heavy;

			for (int i = 0; i < ft->arguments->size(); i++) {
				CompileValue cv;
				cv.lvalue = true;
				cv.t = (*ft->arguments)[i];
				unsigned int argloc = function->register_local(cv.t->irtype);
				StackManager::stack_push(argnames[i].buffer,cv, argloc);
			}


			Cursor c = block;
			Corrosive::CompileContextExt cctx;
			cctx.function = function;
			cctx.unit = this;
			cctx.basic.module = ctx.module;
			cctx.basic.parent_namespace = parent_pack;
			cctx.basic.parent_struct = nullptr;
			cctx.basic.template_ctx = nullptr;
			cctx.block = entry;

			Corrosive::CompileValue cv = Expression::parse(c, cctx, Corrosive::CompileType::compile);
			IRBuilder::build_yield(cctx.block);
			IRBuilder::build_ret(cctx.block);

			StackManager::stack_restore(stack);
			compile_progress = 2;
			return;
		}
		else if (compile_progress == 2) {
			return;
		}
		else {
			throw_specific_error(name, "This function caused build cycle");
		}

		return;
	}

	void VariableDeclaration::pre_compile(CompileContext& ctx) {
		if (type->irtype != nullptr) return;

		CompileContext nctx = ctx;
		nctx.parent_struct = parent_struct();
		nctx.parent_namespace = parent_pack;
		Type::resolve_package_in_place(type, nctx);
		type->pre_compile(nctx);
	}

	void VariableDeclaration::compile(CompileContext& ctx) {
		if (compile_progress == 0) {
			pre_compile(ctx);

			compile_progress = 1;

			CompileContext nctx = ctx;
			nctx.parent_struct = parent_struct();
			nctx.parent_namespace = parent_pack;
			type->compile(nctx);

			compile_progress = 2;
			return;
		}
		else if (compile_progress == 2) {
			return;
		}
		else {
			throw_specific_error(name, "This variable caused build cycle");
		}

		return;
	}

	void Declaration::pre_compile(CompileContext& ctx) {
		return;
	}
	void Declaration::compile(CompileContext& ctx) {
		return;
	}

}