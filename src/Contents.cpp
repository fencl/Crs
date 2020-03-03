#include "Contents.h"
#include <iostream>
#include "Utilities.h"
#include "Type.h"

namespace Corrosive {
	std::unordered_map<std::string_view, std::unique_ptr<std::unordered_map<std::string_view, StructDeclaration*>>> Contents::NamespaceStruct;
	std::unordered_map<std::string_view, std::unique_ptr<std::unordered_map<std::string_view, TypedefDeclaration*>>> Contents::NamespaceTypedef;


	std::vector<StructDeclaration*> Contents::StaticStructures;


	std::unordered_set<const Type*, Contents::TypeHash, Contents::TypeCompare> Contents::AllTypes(8000);
	std::unordered_set<const std::vector<const Type*>*, Contents::TypeArrayHash, Contents::TypeArrayCompare> Contents::TypeArrays(1000);


	std::unordered_set<const TemplateContext*, Contents::GenericArrayHash, Contents::GenericArrayCompare> Contents::GenericArrays(1000);


	FunctionDeclaration* Contents::entry_point = nullptr;


	
	size_t Contents::TypeHash::operator()(const Type* const& t) const
	{
		return t->hash();
	}
	
	bool Contents::TypeCompare::operator() (const Type* const& t1, const Type* const& t2) const {
		return t1->cmp(*t2) == 0;
	}

	size_t Contents::TypeArrayHash::operator()(const std::vector<const Type*>* const& t) const
	{
		size_t h = 0;
		for (int i = 0; i < t->size(); i++) {
			h ^= rot((*t)[i]->hash(), i);
		}
		return h;
	}


	bool Contents::TypeArrayCompare::operator() (const std::vector<const Type*>* const& t1, const std::vector<const Type*>* const& t2) const {
		if (t1->size() != t2->size()) return false;
		for (int i = 0; i < t1->size(); i++) {
			if ((*t1)[i]->cmp(*(*t2)[i]) != 0) return false;
		}
		return true;
	}
	
	size_t Contents::GenericArrayHash::operator()(const TemplateContext* const& t) const
	{
		size_t h = 0;
		for (int i = 0; i < t->size(); i++) {
			h ^= rot(std::hash<size_t>()((size_t)(*t)[i]), i);
		}
		return h;
	}
	

	bool Contents::GenericArrayCompare::operator() (const TemplateContext* const& t1, const TemplateContext* const& t2) const {
		if (t1->size() != t2->size()) return false;

		for (int i = 0; i < t1->size(); i++) {
			if ((*t1)[i]->cmp(*(*t2)[i]) != 0) return false;

		}
		return true;
	}
	


	const std::vector<const Type*>* Contents::register_type_array(std::vector<const Type*>&& arr) {
		auto f = TypeArrays.find(&arr);
		if (f != TypeArrays.end()) {
			return (*f);
		}
		else {
			const std::vector<const Type*>* nt = new std::vector<const Type*>(std::move(arr));
			auto r = TypeArrays.insert(nt);
			return nt;
		}
	}


	const TemplateContext* Contents::register_generic_array(TemplateContext&& arr) {
		auto f = GenericArrays.find(&arr);
		if (f != GenericArrays.end()) {
			return (*f);
		}
		else {
			const TemplateContext* nt = new TemplateContext(std::move(arr));
			auto r = GenericArrays.insert(nt);
			return nt;
		}
	}

	void Contents::register_namespace(std::string_view p) {
		
		if (NamespaceStruct.find(p) == NamespaceStruct.end()) {
			std::unique_ptr<std::unordered_map<std::string_view, StructDeclaration*>> nspc_s_map = std::make_unique<std::unordered_map<std::string_view, StructDeclaration*>>();
			NamespaceStruct[p] = std::move(nspc_s_map);

			std::unique_ptr<std::unordered_map<std::string_view, TypedefDeclaration*>> nspc_t_map = std::make_unique<std::unordered_map<std::string_view, TypedefDeclaration*>>();
			NamespaceTypedef[p] = std::move(nspc_t_map);
		}
	}

	void Contents::register_struct(std::string_view p, std::string_view n, StructDeclaration* s) {
		std::unordered_map<std::string_view, StructDeclaration*>* nspc_map = NamespaceStruct[p].get();
		(*nspc_map)[n] = s;
	}

	StructDeclaration* Contents::find_struct(std::string_view p, std::string_view n) {
		if (NamespaceStruct.find(p) != NamespaceStruct.end()) {
			std::unordered_map<std::string_view, StructDeclaration*>* nspc_map = NamespaceStruct[p].get();
			if (nspc_map->find(n) != nspc_map->end()) {
				return (*nspc_map)[n];
			}
		}
		return nullptr;
	}

	void Contents::register_typedef(std::string_view p, std::string_view n, TypedefDeclaration* s) {
		std::unordered_map<std::string_view, TypedefDeclaration*>* nspc_map = NamespaceTypedef[p].get();
		(*nspc_map)[n] = s;
	}

	TypedefDeclaration* Contents::find_typedef(std::string_view p, std::string_view n) {
		if (NamespaceTypedef.find(p) != NamespaceTypedef.end()) {
			std::unordered_map<std::string_view, TypedefDeclaration*>* nspc_map = NamespaceTypedef[p].get();
			if (nspc_map->find(n) != nspc_map->end()) {
				return (*nspc_map)[n];
			}
		}
		return nullptr;
	}

}