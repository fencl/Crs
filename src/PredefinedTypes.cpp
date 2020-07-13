#include "PredefinedTypes.h"
#include <iostream>

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

		Ctx::global_namespace()->subtemplates[c.buffer] = std::move(s);
	}


	Type* DefaultTypes::get_type_from_rvalue(ILDataType rval) {
		return primitives[(unsigned char)rval];
	}

	void DefaultTypes::setup() {
		setup_type("void", t_void, { ILSizeType::absolute,0 }, ILDataType::none, ILContext::both);
		setup_type("i8",   t_i8,   { ILSizeType::absolute,1 }, ILDataType::i8, ILContext::both);
		setup_type("bool", t_bool, { ILSizeType::absolute,1 }, ILDataType::ibool, ILContext::both);
		setup_type("i16",  t_i16,  { ILSizeType::absolute,2 }, ILDataType::i16, ILContext::both);
		setup_type("i32",  t_i32,  { ILSizeType::absolute,4 }, ILDataType::i32, ILContext::both);
		setup_type("u8",   t_u8,   { ILSizeType::absolute,1 }, ILDataType::u8, ILContext::both);
		setup_type("u16",  t_u16,  { ILSizeType::absolute,2 }, ILDataType::u16, ILContext::both);
		setup_type("u32",  t_u32,  { ILSizeType::absolute,4 }, ILDataType::u32, ILContext::both);
		setup_type("f32",  t_f32,  { ILSizeType::absolute,4 }, ILDataType::f32, ILContext::both);

		setup_type("f64",  t_f64,  { ILSizeType::absolute,8 }, ILDataType::f64, ILContext::both);
		setup_type("i64",  t_i64,  { ILSizeType::absolute,8 }, ILDataType::i64, ILContext::both);
		setup_type("u64",  t_u64,  { ILSizeType::absolute,8 }, ILDataType::u64, ILContext::both);
		setup_type("ptr",  t_ptr,  { ILSizeType::word,1 }, ILDataType::ptr, ILContext::both);
		setup_type("size", t_size, { ILSizeType::word,1 }, ILDataType::size, ILContext::both);
		setup_type("type", t_type, { ILSizeType::word,1 }, ILDataType::ptr, ILContext::compile);

		primitives[(unsigned char)ILDataType::ibool] = t_bool;
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
		primitives[(unsigned char)ILDataType::size] = t_size;

		auto f_malloc_template = std::make_unique<FunctionTemplate>();
		f_malloc_template->name.buffer = "__malloc__";
		f_malloc_template->is_generic = false;
		f_malloc_template->parent = Ctx::global_namespace();
		f_malloc_template->singe_instance = std::make_unique<FunctionInstance>();
		f_malloc_template->singe_instance->parent = Ctx::global_namespace();
		f_malloc_template->singe_instance->returns.second = t_ptr;
		Cursor arg;
		arg.buffer = "size";
		f_malloc_template->singe_instance->arguments.push_back(std::make_pair(arg,t_size));

		f_malloc_template->compile_state = 2;
		f_malloc_template->singe_instance->compile_state = 3;
		f_malloc_template->singe_instance->func = Ctx::global_module()->create_function();
		f_malloc_template->singe_instance->func->alias = "__malloc__";
		std::vector<Type*> arg_array = {t_size};

		TypeFunction* tf = load_or_register_function_type(std::move(arg_array), f_malloc_template->singe_instance->returns.second,ILContext::both);
		f_malloc_template->singe_instance->type = tf;

		ILBlock* f_malloc_block = ((ILBytecodeFunction*)f_malloc_template->singe_instance->func)->create_and_append_block();

		ILBuilder::build_malloc(f_malloc_block);
		ILBuilder::build_ret(f_malloc_block, ILDataType::ptr);

		Ctx::global_namespace()->subfunctions["__malloc__"] = std::move(f_malloc_template);







		auto f_free_template = std::make_unique<FunctionTemplate>();
		f_free_template->name.buffer = "__free__";
		f_free_template->is_generic = false;
		f_free_template->parent = Ctx::global_namespace();
		f_free_template->singe_instance = std::make_unique<FunctionInstance>();
		f_free_template->singe_instance->parent = Ctx::global_namespace();
		f_free_template->singe_instance->returns.second = t_void;

		arg.buffer = "pointer";
		f_free_template->singe_instance->arguments.push_back(std::make_pair(arg, t_ptr));

		f_free_template->compile_state = 2;
		f_free_template->singe_instance->compile_state = 3;
		f_free_template->singe_instance->func = Ctx::global_module()->create_function();
		f_free_template->singe_instance->func->alias = "__free__";
		arg_array = { t_ptr };

		tf = load_or_register_function_type(std::move(arg_array), f_free_template->singe_instance->returns.second, ILContext::both);
		f_free_template->singe_instance->type = tf;

		ILBlock* f_free_block = ((ILBytecodeFunction*)f_free_template->singe_instance->func)->create_and_append_block();

		ILBuilder::build_free(f_free_block);
		ILBuilder::build_ret(f_free_block, ILDataType::none);

		Ctx::global_namespace()->subfunctions["__free__"] = std::move(f_free_template);




		std_lib.load_data("trait Copy(T:type) {fn Copy: (&T);}\ntrait Move(T:type) {fn Move: (&T);}\ntrait Compare(T:type) {fn Compare: (&T) i8;}\ntrait Drop {fn Drop: ();}\ntrait Default {fn Default: ();}\n"
			"fn copy_slice(T: type): (to: []T, from: []T) { let i=0; while(i<from.count) { to[i] = from[i]; i = i+1;} }"
			,"standard_library<buffer>");
		std_lib.pair_braces();
		std_lib.register_debug();

		Cursor c = std_lib.read_first();
		Declaration::parse_global(c, Ctx::global_namespace());
		
		tr_copy = Ctx::global_namespace()->subtraits["Copy"].get();
		tr_move = Ctx::global_namespace()->subtraits["Move"].get();
		tr_compare = Ctx::global_namespace()->subtraits["Compare"].get();
		tr_drop = Ctx::global_namespace()->subtraits["Drop"].get();
		tr_ctor = Ctx::global_namespace()->subtraits["Default"].get();
	}




	size_t DefaultTypes::load_or_register_argument_array(std::vector<Type*> arg_array) {
		return argument_array_storage.register_or_load(std::move(arg_array));
	}

	TypeFunction* DefaultTypes::load_or_register_function_type(std::vector<Type*> arg_array, Type* return_type, ILContext ctx) {
		size_t arg_id = load_or_register_argument_array(std::move(arg_array));

		std::tuple<size_t, Type*,ILContext> key = std::make_tuple(arg_id, return_type, ctx);
		auto t_find = function_types_storage.find(key);
		if (t_find != function_types_storage.end()) {
			return t_find->second.get();
		}
		else {
			std::unique_ptr<TypeFunction> tf = std::make_unique<TypeFunction>();
			tf->argument_array_id = arg_id;
			tf->owner = this;
			tf->return_type = return_type;
			tf->ptr_context = ctx;
			TypeFunction* ret = tf.get();
			function_types_storage[key] = std::move(tf);
			return ret;
		}
	}

	TypeTemplate* DefaultTypes::load_or_register_template_type(std::vector<Type*> arg_array) {
		size_t arg_id = load_or_register_argument_array(std::move(arg_array));

		size_t key = arg_id;

		auto t_find = template_types_storage.find(key);
		if (t_find != template_types_storage.end()) {
			return t_find->second.get();
		}
		else {
			std::unique_ptr<TypeTemplate> tf = std::make_unique<TypeTemplate>();
			tf->argument_array_id = arg_id;
			tf->owner = this;
			TypeTemplate* ret = tf.get();
			template_types_storage[key] = std::move(tf);
			return ret;
		}
	}
}