#include "Type.h"
#include "PredefinedTypes.h"

namespace Corrosive {

	bool Type::can_simple_cast_into(const Type* t) const {
		//ptr case
		const PrimitiveType* pt = dynamic_cast<const PrimitiveType*>(t);
		if (pt != nullptr && ref && pt->name.buffer == "ptr" && pt->package == PredefinedNamespace) return true;

		return false;
	}


	bool PrimitiveType::can_simple_cast_into(const Type* t) const {
		if (Type::can_simple_cast_into(t)) return true;

		// ptr case
		if (package == PredefinedNamespace && name.buffer == "ptr" && ref) return true;

		const PrimitiveType* pt = dynamic_cast<const PrimitiveType*>(t);
		if (pt == nullptr) return false;
		else {
			if (pt->ref != ref) return false;

			if (pt->package != package) return false;
			if (pt->name.buffer != name.buffer) return false;
			if (pt->templates != templates) return false;

			return true;
		}
	}


	bool FunctionType::can_simple_cast_into(const Type* t) const{
		if (Type::can_simple_cast_into(t)) return true;

		const FunctionType* ft = dynamic_cast<const FunctionType*>(t);
		if (ft == nullptr) return false;
		else {
			if (ft->ref != ref) return false;

			if (ft->arguments->size() != arguments->size()) return false;
			if (!returns->can_simple_cast_into(ft->returns)) return false;

			for (int i = 0; i < arguments->size(); i++) {
				if (!(*arguments)[i]->can_simple_cast_into((*ft->arguments)[i])) return false;
			}
			return true;
		}
	}


	bool FunctionType::can_simple_cast_into_ignore_this(const Type* t) const {
		if (Type::can_simple_cast_into(t)) return true;

		const FunctionType* ft = dynamic_cast<const FunctionType*>(t);
		if (ft == nullptr) return false;
		else {
			if (ft->ref != ref) return false;

			if (ft->arguments->size() != arguments->size()) return false;
			if (!returns->can_simple_cast_into(ft->returns)) return false;

			for (int i = 1; i < arguments->size(); i++) {
				if (!(*arguments)[i]->can_simple_cast_into((*ft->arguments)[i])) return false;
			}
			return true;
		}
	}

	bool ArrayType::can_simple_cast_into(const Type* t) const {
		if (Type::can_simple_cast_into(t)) return true;

		const ArrayType* at = dynamic_cast<const ArrayType*>(t);
		if (at == nullptr) return false;
		else {
			if (at->ref != ref) return false;
			if (at->size != size) return false;
			if (!base->can_simple_cast_into(at->base)) return false;

			return true;
		}
	}

	bool TupleType::can_simple_cast_into(const Type* t) const {
		if (Type::can_simple_cast_into(t)) return true;

		const TupleType* tt = dynamic_cast<const TupleType*>(t);
		if (tt == nullptr) return false;
		else {
			if (tt->ref != ref) return false;
			if (tt->types->size() != types->size()) return false;

			for (int i = 1; i < types->size(); i++) {
				if (!(*types)[i]->can_simple_cast_into((*tt->types)[i])) return false;
			}

			return true;
		}
	}

	bool InterfaceType::can_simple_cast_into(const Type* t) const {
		if (Type::can_simple_cast_into(t)) return true;

		const InterfaceType* it = dynamic_cast<const InterfaceType*>(t);
		if (it == nullptr) return false;
		else {
			if (it->ref != ref) return false;

			if (it->types->size() != types->size()) return false;

			for (int i = 1; i < types->size(); i++) {
				if (!(*types)[i]->can_simple_cast_into((*it->types)[i])) return false;
			}
			return true;
		}
	}

}