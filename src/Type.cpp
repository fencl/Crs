#include "Type.h"
#include "Error.h"
#include <iostream>
#include "Declaration.h"
#include "Contents.h"
#include "PredefinedTypes.h"
#include "svtoi.h"

namespace Corrosive {
	

	StructDeclaration* PrimitiveType::Structure() const {
		if (structure_cache == nullptr) {
			if (package == "")
				ThrowSpecificError(name, "Type package was '' (compiler error)");

			((PrimitiveType*)this)->structure_cache = Contents::FindStruct(package, name.Data());
		}
		return structure_cache;
	}

	bool Type::HeavyType() const {
		return heavy_type;
	}

	void ArrayType::ActualSize(unsigned int asz) const { ArrayType* at = (ArrayType*)this;  at->actual_size = asz; }

	LLVMTypeRef Type::LLVMType() const { return llvm_type; }
	LLVMTypeRef Type::LLVMTypeLValue() const { return llvm_lvalue; }
	LLVMTypeRef Type::LLVMTypeRValue() const { return llvm_rvalue; }

	void Type::PrintLn() const {
		Print();
		std::cout << std::endl;
	}

	void Type::Print() const {
		for (unsigned int i = 0; i < ref;i++) {
			std::cout << "*";
		}
	}


	void FunctionType::Print() const {
		returns->Print();
		std::cout << " (";
		for (auto it = arguments->begin(); it != arguments->end(); it++) {
			if (it != arguments->begin()) {
				std::cout << ", ";
			}
			(*it)->Print();
		}
		std::cout << ")";
		Type::Print();
	}

	void ArrayType::Print() const {
		base->Print();

		std::cout << " [";
		if (size.Data() != "") {
			std::cout << actual_size;
		}
		std::cout << "]";
		Type::Print();
	}

	Type::~Type() {}

	unsigned int Type::Ref() const {
		return ref;
	}
	void Type::Ref(unsigned int r) {
		ref = r;
	}


	const Type* FunctionType::Returns() const {
		return returns;
	}

	const std::vector<const Type*>*const & FunctionType::Args() const {
		return arguments;
	}

	const std::vector<const Type*>*& FunctionType::Args() {
		return arguments;
	}


	void FunctionType::Returns(const Type* r) {
		returns = r;
	}


	const Type* ArrayType::Base() const {
		return base;
	}

	void ArrayType::Base(const Type* b) {
		base = b;
	}

	Cursor ArrayType::Size() const { return size; }
	void ArrayType::Size(Cursor s) { size = s; }

	const TemplateContext*const & PrimitiveType::Templates() const { return templates; }
	const TemplateContext*& PrimitiveType::Templates() { return templates; }

	const std::vector<const Type*>*& InterfaceType::Types() { return types; }
	const std::vector<const Type*>* const& InterfaceType::Types() const { return types; }
	
	const std::vector<const Type*>*& TupleType::Types() { return types; }
	const std::vector<const Type*>* const& TupleType::Types() const { return types; }


	void InterfaceType::Print() const {
		std::cout << "<";
		for (auto it = types->begin(); it != types->end(); it++) {
			if (it != types->begin()) {
				std::cout << ", ";
			}
			(*it)->Print();
		}
		std::cout << ">";
		Type::Print();
	}
	
	
	void TupleType::Print() const {
		std::cout << "[";
		for (auto it = types->begin(); it != types->end(); it++) {
			if (it != types->begin()) {
				std::cout << ", ";
			}
			(*it)->Print();
		}
		std::cout << "]";
		Type::Print();
	}

	void PrimitiveType::Print() const {
		if (package != "") {
			std::cout << package;
			std::cout << "::";
		}
		std::cout << name.Data();

		if (templates != nullptr && templates->size()>0) {
			std::cout << "<";
			for (auto it = templates->begin(); it != templates->end(); it++) {
				if (it != templates->begin()) {
					std::cout << ", ";
				}

				if (it->index() == 0)
					std::cout << std::get<0>(*it);
				else
					std::get<1>(*it)->Print();

			}
			std::cout << ">";
		}

		Type::Print();
	}

	void PrimitiveType::Name(Cursor n) {
		name = n;
	}
	Cursor const PrimitiveType::Name() const {
		return name;
	}

	std::string_view const PrimitiveType::Pack() const { return package; }
	void PrimitiveType::Pack(std::string_view p) { package = p; }

	int Type::ID() const { return 0; }
	int PrimitiveType::ID() const { return 1; }
	int FunctionType::ID() const { return 2; }
	int TupleType::ID() const { return 3; }
	int ArrayType::ID() const { return 4; }
	int InterfaceType::ID() const { return 5; }

	bool operator == (const Type& t1, const Type& t2) {
		return t1.Cmp(t2) == 0;
	}

	bool operator != (const Type& t1, const Type& t2) {
		return t1.Cmp(t2) != 0;
	}
	bool operator > (const Type& t1, const Type& t2) {
		return t1.Cmp(t2) > 0;
	}
	bool operator < (const Type& t1, const Type& t2) {
		return t1.Cmp(t2) < 0;
	}

	const Type* Type::CloneRef(unsigned int r) const { return nullptr; }

	const Type* PrimitiveType::CloneRef(unsigned int r) const {
		PrimitiveType rt = *this;
		rt.Ref(r);
		return Contents::EmplaceType(rt);
	}

	const Type* ArrayType::CloneRef(unsigned int r) const {
		ArrayType rt = *this;
		rt.Ref(r);
		return Contents::EmplaceType(rt);
	}

	const Type* TupleType::CloneRef(unsigned int r) const {
		TupleType rt = *this;
		rt.Ref(r);
		return Contents::EmplaceType(rt);
	}

	const Type* InterfaceType::CloneRef(unsigned int r) const {
		InterfaceType rt = *this;
		rt.Ref(r);
		return Contents::EmplaceType(rt);
	}

	const Type* FunctionType::CloneRef(unsigned int r) const {
		FunctionType rt = *this;
		rt.Ref(r);
		return Contents::EmplaceType(rt);
	}
}