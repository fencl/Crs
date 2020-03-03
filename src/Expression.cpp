#include "Expression.h"
#include "Operand.h"
#include "Error.h"
#include <algorithm>
#include "Declaration.h"
#include "PredefinedTypes.h"
#include <iostream>
#include <llvm/Target.h>

namespace Corrosive {
	void Expression::rvalue(CompileContextExt& ctx,CompileValue& value,CompileType cpt) {
		if (value.lvalue) {
			if (cpt == CompileType::compile) {
				value.lvalue = false;
				value.v = LLVMBuildLoad2(ctx.builder, value.t->LLVMTypeRValue(), value.v, "");
			}
			else if (cpt == CompileType::Eval) {
				value.lvalue = false;
				void* ptr = (void*)LLVMConstIntGetZExtValue(value.v);
				//int size = LLVMABISizeOfType(ctx.basic.target_layout,value.t->LLVMType());
			}
		}
	}

	void Expression::arith_promote(CompileValue& value,int from, int to) {
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


	int Expression::arith_value(const PrimitiveType* pt) {
		StructDeclarationType sdt = pt->structure->decl_type;

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

	bool Expression::arith_cast(CompileValue& left, CompileValue& right, bool& isfloat,bool& issigned) {
		auto ltp = dynamic_cast<const PrimitiveType*>(left.t);
		auto rtp = dynamic_cast<const PrimitiveType*>(right.t);

		if (ltp == nullptr || rtp == nullptr) {
			return false;
		}
		else {
			int arith_value_left = arith_value(ltp);
			int arith_value_right = arith_value(rtp);

			int arith_value_res = std::max(arith_value_left, arith_value_right);
			if (arith_value_res == 11) return false;

			if (arith_value_left == arith_value_right) {

			}
			else if (arith_value_left > arith_value_right) {
				arith_promote(right, arith_value_right, arith_value_left);
			}
			else {
				arith_promote(left, arith_value_left, arith_value_right);
			}

			issigned = (arith_value_res % 2 == 0);
			isfloat = (arith_value_res >= 9);

			return true;
		}
	}

	CompileValue Expression::emit(Cursor& c, CompileContextExt& ctx, int l, int op, CompileValue left, CompileValue right,CompileType cpt,int next_l,int next_op) {
		bool isf = false;
		bool sig = false;

		rvalue(ctx, left, cpt);
		rvalue(ctx, right, cpt);

		if (!arith_cast(left, right, isf, sig)) {
			throw_specific_error(c, "Types of operands cannot be used in this operation");
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

	CompileValue Expression::parse(Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		return parse_or(c, ctx, cpt);
	}

	CompileValue Expression::parse_and(Cursor& c, CompileContextExt& ctx, CompileType cpt) {

		CompileValue value = Expression::parse_operators(c, ctx, cpt);
		

		while (c.tok == RecognizedToken::DoubleAnd) {
			if (value.t != t_bool) {
				throw_specific_error(c, "Operation requires left operand to be boolean");
			}

			c.move();

			if (cpt == CompileType::ShortCircuit || (value.v != nullptr && LLVMIsAConstantInt(value.v) && !LLVMConstIntGetZExtValue(value.v))) {
				parse_operators(c, ctx, CompileType::ShortCircuit);
			}
			else {

				if (cpt == CompileType::Eval) {
					CompileValue right = Expression::parse_operators(c, ctx, cpt);
					value.v = LLVMConstAnd(value.v, right.v);
				}
				else {
					if (!ctx.fallback_and) {
						ctx.fallback_and = LLVMCreateBasicBlockInContext(LLVMGetGlobalContext(), "");
					}
					rvalue(ctx, value, cpt);
					ctx.incoming_blocks_and.push_back(ctx.block);
					ctx.incoming_values_and.push_back(LLVMConstInt(LLVMInt1Type(), false, false));
					LLVMBasicBlockRef positive_block = LLVMAppendBasicBlock(ctx.function, "");
					LLVMBuildCondBr(ctx.builder, value.v, positive_block, ctx.fallback_and);
					ctx.block = positive_block;
					LLVMPositionBuilderAtEnd(ctx.builder, positive_block);

					value = Expression::parse_operators(c, ctx, cpt);
				}
			}
		}

		if (ctx.fallback_and != nullptr && cpt == CompileType::compile) {
			if (value.t != t_bool) {
				throw_specific_error(c, "Operation requires right operand to be boolean");
			}

			rvalue(ctx, value, cpt);

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



	CompileValue Expression::parse_or(Cursor& c, CompileContextExt& ctx, CompileType cpt) {
		CompileValue value = Expression::parse_and(c, ctx, cpt);
		while (c.tok == RecognizedToken::DoubleOr) {
			if (value.t != t_bool) {
				throw_specific_error(c, "Operation requires left operand to be boolean");
			}

			c.move();

			if (cpt == CompileType::ShortCircuit || (value.v != nullptr && LLVMIsAConstantInt(value.v) && LLVMConstIntGetZExtValue(value.v))) {
				parse_and(c, ctx, CompileType::ShortCircuit);
			}
			else {
				
				if (cpt == CompileType::Eval) {
					CompileValue right = Expression::parse_and(c, ctx, cpt);
					value.v = LLVMConstAnd(value.v, right.v);
				}
				else {
					if (!ctx.fallback_or) {
						ctx.fallback_or = LLVMCreateBasicBlockInContext(LLVMGetGlobalContext(), "");
					}

					rvalue(ctx, value, cpt);

					ctx.incoming_blocks_or.push_back(ctx.block);
					ctx.incoming_values_or.push_back(LLVMConstInt(LLVMInt1Type(), true, false));
					LLVMBasicBlockRef positive_block = LLVMAppendBasicBlock(ctx.function, "");
					LLVMBuildCondBr(ctx.builder, value.v, ctx.fallback_or, positive_block);
					ctx.block = positive_block;
					LLVMPositionBuilderAtEnd(ctx.builder, positive_block);

					value = Expression::parse_and(c, ctx, cpt);
				}
			}
		}

		if (ctx.fallback_or != nullptr && cpt == CompileType::compile) {
			if (value.t != t_bool) {
				throw_specific_error(c, "Operation requires right operand to be boolean");
			}

			rvalue(ctx, value, cpt);

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

	CompileValue Expression::parse_operators(Cursor& c, CompileContextExt& ctx, CompileType cpt) {

		int op_type[5] = { -1 };
		CompileValue layer[5];
		memset(layer, 0, sizeof(layer));

		int current_layer = -1;

		while (true) {
			CompileValue value = Operand::parse(c, ctx, cpt);
			int op_v = -1;
			int op_t = -1;

			if (c.tok == RecognizedToken::And) {
				op_v = 0;
				op_t = 0;
			}
			else if (c.tok == RecognizedToken::Or) {
				op_v = 0;
				op_t = 1;
			}
			else if(c.tok == RecognizedToken::Xor) {
				op_v = 0;
				op_t = 2;
			}
			else if (c.tok == RecognizedToken::DoubleEquals) {
				op_v = 1;
				op_t = 0;
			}
			else if (c.tok == RecognizedToken::NotEquals) {
				op_v = 1;
				op_t = 1;
			}
			else  if (c.tok == RecognizedToken::GreaterThan) {
				op_v = 2;
				op_t = 0;
			}
			else if (c.tok == RecognizedToken::LessThan) {
				op_v = 2;
				op_t = 1;
			}
			else if(c.tok == RecognizedToken::GreaterOrEqual) {
				op_v = 2;
				op_t = 2;
			}
			else if (c.tok == RecognizedToken::LessOrEqual) {
				op_v = 2;
				op_t = 3;
			}
			else if (c.tok == RecognizedToken::Plus) {
				op_v = 3;
				op_t = 0;
			}
			else if (c.tok == RecognizedToken::Minus) {
				op_v = 3;
				op_t = 1;
			}
			else if (c.tok == RecognizedToken::Star) {
				op_v = 4;
				op_t = 0;
			}
			else if (c.tok == RecognizedToken::Slash) {
				op_v = 4;
				op_t = 1;
			}
			else if (c.tok == RecognizedToken::Percent) {
				op_v = 4;
				op_t = 2;
			}
			
			for (int i = current_layer; i >= std::max(op_v, 0); i--) {

				if (i>=0 && layer[i].v != nullptr) {
					CompileValue& left = layer[i];
					CompileValue& right = value;
					CompileType cpt2 = cpt;

					if (cpt == CompileType::compile && LLVMIsConstant(left.v) && LLVMIsConstant(right.v))
						cpt2 = CompileType::Eval;

					value = emit(c, ctx, i, op_type[i], left, right, cpt2, op_v, op_t);
					layer[i].v = nullptr;
				}
			}
			
			

			if (op_v >= 0) {

				layer[op_v] = value;
				op_type[op_v] = op_t;
				current_layer = op_v;

				c.move();
			}
			else {
				return value;
			}
		}
	}
}