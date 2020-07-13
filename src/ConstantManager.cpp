#include <iostream>
#include <vector>
#include <memory>
#include "IL/IL.h"
#include <memory>
#include "ConstantManager.h"

namespace Corrosive {

	std::pair<const std::string_view, uint32_t> ConstantManager::register_string_literal(std::string string) {
		auto empl = string_literals.emplace(std::move(string), 0);

		if (empl.second) {
			empl.first->second = Ctx::global_module()->register_constant((unsigned char*)empl.first->first.data(), empl.first->first.length());
		}

		return std::move(std::make_pair(empl.first->first,empl.first->second));
	}


	std::pair<const std::string_view, uint32_t> ConstantManager::register_string_literal(Cursor& c) {
		auto res = string_holders.find(c);

		if (res == string_holders.end()) {
			std::string holder;
			bool escaped = false;
			for (size_t i = 1; i < c.buffer.size() - 1; ++i) {
				char chr = c.buffer[i];
				if (!escaped) {
					if (chr == '\\') {
						escaped = true;
					}
					else {
						holder.push_back(chr);
					}
				}
				else {
					switch (chr)
					{
						case 'n': holder.push_back('\n'); break;
						case 'r': holder.push_back('\r'); break;
						case 't': holder.push_back('\t'); break;
						case '0': holder.push_back('\0'); break;
						case '\\': holder.push_back('\\'); break;
					}

					escaped = false;
				}
			}

			auto res_id = register_string_literal(std::move(holder));
			string_holders.insert(std::make_pair(c, res_id));
			return res_id;
		}
		else {
			return res->second;
		}
	}

}