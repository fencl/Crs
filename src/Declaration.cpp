#include "Declaration.h"
#include "Error.h"
#include <iostream>
#include "Contents.h"
#include <string>
#include "PredefinedTypes.h"


namespace Corrosive {
	Namespace::~Namespace() {

	}


	Namespace* Namespace::find_name(std::string_view name) {
		auto res = subnamespaces.find(name);
		if (res != subnamespaces.end()) {
			return res->second.get();
		}
		else if (parent!=nullptr){
			return parent->find_name(name);
		}
		else {
			return nullptr;
		}
	}
}