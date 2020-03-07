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

	bool StructDeclaration::pre_compile(CompileContext& ctx) {
		if (iltype != nullptr) return true;

		Contents::StaticStructures.push_back(this);

		if (decl_type == StructDeclarationType::t_i64)
			iltype = ctx.module->t_i64;
		else if (decl_type == StructDeclarationType::t_u64)
			iltype = ctx.module->t_u64;
		else if (decl_type == StructDeclarationType::t_i32)
			iltype = ctx.module->t_i32;
		else if (decl_type == StructDeclarationType::t_u32)
			iltype = ctx.module->t_u32;
		else if (decl_type == StructDeclarationType::t_i16)
			iltype = ctx.module->t_i16;
		else if (decl_type == StructDeclarationType::t_u16)
			iltype = ctx.module->t_u16;
		else if (decl_type == StructDeclarationType::t_i8)
			iltype = ctx.module->t_i8;
		else if (decl_type == StructDeclarationType::t_u8)
			iltype = ctx.module->t_u8;
		else if (decl_type == StructDeclarationType::t_bool)
			iltype = ctx.module->t_bool;
		else if (decl_type == StructDeclarationType::t_ptr)
			iltype = ctx.module->t_ptr;
		else if (decl_type == StructDeclarationType::t_f32)
			iltype = ctx.module->t_f32;
		else if (decl_type == StructDeclarationType::t_f64)
			iltype = ctx.module->t_f64;
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
				bool tmp;
				if (!Type::resolve_package_in_place(ctx, ext,tmp)) return false;

				if (auto exttype = dynamic_cast<const PrimitiveType*>(ext)) {
					if (exttype->templates == nullptr) {
						if (exttype->ref) {
							throw_specific_error(name, "Structure cannot extend references");
							return false;
						}


						std::pair<std::string_view, std::string_view> key = std::make_pair(exttype->package, exttype->name.buffer);

						if (auto fs = exttype->structure) {
							if (!fs->pre_compile(ctx)) return false;
							implements_structures.push_back(fs);
						}
						else {
							throw_specific_error(name, "Extended structure was not found in any package from the lookup queue");
							return false;
						}
					}
					else {
						if (auto fs = exttype->structure) {
							if (!fs->is_generic()) {
								throw_specific_error(exttype->name, "Target structure is not generic");
								return false;
							}
							GenericStructDeclaration* gsd = (GenericStructDeclaration*)fs;
							CompileContext nctx = ctx;
							nctx.template_ctx = exttype->templates;

							StructDeclaration* gen;
							if (!gsd->create_template(nctx, gen)) return false;

							if (!gen->pre_compile(nctx)) return false;
							implements_structures.push_back(gen);

						}
						else {
							throw_specific_error(name, "Extended structure was not found in any package from the lookup queue");
							return false;
						}
					}

				}
				else {
					throw_specific_error(name, "Structure cannot extend non-structural type");
					return false;
				}
			}

			for (auto it = implements_structures.begin(); it != implements_structures.end(); it++) {
				if (!(*it)->is_trait) {
					throw_specific_error(name, "All extended types needs to be classes");
					return false;
				}
				if (!(*it)->pre_compile(ctx)) return false;
			}

			iltype = ctx.module->create_struct_type();

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
					bool tmp;
					if (!Type::resolve_package_in_place(nctx, fdecl->type,tmp)) return false;

					Cursor thisc; thisc.buffer = "this";
					fdecl->argnames.insert(fdecl->argnames.begin(), thisc);
				}

				if (!decl->pre_compile(ctx)) return false;

				if (vdecl = dynamic_cast<VariableDeclaration*>(decl.get())) {
					if (!is_trait)
						((ILStruct*)iltype)->add_member(vdecl->type->iltype);
					else {
						throw_specific_error(vdecl->name, "variable found in trait type");
						return false;
					}
				}
				else if (fdecl != nullptr) {
					if (is_trait)
						((ILStruct*)iltype)->add_member(ctx.module->t_ptr);
				}
			}

			((ILStruct*)iltype)->align_size();

			if (!build_lookup_table()) return false;
			if (!test_interface_complete()) return false;
		}

		return true;
	}

	bool StructDeclaration::compile(CompileContext& ctx) {
		if (compile_progress == 0) {
			if (!pre_compile(ctx)) return false;

			compile_progress = 1;

			for (auto it = implements_structures.begin(); it != implements_structures.end(); it++) {
				if (!(*it)->compile(ctx)) return false;
			}

			for (int i = 0; i < members.size(); i++) {
				std::unique_ptr<Declaration>& decl = members[i];

				if (auto vdecl = dynamic_cast<VariableDeclaration*>(decl.get())) {
					if (!vdecl->type->ref)
						if (!vdecl->compile(ctx)) return false;
				}
			}

			compile_progress = 2;

			return true;
		}
		else if (compile_progress == 2) {
			return true;
		}
		else {
			throw_specific_error(name, "This structure caused build cycle");
			return false;
		}

	}


	bool StructDeclaration::test_interface_complete() {
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
							return false;
						}
						if (nifc->second->is_static != f->is_static) {
							throw_specific_error(name, "Structure has two interfaces with trait function");
							return false;
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
						return false;
					}
					iface_list.erase(nifc);
				}
			}
		}

		for (auto it = iface_list.begin(); it != iface_list.end(); it++) {
			if (!it->second->has_block) {
				throw_specific_error(name, "Structure lacks some functions from its interfaces");
				return false;
			}
		}

		return true;
	}


	bool StructDeclaration::build_lookup_table() {
		if (has_lookup_table) return true;
		has_lookup_table = true;

		unsigned int lookup_id = 0;

		for (auto it = members.begin(); it != members.end(); it++) {
			if (auto f = dynamic_cast<FunctionDeclaration*>(it->get())) {
				std::tuple<Declaration*, unsigned int, std::string_view> val = std::make_tuple(it->get(), 0,"");
				
				auto i = lookup_table.emplace(f->name.buffer, val);
				if (!i.second) {
					throw_specific_error(f->name, "Member with the same name already existed in the structure");
					return false;
				}
			}
			else if (auto v = dynamic_cast<VariableDeclaration*>(it->get())) {
				std::tuple<Declaration*, unsigned int, std::string_view> val = std::make_tuple(it->get(), lookup_id++, "");
				
				auto i = lookup_table.emplace(v->name.buffer, val);
				if (!i.second) {
					throw_specific_error(v->name, "Member with the same name already existed in the structure");
					return false;
				}
			}
		}

		for (auto it = aliases.begin(); it != aliases.end(); it++) {
			Cursor a_nm = it->first;
			Cursor a_fm = it->second;
			auto look = lookup_table.find(a_fm.buffer);
			if (look == lookup_table.end()) {
				throw_specific_error(a_fm, "Member with this name was not declared in this structure");
				return false;
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
						return false;
					}

					if (!alias_struct->build_lookup_table()) return false;

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
							return false;
						}
						else {
							std::tuple<Declaration*, unsigned int, std::string_view> val = std::make_tuple(alias_struct, std::get<1>(look->second), m_it->first);
							auto emp = lookup_table.emplace((std::string_view)m_it->first, val);
							if (!emp.second) {
								throw_specific_error(a_nm, "Member with the same name already exists in the structure");
								return false;
							}
						}
					}
				}
				else {
					throw_specific_error(a_fm, "Alias points to function");
					return false;
				}
			}
		}

		return true;
	}

	bool FunctionDeclaration::pre_compile(CompileContext& ctx) {
		if (type->iltype!=nullptr) return true;

		CompileContext nctx = ctx;
		nctx.parent_struct = parent_struct();
		nctx.parent_namespace = parent_pack;
		bool tmp;
		if (!Type::resolve_package_in_place(nctx,type,tmp)) return false;
		if (!type->pre_compile(nctx)) return false;

		return true;
	}

	bool FunctionDeclaration::compile(CompileContext& ctx) {
		if (compile_progress == 0) {
			if (!pre_compile(ctx)) return false;

			compile_progress = 1;

			CompileContext nctx = ctx;
			nctx.parent_struct = parent_struct();
			nctx.parent_namespace = parent_pack;

			if (!type->compile(nctx)) return false;


			std::string f_name = "f.";
			if (!package.empty()) {
				f_name.append(package);
				f_name.append(".");
			}
			f_name.append(name.buffer);

			unsigned long stack = StackManager::stack_state();

			function = ctx.module->create_function(((const FunctionType*)type)->returns->iltype);
			ILBlock* entry = function->create_block(ILDataType::none);
			function->append_block(entry);
			ILBuilder::build_discard(entry);

			const FunctionType* ft = (const FunctionType*)type;
			bool heavy_return = ft->returns->is_heavy;

			for (int i = 0; i < ft->arguments->size(); i++) {
				CompileValue cv;
				cv.lvalue = true;
				cv.t = (*ft->arguments)[i];
				unsigned int argloc = function->register_local(cv.t->iltype);
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

			Corrosive::CompileValue cv;
			if (!Expression::parse(c, cctx,cv, Corrosive::CompileType::compile)) return false;
			if (!ILBuilder::build_yield(cctx.block)) return false;
			if (!ILBuilder::build_ret(cctx.block)) return false;

			StackManager::stack_restore(stack);
			compile_progress = 2;
			return true;
		}
		else if (compile_progress == 2) {
			return true;
		}
		else {
			throw_specific_error(name, "This function caused build cycle");
			return false;
		}

	}

	bool VariableDeclaration::pre_compile(CompileContext& ctx) {
		if (type->iltype != nullptr) return true;

		CompileContext nctx = ctx;
		nctx.parent_struct = parent_struct();
		nctx.parent_namespace = parent_pack;
		bool tmp;
		if (!Type::resolve_package_in_place(nctx,type,tmp)) return false;
		if (!type->pre_compile(nctx)) return false;

		return true;
	}

	bool VariableDeclaration::compile(CompileContext& ctx) {
		if (compile_progress == 0) {
			pre_compile(ctx);

			compile_progress = 1;

			CompileContext nctx = ctx;
			nctx.parent_struct = parent_struct();
			nctx.parent_namespace = parent_pack;
			if (!type->compile(nctx)) return false;

			compile_progress = 2;
			return true;
		}
		else if (compile_progress == 2) {
			return true;
		}
		else {
			throw_specific_error(name, "This variable caused build cycle");
			return false;
		}
	}

	bool Declaration::pre_compile(CompileContext& ctx) {
		return false;
	}
	bool Declaration::compile(CompileContext& ctx) {
		return false;
	}

}