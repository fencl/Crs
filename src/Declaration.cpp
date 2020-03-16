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

	int StructureInstance::compare(void* p1, void* p2) {
		//TODO test for specific compare function
		return iltype->auto_compare(p1, p2);
	}



	void StructureInstance::move(CompileContext& ctx, void* src, void* dst) {
		//TODO test for specific move function
		iltype->auto_move(src, dst);
	}


	Structure::~Structure() {
		if (instances != nullptr) {
			for (auto&& i : *instances) {
				delete i.first.second;
			}
		}
	}

	bool Structure::GenericTemplateCompare::operator()(const std::pair<unsigned int, void*>& a, const std::pair<unsigned int, void*>& b) const
	{
		if (a.first < b.first) return true;
		if (a.first > b.first) return false;
		size_t offset = 0;
		for (auto&& l : parent->generic_layout) {
			int r = std::get<2>(l).compare(std::get<1>(l),&((unsigned char*)a.second)[offset], &((unsigned char*)b.second)[offset]);
			if (r < 0) return true;
			if (r > 0) return false;
			offset += std::get<1>(l);
		}

		return false;
	}
}