#include "Type.h"
#include "Declaration.h"
#include "Error.h"
#include "PredefinedTypes.h"
#include "Utilities.h"
#include <iostream>
#include "Expression.h"

namespace Corrosive {

	bool Type::compile() {
		return true;
	}

	bool TypeStructureInstance::compile() {
		return owner->compile();
	}

	bool TypeStructureTemplate::compile() {
		return owner->compile();
	}
	
	bool TypeFunctionTemplate::compile() {
		return owner->compile();
	}

	bool TypeTraitTemplate::compile() {
		return owner->compile();
	}

	bool TypeArray::compile() {
		return owner->compile();
	}


	// ===================================================================================== GENERIC BUILD COPY/MOVE/CMP/CTOR/DROP
	
	void Type::build_drop() {
		CompileContext& nctx = CompileContext::get();

		if (rvalue_stacked()) {
			ILBuilder::build_forget(nctx.scope, ILDataType::ptr);
		}
		else {
			ILBuilder::build_forget(nctx.scope, rvalue());
		}
	}

	void Type::build_construct() {
		CompileContext& nctx = CompileContext::get();

		if (rvalue_stacked()) {
			ILBuilder::build_forget(nctx.scope, ILDataType::ptr);
		}
		else {
			ILBuilder::build_forget(nctx.scope, rvalue());
		}
	}

	void Type::build_move(bool rv) {
		//dst(me), src(from)
		
		CompileContext& nctx = CompileContext::get();
		if (rvalue_stacked() || !rv) {
			ILBuilder::build_memcpy2(nctx.scope, size());
		}
		else {
			ILBuilder::build_store2(nctx.scope, rvalue());
		}
	}

	void Type::build_copy(bool rv) {
		//dst(me), src(from)

		CompileContext& nctx = CompileContext::get();
		if (rvalue_stacked() || !rv) {
			ILBuilder::build_memcpy2(nctx.scope, size());
		}
		else {
			ILBuilder::build_store2(nctx.scope, rvalue());
		}
	}

	void Type::build_compare(bool rv) {
		// non rev
		//dst(me), src(compare_to)

		CompileContext& nctx = CompileContext::get();
		if (rvalue_stacked() || !rv) {
			ILBuilder::build_memcmp2(nctx.scope, size());
		}
		else {
			ILBuilder::build_rmemcmp2(nctx.scope, rvalue());
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
		return owner->has_special_constructor;
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
			CompileContext& nctx = CompileContext::get();
			ILBuilder::build_fnptr(nctx.scope, owner->auto_destructor);
			ILBuilder::build_callstart(nctx.scope);
			ILBuilder::build_call(nctx.scope, ILDataType::none, 1);
		}
		else {
			Type::build_drop();
		}
	}

	void TypeStructureInstance::build_construct() {
		if (has_special_constructor()) {
			CompileContext& nctx = CompileContext::get();
			ILBuilder::build_fnptr(nctx.scope, owner->auto_constructor);
			ILBuilder::build_callstart(nctx.scope);
			ILBuilder::build_call(nctx.scope, ILDataType::none, 1);
		}
		else {
			Type::build_construct();
		}
	}

	void TypeStructureInstance::build_move(bool rv) {
		//dst(me), src(from)
		if (has_special_move()) {
			CompileContext& nctx = CompileContext::get();

			// rvalue passed to ctor, create stack copy, no need to drop
			if (rv && !rvalue_stacked()) {
				uint16_t loc_id = nctx.function->register_local(size());
				ILBuilder::build_local(nctx.scope, loc_id);
				ILBuilder::build_store(nctx.scope, rvalue());
				ILBuilder::build_local(nctx.scope, loc_id);
			}

			ILBuilder::build_fnptr(nctx.scope, owner->auto_move);
			ILBuilder::build_callstart(nctx.scope);
			ILBuilder::build_call(nctx.scope, ILDataType::none, 2);
		}
		else {
			Type::build_move(rv);
		}
	}

	void TypeStructureInstance::build_copy(bool rv) {
		//dst(me), src(from)
		if (has_special_copy()) {
			CompileContext& nctx = CompileContext::get();

			// rvalue passed to ctor, create stack copy, no need to drop
			if (rv && !rvalue_stacked()) {
				uint16_t loc_id = nctx.function->register_local(size());
				ILBuilder::build_local(nctx.scope, loc_id);
				ILBuilder::build_store(nctx.scope, rvalue());
				ILBuilder::build_local(nctx.scope, loc_id);
			}

			ILBuilder::build_fnptr(nctx.scope, owner->auto_copy);
			ILBuilder::build_callstart(nctx.scope);
			ILBuilder::build_call(nctx.scope, ILDataType::none, 2);
		}
		else {
			Type::build_copy(rv);
		}
	}

	void TypeStructureInstance::build_compare(bool rv) {
		//dst(me), src(compare to)

		if (has_special_copy()) {
			CompileContext& nctx = CompileContext::get();

			// rvalue passed to ctor, create stack copy
			if (rv && !rvalue_stacked()) {
				uint16_t loc_id = nctx.function->register_local(size());
				ILBuilder::build_local(nctx.scope, loc_id);
				ILBuilder::build_store(nctx.scope, rvalue());
				ILBuilder::build_local(nctx.scope, loc_id);
			}

			ILBuilder::build_fnptr(nctx.scope, owner->auto_compare);
			ILBuilder::build_callstart(nctx.scope);
			ILBuilder::build_call(nctx.scope, ILDataType::i8, 2);
		}
		else {
			Type::build_compare(rv);
		}
	}
}