#include "Declaration.h"
#include "Error.h"
#include <iostream>
#include <string>
#include "PredefinedTypes.h"
#include "Compiler.h"

namespace Corrosive {
	Namespace::~Namespace() {

	}


	FindNameResult Namespace::find_name(std::string_view name) {
		auto res = name_table.find(name);
		if (res != name_table.end()) {

			switch (res->second.first) {
				case 0: return subnamespaces[res->second.second].get();
				case 1: return subtemplates[res->second.second].get();
				case 2: return subfunctions[res->second.second].get();
				case 3: return subtraits[res->second.second].get();
				case 4: return substatics[res->second.second].get();
			}
		}
		else if (parent) {
			return parent->find_name(name);
		}
	}


	bool StructureTemplate::GenericTemplateCompare::operator()(unsigned char* const& a, unsigned char* const& b) const {
		unsigned char* loff = a;
		unsigned char* roff = b;

		for (auto&& l : parent->generic_ctx.generic_layout) {
			int r = std::get<1>(l)->compare(loff,roff);
			if (r < 0) return true;
			if (r > 0) return false;
			size_t off = std::get<1>(l)->size().eval(Compiler::current()->global_module(),compiler_arch);
			loff += off;
			roff += off;
		}

		return false;
	}

	bool TraitTemplate::GenericTemplateCompare::operator()(unsigned char* const& a, unsigned char* const& b) const {
		unsigned char* loff = a;
		unsigned char* roff = b;


		for (auto&& l : parent->generic_ctx.generic_layout) {

			int r = std::get<1>(l)->compare(loff, roff);
			if (r < 0) return true;
			if (r > 0) return false;
			size_t off = std::get<1>(l)->size().eval(Compiler::current()->global_module(), compiler_arch);
			loff += off;
			roff += off;
		}

		return false;
	}

	bool FunctionTemplate::GenericTemplateCompare::operator()(unsigned char* const& a, unsigned char* const& b) const {
		unsigned char* loff = a;
		unsigned char* roff = b;

		for (auto&& l : parent->generic_ctx.generic_layout) {
			int r = std::get<1>(l)->compare(loff, roff);
			if (r < 0) return true;
			if (r > 0) return false;

			size_t off = std::get<1>(l)->size().eval(Compiler::current()->global_module(), compiler_arch);
			loff += off;
			roff += off;
		}

		return false;
	}
}