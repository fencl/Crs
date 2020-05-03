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

	bool TypeTraitInstance::compile() {
		return owner->compile();
	}

	bool TypeArray::compile() {
		return owner->compile();
	}


	void Type::build_move() {
		//src, dst, size
		
		CompileContext& nctx = CompileContext::get();
		if (rvalue_stacked()) {
			ILBuilder::build_const_size(nctx.scope, compile_size(nctx.eval), size(nctx.eval));
			ILBuilder::build_insintric(nctx.scope, ILInsintric::memcpy);
		}
		else {
			ILBuilder::build_store(nctx.scope, rvalue());
		}
	}

	void Type::build_copy() {
		//src, dst, size

		CompileContext& nctx = CompileContext::get();
		if (rvalue_stacked()) {
			ILBuilder::build_const_size(nctx.scope, compile_size(nctx.eval), size(nctx.eval));
			ILBuilder::build_insintric(nctx.scope, ILInsintric::memcpy);
		}
		else {
			ILBuilder::build_store(nctx.scope, rvalue());
		}
	}

	void Type::build_drop() {
		CompileContext& nctx = CompileContext::get();
		ILBuilder::build_forget(nctx.scope, ILDataType::ptr);
	}

	void Type::build_construct() {
		CompileContext& nctx = CompileContext::get();
		ILBuilder::build_forget(nctx.scope, ILDataType::ptr);
	}

	bool Type::has_special_constructor() {
		return false;
	}

	bool Type::has_special_destructor() {
		return false;
	}

	bool TypeArray::has_special_constructor() {
		return owner->has_special_constructor();
	}

	bool TypeArray::has_special_destructor() {
		return owner->has_special_destructor();
	}


	bool TypeStructureInstance::has_special_constructor() {
		return owner->has_special_constructor;
	}

	bool TypeStructureInstance::has_special_destructor() {
		return owner->has_special_constructor;
	}


	void TypeStructureInstance::build_move() {
		//TODO
		Type::build_move();
	}

	void TypeStructureInstance::build_copy() {
		//TODO
		Type::build_copy();
	}

	void TypeStructureInstance::build_drop() {
		CompileContext& nctx = CompileContext::get();

		ILBuilder::build_fnptr(nctx.scope, owner->auto_destructor);
		ILBuilder::build_callstart(nctx.scope);
		ILBuilder::build_call(nctx.scope, ILDataType::none, 1);
	}

	void TypeStructureInstance::build_construct() {
		CompileContext& nctx = CompileContext::get();
		ILBuilder::build_fnptr(nctx.scope, owner->auto_constructor);
		ILBuilder::build_callstart(nctx.scope);
		ILBuilder::build_call(nctx.scope, ILDataType::none, 1);
	}
}