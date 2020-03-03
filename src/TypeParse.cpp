#include "Type.h"
#include "Error.h"
#include <iostream>
#include "Declaration.h"
#include "Contents.h"
#include "PredefinedTypes.h"
#include "svtoi.h"
#include "Expression.h"

namespace Corrosive {
	const Type* Type::parse(Cursor& c, std::vector<Cursor>* argnames) {
		CompileContext cctx;
		cctx.parent_namespace = nullptr;
		cctx.parent_struct = nullptr;
		return parse_direct(cctx, c, argnames);
	}

	const Type* Type::parse_direct(CompileContext& ctx,Cursor& c, std::vector<Cursor>* argnames) {
		const Type* rType = nullptr;


		if (c.Tok() == RecognizedToken::Eof) {
			ThrowEofError(c, "an attempt to parse type");
		}


		if (c.Tok() == RecognizedToken::OpenParenthesis) {
			PrimitiveType pType;
			Cursor voidc;
			voidc.Data("void");

			pType.name = voidc;
			pType.package = PredefinedNamespace;


			rType = Contents::EmplaceType(pType);
		}
		else {
			if (c.Tok() == RecognizedToken::OpenBracket) {
				TupleType fType;
				std::vector<const Type*> tps;
				c.Move();

				while (true) {
					if (c.Tok() == RecognizedToken::CloseBracket) {
						c.Move(); break;
					}

					tps.push_back(Type::parse(c));

					if (c.Tok() == RecognizedToken::Comma) c.Move(); else if (c.Tok() != RecognizedToken::CloseBracket) {
						ThrowWrongTokenError(c, "',' or ']'");
					}
				}
				
				if ((c.Tok() == RecognizedToken::Symbol && c.Data() == "ref") || c.Tok() == RecognizedToken::And) {
					fType.ref = true;
					c.Move();
				}

				fType.types = Contents::RegisterTypeArray(std::move(tps));
				rType = Contents::EmplaceType(fType);

			}
			else if (c.Tok() == RecognizedToken::LessThan) {
				InterfaceType fType;
				std::vector<const Type*> tps;
				c.Move();

				while (true) {
					if (c.Tok() == RecognizedToken::GreaterThan) {
						c.Move(); break;
					}

					tps.push_back(Type::parse(c));

					if (c.Tok() == RecognizedToken::Comma) c.Move(); else if (c.Tok() != RecognizedToken::GreaterThan) {
						ThrowWrongTokenError(c, "',' or '>'");
					}
				}
				
				if ((c.Tok() == RecognizedToken::Symbol && c.Data() == "ref") || c.Tok() == RecognizedToken::And) {
					fType.ref = true;
					c.Move();
				}

				fType.Types() = Contents::RegisterTypeArray(std::move(tps));
				rType = Contents::EmplaceType(fType);
			}
			else {
				if (c.Tok() != RecognizedToken::Symbol) {
					ThrowNotANameError(c);
				}

				PrimitiveType pType;
				pType.name = c;
				c.Move();

				if (c.Tok() == RecognizedToken::DoubleColon) {
					pType.package = pType.name.Data();
					c.Move();
					pType.name = c;
					c.Move();
				}

				if (c.Tok() == RecognizedToken::LessThan) {
					TemplateContext tps;

					c.Move();
					while (true) {
						if (c.Tok() == RecognizedToken::GreaterThan) {
							c.Move(); break;
						}

						tps.push_back(Type::parse(c));

						if (c.Tok() == RecognizedToken::Comma) c.Move(); else if (c.Tok() != RecognizedToken::GreaterThan) {
							ThrowWrongTokenError(c, "',' or '>'");
						}
					}
					pType.templates = Contents::RegisterGenericArray(std::move(tps));
				}


				if ((c.Tok() == RecognizedToken::Symbol && c.Data() == "ref") || c.Tok() == RecognizedToken::And) {
					pType.ref = true;
					c.Move();
				}

				rType = Contents::EmplaceType(pType);
			}
		}


		while (true) {
			if (c.Tok() == RecognizedToken::OpenParenthesis) {
				FunctionType fType;
				std::vector<const Type*> tps;
				fType.returns = rType;

				c.Move();
				while (true) {
					if (c.Tok() == RecognizedToken::CloseParenthesis) {
						c.Move(); break;
					}

					if (argnames != nullptr)
					{
						argnames->push_back(c);
						c.Move();
						if (c.Tok() != RecognizedToken::Colon) {
							ThrowWrongTokenError(c, "':'");
						}
						c.Move();
					}

					tps.push_back(Type::parse(c));

					if (c.Tok() == RecognizedToken::Comma) c.Move(); else if (c.Tok() != RecognizedToken::CloseParenthesis) {
						ThrowWrongTokenError(c, "',' or ')'");
					}
				}

				if ((c.Tok() == RecognizedToken::Symbol && c.Data() == "ref") || c.Tok() == RecognizedToken::And) {
					fType.ref = true;
					c.Move();
				}

				fType.arguments = Contents::RegisterTypeArray(std::move(tps));
				rType = Contents::EmplaceType(fType);
			}
			else if (c.Tok() == RecognizedToken::OpenBracket) {
				ArrayType aType;
				aType.base = rType;

				c.Move();

				aType.size = c;
				Cursor c1 = c;
				c1.Move();
				if (c1.Tok() == RecognizedToken::CloseBracket) {
					aType.has_simple_size = true;
					if (c.Tok() == RecognizedToken::Number)
						aType.actual_size = (unsigned int)svtoi(c.Data());
					else if (c.Tok() == RecognizedToken::UnsignedNumber)
						aType.actual_size = (unsigned int)svtoi(c.Data().substr(0,c.Data().size()-1));
					else if (c.Tok() == RecognizedToken::LongNumber)
						aType.actual_size = (unsigned int)svtoi(c.Data().substr(0, c.Data().size() - 1));
					else if (c.Tok() == RecognizedToken::UnsignedLongNumber)
						aType.actual_size = (unsigned int)svtoi(c.Data().substr(0, c.Data().size() - 2));

					c = c1;
				}
				else {
					aType.has_simple_size = false;

					if (ctx.parent_namespace == nullptr && ctx.parent_struct == nullptr) {
						CompileContextExt ccext;
						Expression::parse(c, ccext, CompileType::ShortCircuit);
					}
					else {
						CompileContextExt ccext;
						ccext.basic = ctx;
						Cursor ce = c;
						CompileValue v = Expression::parse(c, ccext, CompileType::Eval);
						if (v.t == t_i8 || v.t == t_i16 || v.t == t_i32 || v.t == t_i64) {
							long long cv = LLVMConstIntGetSExtValue(v.v);
							if (cv <= 0) {
								ThrowSpecificError(ce, "Array cannot be created with negative or zero size");
							}
							aType.actual_size = (unsigned int)cv;
						}
						else if (v.t == t_u8 || v.t == t_u16 || v.t == t_u32 || v.t == t_u64) {
							unsigned long long cv = LLVMConstIntGetZExtValue(v.v);
							if (cv == 0) {
								ThrowSpecificError(ce, "Array cannot be created with zero size");
							}
							aType.actual_size = (unsigned int)cv;
						}
						else {
							ThrowSpecificError(ce, "Array type must have constant integer size");
						}
					}

					if (c.Tok() != RecognizedToken::CloseBracket) {
						ThrowWrongTokenError(c, "']'");
					}
				}
				c.Move();

				if ((c.Tok() == RecognizedToken::Symbol && c.Data() == "ref") || c.Tok() == RecognizedToken::And) {
					aType.ref = true;
					c.Move();
				}

				rType = Contents::EmplaceType(aType);
			}
			else break;
		}

		return rType;
	}
}