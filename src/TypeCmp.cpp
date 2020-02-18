#include "Type.h"

namespace Corrosive {

	static inline size_t rot(size_t n, int c)
	{
		const unsigned int mask = (CHAR_BIT * sizeof(n) - 1);
		c &= mask;
		return (n >> c) | (n << ((-c) & mask));
	}

	int Type::Cmp(const Type& t2) const {
		if (ID() < t2.ID()) return -1;
		if (ID() > t2.ID()) return 1;
		if (Ref() < t2.Ref()) return -1;
		if (Ref() > t2.Ref()) return 1;
		return 0;
	}

	size_t Type::Hash() const {
		return std::hash<int>()(ID()) ^ rot(std::hash<int>()(Ref()), 1);
	}

	int PrimitiveType::Cmp(const Type& t2) const {
		int tcmp = Type::Cmp(t2);
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

	size_t PrimitiveType::Hash() const {
		size_t h = Type::Hash();
		h ^= rot(std::hash<std::string_view>()(Name().Data()), 2) ^ rot(std::hash<std::string_view>()(Pack()), 3);
		h ^= rot(std::hash<size_t>()((size_t)Templates()), 4);
		return h;
	}

	int FunctionType::Cmp(const Type& t2) const {
		int tcmp = Type::Cmp(t2);
		if (tcmp != 0) return tcmp;

		FunctionType& ft = (FunctionType&)t2;
		if (returns < ft.returns) return -1;
		if (returns > ft.returns) return 1;

		if (arguments < ft.arguments) return -1;
		if (arguments > ft.arguments) return 1;

		return 0;
	}

	size_t FunctionType::Hash() const {
		size_t h = Type::Hash();
		h ^= rot(std::hash<size_t>()((size_t)Returns()), 5);
		h ^= rot(std::hash<size_t>()((size_t)Args()), 6);

		return h;
	}

	int TupleType::Cmp(const Corrosive::Type& t2) const {
		int tcmp = Type::Cmp(t2);
		if (tcmp != 0) return tcmp;

		TupleType& ft = (TupleType&)t2;
		if (Types() < ft.Types()) return -1;
		if (Types() > ft.Types()) return 1;

		return 0;
	}

	size_t TupleType::Hash() const {
		size_t h = Type::Hash();
		h ^= rot(std::hash<size_t>()((size_t)Types()), 7);
		return h;
	}


	int InterfaceType::Cmp(const Corrosive::Type& t2) const {
		int tcmp = Type::Cmp(t2);
		if (tcmp != 0) return tcmp;

		InterfaceType& ft = (InterfaceType&)t2;
		if (Types() < ft.Types()) return -1;
		if (Types() > ft.Types()) return 1;
		return 0;
	}

	size_t InterfaceType::Hash() const {
		size_t h = Type::Hash();

		h ^= rot(std::hash<size_t>()((size_t)Types()), 8);
		return h;
	}

	int ArrayType::Cmp(const Type& t2) const {
		int tcmp = Type::Cmp(t2);
		if (tcmp != 0) return tcmp;


		ArrayType& ft = (ArrayType&)t2;
		if (base < ft.base) return -1;
		if (base > ft.base) return 1;

		if (size.Data() < ft.size.Data()) return -1;
		if (size.Data() > ft.size.Data()) return 1;

		return 0;
	}


	size_t ArrayType::Hash() const {
		size_t h = Type::Hash();
		h ^= rot(std::hash<size_t>()((size_t)Base()), 9);
		h ^= rot(std::hash<std::string_view>()(Size().Data()), 10);
		return h;
	}

}

namespace std {
	size_t hash<Corrosive::Type>::operator()(const Corrosive::Type& t) const { return t.Hash(); }
}