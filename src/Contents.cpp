#include "Contents.h"
#include <iostream>

#include "Type.h"

namespace Corrosive {
	std::unordered_map<std::string_view, std::unique_ptr<std::unordered_map<std::string_view, StructDeclaration*>>> Contents::NamespaceStruct;
	std::unordered_map<std::string_view, std::unique_ptr<std::unordered_map<std::string_view, TypedefDeclaration*>>> Contents::NamespaceTypedef;


	std::vector<StructDeclaration*> Contents::StaticStructures;


	std::unordered_set<const Type*, Contents::TypeHash, Contents::TypeCompare> Contents::AllTypes(8000);
	std::unordered_set<const std::vector<const Type*>*, Contents::TypeArrayHash, Contents::TypeArrayCompare> Contents::TypeArrays(1000);


	std::unordered_set<const std::vector<std::variant<unsigned int, const Type*>>*, Contents::GenericArrayHash, Contents::GenericArrayCompare> Contents::GenericArrays(1000);


	FunctionDeclaration* Contents::entry_point = nullptr;

	const std::vector<const Type*>* Contents::RegisterTypeArray(std::vector<const Type*>&& arr) {
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


	const std::vector<std::variant<unsigned int, const Type*>>* Contents::RegisterGenericArray(std::vector<std::variant<unsigned int, const Type*>>&& arr) {
		auto f = GenericArrays.find(&arr);
		if (f != GenericArrays.end()) {
			return (*f);
		}
		else {
			const std::vector<std::variant<unsigned int, const Type*>>* nt = new std::vector<std::variant<unsigned int, const Type*>>(std::move(arr));
			auto r = GenericArrays.insert(nt);
			return nt;
		}
	}

	void Contents::RegisterNamespace(std::string_view p) {
		
		if (NamespaceStruct.find(p) == NamespaceStruct.end()) {
			std::unique_ptr<std::unordered_map<std::string_view, StructDeclaration*>> nspc_s_map = std::make_unique<std::unordered_map<std::string_view, StructDeclaration*>>();
			NamespaceStruct[p] = std::move(nspc_s_map);

			std::unique_ptr<std::unordered_map<std::string_view, TypedefDeclaration*>> nspc_t_map = std::make_unique<std::unordered_map<std::string_view, TypedefDeclaration*>>();
			NamespaceTypedef[p] = std::move(nspc_t_map);
		}
	}

	void Contents::RegisterStruct(std::string_view p, std::string_view n, StructDeclaration* s) {
		std::unordered_map<std::string_view, StructDeclaration*>* nspc_map = NamespaceStruct[p].get();
		(*nspc_map)[n] = s;
	}

	StructDeclaration* Contents::FindStruct(std::string_view p, std::string_view n) {
		if (NamespaceStruct.find(p) != NamespaceStruct.end()) {
			std::unordered_map<std::string_view, StructDeclaration*>* nspc_map = NamespaceStruct[p].get();
			if (nspc_map->find(n) != nspc_map->end()) {
				return (*nspc_map)[n];
			}
		}
		return nullptr;
	}

	void Contents::RegisterTypedef(std::string_view p, std::string_view n, TypedefDeclaration* s) {
		std::unordered_map<std::string_view, TypedefDeclaration*>* nspc_map = NamespaceTypedef[p].get();
		(*nspc_map)[n] = s;
	}

	TypedefDeclaration* Contents::FindTypedef(std::string_view p, std::string_view n) {
		if (NamespaceTypedef.find(p) != NamespaceTypedef.end()) {
			std::unordered_map<std::string_view, TypedefDeclaration*>* nspc_map = NamespaceTypedef[p].get();
			if (nspc_map->find(n) != nspc_map->end()) {
				return (*nspc_map)[n];
			}
		}
		return nullptr;
	}


	LLVMTargetDataRef Contents::TargetData;
}