#include "Type.h"
#include "PredefinedTypes.h"

namespace Corrosive {

	bool Type::CanPrimCastInto(const Type* t) const {
		//ptr case
		const PrimitiveType* pt = dynamic_cast<const PrimitiveType*>(t);
		if (pt != nullptr && ref && pt->Name().Data() == "ptr" && pt->Pack() == PredefinedNamespace) return true;

		return false;
	}


	bool PrimitiveType::CanPrimCastInto(const Type* t) const {
		if (Type::CanPrimCastInto(t)) return true;

		// ptr case
		if (package == PredefinedNamespace && name.Data() == "ptr" && ref) return true;

		const PrimitiveType* pt = dynamic_cast<const PrimitiveType*>(t);
		if (pt == nullptr) return false;
		else {
			if (pt->ref != ref) return false;

			if (pt->package != package) return false;
			if (pt->name.Data() != name.Data()) return false;
			if (pt->templates != templates) return false;

			return true;
		}
	}


	bool FunctionType::CanPrimCastInto(const Type* t) const{
		if (Type::CanPrimCastInto(t)) return true;

		const FunctionType* ft = dynamic_cast<const FunctionType*>(t);
		if (ft == nullptr) return false;
		else {
			if (ft->ref != ref) return false;

			if (ft->Args()->size() != Args()->size()) return false;
			if (!Returns()->CanPrimCastInto(ft->Returns())) return false;

			for (int i = 0; i < Args()->size(); i++) {
				if (!(*Args())[i]->CanPrimCastInto((*ft->Args())[i])) return false;
			}
			return true;
		}
	}


	bool FunctionType::CanPrimCastIntoIgnoreThis(const Type* t) const {
		if (Type::CanPrimCastInto(t)) return true;

		const FunctionType* ft = dynamic_cast<const FunctionType*>(t);
		if (ft == nullptr) return false;
		else {
			if (ft->ref != ref) return false;

			if (ft->Args()->size() != Args()->size()) return false;
			if (!Returns()->CanPrimCastInto(ft->Returns())) return false;

			for (int i = 1; i < Args()->size(); i++) {
				if (!(*Args())[i]->CanPrimCastInto((*ft->Args())[i])) return false;
			}
			return true;
		}
	}

	bool ArrayType::CanPrimCastInto(const Type* t) const {
		if (Type::CanPrimCastInto(t)) return true;

		const ArrayType* at = dynamic_cast<const ArrayType*>(t);
		if (at == nullptr) return false;
		else {
			if (at->ref != ref) return false;
			if (at->Size().Data() != Size().Data()) return false;
			if (!Base()->CanPrimCastInto(at->Base())) return false;

			return true;
		}
	}

	bool TupleType::CanPrimCastInto(const Type* t) const {
		if (Type::CanPrimCastInto(t)) return true;

		const TupleType* tt = dynamic_cast<const TupleType*>(t);
		if (tt == nullptr) return false;
		else {
			if (tt->ref != ref) return false;
			if (tt->Types()->size() != Types()->size()) return false;

			for (int i = 1; i < Types()->size(); i++) {
				if (!(*Types())[i]->CanPrimCastInto((*tt->Types())[i])) return false;
			}

			return true;
		}
	}

	bool InterfaceType::CanPrimCastInto(const Type* t) const {
		if (Type::CanPrimCastInto(t)) return true;

		const InterfaceType* it = dynamic_cast<const InterfaceType*>(t);
		if (it == nullptr) return false;
		else {
			if (it->ref != ref) return false;

			if (it->Types()->size() != Types()->size()) return false;

			for (int i = 1; i < Types()->size(); i++) {
				if (!(*Types())[i]->CanPrimCastInto((*it->Types())[i])) return false;
			}
			return true;
		}
	}

}