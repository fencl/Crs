#include <iostream>
#include <vector>
#include <memory>
#include "IL/IL.h"
#include <memory>
#include "ConstantManager.h"

namespace Corrosive {

	uint32_t ConstantManager::register_string_literal(std::string_view string) {
		auto empl = string_literals.emplace(string, 0);

		if (empl.second) {
			empl.first->second = Ctx::global_module()->register_constant((unsigned char*)string.data(), string.length());
		}

		return empl.first->second;
	}

}