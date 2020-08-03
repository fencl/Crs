#include "PredefinedTypes.h"
#include <iostream>
#include "Operand.h"
#include "Compiler.h"


// for library test
#include <Windows.h>

namespace Corrosive {

	const char* PredefinedNamespace = "corrosive";


	void setup_type(std::string_view name, Type*& into, ILSize size, ILDataType ildt, ILContext context, AstRootNode* root) {

		std::unique_ptr<AstStructureNode> node = std::make_unique<AstStructureNode>();
		node->is_generic = false;
		node->name_string = name;
		node->parent = root;

		std::unique_ptr<StructureTemplate> s = std::make_unique<StructureTemplate>();
		s->ast_node = node.get();
		Cursor c;

		s->parent = Compiler::current()->global_namespace();

		s->compile_state = 2;
		s->single_instance = std::make_unique<StructureInstance>();

		s->single_instance->compile_state = 3;
		s->single_instance->context = context;
		s->single_instance->size = size;
		s->single_instance->rvalue = ildt;
		s->single_instance->structure_type = StructureInstanceType::primitive_structure;
		s->single_instance->type = std::make_unique<TypeStructureInstance>();
		s->single_instance->type->owner = s->single_instance.get();
		s->single_instance->ast_node = node.get();


		into = s->single_instance->type.get();

		 Compiler::current()->global_namespace()->name_table[name] = std::make_pair((uint8_t)1, (uint32_t)Compiler::current()->global_namespace()->subtemplates.size());
		 Compiler::current()->global_namespace()->subtemplates.push_back(std::move(s));

		root->global_namespace->structures.push_back(std::move(node));
	}


	Type* DefaultTypes::get_type_from_rvalue(ILDataType rval) {
		return primitives[(unsigned char)rval];
	}


	void print_provider(ILEvaluator* eval) {

		auto ptr = eval->pop_register_value<dword_t>();
		const char* text = (const char*)ptr.p1;
		size_t size = (size_t)ptr.p2;

		std::basic_string_view<char> sv(text, size);
		std::cout << sv;
	}

	size_t allocated_counter = 0;

	void malloc_provider(ILEvaluator* eval) {
		auto size = eval->pop_register_value<size_t>();
		auto ref = malloc(size);
		eval->write_register_value(ref);
		++allocated_counter;
	}

	void free_provider(ILEvaluator* eval) {
		auto ref = eval->pop_register_value<void*>();
		free(ref);
		--allocated_counter;
	}

	void share_provider(ILEvaluator* eval) {
		auto ptr = eval->pop_register_value<dword_t>();
		const char* text = (const char*)ptr.p1;
		size_t size = (size_t)ptr.p2;
		std::basic_string_view<char> sv(text, size);
		void* lib = LoadLibraryA(std::string(sv).c_str());
		eval->write_register_value<void*>(lib);
	}

	void function_provider(ILEvaluator* eval) {
		auto ptr = eval->pop_register_value<dword_t>();
		const char* text = (const char*)ptr.p1;
		size_t size = (size_t)ptr.p2;
		std::basic_string_view<char> sv(text, size);
		auto lib = eval->pop_register_value<void*>();
		eval->write_register_value<void*>(GetProcAddress((HMODULE)lib, std::string(sv).c_str()));
	}


	void release_provider(ILEvaluator* eval) {
		auto lib = eval->pop_register_value<void*>();
		FreeLibrary((HMODULE)lib);
	}


	void DefaultTypes::ask_for(ILEvaluator* eval) {

		auto ptr = eval->pop_register_value<dword_t>();
		const char* data = (const char*)ptr.p1;
		size_t size = (size_t)ptr.p2;
		std::basic_string_view<char> data_string(data, size);

		if (data_string == "compiler_standard_libraries") {
			Compiler::current()->register_ext_function({ "std","print_slice" }, print_provider);
			Compiler::current()->register_ext_function({ "std","malloc" }, malloc_provider);
			Compiler::current()->register_ext_function({ "std","free" }, free_provider);
			Compiler::current()->register_ext_function({ "std","library","share" }, share_provider);
			Compiler::current()->register_ext_function({ "std","library","function" }, function_provider);
			Compiler::current()->register_ext_function({ "std","library","release" }, release_provider);
		}
	}

	void DefaultTypes::setup() {

		std_lib.load_data(
			"fn(T: type) copy_slice: (to: []T, from: []T) { let i=0; while(i<from.count) { to[i] = from[i]; i = i+1;} }\n"
			"fn(T: type) invalidate_slice: (slice: &[]T) { (*slice).ptr = null; (*slice).size=0; }\n"
			"namespace compiler {\n"
			"fn compile ext reference_of: (type) type;\n"
			"fn compile ext array_of: (type, u32) type;\n"
			"fn compile ext subtype_of: (type, []u8) type;\n"
			"fn compile ext slice_of: (type) type;\n"
			"fn compile ext type_size: (type) size;\n"
			"fn compile ext require: ([]u8);\n"
			"fn compile ext ask_for: ([]u8);\n"
			"}"
			, "standard_library<buffer>");

		std_lib.pair_tokens();
		std_lib.register_debug();
		std_lib.root_node = AstRootNode::parse(&std_lib);
		std_lib.root_node->populate();

		setup_type("void", t_void, { ILSizeType::absolute,0 }, ILDataType::none, ILContext::both, std_lib.root_node.get());
		setup_type("i8", t_i8, { ILSizeType::absolute,1 }, ILDataType::i8, ILContext::both, std_lib.root_node.get());
		setup_type("bool", t_bool, { ILSizeType::absolute,1 }, ILDataType::i8, ILContext::both, std_lib.root_node.get());
		setup_type("i16", t_i16, { ILSizeType::absolute,2 }, ILDataType::i16, ILContext::both, std_lib.root_node.get());
		setup_type("i32", t_i32, { ILSizeType::absolute,4 }, ILDataType::i32, ILContext::both, std_lib.root_node.get());
		setup_type("u8", t_u8, { ILSizeType::absolute,1 }, ILDataType::u8, ILContext::both, std_lib.root_node.get());
		setup_type("u16", t_u16, { ILSizeType::absolute,2 }, ILDataType::u16, ILContext::both, std_lib.root_node.get());
		setup_type("u32", t_u32, { ILSizeType::absolute,4 }, ILDataType::u32, ILContext::both, std_lib.root_node.get());
		setup_type("f32", t_f32, { ILSizeType::absolute,4 }, ILDataType::f32, ILContext::both, std_lib.root_node.get());
		setup_type("f64", t_f64, { ILSizeType::absolute,8 }, ILDataType::f64, ILContext::both, std_lib.root_node.get());
		setup_type("i64", t_i64, { ILSizeType::absolute,8 }, ILDataType::i64, ILContext::both, std_lib.root_node.get());
		setup_type("u64", t_u64, { ILSizeType::absolute,8 }, ILDataType::u64, ILContext::both, std_lib.root_node.get());
		setup_type("ptr", t_ptr, { ILSizeType::word,1 }, ILDataType::word, ILContext::both, std_lib.root_node.get());
		setup_type("size", t_size, { ILSizeType::word,1 }, ILDataType::word, ILContext::both, std_lib.root_node.get());
		setup_type("type", t_type, { ILSizeType::word,1 }, ILDataType::word, ILContext::compile, std_lib.root_node.get());

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

		f_build_reference = Compiler::current()->register_ext_function({ "compiler","reference_of" }, Operand::priv_build_reference);
		f_build_array = Compiler::current()->register_ext_function({ "compiler","array_of" }, Operand::priv_build_array);
		f_build_subtype = Compiler::current()->register_ext_function({ "compiler","subtype_of" }, Operand::priv_build_subtype);
		f_build_slice = Compiler::current()->register_ext_function({ "compiler","slice_of" }, Operand::priv_build_slice);
		f_type_size = Compiler::current()->register_ext_function({ "compiler","type_size" }, Operand::priv_type_size);
		f_type_size = Compiler::current()->register_ext_function({ "compiler","require" }, Source::require_wrapper);
		f_type_size = Compiler::current()->register_ext_function({ "compiler","ask_for" }, DefaultTypes::ask_for);

		std::vector<Type*> args;
		t_build_script = load_or_register_function_type(ILCallingConvention::bytecode, std::move(args), t_void, ILContext::compile);
	}




	std::pair<size_t, bool> DefaultTypes::load_or_register_argument_array(std::vector<Type*> arg_array) {
		return argument_array_storage.register_or_load(std::move(arg_array));
	}

	TypeFunction* DefaultTypes::load_or_register_function_type(ILCallingConvention call_conv, std::vector<Type*> arg_array, Type* return_type, ILContext ctx) {
		std::pair<size_t, bool> arg_id = load_or_register_argument_array(std::move(arg_array));

		std::tuple<ILCallingConvention, size_t, Type*, ILContext> key = std::make_tuple(call_conv, arg_id.first, return_type, ctx);
		auto t_find = function_types_storage.find(key);
		if (t_find != function_types_storage.end()) {
			return t_find->second.get();
		}
		else {
			std::unique_ptr<TypeFunction> tf = std::make_unique<TypeFunction>();
			tf->argument_array_id = arg_id.first;
			tf->call_conv = call_conv;
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