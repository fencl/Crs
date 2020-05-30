#include "Declaration.h"
#include "Error.h"
#include <iostream>
#include <string>
#include "PredefinedTypes.h"


namespace Corrosive {
	Namespace::~Namespace() {

	}


	void Namespace::find_name(std::string_view name, Namespace*& subnamespace, StructureTemplate*& subtemplate, FunctionTemplate*& subfunction, TraitTemplate*& subtrait) {
		auto res = subnamespaces.find(name);
		if (res != subnamespaces.end()) {
			subnamespace= res->second.get();
			subtemplate = nullptr;
			subfunction = nullptr;
			subtrait = nullptr;
		}
		else {
			auto res2 = subtemplates.find(name);
			if (res2 != subtemplates.end()) {
				subtemplate = res2->second.get();
				subnamespace = nullptr;
				subfunction = nullptr;
				subtrait = nullptr;
			}
			else {
				auto res3 = subfunctions.find(name);
				if (res3 != subfunctions.end()) {
					subnamespace = nullptr;
					subtemplate = nullptr;
					subtrait = nullptr;
					subfunction = res3->second.get();
				}
				else {
					auto res4 = subtraits.find(name);
					if (res4 != subtraits.end()) {
						subnamespace = nullptr;
						subtemplate = nullptr;
						subfunction = nullptr;
						subtrait = res4->second.get();
					}
					else {
						if (parent != nullptr) {
							parent->find_name(name, subnamespace, subtemplate, subfunction,subtrait);
						}
						else {
							subtemplate = nullptr;
							subnamespace = nullptr;
							subfunction = nullptr;
						}
					}
				}
			}
		}
	}


	bool StructureTemplate::GenericTemplateCompare::operator()(unsigned char* const& a, unsigned char* const& b) const {
		unsigned char* loff = a;
		unsigned char* roff = b;

		for (auto&& l : parent->generic_layout) {

			int r = std::get<1>(l)->compare(ctx.eval, loff,roff);
			if (r < 0) return true;
			if (r > 0) return false;
			size_t off = std::get<1>(l)->size().eval(compiler_arch);
			loff += off;
			roff += off;
		}

		return false;
	}

	bool TraitTemplate::GenericTemplateCompare::operator()(unsigned char* const& a, unsigned char* const& b) const {
		unsigned char* loff = a;
		unsigned char* roff = b;
		CompileContext& nctx = CompileContext::get();

		for (auto&& l : parent->generic_layout) {

			int r = std::get<1>(l)->compare(nctx.eval, loff, roff);
			if (r < 0) return true;
			if (r > 0) return false;
			size_t off = std::get<1>(l)->size().eval(compiler_arch);
			loff += off;
			roff += off;
		}

		return false;
	}

	bool FunctionTemplate::GenericTemplateCompare::operator()(const std::pair<unsigned int, unsigned char*>& a, const std::pair<unsigned int, unsigned char*>& b) const {
		if (a.first < b.first) return true;
		if (a.first > b.first) return false;
		CompileContext& nctx = CompileContext::get();

		unsigned char* loff = a.second;
		unsigned char* roff = b.second;

		for (auto&& l : parent->generic_layout) {
			int r = std::get<1>(l)->compare(nctx.eval, loff, roff);
			if (r < 0) return true;
			if (r > 0) return false;

			size_t off = std::get<1>(l)->size().eval(compiler_arch);
			loff += off;
			roff += off;
		}

		return false;
	}
}