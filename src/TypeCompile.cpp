#include "Type.h"
#include "Declaration.h"
#include "Error.h"
#include "PredefinedTypes.h"
#include "Utilities.h"
#include <iostream>
#include "Expression.h"

namespace Corrosive {

	void Type::compile() {
	}

	void TypeStructureInstance::compile() {
		owner->compile();
	}

	void TypeStructureTemplate::compile() {
		owner->compile();
	}
	
	void TypeFunctionTemplate::compile() {
		owner->compile();
	}

	void TypeTraitTemplate::compile() {
		owner->compile();
	}

	void TypeArray::compile() {
		owner->compile();
	}


	// ===================================================================================== GENERIC BUILD COPY/MOVE/CMP/CTOR/DROP
	

	/*
	Avoid previous mistakes:
		type drop and construct has to be done on lvalue or stacked rvalue currently
		If i ever implement faster rvalue structures, this will have to change.

		If structure is allowed to have register rvalue, 
		it cannot have its own specific constructor/destructor
	*/

	void Type::build_drop() {
		ILBuilder::build_forget(Ctx::scope(), ILDataType::ptr);
	}

	void Type::build_construct() {
		ILBuilder::build_forget(Ctx::scope(), ILDataType::ptr);
	}


	void Type::build_copy() {
		//dst(me), src(from)

		if (rvalue_stacked()) {
			ILBuilder::build_memcpy_rev(Ctx::scope(), size());
		}
		else {
			ILBuilder::build_store_rev(Ctx::scope(), rvalue());
		}
	}

	void Type::build_move() {
		//dst(me), src(from)

		if (rvalue_stacked()) {
			ILBuilder::build_memcpy_rev(Ctx::scope(), size());
		}
		else {
			ILBuilder::build_store_rev(Ctx::scope(), rvalue());
		}
	}

	void Type::build_compare() {
		// non rev
		//dst(me), src(compare_to)

		if (rvalue_stacked()) {
			ILBuilder::build_memcmp_rev(Ctx::scope(), size());
		}
		else {
			ILBuilder::build_rmemcmp2(Ctx::scope(), rvalue());
		}
	}

	// ===========================================================================================



	bool Type::has_special_constructor() {
		return false;
	}

	bool Type::has_special_destructor() {
		return false;
	}

	bool Type::has_special_copy() {
		return false;
	}

	bool Type::has_special_move() {
		return false;
	}

	bool Type::has_special_compare() {
		return false;
	}

	bool TypeArray::has_special_constructor() {
		return owner->has_special_constructor();
	}

	bool TypeArray::has_special_destructor() {
		return owner->has_special_destructor();
	}

	bool TypeArray::has_special_copy() {
		return owner->has_special_copy();
	}

	bool TypeArray::has_special_move() {
		return owner->has_special_move();
	}

	bool TypeArray::has_special_compare() {
		return owner->has_special_compare();
	}

	bool TypeStructureInstance::has_special_constructor() {
		return owner->has_special_constructor;
	}

	bool TypeStructureInstance::has_special_destructor() {
		return owner->has_special_destructor;
	}

	bool TypeStructureInstance::has_special_copy() {
		return owner->has_special_copy;
	}

	bool TypeStructureInstance::has_special_move() {
		return owner->has_special_move;
	}

	bool TypeStructureInstance::has_special_compare() {
		return owner->has_special_compare;
	}

	


	void TypeStructureInstance::build_drop() {
		if (has_special_destructor()) {
			ILBuilder::build_fnptr(Ctx::scope(), owner->auto_destructor);
			ILBuilder::build_callstart(Ctx::scope());
			ILBuilder::build_call(Ctx::scope(), ILDataType::none, 1);
		}
		else {
			Type::build_drop();
		}
	}

	void TypeStructureInstance::build_construct() {
		if (has_special_constructor()) {
			ILBuilder::build_fnptr(Ctx::scope(), owner->auto_constructor);
			ILBuilder::build_callstart(Ctx::scope());
			ILBuilder::build_call(Ctx::scope(), ILDataType::none, 1);
		}
		else {
			Type::build_construct();
		}
	}

	

	void TypeStructureInstance::build_copy() {
		//dst(me), src(from)
		if (has_special_copy()) {
			ILBuilder::build_fnptr(Ctx::scope(), owner->auto_copy);
			ILBuilder::build_callstart(Ctx::scope());
			ILBuilder::build_call(Ctx::scope(), ILDataType::none, 2);
		}
		else {
			Type::build_copy();
		}
	}

	void TypeStructureInstance::build_move() {
		//dst(me), src(from)
		if (has_special_move()) {
			ILBuilder::build_fnptr(Ctx::scope(), owner->auto_move);
			ILBuilder::build_callstart(Ctx::scope());
			ILBuilder::build_call(Ctx::scope(), ILDataType::none, 2);
		}
		else {
			Type::build_move();
		}
	}

	void TypeStructureInstance::build_compare() {
		//dst(me), src(compare to)

		if (has_special_copy()) {
			ILBuilder::build_fnptr(Ctx::scope(), owner->auto_compare);
			ILBuilder::build_callstart(Ctx::scope());
			ILBuilder::build_call(Ctx::scope(), ILDataType::i8, 2);
		}
		else {
			Type::build_compare();
		}
	}
}