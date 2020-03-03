#include "Type.h"
#include "Utilities.h"
#include <iostream>

namespace Corrosive {

	/*
	 *	Type :: cmp
	 *	compare two types
	 *
	 *	returns -1 for 'this is "smaller" type than t2' (ordering for maps)
	 *			 1 for 'this is "larger" type than t2' 
	 *			 0 for the same type
	 */


	int Type::cmp(const Type& t2) const {
		if (id() < t2.id()) return -1;
		if (id() > t2.id()) return 1;
		if (ref < t2.ref) return -1;
		if (ref > t2.ref) return 1;
		return 0;
	}

	int PrimitiveType::cmp(const Type& t2) const {
		int tcmp = Type::cmp(t2);
		if (tcmp != 0) return tcmp;

		PrimitiveType& pt = (PrimitiveType&)t2;

		if (name.Data() < pt.name.Data()) return -1;
		if (name.Data() > pt.name.Data()) return 1;
		if (package < pt.package) return -1;
		if (package > pt.package) return 1;

		if (templates < pt.templates) return -1;
		if (templates < pt.templates) return 1;
		return 0;
	}

	int FunctionType::cmp(const Type& t2) const {
		int tcmp = Type::cmp(t2);
		if (tcmp != 0) return tcmp;

		FunctionType& ft = (FunctionType&)t2;
		if (returns < ft.returns) return -1;
		if (returns > ft.returns) return 1;

		if (arguments < ft.arguments) return -1;
		if (arguments > ft.arguments) return 1;

		return 0;
	}

	int TupleType::cmp(const Corrosive::Type& t2) const {
		int tcmp = Type::cmp(t2);
		if (tcmp != 0) return tcmp;

		TupleType& ft = (TupleType&)t2;
		if (types < ft.types) return -1;
		if (types > ft.types) return 1;

		return 0;
	}

	int InterfaceType::cmp(const Corrosive::Type& t2) const {
		int tcmp = Type::cmp(t2);
		if (tcmp != 0) return tcmp;

		InterfaceType& ft = (InterfaceType&)t2;
		if (types < ft.types) return -1;
		if (types > ft.types) return 1;
		return 0;
	}

	int ArrayType::cmp(const Type& t2) const {
		int tcmp = Type::cmp(t2);
		if (tcmp != 0) return tcmp;


		ArrayType& ft = (ArrayType&)t2;
		if (base < ft.base) return -1;
		if (base > ft.base) return 1;

		if (actual_size < ft.actual_size) return -1;
		if (actual_size > ft.actual_size) return 1;

		if (actual_size == 0) {
			if (size.Data() < ft.size.Data()) return -1;
			if (size.Data() > ft.size.Data()) return 1;

			if (has_simple_size < ft.has_simple_size) return -1;
			if (has_simple_size > ft.has_simple_size) return 1;

			if (!has_simple_size) {
				if (size.Offset() < ft.size.Offset()) return -1;
				if (size.Offset() > ft.size.Offset()) return 1;
				if (size.Source() < ft.size.Source()) return -1;
				if (size.Source() > ft.size.Source()) return 1;
			}
		}

		return 0;
	}

	/*
	 *	Type :: hash
	 *	returns hash int of the type
	 *	used for unordered_map
	 */


	size_t Type::hash() const {
		return std::hash<int>()(id()) ^ rot(std::hash<int>()(ref), 1);
	}

	size_t PrimitiveType::hash() const {
		size_t h = Type::hash();
		h ^= rot(std::hash<std::string_view>()(name.Data()), 2) ^ rot(std::hash<std::string_view>()(package), 3);
		h ^= rot(std::hash<size_t>()((size_t)templates), 4);
		return h;
	}

	size_t FunctionType::hash() const {
		size_t h = Type::hash();
		h ^= rot(std::hash<size_t>()((size_t)returns), 5);
		h ^= rot(std::hash<size_t>()((size_t)arguments), 6);

		return h;
	}

	size_t TupleType::hash() const {
		size_t h = Type::hash();
		h ^= rot(std::hash<size_t>()((size_t)types), 7);
		return h;
	}

	size_t InterfaceType::hash() const {
		size_t h = Type::hash();

		h ^= rot(std::hash<size_t>()((size_t)types), 8);
		return h;
	}

	size_t ArrayType::hash() const {
		size_t h = Type::hash();
		h ^= rot(std::hash<size_t>()((size_t)base), 9);
		if (actual_size == 0) {
			if (has_simple_size) {
				h ^= rot(std::hash<std::string_view>()(size.Data()), 10);
			}
			else {
				h ^= rot(std::hash<std::string_view>()(size.Data()), 11) ^ rot(std::hash<size_t>()(size.Offset()), 12) ^ rot(std::hash<const void*>()(size.Source()), 13);
			}
		}
		else
			h ^= rot(std::hash<unsigned int>()(actual_size), 14);

		return h;
	}

}

namespace std {
	size_t hash<Corrosive::Type>::operator()(const Corrosive::Type& t) const { return t.hash(); }
}