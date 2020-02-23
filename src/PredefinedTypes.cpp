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


	void InitPredefinedTypes(std::vector<std::unique_ptr<Declaration>>& into) {
		Contents::RegisterNamespace(PredefinedNamespace);
		std::unique_ptr<NamespaceDeclaration> p_nspc = std::make_unique<NamespaceDeclaration>();
		Cursor n_c; n_c.Data(PredefinedNamespace);
		p_nspc->Name(n_c);
		p_nspc->Pack(PredefinedNamespace);
		p_nspc->ParentPack(p_nspc.get());


		std::unique_ptr<StructDeclaration> t_u8 = std::make_unique<StructDeclaration>();
		n_c.Data("u8");
		t_u8->DeclType(StructDeclarationType::t_u8);
		t_u8->Name(n_c);
		t_u8->Pack(PredefinedNamespace);
		t_u8->ParentPack(p_nspc.get());
		t_u8->Parent(p_nspc.get());
		Contents::RegisterStruct(PredefinedNamespace, "u8", t_u8.get());
		p_nspc->Members.push_back(std::move(t_u8));

		PrimitiveType pt_u8;
		pt_u8.Name(n_c);
		pt_u8.Pack(PredefinedNamespace);
		Corrosive::t_u8 = Contents::EmplaceType(pt_u8);

		std::unique_ptr<StructDeclaration> t_u16 = std::make_unique<StructDeclaration>();
		n_c.Data("u16");
		t_u16->DeclType(StructDeclarationType::t_u16);
		t_u16->Name(n_c);
		t_u16->Pack(PredefinedNamespace);
		t_u16->ParentPack(p_nspc.get());
		t_u16->Parent(p_nspc.get());
		Contents::RegisterStruct(PredefinedNamespace, "u16", t_u16.get());
		p_nspc->Members.push_back(std::move(t_u16));

		PrimitiveType pt_u16;
		pt_u16.Name(n_c);
		pt_u16.Pack(PredefinedNamespace);
		Corrosive::t_u16 = Contents::EmplaceType(pt_u16);

		std::unique_ptr<StructDeclaration> t_u32 = std::make_unique<StructDeclaration>();
		n_c.Data("u32");
		t_u32->DeclType(StructDeclarationType::t_u32);
		t_u32->Name(n_c);
		t_u32->Pack(PredefinedNamespace);
		t_u32->ParentPack(p_nspc.get());
		t_u32->Parent(p_nspc.get());
		Contents::RegisterStruct(PredefinedNamespace, "u32", t_u32.get());
		p_nspc->Members.push_back(std::move(t_u32));

		PrimitiveType pt_u32;
		pt_u32.Name(n_c);
		pt_u32.Pack(PredefinedNamespace);
		Corrosive::t_u32 = Contents::EmplaceType(pt_u32);

		std::unique_ptr<StructDeclaration> t_u64 = std::make_unique<StructDeclaration>();
		n_c.Data("u64");
		t_u64->DeclType(StructDeclarationType::t_u64);
		t_u64->Name(n_c);
		t_u64->Pack(PredefinedNamespace);
		t_u64->ParentPack(p_nspc.get());
		t_u64->Parent(p_nspc.get());
		Contents::RegisterStruct(PredefinedNamespace, "u64", t_u64.get());
		p_nspc->Members.push_back(std::move(t_u64));


		PrimitiveType pt_u64;
		pt_u64.Name(n_c);
		pt_u64.Pack(PredefinedNamespace);
		Corrosive::t_u64 = Contents::EmplaceType(pt_u64);

		std::unique_ptr<StructDeclaration> t_i8 = std::make_unique<StructDeclaration>();
		n_c.Data("i8");
		t_i8->DeclType(StructDeclarationType::t_i8);
		t_i8->Name(n_c);
		t_i8->Pack(PredefinedNamespace);
		t_i8->ParentPack(p_nspc.get());
		t_i8->Parent(p_nspc.get());
		Contents::RegisterStruct(PredefinedNamespace, "i8", t_i8.get());
		p_nspc->Members.push_back(std::move(t_i8));


		PrimitiveType pt_i8;
		pt_i8.Name(n_c);
		pt_i8.Pack(PredefinedNamespace);
		Corrosive::t_i8 = Contents::EmplaceType(pt_i8);

		std::unique_ptr<StructDeclaration> t_i16 = std::make_unique<StructDeclaration>();
		n_c.Data("i16");
		t_i16->DeclType(StructDeclarationType::t_i16);
		t_i16->Name(n_c);
		t_i16->Pack(PredefinedNamespace);
		t_i16->ParentPack(p_nspc.get());
		t_i16->Parent(p_nspc.get());
		Contents::RegisterStruct(PredefinedNamespace, "i16", t_i16.get());
		p_nspc->Members.push_back(std::move(t_i16));

		PrimitiveType pt_i16;
		pt_i16.Name(n_c);
		pt_i16.Pack(PredefinedNamespace);
		Corrosive::t_i16 = Contents::EmplaceType(pt_i16);

		std::unique_ptr<StructDeclaration> t_i32 = std::make_unique<StructDeclaration>();
		n_c.Data("i32");
		t_i32->DeclType(StructDeclarationType::t_i32);
		t_i32->Name(n_c);
		t_i32->Pack(PredefinedNamespace);
		t_i32->ParentPack(p_nspc.get());
		t_i32->Parent(p_nspc.get());
		Contents::RegisterStruct(PredefinedNamespace, "i32", t_i32.get());
		p_nspc->Members.push_back(std::move(t_i32));

		PrimitiveType pt_i32;
		pt_i32.Name(n_c);
		pt_i32.Pack(PredefinedNamespace);
		Corrosive::t_i32 = Contents::EmplaceType(pt_i32);


		std::unique_ptr<StructDeclaration> t_i64 = std::make_unique<StructDeclaration>();
		n_c.Data("i64");
		t_i64->DeclType(StructDeclarationType::t_i64);
		t_i64->Name(n_c);
		t_i64->Pack(PredefinedNamespace);
		t_i64->ParentPack(p_nspc.get());
		t_i64->Parent(p_nspc.get());
		Contents::RegisterStruct(PredefinedNamespace, "i64", t_i64.get());
		p_nspc->Members.push_back(std::move(t_i64));

		PrimitiveType pt_i64;
		pt_i64.Name(n_c);
		pt_i64.Pack(PredefinedNamespace);
		Corrosive::t_i64 = Contents::EmplaceType(pt_i64);

		std::unique_ptr<StructDeclaration> t_f32 = std::make_unique<StructDeclaration>();
		n_c.Data("f32");
		t_f32->DeclType(StructDeclarationType::t_f32);
		t_f32->Name(n_c);
		t_f32->Pack(PredefinedNamespace);
		t_f32->ParentPack(p_nspc.get());
		t_f32->Parent(p_nspc.get());
		Contents::RegisterStruct(PredefinedNamespace, "f32", t_f32.get());
		p_nspc->Members.push_back(std::move(t_f32));


		PrimitiveType pt_f32;
		pt_f32.Name(n_c);
		pt_f32.Pack(PredefinedNamespace);
		Corrosive::t_f32 = Contents::EmplaceType(pt_f32);


		std::unique_ptr<StructDeclaration> t_f64 = std::make_unique<StructDeclaration>();
		n_c.Data("f64");
		t_f64->DeclType(StructDeclarationType::t_f64);
		t_f64->Name(n_c);
		t_f64->Pack(PredefinedNamespace);
		t_f64->ParentPack(p_nspc.get());
		t_f64->Parent(p_nspc.get());
		Contents::RegisterStruct(PredefinedNamespace, "f64", t_f64.get());
		p_nspc->Members.push_back(std::move(t_f64));

		PrimitiveType pt_f64;
		pt_f64.Name(n_c);
		pt_f64.Pack(PredefinedNamespace);
		Corrosive::t_f64 = Contents::EmplaceType(pt_f64);


		std::unique_ptr<StructDeclaration> t_bool = std::make_unique<StructDeclaration>();
		n_c.Data("bool");
		t_bool->DeclType(StructDeclarationType::t_bool);
		t_bool->Name(n_c);
		t_bool->Pack(PredefinedNamespace);
		t_bool->ParentPack(p_nspc.get());
		t_bool->Parent(p_nspc.get());
		Contents::RegisterStruct(PredefinedNamespace, "bool", t_bool.get());
		p_nspc->Members.push_back(std::move(t_bool));


		PrimitiveType pt_bool;
		pt_bool.Name(n_c);
		pt_bool.Pack(PredefinedNamespace);
		Corrosive::t_bool = Contents::EmplaceType(pt_bool);


		std::unique_ptr<StructDeclaration> t_ptr = std::make_unique<StructDeclaration>();
		n_c.Data("ptr");
		t_ptr->DeclType(StructDeclarationType::t_ptr);
		t_ptr->Name(n_c);
		t_ptr->Pack(PredefinedNamespace);
		t_ptr->ParentPack(p_nspc.get());
		t_ptr->Parent(p_nspc.get());
		Contents::RegisterStruct(PredefinedNamespace, "ptr", t_ptr.get());
		p_nspc->Members.push_back(std::move(t_ptr));


		std::unique_ptr<StructDeclaration> t_string = std::make_unique<StructDeclaration>();
		n_c.Data("string");
		t_string->DeclType(StructDeclarationType::t_string);
		t_string->Name(n_c);
		t_string->Pack(PredefinedNamespace);
		t_string->ParentPack(p_nspc.get());
		t_string->Parent(p_nspc.get());
		Contents::RegisterStruct(PredefinedNamespace, "string", t_string.get());
		p_nspc->Members.push_back(std::move(t_string));

		std::unique_ptr<GenericStructDeclaration> t_array = std::make_unique<GenericStructDeclaration>();
		n_c.Data("array");
		t_array->DeclType(StructDeclarationType::t_array);
		t_array->Name(n_c);
		t_array->Pack(PredefinedNamespace);
		t_array->ParentPack(p_nspc.get());
		t_array->Parent(p_nspc.get());
		t_array->Generics()["T"] = 0;
		Contents::RegisterStruct(PredefinedNamespace, "array", t_array.get());
		std::unique_ptr<VariableDeclaration> t_array_val = std::make_unique<VariableDeclaration>();
		n_c.Data("value"); 
		t_array_val->Name(n_c);
		t_array_val->Pack(PredefinedNamespace);
		t_array_val->ParentPack(p_nspc.get());
		t_array_val->Parent(t_array.get());
		PrimitiveType t_array_val_type;
		n_c.Data("T");
		t_array_val_type.Name(n_c);
		t_array_val->Type(Contents::EmplaceType(t_array_val_type));
		t_array->Members.push_back(std::move(t_array_val));
		p_nspc->Members.push_back(std::move(t_array));


		std::unique_ptr<GenericStructDeclaration> t_tuple = std::make_unique<GenericStructDeclaration>();
		n_c.Data("tuple");
		t_tuple->DeclType(StructDeclarationType::t_tuple);
		t_tuple->Name(n_c);
		t_tuple->Pack(PredefinedNamespace);
		t_tuple->ParentPack(p_nspc.get());
		t_tuple->Parent(p_nspc.get());
		t_tuple->Generics()["T"] = 0;
		Contents::RegisterStruct(PredefinedNamespace, "tuple", t_tuple.get());
		std::unique_ptr<VariableDeclaration> t_tuple_val = std::make_unique<VariableDeclaration>();
		n_c.Data("value");
		t_tuple_val->Name(n_c);
		t_tuple_val->Pack(PredefinedNamespace);
		t_tuple_val->ParentPack(p_nspc.get());
		t_tuple_val->Parent(t_tuple.get());
		PrimitiveType t_tuple_val_type;
		n_c.Data("T");
		t_tuple_val_type.Name(n_c);
		t_tuple_val->Type(Contents::EmplaceType(t_tuple_val_type));
		t_tuple->Members.push_back(std::move(t_tuple_val));
		p_nspc->Members.push_back(std::move(t_tuple));

		into.push_back(std::move(p_nspc));



		n_c.Data("ptr");
		PrimitiveType pt_ptr;
		pt_ptr.Name(n_c);
		pt_ptr.Pack(PredefinedNamespace);
		Corrosive::t_ptr = Contents::EmplaceType(pt_ptr);
		pt_ptr.ref = true;
		Corrosive::t_ptr_ref = Contents::EmplaceType(pt_ptr);


	}
}