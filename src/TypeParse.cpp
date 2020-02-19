#include "Type.h"
#include "Error.h"
#include <iostream>
#include "Declaration.h"
#include "Contents.h"
#include "PredefinedTypes.h"
#include "svtoi.h"
#include "Expression.h"

namespace Corrosive {
	const Type* Type::Parse(Cursor& c, std::vector<Cursor>* argnames) {
		CompileContext cctx;
		cctx.parent_namespace = nullptr;
		cctx.parent_struct = nullptr;
		return ParseDirect(cctx, c, argnames);
	}

	const Type* Type::ParseDirect(CompileContext& ctx,Cursor& c, std::vector<Cursor>* argnames) {
		const Type* rType = nullptr;


		if (c.Tok() == RecognizedToken::Eof) {
			ThrowEofError(c, "an attempt to parse type");
		}


		if (c.Tok() == RecognizedToken::OpenParenthesis) {
			PrimitiveType pType;
			Cursor voidc;
			voidc.Data("void");

			pType.Name(voidc);
			pType.Pack(PredefinedNamespace);


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

					tps.push_back(Type::Parse(c));

					if (c.Tok() == RecognizedToken::Comma) c.Move(); else if (c.Tok() != RecognizedToken::CloseBracket) {
						ThrowWrongTokenError(c, "',' or ']'");
					}
				}

				if ((c.Tok() == RecognizedToken::Symbol && c.Data() == "ref") || c.Tok() == RecognizedToken::Star) {
					while (true) {
						if (c.Tok() == RecognizedToken::Star || c.Data() == "ref") {
							fType.Ref(fType.Ref() + 1);
							c.Move();
						}
						else break;
					}
				}

				fType.Types() = Contents::RegisterTypeArray(std::move(tps));
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

					tps.push_back(Type::Parse(c));

					if (c.Tok() == RecognizedToken::Comma) c.Move(); else if (c.Tok() != RecognizedToken::GreaterThan) {
						ThrowWrongTokenError(c, "',' or '>'");
					}
				}

				if ((c.Tok() == RecognizedToken::Symbol && c.Data() == "ref") || c.Tok() == RecognizedToken::Star) {
					while (true) {
						if (c.Tok() == RecognizedToken::Star || c.Data() == "ref") {
							fType.Ref(fType.Ref() + 1);
							c.Move();
						}
						else break;
					}
				}

				fType.Types() = Contents::RegisterTypeArray(std::move(tps));
				rType = Contents::EmplaceType(fType);
			}
			else {
				if (c.Tok() != RecognizedToken::Symbol) {
					ThrowNotANameError(c);
				}

				PrimitiveType pType;
				pType.Name(c);
				c.Move();

				if (c.Tok() == RecognizedToken::DoubleColon) {
					pType.Pack(pType.Name().Data());
					c.Move();
					pType.Name(c);
					c.Move();
				}

				if (c.Tok() == RecognizedToken::LessThan) {
					TemplateContext tps;

					c.Move();
					while (true) {
						if (c.Tok() == RecognizedToken::GreaterThan) {
							c.Move(); break;
						}

						tps.push_back(Type::Parse(c));

						if (c.Tok() == RecognizedToken::Comma) c.Move(); else if (c.Tok() != RecognizedToken::GreaterThan) {
							ThrowWrongTokenError(c, "',' or '>'");
						}
					}
					pType.Templates() = Contents::RegisterGenericArray(std::move(tps));
				}

				if ((c.Tok() == RecognizedToken::Symbol && c.Data() == "ref") || c.Tok() == RecognizedToken::Star) {
					while (true) {
						if (c.Tok() == RecognizedToken::Star || c.Data() == "ref") {
							pType.Ref(pType.Ref() + 1);
							c.Move();
						}
						else break;
					}
				}

				rType = Contents::EmplaceType(pType);
			}
		}


		while (true) {
			if (c.Tok() == RecognizedToken::OpenParenthesis) {
				FunctionType fType;
				std::vector<const Type*> tps;
				fType.Returns(rType);

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

					tps.push_back(Type::Parse(c));

					if (c.Tok() == RecognizedToken::Comma) c.Move(); else if (c.Tok() != RecognizedToken::CloseParenthesis) {
						ThrowWrongTokenError(c, "',' or ')'");
					}
				}

				if ((c.Tok() == RecognizedToken::Symbol && c.Data() == "ref") || c.Tok() == RecognizedToken::Star) {
					while (true) {
						if (c.Tok() == RecognizedToken::Star || c.Data() == "ref") {
							fType.Ref(fType.Ref() + 1);
							c.Move();
						}
						else break;
					}
				}

				fType.Args() = Contents::RegisterTypeArray(std::move(tps));
				rType = Contents::EmplaceType(fType);
			}
			else if (c.Tok() == RecognizedToken::OpenBracket) {
				ArrayType aType;
				aType.Base(rType);

				c.Move();

				aType.Size(c);
				Cursor c1 = c;
				c1.Move();
				if (c1.Tok() == RecognizedToken::CloseBracket) {
					aType.HasSimpleSize(true);
					if (c.Tok() == RecognizedToken::Number)
						aType.ActualSize((unsigned int)svtoi(c.Data()));
					else if (c.Tok() == RecognizedToken::UnsignedNumber)
						aType.ActualSize((unsigned int)svtoi(c.Data().substr(0,c.Data().size()-1)));
					else if (c.Tok() == RecognizedToken::LongNumber)
						aType.ActualSize((unsigned int)svtoi(c.Data().substr(0, c.Data().size() - 1)));
					else if (c.Tok() == RecognizedToken::UnsignedLongNumber)
						aType.ActualSize((unsigned int)svtoi(c.Data().substr(0, c.Data().size() - 2)));

					c = c1;
				}
				else {
					aType.HasSimpleSize(false);

					if (ctx.parent_namespace == nullptr && ctx.parent_struct == nullptr) {
						CompileContextExt ccext;
						Expression::Parse(c, ccext, CompileType::ShortCircuit);
					}
					else {
						CompileContextExt ccext;
						ccext.basic = ctx;
						Cursor ce = c;
						CompileValue v = Expression::Parse(c, ccext, CompileType::Eval);
						if (v.t == t_i8 || v.t == t_i16 || v.t == t_i32 || v.t == t_i64) {
							long long cv = LLVMConstIntGetSExtValue(v.v);
							if (cv <= 0) {
								ThrowSpecificError(ce, "Array cannot be created with negative or zero size");
							}
							aType.ActualSize((unsigned int)cv);
						}
						else if (v.t == t_u8 || v.t == t_u16 || v.t == t_u32 || v.t == t_u64) {
							unsigned long long cv = LLVMConstIntGetZExtValue(v.v);
							if (cv == 0) {
								ThrowSpecificError(ce, "Array cannot be created with zero size");
							}
							aType.ActualSize((unsigned int)cv);
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

				/*int lvl = 1;
				while (c.Tok() != RecognizedToken::Eof && lvl > 0) {
					if (c.Tok() == RecognizedToken::CloseBracket) {
						lvl -= 1;
					}
					else if (c.Tok() == RecognizedToken::OpenBracket) {
						lvl += 1;
					}
					c.Move();
				}*/

				if ((c.Tok() == RecognizedToken::Symbol && c.Data() == "ref") || c.Tok() == RecognizedToken::Star) {
					while (true) {
						if (c.Tok() == RecognizedToken::Star || c.Data() == "ref") {
							aType.Ref(aType.Ref() + 1);
							c.Move();
						}
						else break;
					}
				}

				rType = Contents::EmplaceType(aType);
			}
			else break;
		}

		return rType;
	}
}