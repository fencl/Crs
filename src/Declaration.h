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
	
	enum class StructDeclarationType : unsigned char {
		t_u8, t_u16, t_u32, t_u64, t_i8, t_i16, t_i32, t_i64, t_f32, t_f64, t_bool, t_ptr
	};

	enum class NamespaceType : unsigned char {
		t_namespace, t_struct_template, t_struct_instance
	};

	class StructureTemplate;
	class Namespace {
	public:
		virtual ~Namespace();
		NamespaceType namespace_type;

		Cursor name;
		Namespace* parent = nullptr;
		std::map<std::string_view, std::unique_ptr<Namespace>> subnamespaces;
		std::map<std::string_view, std::unique_ptr<StructureTemplate>> subtemplates;

		static bool parse(Cursor& c, CompileContext& ctx, std::unique_ptr<Namespace>& into);
		void find_name(std::string_view name, Namespace*& subnamespace, StructureTemplate*& subtemplate);
	};

	class StructureTemplate;
	class StructureInstance : public Namespace {
	public:

		void add_member(CompileContext& ctx, Type* t);

		std::vector<std::pair<Cursor,Type*>> member_vars;
		std::vector<std::pair<ILFunction*, Type*>> member_funcs;

		StructureTemplate* generator;

		size_t compile_time_size_in_bytes;
		unsigned int runtime_size;
		unsigned int runtime_alignment;

		void* key = nullptr;

		int compare(ILEvaluator* eval, void* p1, void* p2);
		void move(ILEvaluator* eval, void* src, void* dst);

		void insert_key_on_stack(CompileContext& ctx);

		std::unique_ptr<TypeInstance> type;
		bool compile(CompileContext& ctx);

		unsigned int compile_state = 0;
	};

	struct StructureTemplateMemberVar {
		Cursor name;
		Cursor type;
	};

	struct StructureTemplateMemberFunc {
		Cursor name;
		Cursor type;
		Cursor block;
	};


	struct StructureTemplateSubtemplate {
		Cursor cursor;
	};


	class StructureTemplate {
	public:
		virtual ~StructureTemplate();
		Namespace* parent = nullptr;
		Cursor name;
		Cursor annotation;
		StructureInstance* template_parent = nullptr;

		std::unique_ptr<TypeStructure> type;

		std::unique_ptr<StructureInstance> singe_instance = nullptr;

		size_t generate_heap_size = 0;

		bool is_generic = false;
		std::vector<StructureTemplateMemberVar> member_vars;
		std::vector<StructureTemplateMemberFunc> member_funcs;
		std::vector<StructureTemplateSubtemplate> member_templates;

		bool generate(CompileContext& ctx, void* argdata, StructureInstance*& out);

		bool compile(CompileContext& ctx);
		unsigned int type_compare_heap_size = 0;

		static bool parse(Cursor &c, CompileContext& ctx, Namespace* parent, std::unique_ptr<StructureTemplate>& into);

		unsigned int compile_state = 0;
		std::vector<std::tuple<Cursor,Type*>> generic_layout;
		bool rvalue_stacked = true;

	private:
		struct GenericTemplateCompare {
			StructureTemplate* parent;
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