#include "PredefinedTypes.h"
#include <iostream>
#include "Operand.h"

namespace Corrosive {

	const char* PredefinedNamespace = "corrosive";

	
	void DefaultTypes::setup_type(std::string_view name,Type*& into, ILSize size, ILDataType ildt,ILContext context) {
		std::unique_ptr<StructureTemplate> s = std::make_unique<StructureTemplate>();
		Cursor c;
		c.buffer = name;

		s->parent = Ctx::global_namespace();
		s->name = c;
		s->parent = Ctx::global_namespace();

		s->compile();
		StructureInstance* sinst;
		s->generate(nullptr, sinst);
		sinst->compile();

		sinst->context = context;
		sinst->size = size;
		sinst->rvalue = ildt;
		sinst->structure_type = StructureInstanceType::primitive_structure;

		into = sinst->type.get();

		Ctx::global_namespace()->name_table[c.buffer] = std::make_pair((uint8_t)1, (uint32_t)Ctx::global_namespace()->subtemplates.size());
		Ctx::global_namespace()->subtemplates.push_back(std::move(s));
	}


	Type* DefaultTypes::get_type_from_rvalue(ILDataType rval) {
		return primitives[(unsigned char)rval];
	}

	void DefaultTypes::setup() {
		setup_type("void", t_void, { ILSizeType::absolute,0 }, ILDataType::none, ILContext::both);
		setup_type("i8",   t_i8,   { ILSizeType::absolute,1 }, ILDataType::i8, ILContext::both);
		setup_type("bool", t_bool, { ILSizeType::absolute,1 }, ILDataType::i8, ILContext::both);
		setup_type("i16",  t_i16,  { ILSizeType::absolute,2 }, ILDataType::i16, ILContext::both);
		setup_type("i32",  t_i32,  { ILSizeType::absolute,4 }, ILDataType::i32, ILContext::both);
		setup_type("u8",   t_u8,   { ILSizeType::absolute,1 }, ILDataType::u8, ILContext::both);
		setup_type("u16",  t_u16,  { ILSizeType::absolute,2 }, ILDataType::u16, ILContext::both);
		setup_type("u32",  t_u32,  { ILSizeType::absolute,4 }, ILDataType::u32, ILContext::both);
		setup_type("f32",  t_f32,  { ILSizeType::absolute,4 }, ILDataType::f32, ILContext::both);

		setup_type("f64",  t_f64,  { ILSizeType::absolute,8 }, ILDataType::f64, ILContext::both);
		setup_type("i64",  t_i64,  { ILSizeType::absolute,8 }, ILDataType::i64, ILContext::both);
		setup_type("u64",  t_u64,  { ILSizeType::absolute,8 }, ILDataType::u64, ILContext::both);
		setup_type("ptr",  t_ptr,  { ILSizeType::word,1 }, ILDataType::word, ILContext::both);
		setup_type("size", t_size, { ILSizeType::word,1 }, ILDataType::word, ILContext::both);
		setup_type("type", t_type, { ILSizeType::word,1 }, ILDataType::word, ILContext::compile);

		primitives[(unsigned char)ILDataType::u8] = t_u8;
		primitives[(unsigned char)ILDataType::u16] = t_u16;
		primitives[(unsigned char)ILDataType::u32] = t_u32;
		primitives[(unsigned char)ILDataType::u64] = t_u64;
		primitives[(unsigned char)ILDataType::i8] = t_i8;
		primitives[(unsigned char)ILDataType::i16] = t_i16;
		primitives[(unsigned char)ILDataType::i32] = t_i32;
		primitives[(unsigned char)ILDataType::i64] = t_i64;
		primitives[(unsigned char)ILDataType::f32] = t_f32;
		primitives[(unsigned char)ILDataType::f64] = t_f64;
		primitives[(unsigned char)ILDataType::word] = t_size;

		std_lib.load_data("trait Copy(T:type) {fn Copy: (&T);}\ntrait Move(T:type) {fn Move: (&T);}\ntrait Compare(T:type) {fn Compare: (&T) i8;}\ntrait Drop {fn Drop: ();}\ntrait Default {fn Default: ();}\n"
			"fn copy_slice(T: type): (to: []T, from: []T) { let i=0; while(i<from.count) { to[i] = from[i]; i = i+1;} }\n"
			"fn invalidate_slice(T: type): (slice: &[]T) { (*slice).ptr = null; (*slice).size=0; }\n"
			"namespace compiler {\n"
				"fn compile ext reference_of: (type) type;\n"
				"fn compile ext array_of: (type, u32) type;\n"
				"fn compile ext subtype_of: (type, []u8) type;\n"
				"fn compile ext slice_of: (type) type;\n"
				"fn compile ext type_size: (type) size;\n"
			"}"
			,"standard_library<buffer>");
		std_lib.pair_braces();
		std_lib.register_debug();

		Cursor c = std_lib.read_first();
		Declaration::parse_global(c, Ctx::global_namespace());
		
		tr_copy = Ctx::global_namespace()->subtraits[Ctx::global_namespace()->name_table["Copy"].second].get();
		tr_move = Ctx::global_namespace()->subtraits[Ctx::global_namespace()->name_table["Move"].second].get();
		tr_compare = Ctx::global_namespace()->subtraits[Ctx::global_namespace()->name_table["Compare"].second].get();
		tr_drop = Ctx::global_namespace()->subtraits[Ctx::global_namespace()->name_table["Drop"].second].get();
		tr_ctor = Ctx::global_namespace()->subtraits[Ctx::global_namespace()->name_table["Default"].second].get();

		f_build_reference = Ctx::register_ext_function({ "compiler","reference_of" }, Operand::priv_build_reference);
		f_build_array = Ctx::register_ext_function({ "compiler","array_of" }, Operand::priv_build_array);
		f_build_subtype = Ctx::register_ext_function({ "compiler","subtype_of" }, Operand::priv_build_subtype);
		f_build_slice = Ctx::register_ext_function({ "compiler","slice_of" }, Operand::priv_build_slice);
		f_type_size = Ctx::register_ext_function({ "compiler","type_size" }, Operand::priv_type_size);
	}




	std::pair<size_t,bool> DefaultTypes::load_or_register_argument_array(std::vector<Type*> arg_array) {
		return argument_array_storage.register_or_load(std::move(arg_array));
	}

	TypeFunction* DefaultTypes::load_or_register_function_type(std::vector<Type*> arg_array, Type* return_type, ILContext ctx) {
		std::pair<size_t,bool> arg_id = load_or_register_argument_array(std::move(arg_array));

		std::tuple<size_t, Type*,ILContext> key = std::make_tuple(arg_id.first, return_type, ctx);
		auto t_find = function_types_storage.find(key);
		if (t_find != function_types_storage.end()) {
			return t_find->second.get();
		}
		else {
			std::unique_ptr<TypeFunction> tf = std::make_unique<TypeFunction>();
			tf->argument_array_id = arg_id.first;
			tf->owner = this;
			tf->return_type = return_type;
			tf->ptr_context = ctx;
			tf->il_function_decl = UINT32_MAX;

			TypeFunction* ret = tf.get();
			function_types_storage[key] = std::move(tf);
			return ret;
		}
	}

	TypeTemplate* DefaultTypes::load_or_register_template_type(std::vector<Type*> arg_array) {
		std::pair<size_t, bool> arg_id = load_or_register_argument_array(std::move(arg_array));

		size_t key = arg_id.first;

		auto t_find = template_types_storage.find(key);
		if (t_find != template_types_storage.end()) {
			return t_find->second.get();
		}
		else {
			std::unique_ptr<TypeTemplate> tf = std::make_unique<TypeTemplate>();
			tf->argument_array_id = arg_id.first;
			tf->owner = this;
			TypeTemplate* ret = tf.get();
			template_types_storage[key] = std::move(tf);
			return ret;
		}
	}
}