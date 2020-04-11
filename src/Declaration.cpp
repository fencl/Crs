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

	int TraitInstance::compare(CompileContext& ctx, unsigned char* p1, unsigned char* p2) {
		//TODO test for specific compare function
		return memcmp(p1, p2, ctx.default_types->t_ptr->compile_size(ctx)*2);
	}



	void TraitInstance::move(CompileContext& ctx, unsigned char* src, unsigned char* dst) {
		//TODO test for specific move function
		memcpy(dst, src, ctx.default_types->t_ptr->compile_size(ctx) * 2);
	}

	int StructureInstance::compare(CompileContext& ctx, unsigned char* p1, unsigned char* p2) {
		//TODO test for specific compare function

		if (member_vars.size() == 0) {
			return memcmp(p1, p2, compile_size);
		}
		else {
			for (auto&& m : member_vars) {
				int c = m.type->compare(ctx, p1, p2);
				if (c != 0) return c;
				size_t ms = m.type->compile_size(ctx);
				p1 += ms;
				p2 += ms;
			}

			return 0;
		}
	}



	void StructureInstance::move(CompileContext& ctx, unsigned char* src, unsigned char* dst) {
		//TODO test for specific move function
		

		if (member_vars.size() == 0) {
			memcpy(dst, src, compile_size);
		}
		else {
			for (auto&& m : member_vars) {
				m.type->move(ctx, src, dst);

				size_t ms = m.type->compile_size(ctx);
				src += ms;
				dst += ms;
			}
		}
	}


	bool StructureTemplate::GenericTemplateCompare::operator()(unsigned char* const& a, unsigned char* const& b) const {
		unsigned char* loff = a;
		unsigned char* roff = b;

		for (auto&& l : parent->generic_layout) {

			int r = std::get<1>(l)->compare((CompileContext&)ctx, loff,roff);
			if (r < 0) return true;
			if (r > 0) return false;
			unsigned int off = std::get<1>(l)->compile_size((CompileContext&)ctx);
			loff += off;
			roff += off;
		}

		return false;
	}

	bool TraitTemplate::GenericTemplateCompare::operator()(unsigned char* const& a, unsigned char* const& b) const {
		unsigned char* loff = a;
		unsigned char* roff = b;

		for (auto&& l : parent->generic_layout) {

			int r = std::get<1>(l)->compare((CompileContext&)ctx, loff, roff);
			if (r < 0) return true;
			if (r > 0) return false;
			unsigned int off = std::get<1>(l)->compile_size((CompileContext&)ctx);
			loff += off;
			roff += off;
		}

		return false;
	}

	bool FunctionTemplate::GenericTemplateCompare::operator()(const std::pair<unsigned int, unsigned char*>& a, const std::pair<unsigned int, unsigned char*>& b) const {
		if (a.first < b.first) return true;
		if (a.first > b.first) return false;

		unsigned char* loff = a.second;
		unsigned char* roff = b.second;

		for (auto&& l : parent->generic_layout) {
			int r = std::get<1>(l)->compare((CompileContext&)ctx, loff, roff);
			if (r < 0) return true;
			if (r > 0) return false;

			unsigned int off = std::get<1>(l)->compile_size((CompileContext&)ctx);
			loff += off;
			roff += off;
		}

		return false;
	}
}