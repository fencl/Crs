#include "Type.h"
#include "Declaration.h"
#include "Error.h"
#include "Contents.h"
#include "PredefinedTypes.h"
#include "Utilities.h"
#include <iostream>
#include "Expression.h"

namespace Corrosive {


	
	bool TypeInstance::compile(CompileContext& ctx) {
		switch (type)
		{
			case Corrosive::TypeInstanceType::type_instance:
				if (!((StructureInstance*)owner_ptr)->compile(ctx)) return false;
				break;
			case Corrosive::TypeInstanceType::type_template:
				if (!((StructureTemplate*)owner_ptr)->compile(ctx)) return false;
				break;
			default:
				exit(1);
		}

		return true;
	}

	bool Type::compile(CompileContext& ctx) {
		if (ref_count == 0) {
			if (!type->compile(ctx)) return false;
		}

		return true;
	}
}