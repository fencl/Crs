#pragma once
#ifndef _predefined_types_crs_h
#define _predefined_types_crs_h
#include <vector>
#include <memory>
#include "Type.h"

namespace Corrosive {
	extern const char* PredefinedNamespace;

	class DefaultTypes {
	public:
		Type t_i8;
		Type t_i16;
		Type t_i32;
		Type t_i64;
		Type t_u8;
		Type t_u16;
		Type t_u32;
		Type t_u64;
		Type t_f32;
		Type t_f64;
		Type t_bool;
		Type t_ptr;
		Type t_ptr_ref;
		Type t_type;
		void setup(CompileContext& ctx);
	private:
		void setup_type(CompileContext& ctx, std::string_view name, Type& into, ILType* t, ILDataType ildt);
	};
}

#endif
