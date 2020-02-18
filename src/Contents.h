#pragma once
#ifndef _contents_crs_h
#define _contents_crs_h

#include "Declaration.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <llvm/Target.h>


namespace Corrosive {
	class Contents {
	public:

		class TypeHash {
		public:
			size_t operator()(const Type* const& t) const
			{
				return t->Hash();
			}
		};

		struct TypeCompare {
		public:
			bool operator() (const Type* const& t1, const Type* const& t2) const {
				return t1->Cmp(*t2) == 0;
			}
		};

		class TypeArrayHash {
		public:
			static inline size_t rot(size_t n, int c)
			{
				const unsigned int mask = (CHAR_BIT * sizeof(n) - 1);
				c &= mask;
				return (n >> c) | (n << ((-c) & mask));
			}

			size_t operator()(const std::vector<const Type*>* const& t) const
			{
				size_t h = 0;
				for (int i = 0; i < t->size(); i++) {
					h ^= rot((*t)[i]->Hash(), i);
				}
				return h;
			}
		};

		struct TypeArrayCompare {
		public:
			bool operator() (const std::vector<const Type*>* const& t1, const std::vector<const Type*>* const& t2) const {
				if (t1->size() != t2->size()) return false;
				for (int i = 0; i < t1->size(); i++) {
					if ((*t1)[i]->Cmp(*(*t2)[i]) != 0) return false;
				}
				return true;
			}
		};

		class GenericArrayHash {
		public:
			static inline size_t rot(size_t n, int c)
			{
				const unsigned int mask = (CHAR_BIT * sizeof(n) - 1);
				c &= mask;
				return (n >> c) | (n << ((-c) & mask));
			}

			size_t operator()(const std::vector<std::variant<unsigned int, const Type*>>* const& t) const
			{
				size_t h = 0;
				for (int i = 0; i < t->size(); i++) {
					if ((*t)[i].index() == 0) {
						h ^= rot(std::hash<unsigned int>()(std::get<0>((*t)[i])), i);
					}
					else {
						h ^= rot(std::hash<size_t>()((size_t)std::get<1>((*t)[i])), i);
					}
				}
				return h;
			}
		};

		struct GenericArrayCompare {
		public:
			bool operator() (const std::vector<std::variant<unsigned int, const Type*>>* const& t1, const std::vector<std::variant<unsigned int, const Type*>>* const& t2) const {
				if (t1->size() != t2->size()) return false;

				for (int i = 0; i < t1->size(); i++) {
					if ((*t1)[i].index() != (*t2)[i].index()) return false;

					if ((*t1)[i].index() == 0) {
						if (std::get<0>((*t1)[i]) != std::get<0>((*t2)[i])) return false;
					}
					else {
						if (std::get<1>((*t1)[i])->Cmp(*std::get<1>((*t2)[i])) != 0) return false;
					}
				}
				return true;
			}
		};

		static std::unordered_set<const Type*, TypeHash, TypeCompare> AllTypes;
		static std::unordered_set<const std::vector<const Type*> *, TypeArrayHash, TypeArrayCompare> TypeArrays;
		static std::unordered_set<const std::vector<std::variant<unsigned int, const Type*>>*, GenericArrayHash, GenericArrayCompare> GenericArrays;

		static const std::vector<const Type*>* RegisterTypeArray(std::vector<const Type*>&& arr);
		static const std::vector<std::variant<unsigned int, const Type*>>* RegisterGenericArray(std::vector<std::variant<unsigned int, const Type*>>&& arr);

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


		static std::unordered_map<std::string_view, std::unique_ptr<std::unordered_map<std::string_view, StructDeclaration*>>> NamespaceStruct;
		static std::unordered_map<std::string_view, std::unique_ptr<std::unordered_map<std::string_view, TypedefDeclaration*>>> NamespaceTypedef;

		static std::vector<StructDeclaration*> StaticStructures;

		static void RegisterNamespace(std::string_view);
		
		static void RegisterStruct(std::string_view,std::string_view, StructDeclaration*);
		static StructDeclaration* FindStruct(std::string_view, std::string_view);

		static void RegisterTypedef(std::string_view, std::string_view, TypedefDeclaration*);
		static TypedefDeclaration* FindTypedef(std::string_view, std::string_view);

		static LLVMTargetDataRef TargetData;
	};
}

#endif
