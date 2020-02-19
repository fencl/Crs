#include "Declaration.h"
#include "Error.h"
#include <iostream>
#include "Contents.h"
#include <string>
#include "PredefinedTypes.h"


namespace Corrosive {
	Declaration::~Declaration() {}


	LLVMTypeRef StructDeclaration::LLVMType() { return llvm_type; }

	Cursor Declaration::Name() const { return name; }
	void Declaration::Name(Cursor c) { name = c; }

	std::string_view const Declaration::Pack() const { return package; }
	void Declaration::Pack(std::string_view p) { package = p; }

	const Declaration* Declaration::Parent() const {
		return parent;
	}
	
	void Declaration::Parent(Declaration* p) {
		parent = p;
	}

	NamespaceDeclaration* Declaration::ParentPack() const { return parent_pack; }
	void Declaration::ParentPack(NamespaceDeclaration* pp) { parent_pack = pp; }


	StructDeclaration* Declaration::ParentStruct() const {
		return dynamic_cast<StructDeclaration*>(parent);
	}

	bool StructDeclaration::Extending() { return isextending; }
	void StructDeclaration::Extending(bool e) { isextending = e; }

	
	const Corrosive::Type*& VariableDeclaration::Type() {
		return type;
	}

	void VariableDeclaration::Type(const Corrosive::Type* t) {
		type = t;
	}


	const Type*& TypedefDeclaration::Type() {
		return type;
	}

	void TypedefDeclaration::Type(const Corrosive::Type* t) {
		type = t;
	}

	bool FunctionDeclaration::Static() const { return isstatic; }
	void FunctionDeclaration::Static(bool b) { isstatic = b; }

	const std::vector<std::pair<StructDeclaration*, const Corrosive::Type*>>& StructDeclaration::Extends() const { return extends; }
	std::vector<std::pair<StructDeclaration*, const Corrosive::Type*>>& StructDeclaration::Extends() { return extends; }


	const Corrosive::Type*& FunctionDeclaration::Type() { return type; }

	void FunctionDeclaration::Type(const Corrosive::Type* t) {
		type = std::move(t);
	}

	bool FunctionDeclaration::HasBlock() const { return hasBlock; }
	void FunctionDeclaration::HasBlock(bool b) { hasBlock = b; }

	Cursor FunctionDeclaration::Block() const { return block; }
	void FunctionDeclaration::Block(Cursor c) { block = c; }


	void Declaration::Print(unsigned int offset) const {

	}

	void VariableDeclaration::Print(unsigned int offset) const {
		for (unsigned int i = 0; i < offset; i++) std::cout << "\t";
		std::cout << "var " << Name().Data() << " : ";
		type->Print();
		std::cout << std::endl;
	}

	void TypedefDeclaration::Print(unsigned int offset) const {
		for (unsigned int i = 0; i < offset; i++) std::cout << "\t";

		std::cout << "type " << Name().Data() << " : ";
		type->Print();
		std::cout << std::endl;
	}

	std::unique_ptr<Declaration> Declaration::Clone() {
		return nullptr;
	}

	std::unique_ptr<Declaration> VariableDeclaration::Clone() {
		std::unique_ptr<VariableDeclaration> d = std::make_unique<VariableDeclaration>();
		d->Name(name);
		d->Pack(package);
		d->Parent(parent);
		d->ParentPack(parent_pack);
		d->Type(type);

		return std::move(d);
	}

	std::unique_ptr<Declaration> FunctionDeclaration::Clone() {
		std::unique_ptr<FunctionDeclaration> d = std::make_unique<FunctionDeclaration>();
		d->Name(name);
		d->Pack(package);
		d->Parent(parent);
		d->ParentPack(parent_pack);
		d->Type(type);
		d->Static(isstatic);
		d->argnames = argnames;
		d->block = block;
		d->hasBlock = hasBlock;
		return std::move(d);
	}


	std::map<const TemplateContext*, std::unique_ptr<StructDeclaration>>& GenericStructDeclaration::Generated() { return generated; }
	const std::map<const TemplateContext*, std::unique_ptr<StructDeclaration>>& GenericStructDeclaration::Generated() const { return generated; }

	int StructDeclaration::GenID() const { return gen_id; }
	void StructDeclaration::GenID(int id) { gen_id = id; }


	void StructDeclaration::Template(const TemplateContext* tc) { template_ctx = tc; }
	const TemplateContext* StructDeclaration::Template() const { return template_ctx; }

	StructDeclaration* GenericStructDeclaration::CreateTemplate(CompileContext& ctx) {
		
		auto it = generated.find(ctx.template_ctx);
		if (it != generated.end()) {
			return it->second.get();
		}
		else {
			std::unique_ptr<StructDeclaration> sd = std::make_unique<StructDeclaration>();

			sd->Template(ctx.template_ctx);

			sd->Name(Name());
			sd->Pack(Pack());
			sd->Parent(parent);
			sd->Class(isClass);
			sd->DeclType(DeclType());
			sd->Template(ctx.template_ctx);

			sd->Aliases = Aliases;

			for (int i = 0; i < Extends().size(); i++) {
				const Corrosive::Type* nex = Extends()[i].second;
				CompileContext nctx = ctx;
				nctx.parent_struct = Extends()[i].first;
				nctx.parent_namespace = Extends()[i].first->ParentPack();

				Type::ResolvePackageInPlace(nex, nctx);
				sd->Extends().push_back(std::make_pair(Extends()[i].first,std::move(nex)));
			}

			sd->Members = std::vector<std::unique_ptr<Declaration>>(Members.size());
			for (int i = 0; i < Members.size(); i++) {
				sd->Members[i] = Members[i]->Clone();

				if (auto vd = dynamic_cast<VariableDeclaration*>(sd->Members[i].get())) {
					CompileContext nctx = ctx;
					nctx.parent_struct = vd->ParentStruct();
					nctx.parent_namespace = vd->ParentPack();

					Type::ResolvePackageInPlace(vd->Type(), nctx);
				}else if (auto fd = dynamic_cast<FunctionDeclaration*>(sd->Members[i].get())) {
					CompileContext nctx = ctx;
					nctx.parent_struct = fd->ParentStruct();
					nctx.parent_namespace = fd->ParentPack();

					Type::ResolvePackageInPlace(fd->Type(), nctx);
				}
			}
			sd->GenID((int)generated.size()+1);
			
			auto ret = sd.get();
 			generated[ctx.template_ctx] = std::move(sd);
			return ret;
			
		}

	}


	Declaration* StructDeclaration::FindDeclarationOfMember(std::string_view name) {
		auto d = LookupTable.find(name);
		if (d == LookupTable.end()) {
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

	std::map<std::string_view, int>& GenericFunctionDeclaration::Generics() { return generic_typenames; }
	const std::map<std::string_view, int>& GenericFunctionDeclaration::Generics() const { return generic_typenames; }

	std::map<std::string_view, int>& GenericStructDeclaration::Generics() { return generic_typenames; }
	const std::map<std::string_view, int>& GenericStructDeclaration::Generics() const { return generic_typenames; }


	void FunctionDeclaration::Print(unsigned int offset) const {

		for (unsigned int i = 0; i < offset; i++) std::cout << "\t";
		std::cout << "function ";

		if (isstatic) {
			std::cout << "static ";
		}

		std::cout << Name().Data();

		std::cout << " : ";

		type->Print();
		std::cout << std::endl;
	}

	void GenericFunctionDeclaration::Print(unsigned int offset) const {

		for (unsigned int i = 0; i < offset; i++) std::cout << "\t";
		std::cout << "generic function ";

		std::cout << Name().Data();

		std::cout << "<";

		int i = 0;
		for (auto it = Generics().begin(); it != Generics().end();it++) {
			if (i++ != 0) std::cout << ", ";
			std::cout << it->first;
		}

		std::cout << "> : ";

		type->Print();
		std::cout << std::endl;
	}

	void StructDeclaration::Print(unsigned int offset) const {

		for (unsigned int i = 0; i < offset; i++) std::cout << "\t";

		if (Class())
			std::cout << "class ";
		else
			std::cout << "struct ";

		if (GenID() > 0) {
			std::cout << GenID() << " ";
		}

		std::cout<< Name().Data();
		if (Extends().size() > 0) {
			std::cout << " : ";
			for (int i = 0; i < Extends().size(); i++) {
				if (i != 0) std::cout << ", ";

				Extends()[i].second->Print();
			}
		}

		std::cout << " {" <<std::endl;
		for (auto it = Members.begin(); it != Members.end(); it++) {
			it->get()->Print(offset+1);
		}

		for (unsigned int i = 0; i < offset; i++) std::cout << "\t";
		std::cout << "}" << std::endl << std::endl;
	}

	void GenericStructDeclaration::Print(unsigned int offset) const {

		for (unsigned int i = 0; i < offset; i++) std::cout << "\t";
		if (Class())
			std::cout << "generic class ";
		else
			std::cout << "generic struct ";
		

		std::cout << name.Data() << " {" << std::endl;
		for (auto it = generated.begin(); it != generated.end(); it++) {
			it->second->Print(offset+1);
		}

		for (unsigned int i = 0; i < offset; i++) std::cout << "\t";
		std::cout << "}" << std::endl << std::endl;
	}

	

	void NamespaceDeclaration::Print(unsigned int offset) const {

		for (unsigned int i = 0; i < offset; i++) std::cout << "\t";
		std::cout << "namespace " << Name().Data() << " {" << std::endl << std::endl;
		
		for (auto it = Members.begin(); it != Members.end(); it++) {
			it->get()->Print(offset+1);
		}

		for (unsigned int i = 0; i < offset; i++) std::cout << "\t";
		std::cout << "}" << std::endl << std::endl;
	}

	bool StructDeclaration::Class() const { return isClass; }
	void StructDeclaration::Class(bool c) { isClass = c; }


	StructDeclarationType StructDeclaration::DeclType() const { return decl_type; }
	void StructDeclaration::DeclType(StructDeclarationType t) { decl_type = t; }

	std::vector<Cursor>* FunctionDeclaration::Argnames() {
		return &argnames;
	}

	bool StructDeclaration::Generic() { return false; }
	bool GenericStructDeclaration::Generic() { return true; }

	const Type* TypedefDeclaration::ResolveType() {
		if (resolve_progress == 0) {
			resolve_progress = 1;
			CompileContext nctx;
			nctx.parent_namespace = ParentPack();
			nctx.parent_struct = ParentStruct();
			nctx.template_ctx = nullptr;

			Type::ResolvePackageInPlace(type, nctx);
			resolve_progress = 2;
			return type;
		}
		else if (resolve_progress == 2) {
			return type;
		}
		else {
			ThrowSpecificError(Name(), "This typedef caused a build cycle");
		}

		return nullptr;
	}
}