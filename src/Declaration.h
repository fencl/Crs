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
	class FunctionTemplate;
	class FunctionInstance;

	class Namespace {
	public:
		virtual ~Namespace();
		NamespaceType namespace_type;

		Cursor name;
		Namespace* parent = nullptr;
		std::map<std::string_view, std::unique_ptr<Namespace>> subnamespaces;
		std::map<std::string_view, std::unique_ptr<StructureTemplate>> subtemplates;
		std::map<std::string_view, std::unique_ptr<FunctionTemplate>> subfunctions;

		static bool parse(Cursor& c, CompileContext& ctx, std::unique_ptr<Namespace>& into);
		void find_name(std::string_view name, Namespace*& subnamespace, StructureTemplate*& subtemplate, FunctionTemplate*& subfunction);
	};

	struct StructureInstanceMemberRecord {
		Cursor definition;
		Type* type;

		uint32_t offset;
		uint32_t compile_offset;
	};

	class StructureInstance : public Namespace {
	public:

		std::map<std::string_view, size_t> member_table;

		std::vector<StructureInstanceMemberRecord> member_vars;

		StructureTemplate* generator;

		uint32_t size;
		uint32_t alignment;


		uint32_t compile_size;
		uint32_t compile_alignment;

		unsigned char* key;

		int compare(CompileContext& ctx, unsigned char* p1, unsigned char* p2);
		void move(CompileContext& ctx, unsigned char* src, unsigned char* dst);

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
		Cursor annotation;
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

		bool generate(CompileContext& ctx, unsigned char* argdata, StructureInstance*& out);

		bool compile(CompileContext& ctx);

		static bool parse(Cursor &c, CompileContext& ctx, Namespace* parent, std::unique_ptr<StructureTemplate>& into);

		unsigned int compile_state = 0;
		std::vector<std::tuple<Cursor,Type*>> generic_layout;
		bool rvalue_stacked = true;

	private:
		struct GenericTemplateCompare {
			StructureTemplate* parent;
			CompileContext ctx;
			bool operator()(unsigned char* const& a, unsigned char* const& b) const;
		};
		GenericTemplateCompare gen_template_cmp;
	public:
		std::unique_ptr<std::map<unsigned char*, std::unique_ptr<StructureInstance>, GenericTemplateCompare>> instances = nullptr;
	};

	class FunctionInstance {
	public:
		ILFunction* func = nullptr;
		FunctionTemplate* generator;

		Cursor block;

		std::vector<std::pair<Cursor,Type*>> arguments;
		Type* returns;

		unsigned char* key;

		bool compile(CompileContext& ctx);

		unsigned int compile_state = 0;

	};

	class FunctionTemplate {
	public:
		inline virtual ~FunctionTemplate() {}

		Namespace* parent = nullptr;
		Cursor name;
		Cursor annotation;
		Cursor type;
		Cursor block;

		StructureInstance* template_parent = nullptr;
		std::unique_ptr<FunctionInstance> singe_instance = nullptr;

		size_t generate_heap_size = 0;

		bool is_generic = false;
		bool generate(CompileContext& ctx, unsigned char* argdata, FunctionInstance*& out);
		bool compile(CompileContext& ctx);

		unsigned int compile_state = 0;
		std::vector<std::tuple<Cursor, Type*>> generic_layout;
	private:
		struct GenericTemplateCompare {
			FunctionTemplate* parent;
			CompileContext ctx;
			bool operator()(const std::pair<unsigned int, unsigned char*>& a, const std::pair<unsigned int, unsigned char*>& b) const;
		};
		GenericTemplateCompare gen_template_cmp;
	public:
		std::unique_ptr<std::map<std::pair<unsigned int, unsigned char*>, std::unique_ptr<FunctionInstance>, GenericTemplateCompare>> instances = nullptr;
	};

	class Declaration {
	public:
		static bool parse_global(Cursor& c, CompileContext& ctx, Namespace& into);
	};

}

#endif