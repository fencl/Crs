#pragma once
#ifndef _declaration_crs_h
#define _declaration_crs_h


#include "CompileContext.h"
#include <memory>
#include <vector>
#include "Cursor.h"
#include <unordered_map>
#include <map>
#include "Type.h"
#include "IL/IL.h"

namespace Corrosive {
	
	enum class StructDeclarationType {
		t_u8, t_u16, t_u32, t_u64, t_i8, t_i16, t_i32, t_i64, t_f32, t_f64, t_bool, t_ptr
	};

	class Namespace {
	public:
		virtual ~Namespace();
		Cursor name;
		Namespace* parent = nullptr;
		std::map<std::string_view, std::unique_ptr<Namespace>> subnamespaces;
		static bool parse(Cursor& c, CompileContext& ctx, std::unique_ptr<Namespace>& into);
		Namespace* find_name(std::string_view name);
	};

	struct StructureMemberVar {
		Cursor name;
		Cursor type;
	};

	struct StructureMemberFunc {
		Cursor name;
		Cursor type;
		Cursor block;
	};

	class Structure;
	class StructureInstance {
	public:
		std::map<std::string_view,std::pair<Cursor,Type>> member_vars;
		std::map<std::string_view,std::pair<ILFunction*,Type>> member_funcs;
		Structure* generator;
		ILType* iltype = nullptr;

		int compare(ILEvaluator* eval, void* p1, void* p2);
		void move(CompileContext& ctx, void* src, void* dst);

		std::unique_ptr<TypeInstance> type;
		bool compile(CompileContext& ctx);

		unsigned int compile_state = 0;
	};

	class Structure : public Namespace {
	public:
		virtual ~Structure();
		Cursor generic_types;
		std::unique_ptr<TypeInstance> type;

		std::unique_ptr<StructureInstance> singe_instance = nullptr;

		size_t generate_heap_size = 0;

		bool is_generic = false;
		std::vector<StructureMemberVar> member_vars;
		std::vector<StructureMemberFunc> member_funcs;

		bool generate(CompileContext& ctx, void* argdata, StructureInstance*& out);

		bool compile(CompileContext& ctx);
		unsigned int type_compare_heap_size = 0;

		static bool parse(Cursor &c, CompileContext& ctx, std::unique_ptr<Structure>& into);

		unsigned int compile_state = 0;
		std::vector<std::tuple<Cursor,Type>> generic_layout;
		bool rvalue_stacked = true;

	private:
		struct GenericTemplateCompare {
			Structure* parent;
			ILEvaluator* eval;
			bool operator()(const std::pair<unsigned int, void*>& a, const std::pair<unsigned int, void*>& b) const;
		};
		GenericTemplateCompare gen_template_cmp;
	public:
		std::unique_ptr<std::map<std::pair<unsigned int, void*>, std::unique_ptr<StructureInstance>, GenericTemplateCompare>> instances = nullptr;
	};

	class Declaration {
	public:
		static bool parse_global(Cursor& c, CompileContext& ctx, Namespace& into);
	};

}

#endif