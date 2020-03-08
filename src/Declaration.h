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

	struct StructureMember {
		Cursor name;
		Cursor type;
	};

	class Structure;
	class StructureInstance {
	public:
		std::map<std::string_view,Type> members;
		Structure* generator;
		ILType* iltype = nullptr;

		std::unique_ptr<InstanceType> type;
		bool compile(CompileContext& ctx);

		unsigned int compile_state = 0;
	};

	class Structure : public Namespace {
	public:
		Cursor generic_types;
		std::unique_ptr<DirectType> type;

		std::unique_ptr<StructureInstance> singe_instance = nullptr;

		size_t generate_heap_size = 0;

		ILFunction* generator;
		bool is_generic = false;
		std::vector<StructureMember> members;
		std::map<std::unique_ptr<unsigned char[]>, std::unique_ptr<StructureInstance>> instances;

		bool compile(CompileContext& ctx);
		unsigned int type_compare_heap_size = 0;

		static bool parse(Cursor &c, CompileContext& ctx, std::unique_ptr<Structure>& into);

		unsigned int compile_state = 0;
	};

	class Declaration {
	public:
		static bool parse_global(Cursor& c, CompileContext& ctx, Namespace& into);
	};

}

#endif