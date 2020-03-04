#include "Type.h"
#include "Error.h"
#include <iostream>
#include "Declaration.h"
#include "Contents.h"
#include "PredefinedTypes.h"
#include "Utilities.h"

namespace Corrosive {
	

	/*StructDeclaration* PrimitiveType::Structure() const {
		if (structure == nullptr) {
			if (package == "")
				ThrowSpecificError(name, "Type package was '' (compiler error)");

			((PrimitiveType*)this)->structure = Contents::FindStruct(package, name.Data());
		}
		return structure;
	}*/

	LLVMTypeRef Type::LLVMType() const { return llvm_type; }
	LLVMTypeRef Type::LLVMTypeLValue() const { return llvm_lvalue; }
	LLVMTypeRef Type::LLVMTypeRValue() const { return llvm_rvalue; }

	void Type::print_ln() const {
		print();
		std::cout << std::endl;
	}

	void Type::print() const {

		if (ref) {
			std::cout << "&";
		}
	}


	void FunctionType::print() const {
		returns->print();
		std::cout << " (";
		for (auto it = arguments->begin(); it != arguments->end(); it++) {
			if (it != arguments->begin()) {
				std::cout << ", ";
			}
			(*it)->print();
		}
		std::cout << ")";
		Type::print();
	}

	void ArrayType::print() const {
		base->print();

		std::cout << " [";

		std::cout << size;

		std::cout << "]";
		Type::print();
	}

	Type::~Type() {}

	void InterfaceType::print() const {
		std::cout << "<";
		for (auto it = types->begin(); it != types->end(); it++) {
			if (it != types->begin()) {
				std::cout << ", ";
			}
			(*it)->print();
		}
		std::cout << ">";
		Type::print();
	}
	
	
	void TupleType::print() const {
		std::cout << "[";
		for (auto it = types->begin(); it != types->end(); it++) {
			if (it != types->begin()) {
				std::cout << ", ";
			}
			(*it)->print();
		}
		std::cout << "]";
		Type::print();
	}

	void PrimitiveType::print() const {
		if (package != "") {
			std::cout << package;
			std::cout << "::";
		}
		std::cout << name.buffer;

		if (templates != nullptr && templates->size()>0) {
			std::cout << "<";
			for (auto it = templates->begin(); it != templates->end(); it++) {
				if (it != templates->begin()) {
					std::cout << ", ";
				}

				(*it)->print();

			}
			std::cout << ">";
		}

		Type::print();
	}

	int Type::id() const { return 0; }
	int PrimitiveType::id() const { return 1; }
	int FunctionType::id() const { return 2; }
	int TupleType::id() const { return 3; }
	int ArrayType::id() const { return 4; }
	int InterfaceType::id() const { return 5; }

	bool operator == (const Type& t1, const Type& t2) {
		return t1.cmp(t2) == 0;
	}

	bool operator != (const Type& t1, const Type& t2) {
		return t1.cmp(t2) != 0;
	}
	bool operator > (const Type& t1, const Type& t2) {
		return t1.cmp(t2) > 0;
	}
	bool operator < (const Type& t1, const Type& t2) {
		return t1.cmp(t2) < 0;
	}

	const Type* Type::clone_ref(bool r) const { 
		if (ref && r) {
			return t_ptr_ref;
		}
		else if (!ref && r) {
			return nullptr;
		}
		else if (ref == r) {
			return this;
		}

		return nullptr;
	}

	const Type* PrimitiveType::clone_ref(bool r) const {
		if (ref && r) {
			return t_ptr_ref;
		}
		else if (!ref && r) {

			PrimitiveType rt = *this;
			rt.ref = true;
			return Contents::emplace_type(rt);
		}
		else if (ref == r) {
			return this;
		}

		return nullptr;
	}

	const Type* ArrayType::clone_ref(bool r) const {
		if (ref && r) {
			return t_ptr_ref;
		}
		else if (!ref && r) {

			ArrayType rt = *this;
			rt.ref = true;
			return Contents::emplace_type(rt);
		}
		else if (ref == r) {
			return this;
		}

		return nullptr;
	}

	const Type* TupleType::clone_ref(bool r) const {
		if (ref && r) {
			return t_ptr_ref;
		}
		else if (!ref && r) {

			TupleType rt = *this;
			rt.ref = true;
			return Contents::emplace_type(rt);
		}
		else if (ref == r) {
			return this;
		}

		return nullptr;
	}

	const Type* InterfaceType::clone_ref(bool r) const {
		if (ref && r) {
			return t_ptr_ref;
		}
		else if (!ref && r) {

			InterfaceType rt = *this;
			rt.ref = true;
			return Contents::emplace_type(rt);
		}
		else if (ref == r) {
			return this;
		}

		return nullptr;
	}

	const Type* FunctionType::clone_ref(bool r) const {
		if (ref && r) {
			return t_ptr_ref;
		}
		else if (!ref && r) {
			FunctionType rt = *this;
			rt.ref = true;
			return Contents::emplace_type(rt);
		}
		else if (ref == r) {
			return this;
		}

		return nullptr;
	}
}