#include "Type.hpp"
#include "Declaration.hpp"
#include "Error.hpp"
#include "BuiltIn.hpp"
#include <iostream>
#include "Expression.hpp"
#include "Compiler.hpp"

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

			auto& args = Compiler::current()->types()->argument_array_storage.get(argument_array_id);

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

			il_function_decl = Compiler::current()->global_module()->register_function_decl(std::make_tuple(call_conv,ret_t, std::move(decl_args)));
		}
	}
}