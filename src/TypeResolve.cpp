#include "Type.h"
#include "Contents.h"
#include "Error.h"
#include "PredefinedTypes.h"
#include "Utilities.h"
#include "Expression.h"

namespace Corrosive {

	bool Type::resolve_package_in_place(const Type*& t, CompileContext& ctx) {
		const Type* nt = t->resolve_package(ctx);
		if (nt != nullptr) {
			t = nt;
			return true;
		}

		return false;
	}

	const Type* Type::resolve_package(CompileContext& ctx) const {
		return nullptr;
	}

	const Type* FunctionType::resolve_package(CompileContext& ctx) const {
		FunctionType rt = *this;
		bool mod = false;
		bool mod2 = false;

		std::vector<const Type*> rtp = *rt.arguments;

		mod |= resolve_package_in_place(rt.returns,ctx);
		
		for (auto it = rtp.begin(); it != rtp.end(); it++) {
			mod2 |= resolve_package_in_place((*it), ctx);
		}

		if (mod2) {
			rt.arguments = Contents::register_type_array(std::move(rtp));
			mod = true;
		}

		if (mod)
			return Contents::EmplaceType(rt);
		else
			return nullptr;
	}

	const Type* ArrayType::resolve_package(CompileContext& ctx) const {

		ArrayType rt = *this;
		bool mod = false;

		mod |= resolve_package_in_place(rt.base, ctx);

		CompileContextExt cctxext;
		cctxext.basic = ctx;

		Cursor cex = size;
		CompileValue v = Expression::parse(cex, cctxext, CompileType::Eval);

		if (v.t == t_i8 || v.t == t_i16 || v.t == t_i32 || v.t == t_i64) {
			long long cv = LLVMConstIntGetSExtValue(v.v);
			if (cv <= 0) {
				throw_specific_error(size, "Array cannot be created with negative or zero size");
			}
			rt.actual_size = (unsigned int)cv;
		}
		else if (v.t == t_u8 || v.t == t_u16 || v.t == t_u32 || v.t == t_u64) {
			unsigned long long cv = LLVMConstIntGetZExtValue(v.v);
			if (cv == 0) {
				throw_specific_error(size, "Array cannot be created with zero size");
			}
			rt.actual_size = (unsigned int)cv;
		}
		else {
			throw_specific_error(size, "Array type must have constant integer size");
		}

		if (mod)
			return Contents::EmplaceType(rt);
		else 
			return nullptr;
	}



	const Type* InterfaceType::resolve_package(CompileContext& ctx) const {
		InterfaceType rt = *this;
		bool mod = false;
		bool mod2 = false;

		std::vector<const Type*> rtp = *rt.types;

		for (auto it = rtp.begin(); it != rtp.end(); it++) {
			mod2|=resolve_package_in_place(*it, ctx);
		}
		
		if (mod2) {
			rt.types = Contents::register_type_array(std::move(rtp));
			mod = true;
		}

		if (mod)
			return Contents::EmplaceType(rt);
		else
			return nullptr;
	}

	const Type* TupleType::resolve_package(CompileContext& ctx) const {
		TupleType rt = *this;
		bool mod = false;
		bool mod2 = false;

		std::vector<const Type*> rtp = *rt.types;

		for (auto it = rtp.begin(); it != rtp.end(); it++) {
			mod2|=resolve_package_in_place(*it, ctx);
		}

		if (mod2) {
			rt.types = Contents::register_type_array(std::move(rtp));
			mod = true;
		}

		if (mod)
			return Contents::EmplaceType(rt);
		else
			return nullptr;
	}


	const Type* PrimitiveType::resolve_package(CompileContext& ctx) const {
		PrimitiveType rt = *this;
		bool mod = false;
		bool mod2 = false;

		if (rt.templates != nullptr) {
			std::vector<const Type*> tps = *rt.templates;

			for (auto it = tps.begin(); it != tps.end(); it++) {
				mod2 |= resolve_package_in_place(*it, ctx);
			}

			if (mod2) {
				rt.templates = Contents::register_generic_array(std::move(tps));
				mod = true;
			}
		}


		if (package == "") {

			if (ctx.template_ctx != nullptr && ctx.parent_struct != nullptr && ctx.parent_struct->is_generic()) {
				GenericStructDeclaration* gs = (GenericStructDeclaration*)ctx.parent_struct;

				if (gs->generic_typenames.size() != ctx.template_ctx->size()) {
					throw_specific_error(name, "Target structure has different number of generic typenames");
				}

				auto tcf = gs->generic_typenames.find(name.data);
				if (tcf != gs->generic_typenames.end()) {
					const std::variant<unsigned int, const Type*>& tci = (*ctx.template_ctx)[tcf->second];

					if (tci.index() == 0) {
						throw_specific_error(name, "Generic argument referenced is integer, not type");
					}

					const Type* nptr = std::get<1>(tci);
					return nptr->clone_ref(ref);
				}
			}


			std::vector<std::string_view> lookup;

			lookup.push_back(PredefinedNamespace);
			if (ctx.parent_namespace != nullptr) {
				lookup.push_back(ctx.parent_namespace->package);
			}
			lookup.push_back("g");

			if (ctx.parent_namespace != nullptr) {
				size_t ls = lookup.size();
				lookup.resize(ls + ctx.parent_namespace->queue.size());

				for (size_t i = 0; i < ctx.parent_namespace->queue.size(); i++) {
					lookup[ls + i] = ctx.parent_namespace->queue[i];
				}
			}

			for (auto look = lookup.begin(); look != lookup.end(); look++) {

				if (auto td = Contents::find_typedef(*look, name.data)) {
					if (templates != nullptr) {
						//TODO: i can implement this easily, just have to stop being lazy.
						throw_specific_error(name, "Type with generic declaration points to type definition that is not generic.");
					}

					const Type* nt = td->resolve_type();

					return nt->clone_ref(ref);
				}
				else if (auto sd = Contents::find_struct(*look, name.data)) {
					rt.structure = sd;
					rt.package = *look;

					return Contents::EmplaceType(rt);
				}
			}

			throw_specific_error(name, "This type was not found in any package from the lookup queue");

			return nullptr;
		}

		if (mod)
			return Contents::EmplaceType(rt);
		else
			return nullptr;


	}
}