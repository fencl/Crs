#include "Type.hpp"
#include "Declaration.hpp"
#include "Error.hpp"
#include "BuiltIn.hpp"
#include <iostream>
#include "Expression.hpp"
#include "Compiler.hpp"

namespace Corrosive {

	errvoid Type::compile() {
		return err::ok;
	}

	errvoid TypeStructureInstance::compile() {
		return owner->compile();
	}

	errvoid TypeStructureTemplate::compile() {
		return owner->compile();
	}
	
	errvoid TypeFunctionTemplate::compile() {
		return owner->compile();
	}

	errvoid TypeTraitTemplate::compile() {
		return owner->compile();
	}

	errvoid TypeArray::compile() {
		return owner->compile();
	}

	errvoid TypeFunction::compile() {
		if (il_function_decl == UINT32_MAX) {

			if (!return_type->compile()) return err::ok;

			auto& args = Compiler::current()->types()->argument_array_storage.get(argument_array_id);

			for (auto&& a : args) {
				if (!a->compile()) return err::ok;
			}

			std::size_t ret_ref_offset = (return_type->rvalue_stacked() ? 1 : 0);

			std::vector<ILDataType> decl_args(args.size() + ret_ref_offset);
			for (std::size_t i = ret_ref_offset; i < args.size() + ret_ref_offset; ++i) {
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

		return err::ok;
	}
}