#include "Operand.h"
#include "Error.h"
#include "Utilities.h"
#include "PredefinedTypes.h"
#include "Expression.h"
#include "StackManager.h"

#include <iostream>

namespace Corrosive {

	void Operand::parse_expression(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		c.Move();
		ret = Expression::parse(c, ctx, cpt);
		if (c.Tok() != RecognizedToken::CloseParenthesis) {
			ThrowWrongTokenError(c, "')'");
		}
		c.Move();
	}

	void Operand::parse_symbol(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		if (c.Data() == "true") {
			c.Move();
			ret.lvalue = false;
			ret.t = t_bool;
			ret.v = LLVMConstInt(LLVMInt1Type(), true, false);
		}
		else if (c.Data() == "false") {
			c.Move();
			ret.lvalue = false;
			ret.t = t_bool;
			ret.v = LLVMConstInt(LLVMInt1Type(), false, false);
		}
		else {
			Cursor pack;
			Cursor name = c;
			c.Move();
			if (c.Tok() == RecognizedToken::DoubleColon) {
				c.Move();
				if (c.Tok() != RecognizedToken::Symbol) {
					ThrowNotANameError(c);
				}
				pack = name;
				name = c;
				c.Move();
			}

			if (pack.Data().empty()) {
				if (auto sitm = StackManager::StackFind(name.Data())) {
					ret = sitm->value;
				}
				else {
					ThrowVariableNotFound(name);
				}
			}
		}
	}


	void Operand::parse_number(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		bool usg = c.Tok() == RecognizedToken::UnsignedNumber;

		std::string_view ndata;
		if (usg)
			ndata = c.Data().substr(0, c.Data().size() - 1);
		else
			ndata = c.Data();

		unsigned long long d = svtoi(ndata);
		c.Move();

		if (cpt != CompileType::ShortCircuit)
			ret.v = LLVMConstInt(LLVMInt32Type(), d, !usg);

		ret.t = usg ? Corrosive::t_u32 : Corrosive::t_i32;
		ret.lvalue = false;
	}


	void Operand::parse_long_number(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		bool usg = c.Tok() == RecognizedToken::UnsignedLongNumber;

		std::string_view ndata;
		if (usg)
			ndata = c.Data().substr(0, c.Data().size() - 2);
		else
			ndata = c.Data().substr(0, c.Data().size() - 1);

		unsigned long long d = svtoi(ndata);
		c.Move();

		if (cpt != CompileType::ShortCircuit)
			ret.v = LLVMConstInt(LLVMInt64Type(), d, !usg);

		ret.t = usg ? Corrosive::t_u64 : Corrosive::t_i64;
		ret.lvalue = false;
	}


	void Operand::parse_float_number(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		bool dbl = c.Tok() == RecognizedToken::DoubleNumber;

		std::string_view ndata;
		if (dbl)
			ndata = c.Data().substr(0, c.Data().size() - 1);
		else
			ndata = c.Data();

		double d = svtod(ndata);
		c.Move();

		if (cpt != CompileType::ShortCircuit)
			ret.v = LLVMConstReal(dbl ? LLVMDoubleType() : LLVMFloatType(), d);

		ret.t = dbl ? Corrosive::t_f64 : Corrosive::t_f32;
		ret.lvalue = false;
	}


	void Operand::parse_array_operator(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		auto array_type = dynamic_cast<const ArrayType*>(ret.t);
		if (array_type == nullptr) {
			ThrowSpecificError(c, "Operator requires array type");
		}
		c.Move();
		CompileValue v = Expression::parse(c, ctx, cpt);
		//check for integer type
		if (c.Tok() != RecognizedToken::CloseBracket) {
			ThrowWrongTokenError(c, "']'");
		}
		c.Move();

		LLVMValueRef ind[] = { v.v };
		ret.v = LLVMBuildGEP2(ctx.builder, array_type->base->LLVMType(), ret.v, ind, 1, "");

		ret.t = array_type->base;
		ret.lvalue = true;
	}


	void Operand::parse_dot_operator(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		auto prim_type = dynamic_cast<const PrimitiveType*>(ret.t);
		if (prim_type == nullptr) {
			ThrowSpecificError(c, "Operator requires primitive type");
		}
		c.Move();

		StructDeclaration* sd = prim_type->structure;

		/*while (true)*/ {

			auto lt = sd->lookup_table.find(c.Data());
			if (lt == sd->lookup_table.end()) {
				ThrowSpecificError(c, "Member was not found");
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
					ThrowSpecificError(c, "functions not implemented yet");
				}
				else {
					ThrowSpecificError(c, "Aliases not implemented yet");
				}
			}
		}

		c.Move();
	}







	CompileValue Operand::parse(Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		CompileValue ret;
		ret.lvalue = false;
		ret.t = nullptr;
		ret.v = nullptr;


		if (c.Tok() == RecognizedToken::OpenParenthesis) {
			parse_expression(ret, c, ctx, cpt);
		}
		else if (c.Tok() == RecognizedToken::Symbol) {
			parse_symbol(ret, c, ctx, cpt);
		}
		else if (c.Tok() == RecognizedToken::Number || c.Tok() == RecognizedToken::UnsignedNumber) {
			parse_number(ret, c, ctx, cpt);
		}
		else if (c.Tok() == RecognizedToken::LongNumber || c.Tok() == RecognizedToken::UnsignedLongNumber) {
			parse_long_number(ret, c, ctx, cpt);
		}
		else if (c.Tok() == RecognizedToken::FloatNumber || c.Tok() == RecognizedToken::DoubleNumber) {
			parse_float_number(ret, c, ctx, cpt);
		}
		else {
			ThrowSpecificError(c, "Expected to parse operand");
		}


		while (true) {
			if (c.Tok() == RecognizedToken::OpenBracket) {
				parse_array_operator(ret, c, ctx, cpt);
			}
			else if (c.Tok() == RecognizedToken::Dot) {
				parse_dot_operator(ret, c, ctx, cpt);
			}else break;
		}

		return ret;
	}
}