#pragma once
#ifndef _predefined_types_crs_h
#define _predefined_types_crs_h
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include "Type.hpp"
#include "Source.hpp"

namespace Corrosive {
	class Compiler;
	extern const char* PredefinedNamespace;

	template <typename T>
	class idset {
	public:
		inline std::pair<std::size_t,bool> register_or_load(T val) {
			if (data.size() == 0) data.resize(2);
			data[0] = std::move(val);

			auto lf = lookup.find(0);
			if (lf != lookup.end()) {
				return std::make_pair(lf->second, false);
			}
			else {
				data.push_back(std::move(data[0]));
				lookup[data.size() - 1] = data.size() - 1;
				return std::make_pair(data.size() - 1, true);
			}
		}

		inline T& get(std::size_t id) {
			return data[id];
		}

		inline idset() : lookup(cmp(this)) {

		}

	private:
		struct cmp {
			inline cmp(idset<T>* own) : owner(own) {

			}

			idset<T>* owner;
			inline bool operator()(const std::size_t& a, const std::size_t& b) const {
				return owner->data[a] < owner->data[b];
			}
		};

		std::vector<T> data;
		std::map<std::size_t, std::size_t, cmp> lookup;
	};

	class FunctionInstance;

	class StandardLibraryCode {
	public:
		static void* 	share(dword_t slice);
		static void* 	function(void* lib, dword_t slice);
		static void 	release(void* lib);

		static void* 	malloc(std::size_t size);
		static void* 	realloc(void* ptr, std::size_t size);
		static void 	free(void* ref);

		static void     link(ILModule* mod);

		static void*	stdout_stream();
		static void*	stderr_stream();
		static void		fwrite(void* f, dword_t slice);
		static void		fclose(void* f);
		static void*	fopen(dword_t slice, std::uint8_t m);
	};

	class BuiltInCode {
	public:
		static void 	ask_for(dword_t slice);
		static void 	print_type(Type* t);
		static void 	compile(std::uint8_t);
		static Type* 	build_array(std::uint32_t size, Type* t);
		static Type* 	build_reference(Type* t);
		static Type* 	build_subtype(Type* t, dword_t slice);
		static Type* 	build_slice(Type* t);
		static std::size_t 	type_size(Type* t);
	};

	class BuiltInTypes {
	public:
		Type* t_i8;
		Type* t_i16;
		Type* t_i32;
		Type* t_i64;
		Type* t_u8;
		Type* t_u16;
		Type* t_u32;
		Type* t_u64;
		Type* t_f32;
		Type* t_f64;
		Type* t_bool;
		Type* t_ptr;
		Type* t_type;
		Type* t_void;
		Type* t_size;
		TypeFunction* t_build_script;

		Type* primitives [(unsigned char)ILDataType::undefined];

		Type* get_type_from_rvalue(ILDataType rval);

		idset<std::vector<Type*>> argument_array_storage;
		std::map<std::tuple<ILCallingConvention, std::size_t,Type*,ILContext>, std::unique_ptr<TypeFunction>> function_types_storage;
		std::map<std::size_t, std::unique_ptr<TypeTemplate>> template_types_storage;


		std::pair<std::size_t, bool> load_or_register_argument_array(std::vector<Type*> arg_array);

		TypeFunction* load_or_register_function_type(ILCallingConvention call_conv, std::vector<Type*> arg_array, Type* return_type, ILContext ctx);
		TypeTemplate* load_or_register_template_type(std::vector<Type*> arg_array);

		Source std_lib;

		FunctionInstance* f_build_reference;
		FunctionInstance* f_build_array;
		FunctionInstance* f_build_slice;
		FunctionInstance* f_build_subtype;
		FunctionInstance* f_type_size;

		errvoid setup();
	};
}

#endif
