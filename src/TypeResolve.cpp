#include "Type.h"
#include "Contents.h"
#include "Error.h"
#include "PredefinedTypes.h"
#include "Utilities.h"
#include "Expression.h"

namespace Corrosive {

	bool Type::resolve_package_in_place(CompileContext& ctx,const Type*& t, bool& mod) {
		const Type* nt;
		if (!t->resolve_package(ctx,nt)) return false;

		if (nt != nullptr) {
			t = nt;
			mod = true;
		}

		return true;
	}

	bool Type::resolve_package(CompileContext& ctx, const Type*& into) const {
		into = nullptr;
		return true;
	}

	bool FunctionType::resolve_package(CompileContext& ctx, const Type*& into) const {
		FunctionType rt = *this;
		bool mod = false;
		bool mod2 = false;

		std::vector<const Type*> rtp = *rt.arguments;

		if (!resolve_package_in_place(ctx, rt.returns, mod)) return false;
		
		for (auto it = rtp.begin(); it != rtp.end(); it++) {
			if (!resolve_package_in_place(ctx, (*it), mod2)) false;
		}

		if (mod2) {
			rt.arguments = Contents::register_type_array(std::move(rtp));
			mod = true;
		}

		if (mod)
			into = Contents::emplace_type(rt);
		else
			into = nullptr;

		return true;
	}

	bool ArrayType::resolve_package(CompileContext& ctx, const Type*& into) const {

		ArrayType rt = *this;
		bool mod = false;

		if (!resolve_package_in_place(ctx, rt.base, mod)) return false;

		if (mod)
			into = Contents::emplace_type(rt);
		else 
			into = nullptr;

		return true;
	}



	bool InterfaceType::resolve_package(CompileContext& ctx, const Type*& into) const {
		InterfaceType rt = *this;
		bool mod = false;
		bool mod2 = false;

		std::vector<const Type*> rtp = *rt.types;

		for (auto it = rtp.begin(); it != rtp.end(); it++) {
			if (!resolve_package_in_place(ctx, *it, mod2)) false;
		}
		
		if (mod2) {
			rt.types = Contents::register_type_array(std::move(rtp));
			mod = true;
		}

		if (mod)
			into = Contents::emplace_type(rt);
		else
			into = nullptr;

		return true;
	}

	bool TupleType::resolve_package(CompileContext& ctx, const Type*& into) const {
		TupleType rt = *this;
		bool mod = false;
		bool mod2 = false;

		std::vector<const Type*> rtp = *rt.types;

		for (auto it = rtp.begin(); it != rtp.end(); it++) {
			if (!resolve_package_in_place(ctx, *it, mod2)) return false;
		}

		if (mod2) {
			rt.types = Contents::register_type_array(std::move(rtp));
			mod = true;
		}

		if (mod)
			into = Contents::emplace_type(rt);
		else
			into = nullptr;
		return true;
	}


	bool PrimitiveType::resolve_package(CompileContext& ctx, const Type*& into) const {
		PrimitiveType rt = *this;
		bool mod = false;
		bool mod2 = false;

		if (rt.templates != nullptr) {
			std::vector<const Type*> tps = *rt.templates;

			for (auto it = tps.begin(); it != tps.end(); it++) {
				if (!resolve_package_in_place(ctx, *it, mod2)) return false;
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
					return false;
				}

				auto tcf = gs->generic_typenames.find(name.buffer);
				if (tcf != gs->generic_typenames.end()) {
					const std::variant<unsigned int, const Type*>& tci = (*ctx.template_ctx)[tcf->second];

					if (tci.index() == 0) {
						throw_specific_error(name, "Generic argument referenced is integer, not type");
						return false;
					}

					const Type* nptr = std::get<1>(tci);
					into= nptr->clone_ref(ref);
					return true;
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

				if (auto td = Contents::find_typedef(*look, name.buffer)) {
					if (templates != nullptr) {
						//TODO: i can implement this easily, just have to stop being lazy.
						throw_specific_error(name, "Type with generic declaration points to type definition that is not generic.");
						return false;
					}

					const Type* nt;
					if (!td->resolve_type(nt)) return false;

					into = nt->clone_ref(ref);
					return true;
				}
				else if (auto sd = Contents::find_struct(*look, name.buffer)) {
					rt.structure = sd;
					rt.package = *look;

					into = Contents::emplace_type(rt);
					return true;
				}
			}

			throw_specific_error(name, "This type was not found in any package from the lookup queue");
			return false;
		}

		if (mod)
			into = Contents::emplace_type(rt);
		else
			into = nullptr;

		return true;
	}
}