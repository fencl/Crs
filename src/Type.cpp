#include "Type.h"
#include "Error.h"
#include <iostream>
#include "Declaration.h"
#include "PredefinedTypes.h"
#include "Utilities.h"
#include <csetjmp>
#include "Compiler.h"

namespace Corrosive {

	extern jmp_buf sandbox;

	/*int8_t Type::compare(unsigned char* me, unsigned char* to) {
		if (setjmp(sandbox) == 0) {
			return (int8_t)memcmp(me, to, size().eval(Compiler::current()->global_module(), compiler_arch));
		}
		else {
			throw_runtime_handler_exception(Compiler::current()->evaluator());
		}

		return 0;
	}

	void Type::copy(unsigned char* me, unsigned char* from) {
		if (setjmp(sandbox) == 0) {
			memcpy(me, from, size().eval(Compiler::current()->global_module(), compiler_arch));
		}
		else {
			throw_runtime_handler_exception(Compiler::current()->evaluator());
		}
	}*/


	// ==========================================================================   RVALUE

	bool Type::rvalue_stacked() {
		return false;
	}
	
	bool TypeSlice::rvalue_stacked() {
		return false;
	}

	bool TypeStructureInstance::rvalue_stacked() {
		return owner->structure_type == StructureInstanceType::normal_structure;
	}

	bool TypeTraitInstance::rvalue_stacked() {
		return true;
	}
	
	ILDataType Type::rvalue() {
		return ILDataType::none;
	}

	ILDataType TypeStructureInstance::rvalue() {
		return owner->rvalue;
	}

	ILDataType TypeReference::rvalue() {
		return ILDataType::word;
	}

	ILDataType TypeArray::rvalue() {
		return ILDataType::word;
	}

	ILDataType TypeTraitInstance::rvalue() {
		return ILDataType::word;
	}

	ILDataType TypeSlice::rvalue() {
		return ILDataType::dword;
	}

	ILDataType TypeFunction::rvalue() {
		return ILDataType::word;
	}

	ILDataType TypeTemplate::rvalue() {
		return ILDataType::word;
	}

	// ==============================================================================================



	TypeReference* Type::generate_reference() {
		if (reference == nullptr) {
			reference = std::make_unique<TypeReference>();
			reference->owner = this;
		}

		return reference.get();
	}
	
	TypeSlice* Type::generate_slice() {
		if (slice == nullptr) {
			slice = std::make_unique<TypeSlice>();
			slice->owner = this;
		}

		return slice.get();
	}


	TypeArray* Type::generate_array(unsigned int count) {
		
		auto f = arrays.find(count);
		if (f == arrays.end()) {
			std::unique_ptr<TypeArray> ti = std::make_unique<TypeArray>();
			ti->owner = this;
			
			ti->table = Compiler::current()->global_module()->register_array_table();
			Compiler::current()->global_module()->array_tables[ti->table].count = count;
			Compiler::current()->global_module()->array_tables[ti->table].element = size();
			TypeArray* rt = ti.get();
			arrays[count] = std::move(ti);
			return rt;
		}
		else {
			return f->second.get();
		}
	}



	// ==============================================================================================  PRINT

	void Type::print(std::ostream& os) {
		os << "?";
	}

	void TypeStructureInstance::print(std::ostream& os) {
		os << ((AstStructureNode*)owner->ast_node)->name_string;
	}

	void TypeTraitInstance::print(std::ostream& os) {
		os << owner->ast_node->name_string;
	}

	void TypeStructureTemplate::print(std::ostream& os) {
		os << owner->ast_node->name_string;
	}

	void TypeFunctionTemplate::print(std::ostream& os) {
		os << owner->ast_node->name_string;
	}

	void TypeTraitTemplate::print(std::ostream& os) {
		os << owner->ast_node->name_string;
	}

	void TypeReference::print(std::ostream& os) {
		os << "&";
		owner->print(os);
	}

	void TypeSlice::print(std::ostream& os) {
		os << "[]";
		owner->print(os);
	}

	void TypeArray::print(std::ostream& os) {
		os << "[" << Compiler::current()->global_module()->array_tables[table].count << "]";
		owner->print(os);
	}

	void TypeFunction::print(std::ostream& os) {
		os << "fn";
		if (ptr_context == ILContext::compile) {
			os << " compile";
		}
		else if (ptr_context == ILContext::runtime) {
			os << " runtime";
		}
		os << "(";
		std::vector<Type*> args = Compiler::current()->types()->argument_array_storage.get(argument_array_id);
		for (auto arg = args.begin(); arg != args.end(); arg++) {
			if (arg != args.begin())
				os << ", ";
			(*arg)->print(os);
		}
		os << ") ";
		return_type->print(os);
	}

	void TypeTemplate::print(std::ostream& os) {
		os << "type(";
		std::vector<Type*> args = owner->argument_array_storage.get(argument_array_id);
		for (auto arg = args.begin(); arg != args.end(); arg++) {
			if (arg != args.begin())
				os << ", ";
			(*arg)->print(os);
		}
		os << ")";
	}

	// ==============================================================================================  SIZE/ALIGNMENT

	ILSize Type::size() {
		return { ILSizeType::absolute,0 };
	}


	ILSize TypeStructureInstance::size() {
		return owner->size;
	}

	ILSize TypeTraitInstance::size() {
		return ILSize::double_ptr;
	}


	ILSize TypeReference::size() {
		return ILSize::single_ptr;
	}

	ILSize TypeSlice::size() {
		return ILSize::double_ptr;
	}


	ILSize TypeFunction::size() {
		return ILSize::single_ptr;
	}
	ILSize TypeTemplate::size() {
		return ILSize::single_ptr;
	}

	ILSize TypeArray::size() {
		return  ILSize(ILSizeType::array, table);
	}


	// ==============================================================================================  CONTEXT


	ILContext Type::context() { return ILContext::compile; }

	ILContext TypeFunction::context() { return ptr_context; }

	ILContext TypeReference::context() { return owner->context(); }

	ILContext TypeSlice::context() { return owner->context(); }

	ILContext TypeArray::context() { return owner->context(); }

	ILContext TypeTraitInstance::context() { return owner->ast_node->context; }

	ILContext TypeStructureInstance::context() {
		return owner->context;
	}

}