#include "Declaration.h"
#include "Error.h"
#include <iostream>
#include <string>
#include "PredefinedTypes.h"


namespace Corrosive {
	Namespace::~Namespace() {

	}


	void Namespace::find_name(std::string_view name, Namespace*& subnamespace, StructureTemplate*& subtemplate, FunctionTemplate*& subfunction) {
		auto res = subnamespaces.find(name);
		if (res != subnamespaces.end()) {
			subnamespace= res->second.get();
			subtemplate = nullptr;
			subfunction = nullptr;
		}
		else {
			auto res2 = subtemplates.find(name);
			if (res2 != subtemplates.end()) {
				subtemplate = res2->second.get();
				subnamespace = nullptr;
				subfunction = nullptr;
			}
			else {
				auto res3 = subfunctions.find(name);
				if (res3 != subfunctions.end()) {
					subnamespace = nullptr;
					subtemplate = nullptr;
					subfunction = res3->second.get();
				}
				else {
					if (parent != nullptr) {
						parent->find_name(name, subnamespace, subtemplate,subfunction);
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

	int StructureInstance::compare(CompileContext& ctx, unsigned char* p1, unsigned char* p2) {
		//TODO test for specific compare function

		if (member_vars.size() == 0) {
			return memcmp(p1, p2, size);
		}
		else {
			for (auto&& m : member_vars) {
				int c = m.type->compare(ctx, p1, p2);
				if (c != 0) return c;
				size_t ms = m.type->size(ctx);
				p1 += ms;
				p2 += ms;
			}

			return 0;
		}
	}



	void StructureInstance::move(CompileContext& ctx, unsigned char* src, unsigned char* dst) {
		//TODO test for specific move function
		

		if (member_vars.size() == 0) {
			memcpy(dst, src, size);
		}
		else {
			for (auto&& m : member_vars) {
				m.type->move(ctx, src, dst);

				size_t ms = m.type->size(ctx);
				src += ms;
				dst += ms;
			}
		}
	}


	StructureTemplate::~StructureTemplate() {
		if (instances != nullptr) {
			for (auto&& i : *instances) {
				//delete i.first.second;
			}
		}
	}

	bool StructureTemplate::GenericTemplateCompare::operator()(unsigned char* const& a, unsigned char* const& b) const
	{
		unsigned char* loff = a;
		unsigned char* roff = b;

		for (auto&& l : parent->generic_layout) {

			int r = std::get<1>(l)->compare((CompileContext&)ctx, loff,roff);
			if (r < 0) return true;
			if (r > 0) return false;
			unsigned int off = std::get<1>(l)->size((CompileContext&)ctx);
			loff += off;
			roff += off;
		}

		return false;
	}

	bool FunctionTemplate::GenericTemplateCompare::operator()(const std::pair<unsigned int, unsigned char*>& a, const std::pair<unsigned int, unsigned char*>& b) const
	{
		if (a.first < b.first) return true;
		if (a.first > b.first) return false;

		unsigned char* loff = a.second;
		unsigned char* roff = b.second;

		for (auto&& l : parent->generic_layout) {
			int r = std::get<1>(l)->compare((CompileContext&)ctx, loff, roff);
			if (r < 0) return true;
			if (r > 0) return false;

			unsigned int off = std::get<1>(l)->size((CompileContext&)ctx);
			loff += off;
			roff += off;
		}

		return false;
	}
}