#include "PredefinedTypes.h"
#include "Contents.h"
namespace Corrosive {

	const char* PredefinedNamespace = "corrosive";

	const Type* t_i8 = nullptr;
	const Type* t_i16 = nullptr;
	const Type* t_i32 = nullptr;
	const Type* t_i64 = nullptr;
	const Type* t_u8 = nullptr;
	const Type* t_u16 = nullptr;
	const Type* t_u32 = nullptr;
	const Type* t_u64 = nullptr;
	const Type* t_f32 = nullptr;
	const Type* t_f64 = nullptr;
	const Type* t_bool = nullptr;
	const Type* t_ptr = nullptr;
	const Type* t_ptr_ref = nullptr;

	void InitPrimitiveType(std::string_view name, NamespaceDeclaration* p_nspc, StructDeclarationType decl_type,const Corrosive::Type* & into) {
		Cursor n_c;
		std::unique_ptr<StructDeclaration> new_type = std::make_unique<StructDeclaration>();
		n_c.Data(name);
		new_type->decl_type = decl_type;
		new_type->name = n_c;
		new_type->package = PredefinedNamespace;
		new_type->parent_pack = p_nspc;
		new_type->parent = p_nspc;
		Contents::register_struct(PredefinedNamespace, name, new_type.get());
		p_nspc->members.push_back(std::move(new_type));

		PrimitiveType pt_new_type;
		pt_new_type.name = n_c;
		pt_new_type.package = PredefinedNamespace;
		into = Contents::EmplaceType(pt_new_type);
	}

	void InitPredefinedTypes(std::vector<std::unique_ptr<Declaration>>& into) {
		Contents::register_namespace(PredefinedNamespace);
		std::unique_ptr<NamespaceDeclaration> p_nspc = std::make_unique<NamespaceDeclaration>();
		Cursor n_c; n_c.Data(PredefinedNamespace);
		p_nspc->name = n_c;
		p_nspc->package =PredefinedNamespace;
		p_nspc->parent_pack = p_nspc.get();


		
		InitPrimitiveType("u8",   p_nspc.get(), StructDeclarationType::t_u8,   Corrosive::t_u8);
		InitPrimitiveType("u16",  p_nspc.get(), StructDeclarationType::t_u16,  Corrosive::t_u16);
		InitPrimitiveType("u32",  p_nspc.get(), StructDeclarationType::t_u32,  Corrosive::t_u32);
		InitPrimitiveType("u64",  p_nspc.get(), StructDeclarationType::t_u64,  Corrosive::t_u64);
		InitPrimitiveType("i8",   p_nspc.get(), StructDeclarationType::t_i8,   Corrosive::t_i8);
		InitPrimitiveType("i16",  p_nspc.get(), StructDeclarationType::t_i16,  Corrosive::t_i16);
		InitPrimitiveType("i32",  p_nspc.get(), StructDeclarationType::t_i32,  Corrosive::t_i32);
		InitPrimitiveType("i64",  p_nspc.get(), StructDeclarationType::t_i64,  Corrosive::t_i64);
		InitPrimitiveType("f32",  p_nspc.get(), StructDeclarationType::t_f32,  Corrosive::t_f32);
		InitPrimitiveType("f64",  p_nspc.get(), StructDeclarationType::t_f64,  Corrosive::t_f64);
		InitPrimitiveType("bool", p_nspc.get(), StructDeclarationType::t_bool, Corrosive::t_bool);
		InitPrimitiveType("ptr",  p_nspc.get(), StructDeclarationType::t_ptr,  Corrosive::t_ptr);

		PrimitiveType pt_ptr_ref = *(PrimitiveType*)Corrosive::t_ptr;
		pt_ptr_ref.ref = true;
		Corrosive::t_ptr_ref = Contents::EmplaceType(pt_ptr_ref);



		std::unique_ptr<GenericStructDeclaration> t_array = std::make_unique<GenericStructDeclaration>();
		n_c.Data("array");
		t_array->decl_type = StructDeclarationType::t_array;
		t_array->name =n_c;
		t_array->package = PredefinedNamespace;
		t_array->parent_pack = p_nspc.get();
		t_array->parent = p_nspc.get();
		t_array->generic_typenames["T"] = 0;
		Contents::register_struct(PredefinedNamespace, "array", t_array.get());
		std::unique_ptr<VariableDeclaration> t_array_val = std::make_unique<VariableDeclaration>();
		n_c.Data("value"); 
		t_array_val->name = n_c;
		t_array_val->package = PredefinedNamespace;
		t_array_val->parent_pack = p_nspc.get();
		t_array_val->parent = t_array.get();
		PrimitiveType t_array_val_type;
		n_c.Data("T");
		t_array_val_type.name = n_c;
		t_array_val->type = Contents::EmplaceType(t_array_val_type);
		t_array->members.push_back(std::move(t_array_val));
		p_nspc->members.push_back(std::move(t_array));


		std::unique_ptr<GenericStructDeclaration> t_tuple = std::make_unique<GenericStructDeclaration>();
		n_c.Data("tuple");
		t_tuple->decl_type = StructDeclarationType::t_tuple;
		t_tuple->name = n_c;
		t_tuple->package = PredefinedNamespace;
		t_tuple->parent_pack = p_nspc.get();
		t_tuple->parent = p_nspc.get();
		t_tuple->generic_typenames["T"] = 0;
		Contents::register_struct(PredefinedNamespace, "tuple", t_tuple.get());
		std::unique_ptr<VariableDeclaration> t_tuple_val = std::make_unique<VariableDeclaration>();
		n_c.Data("value");
		t_tuple_val->name = n_c;
		t_tuple_val->package = PredefinedNamespace;
		t_tuple_val->parent_pack =p_nspc.get();
		t_tuple_val->parent = t_tuple.get();
		PrimitiveType t_tuple_val_type;
		n_c.Data("T");
		t_tuple_val_type.name = n_c;
		t_tuple_val->type = Contents::EmplaceType(t_tuple_val_type);
		t_tuple->members.push_back(std::move(t_tuple_val));
		p_nspc->members.push_back(std::move(t_tuple));

		into.push_back(std::move(p_nspc));
	}
}