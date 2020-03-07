#include "Operand.h"
#include "Error.h"
#include "Utilities.h"
#include "PredefinedTypes.h"
#include "Expression.h"
#include "StackManager.h"

#include <iostream>

namespace Corrosive {


	bool Operand::parse(Cursor& c, CompileContextExt& ctx, CompileValue& res, CompileType cpt) {
		CompileValue ret;
		ret.lvalue = false;
		ret.t = nullptr;

		switch (c.tok) {
			case RecognizedToken::OpenParenthesis: {
					if (!parse_expression(ret, c, ctx, cpt)) return false;
				}break;

			case RecognizedToken::Symbol: {
					if (!parse_symbol(ret, c, ctx, cpt)) return false;
				}break;

			case RecognizedToken::Number:
			case RecognizedToken::UnsignedNumber: {
					if (!parse_number(ret, c, ctx, cpt)) return false;
				}break;

			case RecognizedToken::LongNumber:
			case RecognizedToken::UnsignedLongNumber: {
					if (!parse_long_number(ret, c, ctx, cpt)) return false;
				}break;

			case RecognizedToken::FloatNumber:
			case RecognizedToken::DoubleNumber: {
					if (!parse_float_number(ret, c, ctx, cpt)) return false;
				}break;

			default: {
					throw_specific_error(c, "Expected to parse operand");
					return false;
				} break;
		}

		while (true) {
			switch (c.tok) {
				case RecognizedToken::OpenBracket: {
						if (!parse_array_operator(ret, c, ctx, cpt)) return false;
					}break;
				case RecognizedToken::Dot: {
						if (!parse_dot_operator(ret, c, ctx, cpt)) return false;
					}break;
				default: goto break_while;
			}

			continue;
		break_while:
			break;
		}

		res = ret;
		return true;
	}







	bool Operand::parse_expression(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		c.move();
		if (!Expression::parse(c, ctx, ret, cpt)) return false;
		if (c.tok != RecognizedToken::CloseParenthesis) {
			throw_wrong_token_error(c, "')'");
			return false;
		}
		c.move();

		return true;
	}



	bool Operand::parse_symbol(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		if (c.buffer == "true") {
			c.move();
			ret.lvalue = false;
			ret.t = t_bool;
			ILBuilder::build_const_ibool(ctx.block, true);
		}
		else if (c.buffer == "false") {
			c.move();
			ret.lvalue = false;
			ret.t = t_bool;
			ILBuilder::build_const_ibool(ctx.block, false);
		}
		else {
			Cursor pack;
			Cursor name = c;
			c.move();
			if (c.tok == RecognizedToken::DoubleColon) {
				c.move();
				if (c.tok != RecognizedToken::Symbol) {
					throw_not_a_name_error(c);
					return false;
				}
				pack = name;
				name = c;
				c.move();
			}

			if (pack.buffer.empty()) {
				if (auto sitm = StackManager::stack_find(name.buffer)) {
					ILBuilder::build_local(ctx.block,sitm->ir_local);
					ret = sitm->value;
				}
				else {
					throw_variable_not_found_error(name);
					return false;
				}
			}
		}

		return true;
	}




	bool Operand::parse_number(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		bool usg = c.tok == RecognizedToken::UnsignedNumber;

		std::string_view ndata;
		if (usg)
			ndata = c.buffer.substr(0, c.buffer.size() - 1);
		else
			ndata = c.buffer;

		unsigned long long d = svtoi(ndata);
		c.move();

		if (cpt != CompileType::ShortCircuit) {
			if (usg)
				ILBuilder::build_const_u32(ctx.block, (uint32_t)d);
			else
				ILBuilder::build_const_i32(ctx.block, (int32_t)d);
		}

		ret.t = usg ? Corrosive::t_u32 : Corrosive::t_i32;
		ret.lvalue = false;
		return true;
	}





	bool Operand::parse_long_number(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		bool usg = c.tok == RecognizedToken::UnsignedLongNumber;

		std::string_view ndata;
		if (usg)
			ndata = c.buffer.substr(0, c.buffer.size() - 2);
		else
			ndata = c.buffer.substr(0, c.buffer.size() - 1);

		unsigned long long d = svtoi(ndata);
		c.move();

		if (cpt != CompileType::ShortCircuit) {
			if (usg)
				ILBuilder::build_const_u64(ctx.block, d);
			else
				ILBuilder::build_const_i64(ctx.block, d);
		}

		ret.t = usg ? Corrosive::t_u64 : Corrosive::t_i64;
		ret.lvalue = false;
		return true;
	}





	bool Operand::parse_float_number(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		bool dbl = c.tok == RecognizedToken::DoubleNumber;

		std::string_view ndata;
		if (dbl)
			ndata = c.buffer.substr(0, c.buffer.size() - 1);
		else
			ndata = c.buffer;

		double d = svtod(ndata);
		c.move();

		if (cpt != CompileType::ShortCircuit) {
			if (dbl)
				ILBuilder::build_const_f64(ctx.block, d);
			else
				ILBuilder::build_const_f32(ctx.block, (float)d);
		}

		ret.t = dbl ? Corrosive::t_f64 : Corrosive::t_f32;
		ret.lvalue = false;
		return true;
	}





	bool Operand::parse_array_operator(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		auto array_type = dynamic_cast<const ArrayType*>(ret.t);
		if (array_type == nullptr) {
			throw_specific_error(c, "Operator requires array type");
			return false;
		}
		c.move();
		CompileValue v;
		if (!Expression::parse(c, ctx,v, cpt)) return false;
		//check for integer type
		if (c.tok != RecognizedToken::CloseBracket) {
			throw_wrong_token_error(c, "']'");
			return false;
		}
		c.move();

		//LLVMValueRef ind[] = { v.v };
		//ret.v = LLVMBuildGEP2(ctx.builder, array_type->base->LLVMType(), ret.v, ind, 1, "");

		ret.t = array_type->base;
		ret.lvalue = true;
		return true;
	}





	bool Operand::parse_dot_operator(CompileValue& ret, Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		auto prim_type = dynamic_cast<const PrimitiveType*>(ret.t);
		if (prim_type == nullptr) {
			throw_specific_error(c, "Operator requires primitive type");
			return false;
		}
		c.move();

		StructDeclaration* sd = prim_type->structure;

		/*while (true)*/ {

			auto lt = sd->lookup_table.find(c.buffer);
			if (lt == sd->lookup_table.end()) {
				throw_specific_error(c, "Member was not found");
				return false;
			}
			Declaration* decl = std::get<0>(lt->second);
			int mid = std::get<1>(lt->second);

			VariableDeclaration* vdecl = dynamic_cast<VariableDeclaration*>(decl);
			if (vdecl != nullptr) {
				if (!ILBuilder::build_member(ctx.block, ((ILStruct*)sd->iltype), mid)) return false;
				//ret.v = LLVMBuildStructGEP2(ctx.builder, sd->LLVMType(), ret.v, mid, "");
				ret.t = vdecl->type;
				ret.lvalue = true;
			}
			else {
				FunctionDeclaration* fdecl = dynamic_cast<FunctionDeclaration*>(decl);
				if (fdecl != nullptr) {
					throw_specific_error(c, "functions not implemented yet");
					return false;
				}
				else {
					throw_specific_error(c, "Aliases not implemented yet");
					return false;
				}
			}
		}

		c.move();
		return true;
	}
}