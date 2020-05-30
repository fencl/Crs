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
	class TraitTemplate;
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
		std::map<std::string_view, std::unique_ptr<TraitTemplate>> subtraits;

		static bool parse_inner(Cursor& c, Namespace* into);
		static bool parse(Cursor& c, std::unique_ptr<Namespace>& into);
		void find_name(std::string_view name, Namespace*& subnamespace, StructureTemplate*& subtemplate, FunctionTemplate*& subfunction, TraitTemplate*& subtrait);
	};

	enum class StructureInstanceType {
		primitive_structure, compact_structure, normal_structure
	};

	class StructureInstance : public Namespace {
	public:
		std::map<std::string_view, size_t> member_table;
		std::vector<std::pair<Type*,ILSize>> member_vars;

		std::map<TraitInstance*, std::vector<std::unique_ptr<FunctionInstance>>> traitfunctions;

		StructureTemplate* generator;

		StructureInstanceType structure_type = StructureInstanceType::normal_structure;
		ILDataType rvalue = ILDataType::ptr;

		ILSize size;
		ILSize alignment;

		ILContext context = ILContext::both;
		unsigned char* key;

		bool has_special_constructor = false;
		bool has_special_destructor = false;
		bool has_special_copy = false;
		bool has_special_move = false;
		bool has_special_compare = false;

		void insert_key_on_stack(ILEvaluator* eval);

		ILFunction* auto_constructor = nullptr;
		ILFunction* auto_destructor = nullptr;
		ILFunction* auto_copy = nullptr;
		ILFunction* auto_move = nullptr;
		ILFunction* auto_compare = nullptr;

		void build_automatic_constructor();
		void build_automatic_destructor();
		void build_automatic_move();
		void build_automatic_copy();
		void build_automatic_compare();

		std::unique_ptr<TypeStructureInstance> type;
		bool compile();

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
		ILContext context;
	};

	struct StructureTemplateImplFunc {
		Cursor name;
		Cursor type;
		Cursor block;
	};

	struct StructureTemplateSubtemplate {
		Cursor cursor;
	};

	struct StructureTemplateImpl {
		Cursor type;
		std::vector<StructureTemplateImplFunc> functions;
	};


	class StructureTemplate {
	public:
		inline virtual ~StructureTemplate() {}
		Namespace* parent = nullptr;
		Cursor name;
		Cursor annotation;
		StructureInstance* template_parent = nullptr;

		std::unique_ptr<TypeStructureTemplate> type;

		std::unique_ptr<StructureInstance> singe_instance = nullptr;

		size_t generate_heap_size = 0;

		bool is_generic = false;
		std::vector<StructureTemplateMemberVar> member_vars;
		std::vector<StructureTemplateMemberFunc> member_funcs;
		std::vector<StructureTemplateSubtemplate> member_templates;
		std::vector<StructureTemplateImpl> member_implementation;

		bool generate(unsigned char* argdata, StructureInstance*& out);

		bool compile();

		static bool parse(Cursor &c, Namespace* parent, std::unique_ptr<StructureTemplate>& into);

		unsigned int compile_state = 0;
		std::vector<std::tuple<Cursor,Type*>> generic_layout;

	private:
		struct GenericTemplateCompare {
			StructureTemplate* parent;
			CompileContext ctx;
			bool operator()(unsigned char* const& a, unsigned char* const& b) const;
		};
		GenericTemplateCompare gen_template_cmp;
	public:
		std::unique_ptr<std::map<unsigned char*, std::pair<std::unique_ptr<unsigned char[]>,std::unique_ptr<StructureInstance>>, GenericTemplateCompare>> instances = nullptr;
	};

	class TraitTemplate;

	struct TraitInstanceMemberRecord {
		Cursor definition;
		Cursor name;

		TypeFunction* type;

		//uint32_t offset;
		//uint32_t compile_offset;
	};

	class TraitInstance  {
	public:
		std::map<std::string_view, size_t> member_table;
		std::vector<TraitInstanceMemberRecord> member_funcs;

		std::map<StructureInstance*, uint32_t> vtable_instances;

		bool generate_vtable(StructureInstance* forinst, uint32_t& optid);

		Cursor name;
		Namespace* parent = nullptr;

		ILContext context = ILContext::both;

		TraitTemplate* generator;

		unsigned char* key;

		std::unique_ptr<TypeTraitInstance> type;

		void insert_key_on_stack(ILEvaluator* eval);
	};

	struct TraitTemplateMemberFunc {
		Cursor name;
		Cursor type;
	};

	class TraitTemplate {
	public:
		inline virtual ~TraitTemplate() {}
		Namespace* parent = nullptr;
		Cursor name;
		Cursor annotation;
		StructureInstance* template_parent = nullptr;

		std::unique_ptr<TypeTraitTemplate> type;

		std::unique_ptr<TraitInstance> singe_instance = nullptr;

		size_t generate_heap_size = 0;

		bool is_generic = false;

		std::vector<TraitTemplateMemberFunc> member_funcs;

		bool generate(unsigned char* argdata, TraitInstance*& out);

		bool compile();

		static bool parse(Cursor& c, Namespace* parent, std::unique_ptr<TraitTemplate>& into);

		unsigned int compile_state = 0;
		std::vector<std::tuple<Cursor, Type*>> generic_layout;

	private:
		struct GenericTemplateCompare {
			TraitTemplate* parent;
			bool operator()(unsigned char* const& a, unsigned char* const& b) const;
		};
		GenericTemplateCompare gen_template_cmp;
	public:
		std::unique_ptr<std::map<unsigned char*, std::pair<std::unique_ptr<unsigned char[]>, std::unique_ptr<TraitInstance>>, GenericTemplateCompare>> instances = nullptr;
	};

	class FunctionInstance {
	public:
		ILFunction* func = nullptr;
		Namespace* parent;

		Cursor block;
		Cursor name;

		Type* type;

		std::vector<std::pair<Cursor,Type*>> arguments;
		std::pair<Cursor, Type*> returns;
		ILContext context = ILContext::both;

		unsigned char* key;

		bool compile();

		unsigned int compile_state = 0;

	};

	class FunctionTemplate {
	public:
		inline virtual ~FunctionTemplate() {}

		Namespace* parent = nullptr;
		Cursor name;
		Cursor annotation;
		Cursor decl_type;
		Cursor block;

		ILContext context = ILContext::both;
		std::unique_ptr<TypeFunctionTemplate> type;

		StructureInstance* template_parent = nullptr;
		std::unique_ptr<FunctionInstance> singe_instance = nullptr;

		size_t generate_heap_size = 0;

		bool is_generic = false;
		bool generate(unsigned char* argdata, FunctionInstance*& out);
		bool compile();

		unsigned int compile_state = 0;
		std::vector<std::tuple<Cursor, Type*>> generic_layout;
	private:
		struct GenericTemplateCompare {
			FunctionTemplate* parent;
			bool operator()(const std::pair<unsigned int, unsigned char*>& a, const std::pair<unsigned int, unsigned char*>& b) const;
		};
		GenericTemplateCompare gen_template_cmp;
	public:
		std::unique_ptr<std::map<std::pair<unsigned int, unsigned char*>, std::unique_ptr<FunctionInstance>, GenericTemplateCompare>> instances = nullptr;
	};

	class Declaration {
	public:
		static bool parse_global(Cursor& c, Namespace* into);
	};

}

#endif