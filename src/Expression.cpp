#include "Expression.h"
#include "Operand.h"
#include "Error.h"
#include <algorithm>
#include "Declaration.h"
#include "PredefinedTypes.h"
#include <iostream>

namespace Corrosive {


	void Expression::ArithConstPromote(CompileValue& value,int from, int to) {
		if (to == 10) {
			if (from == 9)
				value.v = LLVMConstFPExt(value.v, LLVMDoubleType());
			else {
				if (from % 2 == 1) {
					value.v = LLVMConstUIToFP(value.v, LLVMDoubleType());
				}
				else {
					value.v = LLVMConstSIToFP(value.v, LLVMDoubleType());
				}
			}
			value.t = t_f64;
		}
		else if (to == 9) {
			if (from % 2 == 1) {
				value.v = LLVMConstUIToFP(value.v, LLVMFloatType());
			}
			else {
				value.v = LLVMConstSIToFP(value.v, LLVMFloatType());
			}
			value.t = t_f32;
		}
		else if (to == 8 || to == 7) {
			if (from % 2 == 1) {
				value.v = LLVMConstZExt(value.v, LLVMInt64Type());
			}
			else {
				value.v = LLVMConstSExt(value.v, LLVMInt64Type());
			}

			value.t = (to == 8 ? t_i64 : t_u64);
		}
		else if (to == 6 || to == 5) {
			if (from % 2 == 1) {
				value.v = LLVMConstZExt(value.v, LLVMInt32Type());
			}
			else {
				value.v = LLVMConstSExt(value.v, LLVMInt32Type());
			}

			value.t = (to == 6 ? t_i32 : t_u32);
		}
		else if (to == 4 || to == 3) {
			if (from % 2 == 1) {
				value.v = LLVMConstZExt(value.v, LLVMInt16Type());
			}
			else {
				value.v = LLVMConstSExt(value.v, LLVMInt16Type());
			}

			value.t = (to == 4 ? t_i16 : t_u16);
		}
		else if (to == 2 || to == 1) {
			if (from % 2 == 1) {
				value.v = LLVMConstZExt(value.v, LLVMInt8Type());
			}
			else {
				value.v = LLVMConstSExt(value.v, LLVMInt8Type());
			}

			value.t = (to == 2 ? t_i8 : t_u8);
		}
	}


	int Expression::ArithValue(const PrimitiveType* pt) {
		StructDeclarationType sdt = pt->Structure()->DeclType();

		switch (sdt)
		{
		case Corrosive::StructDeclarationType::t_bool:
			return 0;
		case Corrosive::StructDeclarationType::t_u8:
			return 1;
		case Corrosive::StructDeclarationType::t_u16:
			return 3;
		case Corrosive::StructDeclarationType::t_u32:
			return 5;
		case Corrosive::StructDeclarationType::t_u64:
			return 7;
		case Corrosive::StructDeclarationType::t_i8:
			return 2;
		case Corrosive::StructDeclarationType::t_i16:
			return 4;
		case Corrosive::StructDeclarationType::t_i32:
			return 6;
		case Corrosive::StructDeclarationType::t_i64:
			return 8;
		case Corrosive::StructDeclarationType::t_f32:
			return 9;
		case Corrosive::StructDeclarationType::t_f64:
			return 10;
		default:
			return 11;
			break;
		}
	}

	bool Expression::ArithCast(CompileValue& left, CompileValue& right, bool& isfloat,bool& issigned) {
		auto ltp = dynamic_cast<const PrimitiveType*>(left.t);
		auto rtp = dynamic_cast<const PrimitiveType*>(right.t);

		if (ltp == nullptr || rtp == nullptr) {
			return false;
		}
		else {
			int arith_value_left = ArithValue(ltp);
			int arith_value_right = ArithValue(rtp);

			int arith_value_res = std::max(arith_value_left, arith_value_right);

			if (arith_value_left == arith_value_right) {

			}
			else if (arith_value_left > arith_value_right) {
				ArithConstPromote(right, arith_value_right, arith_value_left);
			}
			else {
				ArithConstPromote(left, arith_value_left, arith_value_right);
			}

			issigned = (arith_value_res % 2 != 0);
			isfloat = (arith_value_res >= 8);

			return true;
		}
	}

	CompileValue Expression::EmitOperator(Cursor& c, CompileContextExt& ctx, int l, int op, CompileValue left, CompileValue right,CompileType cpt,int next_l,int next_op) {
		bool isf = false;
		bool sig = false;

		if (!ArithCast(left, right, isf,sig)) {
			ThrowSpecificError(c, "Types of operands cannot be used in this operation");
		}

		CompileValue ret = left;
		ret.lvalue = false;
		if (l == 1 || l == 2)
			ret.t = t_bool;

		if(cpt != CompileType::ShortCircuit) {
			if (l == 0) {
				if (op == 0) {
					if (cpt == CompileType::Eval)
						ret.v = LLVMConstAnd(left.v, right.v);
					else
						ret.v = LLVMBuildAnd(ctx.builder, left.v, right.v, "");
				}
				else if (op == 1) {
					if (cpt == CompileType::Eval)
						ret.v = LLVMConstOr(left.v, right.v);
					else
						ret.v = LLVMBuildOr(ctx.builder,left.v, right.v,"");
				}
				else if (op == 2) {
					if (cpt == CompileType::Eval)
						ret.v = LLVMConstXor(left.v, right.v); 
					else
						ret.v = LLVMBuildXor(ctx.builder,left.v, right.v,"");
				}
			}
			else if (l == 1) {
				if (isf) {
					LLVMRealPredicate pred;

					if (op == 0)
						pred = LLVMRealPredicate::LLVMRealUEQ;
					else if (op == 1)
						pred = LLVMRealPredicate::LLVMRealUNE;

					if (cpt == CompileType::Eval)
						ret.v = LLVMConstFCmp(pred, left.v, right.v);
					else
						ret.v = LLVMBuildFCmp(ctx.builder, pred, left.v, right.v, "");
				}
				else {
					LLVMIntPredicate pred;

					if (op == 0)
						pred = LLVMIntPredicate::LLVMIntEQ;
					else if (op == 1)
						pred = LLVMIntPredicate::LLVMIntNE;

					if (cpt == CompileType::Eval)
						ret.v = LLVMConstICmp(pred, left.v, right.v);
					else
						ret.v = LLVMBuildICmp(ctx.builder, pred, left.v, right.v, "");
				}
			}
			else if (l == 2) {
				if (isf) {
					LLVMRealPredicate pred;

					if (op == 0)
						pred = LLVMRealPredicate::LLVMRealOGT;
					else if (op ==1)
						pred = LLVMRealPredicate::LLVMRealOLT;
					else if (op == 2)
						pred = LLVMRealPredicate::LLVMRealOGE;
					else if (op == 3)
						pred = LLVMRealPredicate::LLVMRealOLE;

					if (cpt == CompileType::Eval)
						ret.v = LLVMConstFCmp(pred, left.v, right.v);
					else
						ret.v = LLVMBuildFCmp(ctx.builder, pred, left.v, right.v, "");
				}
				else {
					LLVMIntPredicate pred;

					if (sig) {
						if (op == 0)
							pred = LLVMIntPredicate::LLVMIntSGT;
						else if (op == 1)
							pred = LLVMIntPredicate::LLVMIntSLT;
						else if (op == 2)
							pred = LLVMIntPredicate::LLVMIntSGE;
						else if (op == 3)
							pred = LLVMIntPredicate::LLVMIntSLE;
					}
					else {
						if (op == 0)
							pred = LLVMIntPredicate::LLVMIntUGT;
						else if (op == 1)
							pred = LLVMIntPredicate::LLVMIntULT;
						else if (op == 2)
							pred = LLVMIntPredicate::LLVMIntUGE;
						else if (op == 3)
							pred = LLVMIntPredicate::LLVMIntULE;
					}

					if (cpt == CompileType::Eval)
						ret.v = LLVMConstICmp(pred, left.v, right.v);
					else
						ret.v = LLVMBuildICmp(ctx.builder, pred, left.v, right.v, "");
				}
			}
			else if (l == 3) {
				if (op == 0) {
					if (cpt == CompileType::Eval) {
						if (isf)
							ret.v = LLVMConstFAdd(left.v, right.v);
						else
							ret.v = LLVMConstAdd(left.v, right.v);
					}
					else {
						if (isf)
							ret.v = LLVMBuildFAdd(ctx.builder,left.v, right.v,"");
						else
							ret.v = LLVMBuildAdd(ctx.builder,left.v, right.v,"");
					}
				}
				else if (op == 1) {

					if (cpt == CompileType::Eval) {
						if (isf)
							ret.v = LLVMConstFSub(left.v, right.v);
						else
							ret.v = LLVMConstSub(left.v, right.v);
					}
					else if (cpt != CompileType::ShortCircuit) {
						if (isf)
							ret.v = LLVMBuildFSub(ctx.builder,left.v, right.v,"");
						else
							ret.v = LLVMBuildSub(ctx.builder,left.v, right.v,"");
					}
				}
			}
			else if (l == 4) {
				if (op == 0) {

					if (cpt == CompileType::Eval) {
						if (isf)
							ret.v = LLVMConstFMul(left.v, right.v);
						else
							ret.v = LLVMConstMul(left.v, right.v);
					}
					else {
						if (isf)
							ret.v = LLVMBuildFMul(ctx.builder,left.v, right.v,"");
						else
							ret.v = LLVMBuildMul(ctx.builder, left.v, right.v, "");
					}
				}
				else if (op == 1) {

					if (cpt == CompileType::Eval) {
						if (isf)
							ret.v = LLVMConstFDiv(left.v, right.v);
						else {
							if (sig)
								ret.v = LLVMConstSDiv(left.v, right.v);
							else
								ret.v = LLVMConstUDiv(left.v, right.v);
						}
					}
					else {
						if (isf)
							ret.v = LLVMBuildFDiv(ctx.builder, left.v, right.v, "");
						else {
							if (sig)
								ret.v = LLVMBuildSDiv(ctx.builder, left.v, right.v, "");
							else
								ret.v = LLVMBuildUDiv(ctx.builder, left.v, right.v, "");
						}
					}
				}
				else if (op == 2) {

					if (cpt == CompileType::Eval) {
						if (isf)
							ret.v = LLVMConstFRem(left.v, right.v);
						else {
							if (sig)
								ret.v = LLVMConstSRem(left.v, right.v);
							else
								ret.v = LLVMConstURem(left.v, right.v);
						}
					}
					else {
						if (isf)
							ret.v = LLVMBuildFRem(ctx.builder, left.v, right.v, "");
						else {
							if (sig)
								ret.v = LLVMBuildSRem(ctx.builder, left.v, right.v, "");
							else
								ret.v = LLVMBuildURem(ctx.builder, left.v, right.v, "");
						}
					}
				}
			}
		}

		return ret;
	}

	CompileValue Expression::Parse(Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		return Parse1(c, ctx, cpt);
	}

	CompileValue Expression::Parse1(Cursor& c, CompileContextExt& ctx, CompileType cpt) {

		CompileValue value = Expression::Parse2(c, ctx, cpt);
		

		while (c.Tok() == RecognizedToken::DoubleAnd) {
			if (value.t != t_bool) {
				ThrowSpecificError(c, "Operation requires left operand to be boolean");
			}

			c.Move();

			if (cpt == CompileType::ShortCircuit || (value.v != nullptr && LLVMIsAConstantInt(value.v) && !LLVMConstIntGetZExtValue(value.v))) {
				Parse2(c, ctx, CompileType::ShortCircuit);
			}
			else {

				if (cpt == CompileType::Eval) {
					CompileValue right = Expression::Parse2(c, ctx, cpt);
					value.v = LLVMConstAnd(value.v, right.v);
				}
				else {
					if (!ctx.fallback_and) {
						ctx.fallback_and = LLVMCreateBasicBlockInContext(LLVMGetGlobalContext(), "");
					}

					ctx.incoming_blocks_and.push_back(ctx.block);
					ctx.incoming_values_and.push_back(LLVMConstInt(LLVMInt1Type(), false, false));
					LLVMBasicBlockRef positive_block = LLVMAppendBasicBlock(ctx.function, "");
					LLVMBuildCondBr(ctx.builder, value.v, positive_block, ctx.fallback_and);
					ctx.block = positive_block;
					LLVMPositionBuilderAtEnd(ctx.builder, positive_block);

					value = Expression::Parse2(c, ctx, cpt);
				}
			}
		}

		if (ctx.fallback_and != nullptr && cpt == CompileType::Compile) {
			if (value.t != t_bool) {
				ThrowSpecificError(c, "Operation requires right operand to be boolean");
			}

			std::reverse(ctx.incoming_blocks_and.begin(), ctx.incoming_blocks_and.end());
			std::reverse(ctx.incoming_values_and.begin(), ctx.incoming_values_and.end());
			ctx.incoming_blocks_and.push_back(ctx.block);
			ctx.incoming_values_and.push_back(value.v);
			LLVMBuildBr(ctx.builder, ctx.fallback_and);
			


			LLVMAppendExistingBasicBlock(ctx.function, ctx.fallback_and);
			ctx.block = ctx.fallback_and;
			ctx.fallback_and = nullptr;
			LLVMPositionBuilderAtEnd(ctx.builder, ctx.block);
			value.v = LLVMBuildPhi(ctx.builder, LLVMInt1Type(), "");
			value.t = t_bool;
			value.lvalue = false;
			LLVMAddIncoming(value.v, ctx.incoming_values_and.data(), ctx.incoming_blocks_and.data(), (unsigned int)ctx.incoming_blocks_and.size());

			ctx.incoming_values_and.clear();
			ctx.incoming_blocks_and.clear();
		}

		return value;
	}



	CompileValue Expression::Parse2(Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		CompileValue value = Expression::Parse3(c, ctx, cpt);
		while (c.Tok() == RecognizedToken::DoubleOr) {
			if (value.t != t_bool) {
				ThrowSpecificError(c, "Operation requires left operand to be boolean");
			}

			c.Move();

			if (cpt == CompileType::ShortCircuit || (value.v != nullptr && LLVMIsAConstantInt(value.v) && LLVMConstIntGetZExtValue(value.v))) {
				Parse3(c, ctx, CompileType::ShortCircuit);
			}
			else {
				
				if (cpt == CompileType::Eval) {
					CompileValue right = Expression::Parse3(c, ctx, cpt);
					value.v = LLVMConstAnd(value.v, right.v);
				}
				else {
					if (!ctx.fallback_or) {
						ctx.fallback_or = LLVMCreateBasicBlockInContext(LLVMGetGlobalContext(), "");
					}

					ctx.incoming_blocks_or.push_back(ctx.block);
					ctx.incoming_values_or.push_back(LLVMConstInt(LLVMInt1Type(), true, false));
					LLVMBasicBlockRef positive_block = LLVMAppendBasicBlock(ctx.function, "");
					LLVMBuildCondBr(ctx.builder, value.v, ctx.fallback_or, positive_block);
					ctx.block = positive_block;
					LLVMPositionBuilderAtEnd(ctx.builder, positive_block);

					value = Expression::Parse3(c, ctx, cpt);
				}
			}
		}

		if (ctx.fallback_or != nullptr && cpt == CompileType::Compile) {
			if (value.t != t_bool) {
				ThrowSpecificError(c, "Operation requires right operand to be boolean");
			}

			std::reverse(ctx.incoming_blocks_or.begin(), ctx.incoming_blocks_or.end());
			std::reverse(ctx.incoming_values_or.begin(), ctx.incoming_values_or.end());
			ctx.incoming_blocks_or.push_back(ctx.block);
			ctx.incoming_values_or.push_back(value.v);
			LLVMBuildBr(ctx.builder, ctx.fallback_or);

			LLVMAppendExistingBasicBlock(ctx.function, ctx.fallback_or);
			ctx.block = ctx.fallback_or;
			ctx.fallback_or = nullptr;
			LLVMPositionBuilderAtEnd(ctx.builder, ctx.block);
			value.v = LLVMBuildPhi(ctx.builder, LLVMInt1Type(), "");
			value.t = t_bool;
			value.lvalue = false;
			LLVMAddIncoming(value.v, ctx.incoming_values_or.data(), ctx.incoming_blocks_or.data(), (unsigned int)ctx.incoming_blocks_or.size());

			ctx.incoming_values_or.clear();
			ctx.incoming_blocks_or.clear();
		}

		return value;
	}

	CompileValue Expression::Parse3(Cursor& c, CompileContextExt& ctx, CompileType cpt) {

		int op_type[5];
		CompileValue layer[5];
		memset(layer, 0, sizeof(layer));

		int current_layer = -1;

		while (true) {
			CompileValue value = Operand::Parse(c, ctx, cpt);
			int op_v = -1;
			int op_t = -1;

			if (c.Tok() == RecognizedToken::And) {
				op_v = 0;
				op_t = 0;
			}
			else if (c.Tok() == RecognizedToken::Or) {
				op_v = 0;
				op_t = 1;
			}
			else if(c.Tok() == RecognizedToken::Xor) {
				op_v = 0;
				op_t = 2;
			}
			else if (c.Tok() == RecognizedToken::DoubleEquals) {
				op_v = 1;
				op_t = 0;
			}
			else if (c.Tok() == RecognizedToken::NotEquals) {
				op_v = 1;
				op_t = 1;
			}
			else  if (c.Tok() == RecognizedToken::GreaterThan) {
				op_v = 2;
				op_t = 0;
			}
			else if (c.Tok() == RecognizedToken::LessThan) {
				op_v = 2;
				op_t = 1;
			}
			else if(c.Tok() == RecognizedToken::GreaterOrEqual) {
				op_v = 2;
				op_t = 2;
			}
			else if (c.Tok() == RecognizedToken::LessOrEqual) {
				op_v = 2;
				op_t = 3;
			}
			else if (c.Tok() == RecognizedToken::Plus) {
				op_v = 3;
				op_t = 0;
			}
			else if (c.Tok() == RecognizedToken::Minus) {
				op_v = 3;
				op_t = 1;
			}
			else if (c.Tok() == RecognizedToken::Star) {
				op_v = 4;
				op_t = 0;
			}
			else if (c.Tok() == RecognizedToken::Slash) {
				op_v = 4;
				op_t = 1;
			}
			else if (c.Tok() == RecognizedToken::Percent) {
				op_v = 4;
				op_t = 2;
			}
			
			
			for (int i = current_layer; i >= std::max(op_v, 0); i--) {

				if (layer[i].v != nullptr) {
					CompileValue& left = layer[i];
					CompileValue& right = value;
					CompileType cpt2 = cpt;

					if (cpt == CompileType::Compile && LLVMIsConstant(left.v) && LLVMIsConstant(right.v))
						cpt2 = CompileType::Eval;

					value = EmitOperator(c, ctx, i, op_type[i], left, right, cpt2, op_v,op_t);
					layer[i].v = nullptr;
				}
			}
			

			if (op_v >= 0) {

				layer[op_v] = value;
				op_type[op_v] = op_t;
				current_layer = op_v;
				

				c.Move();
			}
			else {
				return value;
			}
		}
	}
}