#define _CRT_SECURE_NO_WARNINGS
#include "BuiltIn.hpp"
#include <iostream>
#include "Operand.hpp"
#include "Compiler.hpp"
#include <cstdio>


#ifdef WINDOWS
// for library test
#include <Windows.h>
#endif

#ifdef LINUX
// for library test
#include <dlfcn.h>
#endif


namespace Corrosive {

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

		 Compiler::current()->global_namespace()->name_table[name] = std::make_pair((std::uint8_t)1, (std::uint32_t)Compiler::current()->global_namespace()->subtemplates.size());
		 Compiler::current()->global_namespace()->subtemplates.push_back(std::move(s));

		root->global_namespace->structures.push_back(std::move(node));
	}


	Type* BuiltInTypes::get_type_from_rvalue(ILDataType rval) {
		return primitives[(unsigned char)rval];
	}


	void BuiltInCode::print_type(Type* t) {
		t->print(std::cout);
	}


	std::size_t allocated_counter = 0;

	void* StandardLibraryCode::malloc(std::size_t size) {
		return std::malloc(size);
		++allocated_counter;
	}

	void* StandardLibraryCode::realloc(void* ptr, std::size_t size) {
		return std::realloc(ptr, size);
	}

	
	void* StandardLibraryCode::stdout_stream() {
		return stdout;
	}
	
	void* StandardLibraryCode::stderr_stream() {
		return stderr;
	}

	void StandardLibraryCode::fwrite(void* f, dword_t slice) {
		std::fwrite(slice.p1,1,(std::size_t)slice.p2, (FILE*)f);
	}
	
	void StandardLibraryCode::fclose(void* f) {
		std::fclose((FILE*)f);
	}

	void* StandardLibraryCode::fopen(dword_t slice, std::uint8_t m) {
		std::basic_string_view<char> view((char*)slice.p1, (std::size_t)slice.p2);
		std::string file(view);
		const char* mode;
		if ((m & 0x01) == 0x01) {
			if ((m&0x08) == 0x08) {
				mode = "wb";
			} else {
				mode = "w";
			}
		}else if ((m & 0x02) == 0x02) {
			if ((m&0x08) == 0x08) {
				mode = "rb";
			} else {
				mode = "r";
			}
		}else if ((m & 0x04) == 0x04) {
			if ((m&0x08) == 0x08) {
				mode = "ab";
			} else {
				mode = "a";
			}
		}
		
		return std::fopen(file.c_str(), mode);
	}


	void StandardLibraryCode::free(void* ref) {
		std::free(ref);
		--allocated_counter;
	}

	void* StandardLibraryCode::share(dword_t slice) {
		std::basic_string_view<char> view((char*)slice.p1, (std::size_t)slice.p2);
	#ifdef WINDOWS
		return LoadLibraryA(std::string(view).c_str());
	#endif

	#ifdef LINUX
		return dlopen(std::string(view).c_str(), RTLD_LAZY);
	#endif
	}

	void* StandardLibraryCode::function(void* lib, dword_t slice) {
		std::basic_string_view<char> view((char*)slice.p1, (std::size_t)slice.p2);
	#ifdef WINDOWS
		return (void*)GetProcAddress((HMODULE)lib, std::string(view).c_str());
	#endif
	#ifdef LINUX
		return (void*)dlsym(lib, std::string(view).c_str());
	#endif
	}

	void StandardLibraryCode::release(void* lib) {
	#ifdef WINDOWS
		FreeLibrary((HMODULE)lib);
	#endif

	#ifdef LINUX
		dlclose(lib);
	#endif
	}
	
	void StandardLibraryCode::link(ILModule* mod) {
		mod->try_link("std::malloc", (void*)StandardLibraryCode::malloc);
		mod->try_link("std::free", (void*)StandardLibraryCode::free);
		mod->try_link("std::realloc", (void*)StandardLibraryCode::realloc);
		mod->try_link("std::library::share", (void*)StandardLibraryCode::share);
		mod->try_link("std::library::function", (void*)StandardLibraryCode::function);
		mod->try_link("std::library::release", (void*)StandardLibraryCode::release);
		mod->try_link("std::stream::stdout", (void*)StandardLibraryCode::stdout_stream);
		mod->try_link("std::stream::stderr", (void*)StandardLibraryCode::stderr_stream);
		mod->try_link("std::stream::write", (void*)StandardLibraryCode::fwrite);
		mod->try_link("std::stream::close", (void*)StandardLibraryCode::fclose);
		mod->try_link("std::stream::open", (void*)StandardLibraryCode::fopen);
	}

	void BuiltInCode::ask_for(dword_t slice) {
		std::basic_string_view<char> data_string((char*)slice.p1, (std::size_t)slice.p2);

		if (data_string == "crsstdlib") {
			FunctionInstance* r;
			if (!Compiler::current()->precompile_native_function(r, "std::malloc", (void*)StandardLibraryCode::malloc)) { ILEvaluator::ex_throw(); return; }
			if (!Compiler::current()->precompile_native_function(r, "std::realloc", (void*)StandardLibraryCode::realloc)) { ILEvaluator::ex_throw(); return; }
			if (!Compiler::current()->precompile_native_function(r, "std::free", (void*)StandardLibraryCode::free)) { ILEvaluator::ex_throw(); return; }
			if (!Compiler::current()->precompile_native_function(r, "std::library::share", (void*)StandardLibraryCode::share)) { ILEvaluator::ex_throw(); return; }
			if (!Compiler::current()->precompile_native_function(r, "std::library::function", (void*)StandardLibraryCode::function)) { ILEvaluator::ex_throw(); return; }
			if (!Compiler::current()->precompile_native_function(r, "std::library::release", (void*)StandardLibraryCode::release)) { ILEvaluator::ex_throw(); return; }
			if (!Compiler::current()->precompile_native_function(r, "std::stream::stdout", (void*)StandardLibraryCode::stdout_stream)) { ILEvaluator::ex_throw(); return; }
			if (!Compiler::current()->precompile_native_function(r, "std::stream::stderr", (void*)StandardLibraryCode::stderr_stream)) { ILEvaluator::ex_throw(); return; }
			if (!Compiler::current()->precompile_native_function(r, "std::stream::write", (void*)StandardLibraryCode::fwrite)) { ILEvaluator::ex_throw(); return; }
			if (!Compiler::current()->precompile_native_function(r, "std::stream::close", (void*)StandardLibraryCode::fclose)) { ILEvaluator::ex_throw(); return; }
			if (!Compiler::current()->precompile_native_function(r, "std::stream::open", (void*)StandardLibraryCode::fopen)) { ILEvaluator::ex_throw(); return; }
		}
	}

	
	Type* BuiltInCode::build_reference(Type* t) {
		if (!Type::assert(t)) {ILEvaluator::ex_throw(); return nullptr;}
		return t->generate_reference();
	}

	Type* BuiltInCode::build_subtype(Type* t, dword_t slice) {
		if (!Type::assert(t)) {ILEvaluator::ex_throw();return nullptr;}

		std::string_view slice_str((char*)slice.p1, (std::size_t)slice.p2);

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


	Type* BuiltInCode::build_array(std::uint32_t size, Type* t) {
		Type::assert(t);
		return t->generate_array(size);
	}

	Type* BuiltInCode::build_slice(Type* t) {
		Type::assert(t);
		return t->generate_slice();
	}

	
	std::size_t BuiltInCode::type_size(Type* t) {
		Type::assert(t);
		return t->size().eval(Compiler::current()->global_module());
	}

	

	void BuiltInCode::compile(std::uint8_t t) {
		if (t == 1) {
			auto res = Compiler::current()->find_name("main");

			ILFunction* main = nullptr;

			if (auto fun = res.get_function()) {
				FunctionInstance* finst;
				if (!fun->generate(nullptr, finst)){ ILEvaluator::ex_throw(); return;}
				auto res = finst->compile();
				if (!res){ ILEvaluator::ex_throw(); return; }
				main = finst->func;
				if (finst->context != ILContext::runtime) {
					throw_runtime_exception(ILEvaluator::active, "Entry point must be declared as runtime-only function.");
					ILEvaluator::ex_throw();
				}
			}
			else {
				throw_runtime_exception(ILEvaluator::active, "Entry point was not found. Please implement main function inside global namespace.");
				ILEvaluator::ex_throw();
			}

			Compiler::current()->global_module()->entry_point = main;
			Compiler::current()->global_module()->exported_functions.push_back(main);
		}
		else if (t == 2) {
			for (auto&& ft : Compiler::current()->exported_functions) {
				FunctionInstance* finst;
				if (!ft->generate(nullptr, finst)) {ILEvaluator::ex_throw();return;}
				if (!finst->compile()){ILEvaluator::ex_throw(); return;}
				Compiler::current()->global_module()->exported_functions.push_back(finst->func);
			}
		}
	}

	errvoid BuiltInTypes::setup() {
		auto scope = ScopeState().context(ILContext::compile);

		std_lib.load_data(
			"static compile ptr = &void;\n"
			"fn(T: type) copy_slice: (to: []T, from: []T) { let i=0; while(i<from.count) { to[i] = from[i]; i = i+1;} }\n"
			"fn(T: type) invalidate_slice: (slice: &[]T) { slice.ptr = null; slice.size=0; }\n"
			"namespace compiler {\n"
			"fn compile native reference_of: (type) type;\n"
			"fn compile native array_of: (type, u32) type;\n"
			"fn compile native subtype_of: (type,[]u8) type;\n"
			"fn compile native slice_of: (type) type;\n"
			"fn compile native type_size: (type) size;\n"
			"fn compile native require: ([]u8);\n"
			"fn compile native build: (u8);\n"
			"fn compile native var: ([]u8, type);\n"
			"fn compile native include: (type);\n"
			"fn compile native var_alias: ([]u8, type);\n"
			"fn compile native link: ([]u8);\n"
			"fn compile native print_type: (type);\n"
			#ifdef WINDOWS
			"static compile platform = cast(u8) 1u;"
			#endif
			#ifdef LINUX
			"static compile platform = cast(u8) 2u;"
			#endif
			"}\n"
			"namespace platform {\n"
			"static compile windows = cast(u8) 1u;\n"
			"static compile linux = cast(u8) 2u;\n"
			"static compile macos = cast(u8) 3u;\n"
			"}\n"
			"namespace binary {\n"
			"static compile program = cast(u8) 1u;\n"
			"static compile library = cast(u8) 2u;\n"
			"}\n"
			, "standard_library<buffer>");

		if (!std_lib.pair_tokens()) return err::fail;
		std_lib.register_debug();
		std::unique_ptr<AstRootNode> node;
		if (!AstRootNode::parse(node, &std_lib)) return err::fail;
		
		std_lib.root_node = std::move(node);
		if (!std_lib.root_node->populate()) return err::fail;

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
		
		if (!Compiler::current()->precompile_native_function(f_build_reference, "compiler::reference_of", (void*)BuiltInCode::build_reference)) return err::fail;
		if (!Compiler::current()->precompile_native_function(f_build_array, "compiler::array_of", (void*)BuiltInCode::build_array)) return err::fail;
		if (!Compiler::current()->precompile_native_function(f_build_subtype, "compiler::subtype_of", (void*)BuiltInCode::build_subtype)) return err::fail;
		if (!Compiler::current()->precompile_native_function(f_build_slice, "compiler::slice_of" , (void*)BuiltInCode::build_slice)) return err::fail;
		if (!Compiler::current()->precompile_native_function(f_type_size, "compiler::type_size", (void*)BuiltInCode::type_size)) return err::fail;
		if (!Compiler::current()->precompile_native_function(f_type_size, "compiler::require", (void*)Source::require_wrapper)) return err::fail;
		if (!Compiler::current()->precompile_native_function(f_type_size, "compiler::build", (void*)BuiltInCode::compile)) return err::fail;
		if (!Compiler::current()->precompile_native_function(f_type_size, "compiler::var", (void*)StructureTemplate::var_wrapper)) return err::fail;
		if (!Compiler::current()->precompile_native_function(f_type_size, "compiler::include", (void*)StructureTemplate::include_wrapper)) return err::fail;
		if (!Compiler::current()->precompile_native_function(f_type_size, "compiler::var_alias", (void*)StructureTemplate::var_alias_wrapper)) return err::fail;
		if (!Compiler::current()->precompile_native_function(f_type_size, "compiler::link", (void*)BuiltInCode::ask_for)) return err::fail;
		if (!Compiler::current()->precompile_native_function(f_type_size, "compiler::print_type", (void*)BuiltInCode::print_type)) return err::fail;

		std::vector<Type*> args;
		t_build_script = load_or_register_function_type(ILCallingConvention::bytecode, std::move(args), t_void, ILContext::compile);

		return err::ok;
	}




	std::pair<std::size_t, bool> BuiltInTypes::load_or_register_argument_array(std::vector<Type*> arg_array) {
		return argument_array_storage.register_or_load(std::move(arg_array));
	}

	TypeFunction* BuiltInTypes::load_or_register_function_type(ILCallingConvention call_conv, std::vector<Type*> arg_array, Type* return_type, ILContext ctx) {
		std::pair<std::size_t, bool> arg_id = load_or_register_argument_array(std::move(arg_array));

		std::tuple<ILCallingConvention, std::size_t, Type*, ILContext> key = std::make_tuple(call_conv, arg_id.first, return_type, ctx);
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
		std::pair<std::size_t, bool> arg_id = load_or_register_argument_array(std::move(arg_array));

		std::size_t key = arg_id.first;

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