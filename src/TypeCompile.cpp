#include "Type.h"
#include "Declaration.h"
#include "Error.h"
#include "PredefinedTypes.h"
#include "Utilities.h"
#include <iostream>
#include "Expression.h"
#include "Compiler.h"

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

	void TypeFunction::compile() {
		if (il_function_decl == UINT32_MAX) {

			return_type->compile();

			auto& args = compiler()->types()->argument_array_storage.get(argument_array_id);

			for (auto&& a : args) {
				a->compile();
			}

			size_t ret_ref_offset = (return_type->rvalue_stacked() ? 1 : 0);

			std::vector<ILDataType> decl_args(args.size() + ret_ref_offset);
			for (size_t i = ret_ref_offset; i < args.size() + ret_ref_offset; ++i) {
				decl_args[i] = args[i - ret_ref_offset]->rvalue();
			}

			ILDataType ret_t = ILDataType::none;
			if (!return_type->rvalue_stacked()) {
				ret_t = return_type->rvalue();
			}
			else {
				decl_args[0] = ILDataType::word;
			}

			il_function_decl = compiler()->global_module()->register_function_decl(std::make_tuple(call_conv,ret_t, std::move(decl_args)));
		}
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
		ILBuilder::build_forget(compiler()->scope(), ILDataType::word);
	}

	void Type::build_construct() {
		ILBuilder::build_forget(compiler()->scope(), ILDataType::word);
	}


	void Type::build_copy() {
		//dst(me), src(from)

		if (rvalue_stacked()) {
			ILBuilder::build_memcpy_rev(compiler()->scope(), size());
		}
		else {
			ILBuilder::build_store_rev(compiler()->scope(), rvalue());
		}
	}

	void Type::build_move() {
		//dst(me), src(from)

		if (rvalue_stacked()) {
			ILBuilder::build_memcpy_rev(compiler()->scope(), size());
		}
		else {
			ILBuilder::build_store_rev(compiler()->scope(), rvalue());
		}
	}

	void Type::build_compare() {
		// non rev
		//dst(me), src(compare_to)

		if (rvalue_stacked()) {
			ILBuilder::build_memcmp_rev(compiler()->scope(), size());
		}
		else {
			ILBuilder::build_rmemcmp2(compiler()->scope(), rvalue());
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
		return owner->auto_constructor != nullptr;
	}

	bool TypeStructureInstance::has_special_destructor() {
		return owner->auto_destructor != nullptr;
	}

	bool TypeStructureInstance::has_special_copy() {
		return owner->auto_copy != nullptr;
	}

	bool TypeStructureInstance::has_special_move() {
		return owner->auto_move != nullptr;
	}

	bool TypeStructureInstance::has_special_compare() {
		return owner->auto_compare != nullptr;
	}

	void TypeStructureInstance::build_drop() {
		if (has_special_destructor()) {
			ILBuilder::build_fncall(compiler()->scope(), owner->auto_destructor);
		}
		else {
			Type::build_drop();
		}
	}

	void TypeStructureInstance::build_construct() {
		if (has_special_constructor()) {
			ILBuilder::build_fncall(compiler()->scope(), owner->auto_constructor);
		}
		else {
			Type::build_construct();
		}
	}

	void TypeStructureInstance::build_copy() {
		//dst(me), src(from)
		if (has_special_copy()) {
			ILBuilder::build_fncall(compiler()->scope(), owner->auto_copy);
		}
		else {
			Type::build_copy();
		}
	}

	void TypeStructureInstance::build_move() {
		//dst(me), src(from)
		if (has_special_move()) {
			ILBuilder::build_fncall(compiler()->scope(), owner->auto_move);
		}
		else {
			Type::build_move();
		}
	}

	void TypeStructureInstance::build_compare() {
		//dst(me), src(compare to)

		if (has_special_copy()) {
			ILBuilder::build_fncall(compiler()->scope(), owner->auto_compare);
		}
		else {
			Type::build_compare();
		}
	}
}