#include "Declaration.h"
#include "Error.h"
#include <iostream>
#include "Contents.h"
#include <string>
#include "PredefinedTypes.h"


namespace Corrosive {
	Declaration::~Declaration() {}


	StructDeclaration* Declaration::parent_struct() const {
		return dynamic_cast<StructDeclaration*>(parent);
	}

	std::unique_ptr<Declaration> Declaration::clone() {
		return nullptr;
	}

	std::unique_ptr<Declaration> VariableDeclaration::clone() {
		std::unique_ptr<VariableDeclaration> d = std::make_unique<VariableDeclaration>();
		*d = *this;
		return std::move(d);
	}

	std::unique_ptr<Declaration> FunctionDeclaration::clone() {
		std::unique_ptr<FunctionDeclaration> d = std::make_unique<FunctionDeclaration>();
		*d = *this;
		return std::move(d);
	}

	bool GenericStructDeclaration::create_template(CompileContext& ctx, StructDeclaration*& into) {
		
		auto it = generated.find(ctx.template_ctx);
		if (it != generated.end()) {
			into = it->second.get();
			return true;
		}
		else {
			std::unique_ptr<StructDeclaration> sd = std::make_unique<StructDeclaration>();

			sd->template_ctx = ctx.template_ctx;

			sd->name = name;
			sd->package = package;
			sd->parent = parent;
			sd->is_trait = is_trait;

			sd->decl_type = decl_type;
			sd->template_ctx = ctx.template_ctx;

			sd->aliases = aliases;

			for (int i = 0; i < implements.size(); i++) {
				const Corrosive::Type* nex = implements[i].second;
				CompileContext nctx = ctx;
				nctx.parent_struct = implements[i].first;
				nctx.parent_namespace = implements[i].first->parent_pack;
				bool tmp;
				if (!Type::resolve_package_in_place(nctx, nex,tmp)) return false;
				sd->implements.push_back(std::make_pair(implements[i].first,std::move(nex)));
			}

			sd->members = std::vector<std::unique_ptr<Declaration>>(members.size());
			for (int i = 0; i < members.size(); i++) {
				sd->members[i] = members[i]->clone();

				if (auto vd = dynamic_cast<VariableDeclaration*>(sd->members[i].get())) {
					CompileContext nctx = ctx;
					nctx.parent_struct = vd->parent_struct();
					nctx.parent_namespace = vd->parent_pack;

					bool tmp;
					if (!Type::resolve_package_in_place(nctx, vd->type,tmp)) false;
				}else if (auto fd = dynamic_cast<FunctionDeclaration*>(sd->members[i].get())) {
					CompileContext nctx = ctx;
					nctx.parent_struct = fd->parent_struct();
					nctx.parent_namespace = fd->parent_pack;
					bool tmp;

					if (!Type::resolve_package_in_place(nctx, fd->type, tmp)) return false;
				}
			}

			sd->gen_id = (int)generated.size()+1;
			
			auto ret = sd.get();
 			generated[ctx.template_ctx] = std::move(sd);
			into = ret;
			return true;
		}
	}


	Declaration* StructDeclaration::FindDeclarationOfMember(std::string_view name) {
		auto d = lookup_table.find(name);
		if (d == lookup_table.end()) {
			return nullptr;
		}
		else {
			Declaration* decl = std::get<0>(d->second);
			if (auto sdecl = dynamic_cast<StructDeclaration*>(decl)) {
				return sdecl->FindDeclarationOfMember(name);
			}
			else {
				return decl;
			}
		}
	}


	void Declaration::print(unsigned int offset) const {

	}

	void VariableDeclaration::print(unsigned int offset) const {
		for (unsigned int i = 0; i < offset; i++) std::cout << "\t";
		std::cout << "var " << name.buffer << " : ";
		type->print();
		std::cout << std::endl;
	}

	void TypedefDeclaration::print(unsigned int offset) const {
		for (unsigned int i = 0; i < offset; i++) std::cout << "\t";

		std::cout << "type " << name.buffer << " : ";
		type->print();
		std::cout << std::endl;
	}

	void FunctionDeclaration::print(unsigned int offset) const {

		for (unsigned int i = 0; i < offset; i++) std::cout << "\t";
		std::cout << "function ";

		if (is_static) {
			std::cout << "static ";
		}

		std::cout << name.buffer;

		std::cout << " : ";

		type->print();
		std::cout << std::endl;
	}

	void GenericFunctionDeclaration::print(unsigned int offset) const {

		for (unsigned int i = 0; i < offset; i++) std::cout << "\t";
		std::cout << "generic function ";

		std::cout << name.buffer;

		std::cout << "<";

		int i = 0;
		for (auto it = generic_typenames.begin(); it != generic_typenames.end();it++) {
			if (i++ != 0) std::cout << ", ";
			std::cout << it->first;
		}

		std::cout << "> : ";

		type->print();
		std::cout << std::endl;
	}

	void StructDeclaration::print(unsigned int offset) const {

		for (unsigned int i = 0; i < offset; i++) std::cout << "\t";

		if (is_trait)
			std::cout << "class ";
		else
			std::cout << "struct ";

		if (gen_id > 0) {
			std::cout << gen_id << " ";
		}

		std::cout<< name.buffer;
		if (implements.size() > 0) {
			std::cout << " : ";
			for (int i = 0; i < implements.size(); i++) {
				if (i != 0) std::cout << ", ";

				implements[i].second->print();
			}
		}

		std::cout << " {" <<std::endl;
		for (auto it = members.begin(); it != members.end(); it++) {
			it->get()->print(offset+1);
		}

		for (unsigned int i = 0; i < offset; i++) std::cout << "\t";
		std::cout << "}" << std::endl << std::endl;
	}

	void GenericStructDeclaration::print(unsigned int offset) const {

		for (unsigned int i = 0; i < offset; i++) std::cout << "\t";
		if (is_trait)
			std::cout << "generic class ";
		else
			std::cout << "generic struct ";
		

		std::cout << name.buffer << " {" << std::endl;
		for (auto it = generated.begin(); it != generated.end(); it++) {
			it->second->print(offset+1);
		}

		for (unsigned int i = 0; i < offset; i++) std::cout << "\t";
		std::cout << "}" << std::endl << std::endl;
	}

	

	void NamespaceDeclaration::print(unsigned int offset) const {

		for (unsigned int i = 0; i < offset; i++) std::cout << "\t";
		std::cout << "namespace " << name.buffer << " {" << std::endl << std::endl;
		
		for (auto it = members.begin(); it != members.end(); it++) {
			it->get()->print(offset+1);
		}

		for (unsigned int i = 0; i < offset; i++) std::cout << "\t";
		std::cout << "}" << std::endl << std::endl;
	}


	bool StructDeclaration::is_generic() { return false; }
	bool GenericStructDeclaration::is_generic() { return true; }

	bool TypedefDeclaration::resolve_type(const Type*& into) {
		if (resolve_progress == 0) {
			resolve_progress = 1;
			CompileContext nctx;
			nctx.parent_namespace = parent_pack;
			nctx.parent_struct = parent_struct();
			nctx.template_ctx = nullptr;
			bool tmp;
			if (!Type::resolve_package_in_place(nctx, type,tmp)) return false;
			resolve_progress = 2;
			into = type;
			return true;
		}
		else if (resolve_progress == 2) {
			into = type;
			return true;
		}
		else {
			throw_specific_error(name, "This typedef caused a build cycle");
			return false;
		}

		into = nullptr;
		return true;
	}
}