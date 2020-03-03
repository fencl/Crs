#include "Operand.h"
#include "Error.h"
#include "Utilities.h"
#include "PredefinedTypes.h"
#include "Expression.h"
#include "StackManager.h"

#include <iostream>

namespace Corrosive {


	CompileValue Operand::parse(Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		CompileValue ret;
		ret.lvalue = false;
		ret.t = nullptr;
		ret.v = nullptr;


		if (c.tok == RecognizedToken::OpenParenthesis) {
			parse_expression(ret, c, ctx, cpt);
		}
		else if (c.tok == RecognizedToken::Symbol) {
			parse_symbol(ret, c, ctx, cpt);
		}
		else if (c.tok == RecognizedToken::Number || c.tok == RecognizedToken::UnsignedNumber) {
			parse_number(ret, c, ctx, cpt);
		}
		else if (c.tok == RecognizedToken::LongNumber || c.tok == RecognizedToken::UnsignedLongNumber) {
			parse_long_number(ret, c, ctx, cpt);
		}
		else if (c.tok == RecognizedToken::FloatNumber || c.tok == RecognizedToken::DoubleNumber) {
			parse_float_number(ret, c, ctx, cpt);
		}
		else {
			throw_specific_error(c, "Expected to parse operand");
		}


		while (true) {
			if (c.tok == RecognizedToken::OpenBracket) {
				parse_array_operator(ret, c, ctx, cpt);
			}
			else if (c.tok == RecognizedToken::Dot) {
				parse_dot_operator(ret, c, ctx, cpt);
			}
			else break;
		}

		return ret;
	}







	void Operand::parse_expression(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		c.move();
		ret = Expression::parse(c, ctx, cpt);
		if (c.tok != RecognizedToken::CloseParenthesis) {
			throw_wrong_token_error(c, "')'");
		}
		c.move();
	}



	void Operand::parse_symbol(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		if (c.data == "true") {
			c.move();
			ret.lvalue = false;
			ret.t = t_bool;
			ret.v = LLVMConstInt(LLVMInt1Type(), true, false);
		}
		else if (c.data == "false") {
			c.move();
			ret.lvalue = false;
			ret.t = t_bool;
			ret.v = LLVMConstInt(LLVMInt1Type(), false, false);
		}
		else {
			Cursor pack;
			Cursor name = c;
			c.move();
			if (c.tok == RecognizedToken::DoubleColon) {
				c.move();
				if (c.tok != RecognizedToken::Symbol) {
					throw_not_a_name_error(c);
				}
				pack = name;
				name = c;
				c.move();
			}

			if (pack.data.empty()) {
				if (auto sitm = StackManager::stack_find(name.data)) {
					ret = sitm->value;
				}
				else {
					throw_variable_not_found_error(name);
				}
			}
		}
	}




	void Operand::parse_number(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		bool usg = c.tok == RecognizedToken::UnsignedNumber;

		std::string_view ndata;
		if (usg)
			ndata = c.data.substr(0, c.data.size() - 1);
		else
			ndata = c.data;

		unsigned long long d = svtoi(ndata);
		c.move();

		if (cpt != CompileType::ShortCircuit)
			ret.v = LLVMConstInt(LLVMInt32Type(), d, !usg);

		ret.t = usg ? Corrosive::t_u32 : Corrosive::t_i32;
		ret.lvalue = false;
	}





	void Operand::parse_long_number(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		bool usg = c.tok == RecognizedToken::UnsignedLongNumber;

		std::string_view ndata;
		if (usg)
			ndata = c.data.substr(0, c.data.size() - 2);
		else
			ndata = c.data.substr(0, c.data.size() - 1);

		unsigned long long d = svtoi(ndata);
		c.move();

		if (cpt != CompileType::ShortCircuit)
			ret.v = LLVMConstInt(LLVMInt64Type(), d, !usg);

		ret.t = usg ? Corrosive::t_u64 : Corrosive::t_i64;
		ret.lvalue = false;
	}





	void Operand::parse_float_number(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		bool dbl = c.tok == RecognizedToken::DoubleNumber;

		std::string_view ndata;
		if (dbl)
			ndata = c.data.substr(0, c.data.size() - 1);
		else
			ndata = c.data;

		double d = svtod(ndata);
		c.move();

		if (cpt != CompileType::ShortCircuit)
			ret.v = LLVMConstReal(dbl ? LLVMDoubleType() : LLVMFloatType(), d);

		ret.t = dbl ? Corrosive::t_f64 : Corrosive::t_f32;
		ret.lvalue = false;
	}





	void Operand::parse_array_operator(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		auto array_type = dynamic_cast<const ArrayType*>(ret.t);
		if (array_type == nullptr) {
			throw_specific_error(c, "Operator requires array type");
		}
		c.move();
		CompileValue v = Expression::parse(c, ctx, cpt);
		//check for integer type
		if (c.tok != RecognizedToken::CloseBracket) {
			throw_wrong_token_error(c, "']'");
		}
		c.move();

		LLVMValueRef ind[] = { v.v };
		ret.v = LLVMBuildGEP2(ctx.builder, array_type->base->LLVMType(), ret.v, ind, 1, "");

		ret.t = array_type->base;
		ret.lvalue = true;
	}





	void Operand::parse_dot_operator(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		auto prim_type = dynamic_cast<const PrimitiveType*>(ret.t);
		if (prim_type == nullptr) {
			throw_specific_error(c, "Operator requires primitive type");
		}
		c.move();

		StructDeclaration* sd = prim_type->structure;

		/*while (true)*/ {

			auto lt = sd->lookup_table.find(c.data);
			if (lt == sd->lookup_table.end()) {
				throw_specific_error(c, "Member was not found");
			}
			Declaration* decl = std::get<0>(lt->second);
			int mid = std::get<1>(lt->second);

			VariableDeclaration* vdecl = dynamic_cast<VariableDeclaration*>(decl);
			if (vdecl != nullptr) {
				ret.v = LLVMBuildStructGEP2(ctx.builder, sd->LLVMType(), ret.v, mid, "");
				ret.t = vdecl->type;
				ret.lvalue = true;
			}
			else {
				FunctionDeclaration* fdecl = dynamic_cast<FunctionDeclaration*>(decl);
				if (fdecl != nullptr) {
					throw_specific_error(c, "functions not implemented yet");
				}
				else {
					throw_specific_error(c, "Aliases not implemented yet");
				}
			}
		}

		c.move();
	}
}