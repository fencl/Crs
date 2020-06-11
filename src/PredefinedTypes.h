#pragma once
#ifndef _predefined_types_crs_h
#define _predefined_types_crs_h
#include <vector>
#include <memory>
#include "Type.h"
#include "Source.h"

namespace Corrosive {
	extern const char* PredefinedNamespace;

	template <typename T>
	class idset {
	public:
		inline size_t register_or_load(T val) {
			if (data.size() == 0) data.resize(2);
			data[0] = std::move(val);

			auto lf = lookup.find(0);
			if (lf != lookup.end()) {
				return lf->second;
			}
			else {
				data.push_back(std::move(data[0]));
				lookup[data.size() - 1] = data.size() - 1;
				return data.size() - 1;
			}
		}

		inline T& get(size_t id) {
			return data[id];
		}

		inline idset() : lookup(cmp(this)) {

		}

	private:
		struct cmp {
			inline cmp(idset<T>* own) : owner(own) {

			}

			idset<T>* owner;
			inline bool operator()(const size_t& a, const size_t& b) const {
				return owner->data[a] < owner->data[b];
			}
		};

		std::vector<T> data;
		std::map<size_t, size_t, cmp> lookup;
	};


	class DefaultTypes {
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

		Cursor debug_info;

		static bool priv_debug_cursor(ILEvaluator* eval_ctx);

		Type* primitives [256];

		Type* get_type_from_rvalue(ILDataType rval);

		idset<Cursor> debug_cursor_storage;
		idset<std::vector<Type*>> argument_array_storage;
		std::map<std::tuple<size_t,Type*,ILContext>, std::unique_ptr<TypeFunction>> function_types_storage;
		std::map<size_t, std::unique_ptr<TypeTemplate>> template_types_storage;


		size_t load_or_register_argument_array(std::vector<Type*> arg_array);
		size_t load_or_register_debug_cursor(Cursor c);

		TypeFunction* load_or_register_function_type(std::vector<Type*> arg_array, Type* return_type, ILContext ctx);
		TypeTemplate* load_or_register_template_type(std::vector<Type*> arg_array);

		Source std_lib;

		TraitTemplate* tr_copy;
		TraitTemplate* tr_move;
		TraitTemplate* tr_compare;
		TraitTemplate* tr_drop;

		bool setup();
	private:
		void setup_type(std::string_view name, Type*& into, ILSize size, ILSize alignment, ILDataType ildt,ILContext runtime);
	};
}

#endif
