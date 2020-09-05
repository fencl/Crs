#include "Type.hpp"
#include "Error.hpp"
#include <iostream>
#include "Declaration.hpp"
#include "BuiltIn.hpp"
#include <csetjmp>
#include "Compiler.hpp"
#include <cstring>

namespace Corrosive {
	errvoid Type::compare_for_generic_storage(std::int8_t& r, unsigned char* me, unsigned char* to) {
		size_t s = size().eval(Compiler::current()->global_module());
		if (!wrap || wrap(sandbox) == 0) {
			auto v = memcmp(me, to, s);
			if (v < 0) { r = -1; return err::ok; }
			else if (v > 0) { r = 1; return err::ok; }
			else { r = 0; return err::ok; }
		}
		else {
			return throw_runtime_handler_exception(Compiler::current()->evaluator());
		}

		r = 0;
		return err::ok;
	}

	errvoid Type::copy_to_generic_storage(unsigned char* me, unsigned char* to) {
		size_t s = size().eval(Compiler::current()->global_module());
		if (!wrap || wrap(sandbox) == 0) {
			std::memcpy(to, me, s);
		}
		else {
			return throw_runtime_handler_exception(Compiler::current()->evaluator());
		}
		return err::ok;
	}

	errvoid TypeSlice::compare_for_generic_storage(std::int8_t& r,unsigned char* me, unsigned char* to) {
		dword_t* me_dw = (dword_t*)me;
		dword_t* to_dw = (dword_t*)to;

		if (!wrap || wrap(sandbox) == 0) {

			if (me_dw->p2 < to_dw->p2) { r = -1; return err::ok; }
			else if (me_dw->p2 > to_dw->p2) { r = 1; return err::ok; }
			auto v = memcmp(me_dw->p1, to_dw->p1, (std::size_t)me_dw->p2);
			if (v < 0) { r = -1; return err::ok; }
			else if (v > 0) { r = 1; return err::ok; }
			else { r = 0; return err::ok; }
		}
		else {
			return throw_runtime_handler_exception(Compiler::current()->evaluator());
		}

		r = 0;
		return err::ok;
	}

	errvoid TypeSlice::copy_to_generic_storage(unsigned char* me, unsigned char* to) {
		dword_t* me_dw = (dword_t*)me;
		dword_t* to_dw = (dword_t*)to;

		std::size_t mp1,mp2;

		if (!wrap || wrap(sandbox) == 0) {
			mp1 = (std::size_t)me_dw->p1;
			mp2 = (std::size_t)me_dw->p2;
		}
		else {
			return throw_runtime_handler_exception(Compiler::current()->evaluator());
		}

		std::uint8_t* dst;
		if (!Compiler::current()->constant_manager()->register_generic_storage(dst, (std::uint8_t*)mp1, mp2, owner)) return err::fail;
		to_dw->p1 = (void*)dst;
		to_dw->p2 = (void*)mp2;
		return err::ok;
	}


	errvoid Type::constantize(Cursor& err, unsigned char* target, unsigned char* source) {
		return throw_specific_error(err, "Cannot create constant value of this type");
	}

	errvoid TypeStructureInstance::constantize(Cursor& err, unsigned char* target, unsigned char* source) {

		if (!wrap || wrap(sandbox) == 0) {
			if (owner->structure_type == StructureInstanceType::primitive_structure && rvalue() != ILDataType::none) {
				

				if (target != nullptr) {
					switch (rvalue())
					{
						case ILDataType::none: break;
						case ILDataType::u8: *(std::uint8_t*)target = *(std::uint8_t*)source; break;
						case ILDataType::u16: *(std::uint16_t*)target = *(std::uint16_t*)source; break;
						case ILDataType::u32: *(std::uint32_t*)target = *(std::uint32_t*)source; break;
						case ILDataType::u64: *(std::uint64_t*)target = *(std::uint64_t*)source; break;
						case ILDataType::i8: *(std::int8_t*)target = *(std::int8_t*)source; break;
						case ILDataType::i16: *(std::int16_t*)target = *(std::int16_t*)source; break;
						case ILDataType::i32: *(std::int32_t*)target = *(std::int32_t*)source; break;
						case ILDataType::i64: *(std::int64_t*)target = *(std::int64_t*)source; break;
						case ILDataType::f32: *(float*)target = *(float*)source; break;
						case ILDataType::f64: *(double*)target = *(double*)source; break;
						case ILDataType::word: return throw_specific_error(err, "Cannot create constant value of this type"); break;
						case ILDataType::dword: return throw_specific_error(err, "Cannot create constant value of this type"); break;
						default: break;
					}
				}
				else {
					Compiler* compiler = Compiler::current();
					switch (rvalue())
					{
						case ILDataType::none: break;
						case ILDataType::u8:  if(!ILBuilder::build_const_u8(compiler->scope(), *(std::uint8_t*)source)) return err::fail; break;
						case ILDataType::u16: if(!ILBuilder::build_const_u16(compiler->scope(), *(std::uint16_t*)source)) return err::fail; break;
						case ILDataType::u32: if(!ILBuilder::build_const_u32(compiler->scope(), *(std::uint32_t*)source)) return err::fail; break;
						case ILDataType::u64: if(!ILBuilder::build_const_u64(compiler->scope(), *(std::uint64_t*)source)) return err::fail; break;
						case ILDataType::i8:  if(!ILBuilder::build_const_i8(compiler->scope(), *(std::int8_t*)source)) return err::fail; break;
						case ILDataType::i16: if(!ILBuilder::build_const_i16(compiler->scope(), *(std::int16_t*)source)) return err::fail; break;
						case ILDataType::i32: if(!ILBuilder::build_const_i32(compiler->scope(), *(std::int32_t*)source)) return err::fail; break;
						case ILDataType::i64: if(!ILBuilder::build_const_i64(compiler->scope(), *(std::int64_t*)source)) return err::fail; break;
						case ILDataType::f32: if (!ILBuilder::build_const_f32(compiler->scope(), *(float*)source)) return err::fail; break;
						case ILDataType::f64: if (!ILBuilder::build_const_f64(compiler->scope(), *(double*)source)) return err::fail; break;
						case ILDataType::word: return throw_specific_error(err, "Cannot create constant value of this type"); break;
						case ILDataType::dword: return throw_specific_error(err, "Cannot create constant value of this type"); break;
						default: break;
					}
				}
			}
			else {
				return throw_specific_error(err, "Cannot create constant value of this type");
			}
		}
		else {
			return throw_runtime_handler_exception(Compiler::current()->evaluator());
		}

		return err::ok;
	}

	errvoid TypeSlice::constantize(Cursor& err, unsigned char* target, unsigned char* source) {
		std::string data;

		Compiler* compiler = Compiler::current();
		dword_t me;
		std::size_t storage_size;
		std::size_t me_ptr;

		if (!wrap || wrap(sandbox) == 0) {	
			me = *(dword_t*)source;
			storage_size = (std::size_t)me.p2;
			me_ptr = (std::size_t)me.p1;
		} else {
			return throw_runtime_handler_exception(Compiler::current()->evaluator());
		}

		data = std::string(storage_size,'\0');
		std::uint8_t* ptr_src = (std::uint8_t*)me_ptr;
		std::uint8_t* ptr_dst = (std::uint8_t*)data.data();
		std::size_t elem_size = owner->size().eval(compiler->global_module());
		std::uint32_t count = (std::uint32_t)(((std::size_t)storage_size)/elem_size);

		for (std::uint32_t i=0;i<count; ++i) {
			if (!owner->constantize(err, ptr_dst, ptr_src)) return err::fail;
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
			if (!wrap || wrap(sandbox) == 0) {	
				dword_t* tg = (dword_t*)target;
				tg->p1 = (void*)val.first.data();
				tg->p2 = (void*)val.first.size();
			} else {
				return throw_runtime_handler_exception(Compiler::current()->evaluator());
			}
		}
		else {
			if (!ILBuilder::build_const_slice(compiler->scope(), val.second, s)) return err::fail;
		}

		return err::ok;
	}
	
	errvoid TypeArray::constantize(Cursor& err, unsigned char* target, unsigned char* source) {
		std::string data;

		
		Compiler* compiler = Compiler::current();
		std::size_t me_size = size().eval(compiler->global_module());
		data = std::string(me_size,'\0');
		
		std::uint8_t* ptr_src = (std::uint8_t*)source;
		std::uint8_t* ptr_dst = (std::uint8_t*)data.data();
		std::size_t elem_size = owner->size().eval(compiler->global_module());
		for (std::size_t i=0;i<(me_size)/elem_size; ++i) {
			if (!owner->constantize(err, ptr_dst, ptr_src)) return err::fail;
			ptr_src += elem_size;
			ptr_dst += elem_size;
		}

		if (target) {
			if (!wrap || wrap(sandbox) == 0) {
				memcpy(target, data.data(), data.size()); // no need to register as constant}
			} else {
				return throw_runtime_handler_exception(Compiler::current()->evaluator());
			}
		}
		else {
			auto val = compiler->constant_manager()->register_constant(std::move(data), size());
			stackid_t local_id = compiler->target()->local_stack_lifetime.append(size());
			compiler->temp_stack()->push_item("$tmp", this, local_id);
			if (!ILBuilder::build_constref(compiler->scope(), val.second)) return err::fail;
			if (!ILBuilder::build_local(compiler->scope(), local_id)) return err::fail;
			if (!ILBuilder::build_memcpy(compiler->scope(), size())) return err::fail;
			if (!ILBuilder::build_local(compiler->scope(), local_id)) return err::fail;
		}
		return err::ok;
	}


	// ==========================================================================   RVALUE
	
	bool Type::rvalue_stacked() {
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


	TypeArray* Type::generate_array(std::uint32_t count) {
		
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
		os << owner->ast_node->name_string << "(template)";
	}

	void TypeFunctionTemplate::print(std::ostream& os) {
		os << owner->ast_node->name_string << "(template)";
	}

	void TypeTraitTemplate::print(std::ostream& os) {
		os << owner->ast_node->name_string << "(template)";
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
		std::size_t c = (size().eval(ILEvaluator::active->parent) / owner->size().eval(ILEvaluator::active->parent));
		os << "[" << c << "]";
		owner->print(os);
	}
	
	void TypeFunctionInstance::print(std::ostream& os) {
		function_type->print(os);
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

	
	errvoid Type::assert(Cursor& c, Type* t) {
		if (!wrap || wrap(sandbox)==0) {
			std::uint64_t num = t->magic() & type_magic_mask;
			if (num != type_magic) {
				return throw_specific_error(c, "Value is not a type");
			}
		} else {
			return throw_specific_error(c, "Value is not a type");
		}

		return err::ok;
	}

	errvoid Type::assert(Type* t) {
		if (!wrap || wrap(sandbox)==0) {
			if ((t->magic() & type_magic_mask) != type_magic) {
				return throw_runtime_exception(Compiler::current()->evaluator(), "Value is not a type");
			}
		} else {
			return throw_runtime_exception(Compiler::current()->evaluator(), "Value is not a type");
		}

		return err::ok;
	}
	
	std::uint64_t Type::magic() { return type_magic | 0x01u; }
	std::uint64_t TypeStructureInstance::magic() { return type_magic | 0x02u; }
	std::uint64_t TypeStructureTemplate::magic() { return type_magic | 0x03u; }
	std::uint64_t TypeTraitInstance::magic() { return type_magic | 0x04u; }
	std::uint64_t TypeTraitTemplate::magic() { return type_magic | 0x05u; }
	std::uint64_t TypeFunction::magic() { return type_magic | 0x06u; }
	std::uint64_t TypeFunctionTemplate::magic() { return type_magic | 0x07u; }
	std::uint64_t TypeSlice::magic() { return type_magic | 0x08u; }
	std::uint64_t TypeArray::magic() { return type_magic | 0x09u; }
	std::uint64_t TypeReference::magic() { return type_magic | 0x0au; }
	std::uint64_t TypeTemplate::magic() { return type_magic | 0x0bu; }
	std::uint64_t TypeFunctionInstance::magic() { return type_magic | 0x0cu; }

}