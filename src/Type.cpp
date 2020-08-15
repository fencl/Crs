#include "Type.hpp"
#include "Error.hpp"
#include <iostream>
#include "Declaration.hpp"
#include "BuiltIn.hpp"
#include <csetjmp>
#include "Compiler.hpp"

namespace Corrosive {
	int8_t Type::compare_for_generic_storage(unsigned char* me, unsigned char* to) {
		if (!wrap || wrap(sandbox) == 0) {
			auto v = memcmp(me, to, size().eval(Compiler::current()->global_module(), compiler_arch));
			if (v < 0) return -1;
			else if (v > 0) return 1;
			else return 0;
		}
		else {
			throw_runtime_handler_exception(Compiler::current()->evaluator());
		}

		return 0;
	}

	void Type::copy_to_generic_storage(unsigned char* me, unsigned char* to) {
		if (!wrap || wrap(sandbox) == 0) {
			memcpy(to, me, size().eval(Compiler::current()->global_module(), compiler_arch));
		}
		else {
			throw_runtime_handler_exception(Compiler::current()->evaluator());
		}
	}

	int8_t TypeSlice::compare_for_generic_storage(unsigned char* me, unsigned char* to) {
		dword_t* me_dw = (dword_t*)me;
		dword_t* to_dw = (dword_t*)to;

		if (!wrap || wrap(sandbox) == 0) {

			if (me_dw->p2 < to_dw->p2) return -1;
			else if (me_dw->p2 > to_dw->p2) return 1;
			auto v = memcmp(me_dw->p1, to_dw->p1, (size_t)me_dw->p2);
			if (v < 0) return -1;
			else if (v > 0) return 1;
			else return 0;
		}
		else {
			throw_runtime_handler_exception(Compiler::current()->evaluator());
		}

		return 0;
	}

	void TypeSlice::copy_to_generic_storage(unsigned char* me, unsigned char* to) {
		dword_t* me_dw = (dword_t*)me;
		dword_t* to_dw = (dword_t*)to;

		if (!wrap || wrap(sandbox) == 0) {
			to_dw->p1 = Compiler::current()->constant_manager()->register_generic_storage((uint8_t*)me_dw->p1, (size_t)me_dw->p2, owner);
			to_dw->p2 = me_dw->p2;
		}
		else {
			throw_runtime_handler_exception(Compiler::current()->evaluator());
		}
	}


	void Type::constantize(Cursor& err, unsigned char* target, unsigned char* source) {
		throw_specific_error(err, "Cannot create constant value of this type");
	}

	void TypeStructureInstance::constantize(Cursor& err, unsigned char* target, unsigned char* source) {

		if (!wrap || wrap(sandbox) == 0) {
			if (owner->structure_type == StructureInstanceType::primitive_structure && rvalue() != ILDataType::none) {
				

				if (target != nullptr) {
					switch (rvalue())
					{
						case ILDataType::none: break;
						case ILDataType::u8: *(uint8_t*)target = *(uint8_t*)source; break;
						case ILDataType::u16: *(uint16_t*)target = *(uint16_t*)source; break;
						case ILDataType::u32: *(uint32_t*)target = *(uint32_t*)source; break;
						case ILDataType::u64: *(uint64_t*)target = *(uint64_t*)source; break;
						case ILDataType::i8: *(int8_t*)target = *(int8_t*)source; break;
						case ILDataType::i16: *(int16_t*)target = *(int16_t*)source; break;
						case ILDataType::i32: *(int32_t*)target = *(int32_t*)source; break;
						case ILDataType::i64: *(int64_t*)target = *(int64_t*)source; break;
						case ILDataType::f32: *(float*)target = *(float*)source; break;
						case ILDataType::f64: *(double*)target = *(double*)source; break;
						case ILDataType::word: throw_specific_error(err, "Cannot create constant value of this type"); break;
						case ILDataType::dword: throw_specific_error(err, "Cannot create constant value of this type"); break;
						default: break;
					}
				}
				else {
					Compiler* compiler = Compiler::current();
					switch (rvalue())
					{
						case ILDataType::none: break;
						case ILDataType::u8: ILBuilder::build_const_u8(compiler->scope(), *(uint8_t*)source); break;
						case ILDataType::u16: ILBuilder::build_const_u16(compiler->scope(), *(uint16_t*)source); break;
						case ILDataType::u32: ILBuilder::build_const_u32(compiler->scope(), *(uint32_t*)source); break;
						case ILDataType::u64: ILBuilder::build_const_u64(compiler->scope(), *(uint64_t*)source); break;
						case ILDataType::i8: ILBuilder::build_const_i8(compiler->scope(), *(int8_t*)source); break;
						case ILDataType::i16: ILBuilder::build_const_i16(compiler->scope(), *(int16_t*)source); break;
						case ILDataType::i32: ILBuilder::build_const_i32(compiler->scope(), *(int32_t*)source); break;
						case ILDataType::i64: ILBuilder::build_const_i64(compiler->scope(), *(int64_t*)source); break;
						case ILDataType::f32: ILBuilder::build_const_f32(compiler->scope(), *(float*)source); break;
						case ILDataType::f64: ILBuilder::build_const_f64(compiler->scope(), *(double*)source); break;
						case ILDataType::word: throw_specific_error(err, "Cannot create constant value of this type"); break;
						case ILDataType::dword: throw_specific_error(err, "Cannot create constant value of this type"); break;
						default: break;
					}
				}
			}
			else {
				throw_specific_error(err, "Cannot create constant value of this type");
			}
		}
		else {
			throw_runtime_handler_exception(Compiler::current()->evaluator());
		}
	}

	void TypeSlice::constantize(Cursor& err, unsigned char* target, unsigned char* source) {
		std::string data;

		if (!wrap || wrap(sandbox) == 0) {
			Compiler* compiler = Compiler::current();
			dword_t me = *(dword_t*)source;
			size_t storage_size = (size_t)me.p2;
			data = std::string(storage_size,'\0');

			uint8_t* ptr_src = (uint8_t*)me.p1;
			uint8_t* ptr_dst = (uint8_t*)data.data();
			size_t elem_size = owner->size().eval(compiler->global_module(), compiler_arch);
			size_t count = ((size_t)me.p2)/elem_size;
			for (size_t i=0;i<count; ++i) {
				owner->constantize(err, ptr_dst, ptr_src);
				ptr_src += elem_size;
				ptr_dst += elem_size;
			}

			ILSize s;
			if (owner->size().type == ILSizeType::table || owner->size().type == ILSizeType::array) {
				s.type = ILSizeType::array;
				s.value = compiler->global_module()->register_array_table(owner->size(),count);
			}else if (owner->size().type == ILSizeType::_0) {
				s.type = ILSizeType::_0;
			}else{
				s = owner->size();
				s.value *= count;
			}

			auto val = compiler->constant_manager()->register_constant(std::move(data), s);

			if (target) {
				dword_t* tg = (dword_t*)target;
				tg->p1 = (void*)val.first.data();
				tg->p2 = (void*)val.first.size();
			}
			else {
				ILBuilder::build_const_slice(compiler->scope(), val.second, s);
			}
			
		}
		else {
			throw_runtime_handler_exception(Compiler::current()->evaluator());
		}
	}
	
	void TypeArray::constantize(Cursor& err, unsigned char* target, unsigned char* source) {
		std::string data;

		if (!wrap || wrap(sandbox) == 0) {
			Compiler* compiler = Compiler::current();
			size_t me_size = size().eval(compiler->global_module(), compiler_arch);
			data = std::string(me_size,'\0');
			
			uint8_t* ptr_src = (uint8_t*)source;
			uint8_t* ptr_dst = (uint8_t*)data.data();
			size_t elem_size = owner->size().eval(compiler->global_module(), compiler_arch);
			for (size_t i=0;i<(me_size)/elem_size; ++i) {
				owner->constantize(err, ptr_dst, ptr_src);
				ptr_src += elem_size;
				ptr_dst += elem_size;
			}

			if (target) {
				memcpy(target, data.data(), data.size()); // no need to register as constant
			}
			else {
				auto val = compiler->constant_manager()->register_constant(std::move(data), size());

				stackid_t local_id = compiler->target()->local_stack_lifetime.append(size());
				compiler->temp_stack()->push_item("$tmp", this, local_id);
				ILBuilder::build_constref(compiler->scope(), val.second);
				ILBuilder::build_local(compiler->scope(), local_id);
				ILBuilder::build_memcpy(compiler->scope(), size());
				ILBuilder::build_local(compiler->scope(), local_id);
			}
		}
		else {
			throw_runtime_handler_exception(Compiler::current()->evaluator());
		}
	}


	// ==========================================================================   RVALUE
	
	bool Type::rvalue_stacked() {
		return false;
	}
	
	bool TypeSlice::rvalue_stacked() {
		return false;
	}

	bool TypeArray::rvalue_stacked() {
		return true;
	}

	bool TypeStructureInstance::rvalue_stacked() {
		return owner->structure_type == StructureInstanceType::normal_structure;
	}

	bool TypeTraitInstance::rvalue_stacked() {
		return false;
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
		return ILDataType::dword;
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


	TypeArray* Type::generate_array(uint32_t count) {
		
		auto f = arrays.find(count);
		if (f == arrays.end()) {
			Compiler* compiler = Compiler::current();
			std::unique_ptr<TypeArray> ti = std::make_unique<TypeArray>();
			ti->owner = this;
			ti->count = count;
			ILSize s = size();

			if (s.type == ILSizeType::table || s.type == ILSizeType::array) {
				ti->size_value.type = ILSizeType::array;
				ti->size_value.value = compiler->global_module()->register_array_table(s, count);
			}else if (s.type == ILSizeType::_0) {
				ti->size_value.type = ILSizeType::_0;
				ti->size_value.value = 0;
			}else {
				ti->size_value = s;
				ti->size_value.value *= count;
			}

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
		//TODO: os << "[" << Compiler::current()->global_module()->array_tables[table].count << "]";
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
		return { ILSizeType::_0,0 };
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
		return ILSize::slice;
	}


	ILSize TypeFunction::size() {
		return ILSize::single_ptr;
	}
	ILSize TypeTemplate::size() {
		return ILSize::single_ptr;
	}

	ILSize TypeArray::size() {
		return size_value; //ILSize(ILSizeType::array, table);
	}


	// ==============================================================================================  CONTEXT


	ILContext Type::context() { return ILContext::compile; }

	ILContext TypeFunction::context() { return ptr_context; }

	ILContext TypeReference::context() { return owner->context(); }

	ILContext TypeSlice::context() { return owner->context(); }

	ILContext TypeArray::context() { return owner->context(); }

	ILContext TypeTraitInstance::context() { return owner->context; }

	ILContext TypeStructureInstance::context() {
		return owner->context;
	}

	
	void Type::assert(Cursor& c, Type* t) {
		if (!wrap || wrap(sandbox)==0) {
			if (t->magic() & type_magic_mask != type_magic) {
				throw_specific_error(c, "Value is not a type");
			}
		} else {
			throw_specific_error(c, "Value is not a type");
		}
	}

	void Type::assert(Type* t) {
		if (!wrap || wrap(sandbox)==0) {
			if (t->magic() & type_magic_mask != type_magic) {
				throw_runtime_exception(Compiler::current()->evaluator(), "Value is not a type");
			}
		} else {
			throw_runtime_exception(Compiler::current()->evaluator(), "Value is not a type");
		}
	}
	
	uint64_t Type::magic() { return type_magic & type_magic_mask & 0x01u; }
	uint64_t TypeStructureInstance::magic() { return type_magic & type_magic_mask & 0x02u; }
	uint64_t TypeStructureTemplate::magic() { return type_magic & type_magic_mask & 0x03u; }
	uint64_t TypeTraitInstance::magic() { return type_magic & type_magic_mask & 0x04u; }
	uint64_t TypeTraitTemplate::magic() { return type_magic & type_magic_mask & 0x05u; }
	uint64_t TypeFunction::magic() { return type_magic & type_magic_mask & 0x06u; }
	uint64_t TypeFunctionTemplate::magic() { return type_magic & type_magic_mask & 0x07u; }
	uint64_t TypeSlice::magic() { return type_magic & type_magic_mask & 0x08u; }
	uint64_t TypeArray::magic() { return type_magic & type_magic_mask & 0x09u; }
	uint64_t TypeReference::magic() { return type_magic & type_magic_mask & 0x0au; }
	uint64_t TypeTemplate::magic() { return type_magic & type_magic_mask & 0x0bu; }

}