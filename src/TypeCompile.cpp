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
}