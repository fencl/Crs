#pragma once
#ifndef _predefined_types_crs_h
#define _predefined_types_crs_h
#include <vector>
#include <memory>
#include "Declaration.h"


namespace Corrosive {
	extern const char* PredefinedNamespace;
	void init_predefined_types(std::vector<std::unique_ptr<Declaration>>& into);
	extern const Type* t_i8;
	extern const Type* t_i16;
	extern const Type* t_i32;
	extern const Type* t_i64;
	extern const Type* t_u8;
	extern const Type* t_u16;
	extern const Type* t_u32;
	extern const Type* t_u64;
	extern const Type* t_f32;
	extern const Type* t_f64;
	extern const Type* t_bool;
	extern const Type* t_ptr;
	extern const Type* t_ptr_ref;
}

#endif
