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

	class GenericInstance;

	class GenericContext {
	public:
		GenericInstance* generator = nullptr;
		size_t generate_heap_size = 0;
		std::vector<std::tuple<Cursor, Type*>> generic_layout;
	};

	class GenericInstance {
	public:
		GenericContext* generator = nullptr;
		unsigned char* key = nullptr;
		void insert_key_on_stack(ILEvaluator* eval);
	};


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

		static void parse_inner(Cursor& c, Namespace* into, GenericInstance* gen_inst);
		static void parse(Cursor& c, std::unique_ptr<Namespace>& into);
		void find_name(std::string_view name, Namespace*& subnamespace, StructureTemplate*& subtemplate, FunctionTemplate*& subfunction, TraitTemplate*& subtrait);
	};

	enum class StructureInstanceType {
		primitive_structure, compact_structure, normal_structure
	};


	class StructureInstance : public Namespace {
	public:
		std::map<std::string_view, std::pair<uint16_t,bool>> member_table;
		std::vector<std::pair<Type*,uint32_t>> member_vars;
		std::vector<uint16_t> member_composites;

		std::map<TraitInstance*, std::vector<std::unique_ptr<FunctionInstance>>> traitfunctions;

		StructureInstanceType structure_type = StructureInstanceType::normal_structure;
		ILDataType rvalue = ILDataType::ptr;

		ILSize size;
		bool wrapper = false;

		ILContext context = ILContext::both;
		
		GenericInstance generic_inst;

		bool has_special_constructor = false;
		bool has_special_destructor = false;
		bool has_special_copy = false;
		bool has_special_move = false;
		bool has_special_compare = false;


		ILFunction* auto_constructor = nullptr;
		ILFunction* auto_destructor = nullptr;
		ILFunction* auto_copy = nullptr;
		ILFunction* auto_move = nullptr;
		ILFunction* auto_compare = nullptr;

		FunctionInstance* impl_copy = nullptr;
		FunctionInstance* impl_move = nullptr;
		FunctionInstance* impl_compare = nullptr;
		FunctionInstance* impl_drop = nullptr;
		FunctionInstance* impl_ctor = nullptr;

		void build_automatic_constructor();
		void build_automatic_destructor();
		void build_automatic_move();
		void build_automatic_copy();
		void build_automatic_compare();

		std::unique_ptr<TypeStructureInstance> type;
		void compile();

		unsigned int compile_state = 0;
	};

	struct StructureTemplateMemberVar {
		Cursor name;
		Cursor type;
		bool composite;
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
		Namespace* parent = nullptr;
		Cursor name;
		Cursor annotation;

		std::unique_ptr<TypeStructureTemplate> type;

		std::unique_ptr<StructureInstance> singe_instance = nullptr;

		bool is_generic = false;

		GenericContext generic_ctx;

		std::vector<StructureTemplateMemberVar> member_vars;
		std::vector<StructureTemplateMemberFunc> member_funcs;
		std::vector<StructureTemplateSubtemplate> member_templates;
		std::vector<StructureTemplateImpl> member_implementation;

		void generate(unsigned char* argdata, StructureInstance*& out);

		void compile();

		static void parse(Cursor &c, Namespace* parent, GenericInstance* gen_inst, std::unique_ptr<StructureTemplate>& into);

		unsigned int compile_state = 0;

	private:
		struct GenericTemplateCompare {
			StructureTemplate* parent;
			Ctx ctx;
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
		ILContext ctx = ILContext::both;
	};

	class TraitInstance {
	public:
		std::map<std::string_view, size_t> member_table;
		std::vector<TraitInstanceMemberRecord> member_funcs;

		std::map<StructureInstance*, uint32_t> vtable_instances;

		void generate_vtable(StructureInstance* forinst, uint32_t& optid);

		Cursor name;
		Namespace* parent = nullptr;

		ILContext context = ILContext::both;

		GenericInstance generic_inst;

		std::unique_ptr<TypeTraitInstance> type;
	};

	struct TraitTemplateMemberFunc {
		Cursor name;
		Cursor type;
		ILContext ctx = ILContext::both;
	};

	class TraitTemplate {
	public:
		Namespace* parent = nullptr;
		Cursor name;
		Cursor annotation;

		std::unique_ptr<TypeTraitTemplate> type;

		std::unique_ptr<TraitInstance> singe_instance = nullptr;

		bool is_generic = false;

		GenericContext generic_ctx;

		std::vector<TraitTemplateMemberFunc> member_funcs;

		void generate(unsigned char* argdata, TraitInstance*& out);

		void compile();

		static void parse(Cursor& c, Namespace* parent, GenericInstance* gen_inst, std::unique_ptr<TraitTemplate>& into);

		unsigned int compile_state = 0;

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

		GenericInstance generic_inst;

		std::vector<std::pair<Cursor,Type*>> arguments;
		std::pair<Cursor, Type*> returns;
		ILContext context = ILContext::both;

		void compile();

		unsigned int compile_state = 0;

	};

	class FunctionTemplate {
	public:

		Namespace* parent = nullptr;
		Cursor name;
		Cursor annotation;
		Cursor decl_type;
		Cursor block;

		ILContext context = ILContext::both;
		std::unique_ptr<TypeFunctionTemplate> type;

		//StructureInstance* template_parent = nullptr;
		std::unique_ptr<FunctionInstance> singe_instance = nullptr;

		GenericContext generic_ctx;

		bool is_generic = false;
		void generate(unsigned char* argdata, FunctionInstance*& out);
		void compile();

		unsigned int compile_state = 0;
	private:
		struct GenericTemplateCompare {
			FunctionTemplate* parent;
			bool operator()(unsigned char* const& a, unsigned char* const& b) const;
		};
		GenericTemplateCompare gen_template_cmp;
	public:
		std::unique_ptr<std::map<unsigned char*, std::pair<std::unique_ptr<unsigned char[]>, std::unique_ptr<FunctionInstance>>, GenericTemplateCompare>> instances = nullptr;
	};

	class Declaration {
	public:
		static void parse_global(Cursor& c, Namespace* into);
	};

}

#endif