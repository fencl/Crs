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
#include "Ast.h"

namespace Corrosive {
	class Compiler;

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
	class StaticInstance;

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
		void insert_key_on_stack(Compiler& compiler);
	};


	class Namespace {
	public:
		virtual ~Namespace();
		AstRegularNode* ast_node;

		//Cursor name;
		Namespace* parent = nullptr;
		std::map<std::string_view, std::pair<uint8_t, uint32_t>> name_table;

		std::vector<std::unique_ptr<Namespace>> subnamespaces;
		std::vector<std::unique_ptr<StructureTemplate>> subtemplates;
		std::vector<std::unique_ptr<FunctionTemplate>> subfunctions;
		std::vector<std::unique_ptr<TraitTemplate>> subtraits;
		std::vector<std::unique_ptr<StaticInstance>> substatics;

		void find_name(std::string_view name, Namespace*& subnamespace, StructureTemplate*& subtemplate, FunctionTemplate*& subfunction, TraitTemplate*& subtrait,StaticInstance*& substatic);
	};

	enum class StructureInstanceType {
		primitive_structure, compact_structure, normal_structure
	};

	enum class MemberTableEntryType : uint8_t {
		var, alias, func
	};

	class StructureInstance : public Namespace {
	public:
		Compiler* compiler;
		std::unique_ptr<TypeStructureInstance> type;

		std::map<std::string_view, std::pair<uint16_t, MemberTableEntryType>>	 member_table;
		std::vector<std::pair<Type*, uint32_t>>									 member_vars;
		std::vector<uint16_t>													 member_composites;
		std::map<TraitInstance*, std::vector<std::unique_ptr<FunctionInstance>>> traitfunctions;

		uint16_t		pass_array_id = 0;
		bool			pass_array_operator = false;

		StructureInstanceType structure_type = StructureInstanceType::normal_structure;

		ILDataType		rvalue = ILDataType::word;
		ILSize			size;
		bool			wrapper = false;
		ILContext		context = ILContext::both;
		GenericInstance generic_inst;

		void compile();

		unsigned char compile_state = 0;
	};


	class StructureTemplate {
	public:
		Compiler*			compiler;
		Namespace*			parent;
		GenericContext		generic_ctx;
		AstStructureNode*	ast_node;

		std::unique_ptr<TypeStructureTemplate> type;

		void generate(unsigned char* argdata, StructureInstance*& out);
		void compile();

		unsigned char compile_state = 0;


	private:
		struct GenericTemplateCompare {
			StructureTemplate* parent;
			bool operator()(unsigned char* const& a, unsigned char* const& b) const;
		};
		GenericTemplateCompare gen_template_cmp;
	public:
		std::unique_ptr<StructureInstance> single_instance = nullptr;
		std::unique_ptr<std::map<unsigned char*, std::pair<std::unique_ptr<unsigned char[]>, std::unique_ptr<StructureInstance>>, GenericTemplateCompare>> instances = nullptr;
	};

	class TraitTemplate;
	class TraitInstance {
	public:
		Compiler* compiler;

		std::map<std::string_view, uint16_t>	member_table;
		std::map<StructureInstance*, uint32_t>	vtable_instances;
		std::vector<TypeFunction*>				member_declarations;

		void generate_vtable(StructureInstance* forinst, uint32_t& optid);

		Namespace*		parent;
		GenericInstance	generic_inst;
		AstTraitNode*	ast_node;

		std::unique_ptr<TypeTraitInstance> type;
	};

	class TraitTemplate {
	public:
		Namespace*		parent;
		AstTraitNode*	ast_node;
		GenericContext	generic_ctx;
		Compiler*		compiler;

		std::unique_ptr<TypeTraitTemplate> type;

		void generate(unsigned char* argdata, TraitInstance*& out);
		void compile();

		unsigned char compile_state = 0;

	private:
		struct GenericTemplateCompare {
			TraitTemplate* parent;
			bool operator()(unsigned char* const& a, unsigned char* const& b) const;
		};
		GenericTemplateCompare gen_template_cmp;
	public:
		std::unique_ptr<TraitInstance> single_instance = nullptr;
		std::unique_ptr<std::map<unsigned char*, std::pair<std::unique_ptr<unsigned char[]>, std::unique_ptr<TraitInstance>>, GenericTemplateCompare>> instances = nullptr;
	};

	class FunctionInstance {
	public:

		Compiler*		compiler;
		ILFunction*		func = nullptr;
		Namespace*		parent;
		TypeFunction*	type;
		GenericInstance generic_inst;

		std::vector<std::pair<Cursor, Type*>>	arguments;
		std::pair<Cursor, Type*>				returns;

		AstFunctionDeclarationNode* ast_node;

		void compile();

		unsigned int compile_state = 0;
	};

	class FunctionTemplate {
	public:
		Namespace*						parent = nullptr;
		AstFunctionDeclarationNode*		ast_node;
		GenericContext					generic_ctx;
		Compiler*						compiler;

		std::unique_ptr<TypeFunctionTemplate> type;

		void generate(unsigned char* argdata, FunctionInstance*& out);
		void compile();

		unsigned char compile_state = 0;
	private:
		struct GenericTemplateCompare {
			FunctionTemplate* parent;
			bool operator()(unsigned char* const& a, unsigned char* const& b) const;
		};
		GenericTemplateCompare gen_template_cmp;
	public:
		std::unique_ptr<FunctionInstance> single_instance = nullptr;
		std::unique_ptr<std::map<unsigned char*, std::pair<std::unique_ptr<unsigned char[]>, std::unique_ptr<FunctionInstance>>, GenericTemplateCompare>> instances = nullptr;
	};

	class StaticInstance {
	public:
		unsigned char compile_state = 0;
		Namespace* parent = nullptr;
		AstStaticNode* ast_node;
		Type* type;
		Compiler* compiler;
		GenericInstance* generator = nullptr;
		uint32_t sid;

		void compile();
	};
}

#endif