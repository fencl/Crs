#pragma once
#ifndef _declaration_crs_h
#define _declaration_crs_h


#include "CompileContext.hpp"
#include <memory>
#include <vector>
#include "Cursor.hpp"
#include <unordered_map>
#include <map>
#include "Type.hpp"
#include "IL/IL.hpp"
#include "Ast.hpp"

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
		std::size_t generate_heap_size = 0;
		std::vector<std::tuple<Cursor, Type*>> generic_layout;
		static bool valid_generic_argument(Type*);
	};

	class GenericInstance {
	public:
		GenericContext* generator = nullptr;
		unsigned char* key = nullptr;
		void insert_key_on_stack();
	};

	enum class FindNameResultType : std::size_t {
		None = 0,
		Namespace = 1,
		Structure = 2,
		Function = 3,
		Trait = 4,
		Static = 5
	};

	class FindNameResult {
	public:
		FindNameResultType type() { return (FindNameResultType)value.index(); }
		Namespace* get_namespace() { return type() == FindNameResultType::Namespace?std::get<1>(value):nullptr; }
		StructureTemplate* get_structure() { return type() == FindNameResultType::Structure ? std::get<2>(value) : nullptr; }
		FunctionTemplate* get_function() { return type() == FindNameResultType::Function ? std::get<3>(value) : nullptr; }
		TraitTemplate* get_trait() { return type() == FindNameResultType::Trait ? std::get<4>(value) : nullptr; }
		StaticInstance* get_static() { return type() == FindNameResultType::Static ? std::get<5>(value) : nullptr; }

		FindNameResult() : value(nullptr) {}
		FindNameResult(std::nullptr_t v) :value(v) {}
		FindNameResult(Namespace* v) :value(v) {}
		FindNameResult(StructureTemplate* v) :value(v) {}
		FindNameResult(FunctionTemplate* v) :value(v) {}
		FindNameResult(TraitTemplate* v) :value(v) {}
		FindNameResult(StaticInstance* v) :value(v) {}

	private:
		std::variant<std::nullptr_t, Namespace*, StructureTemplate*, FunctionTemplate*, TraitTemplate*, StaticInstance*> value;
	};


	class Namespace {
	public:
		virtual ~Namespace() {}

		AstRegularNode* ast_node;
		Namespace* 	    parent = nullptr;

		std::map<std::string_view, std::pair<std::uint8_t, std::uint32_t>> name_table;

		std::vector<std::unique_ptr<Namespace>>         subnamespaces;
		std::vector<std::unique_ptr<StructureTemplate>> subtemplates;
		std::vector<std::unique_ptr<FunctionTemplate>>  subfunctions;
		std::vector<std::unique_ptr<TraitTemplate>>     subtraits;
		std::vector<std::unique_ptr<StaticInstance>>    substatics;

		FindNameResult find_name(std::string_view name);
	};

	enum class StructureInstanceType {
		primitive_structure, compact_structure, normal_structure
	};

	enum class MemberTableEntryType : std::uint8_t {
		var, alias, func
	};

	class StructureInstance : public Namespace {
	public:
		std::unique_ptr<TypeStructureInstance> type;

		StructureInstanceType structure_type = StructureInstanceType::normal_structure;

		ILDataType      rvalue = ILDataType::word;
		ILSize          size;
		bool            wrapper = false;
		ILContext       context;
		GenericInstance generic_inst;

		std::uint16_t pass_array_id = 0;
		bool          pass_array_operator = false;

		std::map<std::string_view, std::pair<tableelement_t, MemberTableEntryType>>	member_table;
		std::vector<std::pair<Type*, tableelement_t>>                               member_vars;
		std::vector<tableelement_t>                                                 member_composites;
		std::map<TraitInstance*, std::vector<std::unique_ptr<FunctionInstance>>>    traitfunctions;

		errvoid compile();
		unsigned char compile_state = 0;
	};


	class StructureTemplate {
	public:
		Namespace*			parent;
		GenericContext		generic_ctx;
		AstStructureNode*	ast_node;

		std::unique_ptr<TypeStructureTemplate> type;

		errvoid generate(unsigned char* argdata, StructureInstance*& out);
		errvoid compile();

		unsigned char compile_state = 0;

		static void var_wrapper(dword_t dw, Type* type);
		static void var_alias_wrapper(dword_t dw, Type* type);


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
		std::map<std::string_view, std::uint16_t>	member_table;
		std::map<StructureInstance*, std::uint32_t>	vtable_instances;
		std::vector<TypeFunction*>				member_declarations;

		errvoid generate_vtable(StructureInstance* forinst, std::uint32_t& optid);

		Namespace*		parent;
		GenericInstance	generic_inst;
		AstTraitNode*	ast_node;
		ILContext		context;

		std::unique_ptr<TypeTraitInstance> type;
	};

	class TraitTemplate {
	public:
		Namespace*		parent;
		AstTraitNode*	ast_node;
		GenericContext	generic_ctx;

		std::unique_ptr<TypeTraitTemplate> type;

		errvoid generate(unsigned char* argdata, TraitInstance*& out);
		errvoid compile();

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
		ILFunction*		func = nullptr;

		Namespace*		parent;
		TypeFunction*	type;
		GenericInstance generic_inst;
		ILContext		context;

		std::vector<Type*>	arguments;
		Type*				returns;

		AstFunctionDeclarationNode* ast_node;

		errvoid compile();

		unsigned int compile_state = 0;
	};

	class FunctionTemplate {
	public:
		Namespace*						parent = nullptr;
		AstFunctionDeclarationNode*		ast_node;
		GenericContext					generic_ctx;

		std::unique_ptr<TypeFunctionTemplate> type;

		errvoid generate(unsigned char* argdata, FunctionInstance*& out);
		errvoid compile();

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
		GenericInstance* generator = nullptr;
		ILContext		context;
		std::uint32_t sid;

		errvoid compile();
	};
}

#endif