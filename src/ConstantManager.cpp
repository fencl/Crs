#include <iostream>
#include <vector>
#include <memory>
#include "IL/IL.hpp"
#include <memory>
#include "ConstantManager.hpp"
#include "Compiler.hpp"

namespace Corrosive {

	std::pair<const std::string_view, uint32_t> ConstantManager::register_constant(std::string string, ILSize s) {
		auto empl = string_literals.emplace(std::move(string), 0);

		if (empl.second) {
			empl.first->second = compiler->global_module()->register_constant((unsigned char*)empl.first->first.data(), s);
		}

		return std::move(std::make_pair(std::string_view(empl.first->first),empl.first->second));
	}


	std::pair<const std::string_view, uint32_t> ConstantManager::register_string_literal(Cursor& c) {
		auto res = string_holders.find(c);

		if (res == string_holders.end()) {
			std::string holder;
			bool escaped = false;
			auto buf = c.buffer();
			for (size_t i = 1; i < c.length - 1; ++i) {
				char chr = buf[i];
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
			ILSize size(ILSizeType::abs8, (tableid_t)holder.length());
			auto res_id = register_constant(std::move(holder), size);
			string_holders.insert(std::make_pair(c, res_id));
			return res_id;
		}
		else {
			return res->second;
		}
	}

	uint8_t* ConstantManager::register_generic_storage(uint8_t* ptr, size_t size, Type* of) {
		std::basic_string_view<uint8_t> view(ptr, size);
		auto res = generic_storage_map.insert(std::make_pair(view, 0));
		if (res.second) {
			res.first->second = generic_storage.size();

			auto arr = std::make_unique<uint8_t[]>(size);
			uint8_t* dst = arr.get();
			
			uint8_t* off_src = ptr;
			uint8_t* off_dst = dst;
			size_t mem_size = of->size().eval(Compiler::current()->global_module(), compiler_arch);

			for (size_t i = 0; i < size / mem_size; i++) {
				of->copy_to_generic_storage(off_src, off_dst);
				off_src += mem_size;
				off_dst += mem_size;
			}

			// unsafe in theory but the data is actually the same so hash and compare should not change
			(std::basic_string_view<uint8_t>&)res.first->first = std::basic_string_view<uint8_t>(arr.get(), size); 

			generic_storage.push_back(std::move(arr));
			return dst;
		}
		else {
			return generic_storage[res.first->second].get();
		}
	}

}