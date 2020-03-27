#include "Declaration.h"
#include "Error.h"
#include <iostream>
#include "Contents.h"
#include <string>
#include "PredefinedTypes.h"


namespace Corrosive {
	Namespace::~Namespace() {

	}


	void Namespace::find_name(std::string_view name, Namespace*& subnamespace, StructureTemplate*& subtemplate) {
		auto res = subnamespaces.find(name);
		if (res != subnamespaces.end()) {
			subnamespace= res->second.get();
			subtemplate = nullptr;
		}
		else {
			auto res2 = subtemplates.find(name);
			if (res2 != subtemplates.end()) {
				subtemplate = res2->second.get();
				subnamespace = nullptr;
			}
			else {
				if (parent != nullptr) {
					parent->find_name(name, subnamespace, subtemplate);
				}
				else {
					subtemplate = nullptr;
					subnamespace = nullptr;
				}
			}
		}
	}

	int StructureInstance::compare(CompileContext& ctx, void* p1, void* p2) {
		//TODO test for specific compare function

		if (member_vars.size() == 0) {
			return memcmp(p1, p2, size);
		}
		else {
			unsigned char* pd1 = (unsigned char*)p1;
			unsigned char* pd2 = (unsigned char*)p2;

			for (auto&& m : member_vars) {
				int c = m.type->compare(ctx, pd1, pd2);
				if (c != 0) return c;
				size_t ms = m.type->size(ctx);
				pd1 += ms;
				pd2 += ms;
			}

			return 0;
		}
	}



	void StructureInstance::move(CompileContext& ctx, void* src, void* dst) {
		//TODO test for specific move function
		

		if (member_vars.size() == 0) {
			memcpy(dst, src, size);
		}
		else {
			unsigned char* srcd = (unsigned char*)src;
			unsigned char* dstd = (unsigned char*)dst;

			for (auto&& m : member_vars) {
				m.type->move(ctx, srcd, dstd);

				size_t ms = m.type->size(ctx);
				srcd += ms;
				dstd += ms;
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

	bool StructureTemplate::GenericTemplateCompare::operator()(const std::pair<unsigned int, void*>& a, const std::pair<unsigned int, void*>& b) const
	{
		if (a.first < b.first) return true;
		if (a.first > b.first) return false;
		size_t offset = 0;
		for (auto&& l : parent->generic_layout) {
			int r = std::get<1>(l)->compare((CompileContext&)ctx, &((unsigned char*)a.second)[offset], &((unsigned char*)b.second)[offset]);
			if (r < 0) return true;
			if (r > 0) return false;
			offset += std::get<1>(l)->size((CompileContext&)ctx);
		}

		return false;
	}

	bool FunctionTemplate::GenericTemplateCompare::operator()(const std::pair<unsigned int, void*>& a, const std::pair<unsigned int, void*>& b) const
	{
		if (a.first < b.first) return true;
		if (a.first > b.first) return false;
		size_t offset = 0;
		for (auto&& l : parent->generic_layout) {
			int r = std::get<1>(l)->compare((CompileContext&)ctx, &((unsigned char*)a.second)[offset], &((unsigned char*)b.second)[offset]);
			if (r < 0) return true;
			if (r > 0) return false;
			offset += std::get<1>(l)->size((CompileContext&)ctx);
		}

		return false;
	}
}