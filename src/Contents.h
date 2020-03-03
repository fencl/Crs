#pragma once
#ifndef _contents_crs_h
#define _contents_crs_h

#include "Declaration.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace Corrosive {
	class Contents {
	public:

		class TypeHash {
		public:
			size_t operator()(const Type* const& t) const;
		};

		struct TypeCompare {
		public:
			bool operator() (const Type* const& t1, const Type* const& t2) const;
		};

		class TypeArrayHash {
		public:
			size_t operator()(const std::vector<const Type*>* const& t) const;
		};

		struct TypeArrayCompare {
		public:
			bool operator() (const std::vector<const Type*>* const& t1, const std::vector<const Type*>* const& t2) const;
		};

		class GenericArrayHash {
		public:
			size_t operator()(const TemplateContext* const& t) const;
		};

		struct GenericArrayCompare {
		public:
			bool operator() (const TemplateContext* const& t1, const TemplateContext* const& t2) const;
		};

		static std::unordered_set<const Type*, TypeHash, TypeCompare>									AllTypes;
		static std::unordered_set<const std::vector<const Type*> *, TypeArrayHash, TypeArrayCompare>	TypeArrays;
		static std::unordered_set<const TemplateContext*, GenericArrayHash, GenericArrayCompare>		GenericArrays;


		static FunctionDeclaration* entry_point;

		template<typename T>
		static const Corrosive::Type* EmplaceType(T &t) {
			auto f = AllTypes.find((const Type*)&t);
			if (f != AllTypes.end()) {
				return (*f);
			}
			else {
				const Corrosive::Type* nt = new T(std::move(t));
				auto r = AllTypes.insert(nt);
				return nt;
			}
		}


		static std::unordered_map<std::string_view, std::unique_ptr<std::unordered_map<std::string_view, StructDeclaration*>>>	NamespaceStruct;
		static std::unordered_map<std::string_view, std::unique_ptr<std::unordered_map<std::string_view, TypedefDeclaration*>>> NamespaceTypedef;

		static std::vector<StructDeclaration*> StaticStructures;


		static const std::vector<const Type*>*	register_type_array		(std::vector<const Type*>&& arr);
		static const TemplateContext*			register_generic_array	(TemplateContext&& arr);

		static void					register_namespace	(std::string_view);
		static void					register_struct		(std::string_view,std::string_view, StructDeclaration*);
		static StructDeclaration*	find_struct			(std::string_view, std::string_view);
		static void					register_typedef	(std::string_view, std::string_view, TypedefDeclaration*);
		static TypedefDeclaration*	find_typedef		(std::string_view, std::string_view);
	};
}

#endif
