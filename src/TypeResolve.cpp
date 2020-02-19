#include "Type.h"
#include "Contents.h"
#include "Error.h"
#include "PredefinedTypes.h"
#include "svtoi.h"
#include "Expression.h"

namespace Corrosive {

	bool Type::ResolvePackageInPlace(const Type*& t, CompileContext& ctx) {
		const Type* nt = t->ResolvePackage(ctx);
		if (nt != nullptr) {
			t = nt;
			return true;
		}

		return false;
	}

	const Type* Type::ResolvePackage(CompileContext& ctx) const {
		return nullptr;
	}

	const Type* FunctionType::ResolvePackage(CompileContext& ctx) const {
		FunctionType rt = *this;
		bool mod = false;
		bool mod2 = false;

		std::vector<const Type*> rtp = *rt.Args();

		mod |= ResolvePackageInPlace(rt.returns,ctx);
		
		for (auto it = rtp.begin(); it != rtp.end(); it++) {
			mod2 |= ResolvePackageInPlace((*it), ctx);
		}

		if (mod2) {
			rt.Args() = Contents::RegisterTypeArray(std::move(rtp));
			mod = true;
		}

		if (mod)
			return Contents::EmplaceType(rt);
		else
			return nullptr;
	}

	const Type* ArrayType::ResolvePackage(CompileContext& ctx) const {

		ArrayType rt = *this;
		bool mod = false;

		mod |= ResolvePackageInPlace(rt.base, ctx);

		CompileContextExt cctxext;
		cctxext.basic = ctx;

		Cursor cex = size;
		CompileValue v = Expression::Parse(cex, cctxext, CompileType::Eval);

		if (v.t == t_i8 || v.t == t_i16 || v.t == t_i32 || v.t == t_i64) {
			long long cv = LLVMConstIntGetSExtValue(v.v);
			if (cv <= 0) {
				ThrowSpecificError(size, "Array cannot be created with negative or zero size");
			}
			rt.actual_size = (unsigned int)cv;
		}
		else if (v.t == t_u8 || v.t == t_u16 || v.t == t_u32 || v.t == t_u64) {
			unsigned long long cv = LLVMConstIntGetZExtValue(v.v);
			if (cv == 0) {
				ThrowSpecificError(size, "Array cannot be created with zero size");
			}
			rt.actual_size = (unsigned int)cv;
		}
		else {
			ThrowSpecificError(size, "Array type must have constant integer size");
		}

		if (mod)
			return Contents::EmplaceType(rt);
		else 
			return nullptr;
	}



	const Type* InterfaceType::ResolvePackage(CompileContext& ctx) const {
		InterfaceType rt = *this;
		bool mod = false;
		bool mod2 = false;

		std::vector<const Type*> rtp = *rt.Types();

		for (auto it = rtp.begin(); it != rtp.end(); it++) {
			mod2|=ResolvePackageInPlace(*it, ctx);
		}
		
		if (mod2) {
			rt.Types() = Contents::RegisterTypeArray(std::move(rtp));
			mod = true;
		}

		if (mod)
			return Contents::EmplaceType(rt);
		else
			return nullptr;
	}

	const Type* TupleType::ResolvePackage(CompileContext& ctx) const {
		TupleType rt = *this;
		bool mod = false;
		bool mod2 = false;

		std::vector<const Type*> rtp = *rt.Types();

		for (auto it = rtp.begin(); it != rtp.end(); it++) {
			mod2|=ResolvePackageInPlace(*it, ctx);
		}

		if (mod2) {
			rt.Types() = Contents::RegisterTypeArray(std::move(rtp));
			mod = true;
		}

		if (mod)
			return Contents::EmplaceType(rt);
		else
			return nullptr;
	}


	const Type* PrimitiveType::ResolvePackage(CompileContext& ctx) const {
		PrimitiveType rt = *this;
		bool mod = false;
		bool mod2 = false;

		if (rt.Templates() != nullptr) {
			std::vector<std::variant<unsigned int, const Type*>> tps = *rt.Templates();

			for (auto it = tps.begin(); it != tps.end(); it++) {
				if (it->index() == 1)
					mod2 |= ResolvePackageInPlace(std::get<1>(*it), ctx);
			}

			if (mod2) {
				rt.Templates() = Contents::RegisterGenericArray(std::move(tps));
				mod = true;
			}
		}


		if (package == "") {

			if (ctx.template_ctx != nullptr && ctx.parent_struct != nullptr && ctx.parent_struct->Generic()) {
				GenericStructDeclaration* gs = (GenericStructDeclaration*)ctx.parent_struct;

				if (gs->Generics().size() != ctx.template_ctx->size()) {
					ThrowSpecificError(name, "Target structure has different number of generic typenames");
				}

				auto tcf = gs->Generics().find(name.Data());
				if (tcf != gs->Generics().end()) {
					const std::variant<unsigned int, const Type*>& tci = (*ctx.template_ctx)[tcf->second];

					if (tci.index() == 0) {
						ThrowSpecificError(name, "Generic argument referenced is integer, not type");
					}

					const Type* nptr = std::get<1>(tci);
					return nptr->CloneRef(nptr->Ref() + Ref());
				}
			}


			std::vector<std::string_view> lookup;

			lookup.push_back(PredefinedNamespace);
			if (ctx.parent_namespace != nullptr) {
				lookup.push_back(ctx.parent_namespace->Pack());
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

				if (auto td = Contents::FindTypedef(*look, name.Data())) {
					if (Templates() != nullptr) {
						//TODO: i can implement this easily, just have to stop being lazy.
						ThrowSpecificError(name, "Type with generic declaration points to type definition that is not generic.");
					}

					const Type* nt = td->ResolveType();

					return nt->CloneRef(nt->Ref() + Ref());
				}
				else if (auto sd = Contents::FindStruct(*look, name.Data())) {
					rt.structure_cache = sd;
					rt.package = *look;

					return Contents::EmplaceType(rt);
				}
			}

			ThrowSpecificError(name, "This type was not found in any package from the lookup queue");

			return nullptr;
		}

		if (mod)
			return Contents::EmplaceType(rt);
		else
			return nullptr;


	}
}