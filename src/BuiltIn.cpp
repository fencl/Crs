#include "BuiltIn.hpp"
#include <iostream>
#include "Operand.hpp"
#include "Compiler.hpp"


#ifdef WINDOWS
// for library test
#include <Windows.h>
#endif


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


	Type* BuiltInTypes::get_type_from_rvalue(ILDataType rval) {
		return primitives[(unsigned char)rval];
	}


	void BuiltInCode::print_type(Type* t) {
		t->print(std::cout);
	}


	size_t allocated_counter = 0;

	void* StandardLibraryCode::malloc(size_t size) {
		return std::malloc(size);
		++allocated_counter;
	}

	void* StandardLibraryCode::realloc(void* ptr, size_t size) {
		return std::realloc(ptr, size);
	}

	void StandardLibraryCode::free(void* ref) {
		std::free(ref);
		--allocated_counter;
	}

	void* StandardLibraryCode::share(dword_t slice) {
		std::basic_string_view<char> view((char*)slice.p1, (size_t)slice.p2);
		return LoadLibraryA(std::string(view).c_str());
	}

	void* StandardLibraryCode::function(void* lib, dword_t slice) {
		std::basic_string_view<char> view((char*)slice.p1, (size_t)slice.p2);
		return GetProcAddress((HMODULE)lib, std::string(view).c_str());
	}

	void StandardLibraryCode::release(void* lib) {
		FreeLibrary((HMODULE)lib);
	}

	void StandardLibraryCode::print(dword_t slice) {
		std::basic_string_view<char> sv((char*)slice.p1, (size_t)slice.p2);
		std::cout << sv;
	}

	void BuiltInCode::entry_point(dword_t slice) {
		std::basic_string_view<char> sv((char*)slice.p1, (size_t)slice.p2);
		Compiler::current()->entry_point = sv;
	}

	
	void StandardLibraryCode::link(ILModule* mod) {
		mod->try_link("std::print_slice", StandardLibraryCode::print);
		mod->try_link("std::malloc", StandardLibraryCode::malloc);
		mod->try_link("std::free", StandardLibraryCode::free);
		mod->try_link("std::realloc", StandardLibraryCode::realloc);
		mod->try_link("std::library::share", StandardLibraryCode::share);
		mod->try_link("std::library::function", StandardLibraryCode::function);
		mod->try_link("std::library::release", StandardLibraryCode::release);
	}

	void BuiltInCode::ask_for(dword_t slice) {
		std::basic_string_view<char> data_string((char*)slice.p1, (size_t)slice.p2);

		if (data_string == "compiler_standard_libraries") {
			Compiler::current()->register_native_function({ "std","print_slice" }, StandardLibraryCode::print);
			Compiler::current()->register_native_function({ "std","malloc" }, StandardLibraryCode::malloc);
			Compiler::current()->register_native_function({ "std","realloc" }, StandardLibraryCode::realloc);
			Compiler::current()->register_native_function({ "std","free" }, StandardLibraryCode::free);
			Compiler::current()->register_native_function({ "std","library","share" }, StandardLibraryCode::share);
			Compiler::current()->register_native_function({ "std","library","function" }, StandardLibraryCode::function);
			Compiler::current()->register_native_function({ "std","library","release" }, StandardLibraryCode::release);
		}
	}

	
	Type* BuiltInCode::build_reference(Type* t) {
		return t->generate_reference();
	}

	Type* BuiltInCode::build_subtype(Type* t, dword_t slice) {
		std::string_view slice_str((char*)slice.p1, (size_t)slice.p2);

		Type* str = Compiler::current()->types()->t_u8->generate_slice();

		if (t->type() == TypeInstanceType::type_structure_instance) {
			TypeStructureInstance* tsi = (TypeStructureInstance*)t;

			auto res = tsi->owner->find_name(slice_str);

			if (auto struct_template = res.get_structure()) {
				struct_template->compile();
				if (!struct_template->ast_node->is_generic) {
					StructureInstance* sinst;
					struct_template->generate(nullptr, sinst);
					sinst->compile();
					return sinst->type.get();
				}
				else {
					return struct_template->type.get();
				}
			}
			else if (auto function_temaplte = res.get_function()) {
				function_temaplte->compile();
				return function_temaplte->type.get();
			}
			else if (auto trait_template = res.get_trait()) {
				trait_template->compile();
				if (!trait_template->ast_node->is_generic) {
					TraitInstance* trait_instance;
					trait_template->generate(nullptr, trait_instance);
					return trait_instance->type.get();
				}
				else {
					return trait_template->type.get();
				}
			}

		}

		return nullptr;
	}


	Type* BuiltInCode::build_array(uint32_t size, Type* t) {
		return t->generate_array(size);
	}

	Type* BuiltInCode::build_slice(Type* t) {
		return t->generate_slice();
	}

	
	size_t BuiltInCode::type_size(Type* t) {
		return t->size().eval(Compiler::current()->global_module(), compiler_arch);
	}

	

	void BuiltInCode::compile() {
		if (!Compiler::current()->entry_point.empty()) {
			auto res = Compiler::current()->find_name(Compiler::current()->entry_point);

			ILFunction* main = nullptr;

			if (auto fun = res.get_function()) {
				FunctionInstance* finst;
				fun->generate(nullptr, finst);
				finst->compile();
				main = finst->func;
			}
			else {
				throw std::exception("Entry point was set but function not found");
			}

			Compiler::current()->global_module()->entry_point = main;
			Compiler::current()->global_module()->exported_functions.push_back(main);
		}
		else {
			for (auto&& ft : Compiler::current()->exported_functions) {
				FunctionInstance* finst;
				ft->generate(nullptr, finst);
				finst->compile();
				Compiler::current()->global_module()->exported_functions.push_back(finst->func);
			}
		}
	}

	void BuiltInTypes::setup() {
		auto scope = ScopeState().context(ILContext::compile);

		std_lib.load_data(
			"static compile ptr = &void;\n"
			"fn(T: type) copy_slice: (to: []T, from: []T) { let i=0; while(i<from.count) { to[i] = from[i]; i = i+1;} }\n"
			"fn(T: type) invalidate_slice: (slice: &[]T) { slice.ptr = null; slice.size=0; }\n"
			"namespace compiler {\n"
			"fn compile native reference_of: (type) type;\n"
			"fn compile native array_of: (type, u32) type;\n"
			"fn compile native native_subtype_of: (type,[]u8) type;\n"
			"fn compile native slice_of: (type) type;\n"
			"fn compile native type_size: (type) size;\n"
			"fn compile native require: ([]u8);\n"
			"fn compile native entry: ([]u8);\n"
			"fn compile native build: ();\n"
			"fn compile native var: ([]u8, type);\n"
			"fn compile native var_alias: ([]u8, type);\n"
			"fn compile native link: ([]u8);\n"
			"fn compile native print_type: (type);\n"
			"}"
			, "standard_library<buffer>");

		std_lib.pair_tokens();
		std_lib.register_debug();
		std_lib.root_node = AstRootNode::parse(&std_lib);
		std_lib.root_node->populate();

		setup_type("void", t_void, { ILSizeType::_0,0 }, ILDataType::none, ILContext::both, std_lib.root_node.get());
		setup_type("i8", t_i8, { ILSizeType::abs8,1 }, ILDataType::i8, ILContext::both, std_lib.root_node.get());
		setup_type("bool", t_bool, { ILSizeType::abs8,1 }, ILDataType::i8, ILContext::both, std_lib.root_node.get());
		setup_type("i16", t_i16, { ILSizeType::abs16,1 }, ILDataType::i16, ILContext::both, std_lib.root_node.get());
		setup_type("i32", t_i32, { ILSizeType::abs32,1 }, ILDataType::i32, ILContext::both, std_lib.root_node.get());
		setup_type("u8", t_u8, { ILSizeType::abs8,1 }, ILDataType::u8, ILContext::both, std_lib.root_node.get());
		setup_type("u16", t_u16, { ILSizeType::abs16,1 }, ILDataType::u16, ILContext::both, std_lib.root_node.get());
		setup_type("u32", t_u32, { ILSizeType::abs32,1 }, ILDataType::u32, ILContext::both, std_lib.root_node.get());
		setup_type("f32", t_f32, { ILSizeType::absf32,1 }, ILDataType::f32, ILContext::both, std_lib.root_node.get());
		setup_type("f64", t_f64, { ILSizeType::absf64,1 }, ILDataType::f64, ILContext::both, std_lib.root_node.get());
		setup_type("i64", t_i64, { ILSizeType::abs64,1 }, ILDataType::i64, ILContext::both, std_lib.root_node.get());
		setup_type("u64", t_u64, { ILSizeType::abs64,1 }, ILDataType::u64, ILContext::both, std_lib.root_node.get());
		setup_type("size", t_size, { ILSizeType::word,1 }, ILDataType::word, ILContext::both, std_lib.root_node.get());
		setup_type("type", t_type, { ILSizeType::ptr,1 }, ILDataType::word, ILContext::compile, std_lib.root_node.get());

		t_ptr = t_void->generate_reference();

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

		f_build_reference = Compiler::current()->register_native_function({ "compiler","reference_of" }, BuiltInCode::build_reference);
		f_build_array = Compiler::current()->register_native_function({ "compiler","array_of" }, BuiltInCode::build_array);
		f_build_subtype = Compiler::current()->register_native_function({ "compiler","subtype_of" }, BuiltInCode::build_subtype);
		f_build_slice = Compiler::current()->register_native_function({ "compiler","slice_of" }, BuiltInCode::build_slice);
		f_type_size = Compiler::current()->register_native_function({ "compiler","type_size" }, BuiltInCode::type_size);
		f_type_size = Compiler::current()->register_native_function({ "compiler","require" }, Source::require_wrapper);
		f_type_size = Compiler::current()->register_native_function({ "compiler","build" }, BuiltInCode::compile);
		f_type_size = Compiler::current()->register_native_function({ "compiler","var" }, StructureTemplate::var_wrapper);
		f_type_size = Compiler::current()->register_native_function({ "compiler","var_alias" }, StructureTemplate::var_alias_wrapper);
		f_type_size = Compiler::current()->register_native_function({ "compiler","entry" }, BuiltInCode::entry_point);
		f_type_size = Compiler::current()->register_native_function({ "compiler","link" }, BuiltInCode::ask_for);
		f_type_size = Compiler::current()->register_native_function({ "compiler","print_type" }, BuiltInCode::print_type);

		std::vector<Type*> args;
		t_build_script = load_or_register_function_type(ILCallingConvention::bytecode, std::move(args), t_void, ILContext::compile);
	}




	std::pair<size_t, bool> BuiltInTypes::load_or_register_argument_array(std::vector<Type*> arg_array) {
		return argument_array_storage.register_or_load(std::move(arg_array));
	}

	TypeFunction* BuiltInTypes::load_or_register_function_type(ILCallingConvention call_conv, std::vector<Type*> arg_array, Type* return_type, ILContext ctx) {
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

	TypeTemplate* BuiltInTypes::load_or_register_template_type(std::vector<Type*> arg_array) {
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