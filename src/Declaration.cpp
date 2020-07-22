#include "Declaration.h"
#include "Error.h"
#include <iostream>
#include <string>
#include "PredefinedTypes.h"
#include "Compiler.h"

namespace Corrosive {
	Namespace::~Namespace() {

	}


	void Namespace::find_name(std::string_view name, Namespace*& subnamespace, StructureTemplate*& subtemplate, FunctionTemplate*& subfunction, TraitTemplate*& subtrait) {
		subnamespace = nullptr;
		subtemplate = nullptr;
		subfunction = nullptr;
		subtrait = nullptr;

		auto res = name_table.find(name);
		if (res != name_table.end()) {

			switch (res->second.first) {
				case 0: subnamespace = subnamespaces[res->second.second].get(); return;
				case 1: subtemplate  = subtemplates[res->second.second].get(); return;
				case 2: subfunction  = subfunctions[res->second.second].get(); return;
				case 3: subtrait     = subtraits[res->second.second].get(); return;
			}
		}
		else if (parent) {
			parent->find_name(name, subnamespace, subtemplate, subfunction, subtrait);
		}
	}


	bool StructureTemplate::GenericTemplateCompare::operator()(unsigned char* const& a, unsigned char* const& b) const {
		unsigned char* loff = a;
		unsigned char* roff = b;

		for (auto&& l : parent->generic_ctx.generic_layout) {
			int r = std::get<1>(l)->compare(loff,roff);
			if (r < 0) return true;
			if (r > 0) return false;
			size_t off = std::get<1>(l)->size().eval(parent->compiler->global_module(),compiler_arch);
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
			size_t off = std::get<1>(l)->size().eval(parent->compiler->global_module(), compiler_arch);
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

			size_t off = std::get<1>(l)->size().eval(parent->compiler->global_module(), compiler_arch);
			loff += off;
			roff += off;
		}

		return false;
	}
}