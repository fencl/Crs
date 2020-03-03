#include "Type.h"
#include "Error.h"
#include <iostream>
#include "Declaration.h"
#include "Contents.h"
#include "PredefinedTypes.h"
#include "Utilities.h"
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


		if (c.tok == RecognizedToken::Eof) {
			throw_eof_error(c, "an attempt to parse type");
		}


		if (c.tok == RecognizedToken::OpenParenthesis) {
			PrimitiveType pType;
			Cursor voidc;
			voidc.data = "void";

			pType.name = voidc;
			pType.package = PredefinedNamespace;


			rType = Contents::EmplaceType(pType);
		}
		else {
			if (c.tok == RecognizedToken::OpenBracket) {
				TupleType fType;
				std::vector<const Type*> tps;
				c.move();

				while (true) {
					if (c.tok == RecognizedToken::CloseBracket) {
						c.move(); break;
					}

					tps.push_back(Type::parse(c));

					if (c.tok == RecognizedToken::Comma) c.move(); else if (c.tok != RecognizedToken::CloseBracket) {
						throw_wrong_token_error(c, "',' or ']'");
					}
				}
				
				if ((c.tok == RecognizedToken::Symbol && c.data == "ref") || c.tok == RecognizedToken::And) {
					fType.ref = true;
					c.move();
				}

				fType.types = Contents::register_type_array(std::move(tps));
				rType = Contents::EmplaceType(fType);

			}
			else if (c.tok == RecognizedToken::LessThan) {
				InterfaceType fType;
				std::vector<const Type*> tps;
				c.move();

				while (true) {
					if (c.tok == RecognizedToken::GreaterThan) {
						c.move(); break;
					}

					tps.push_back(Type::parse(c));

					if (c.tok == RecognizedToken::Comma) c.move(); else if (c.tok != RecognizedToken::GreaterThan) {
						throw_wrong_token_error(c, "',' or '>'");
					}
				}
				
				if ((c.tok == RecognizedToken::Symbol && c.data == "ref") || c.tok == RecognizedToken::And) {
					fType.ref = true;
					c.move();
				}

				fType.types = Contents::register_type_array(std::move(tps));
				rType = Contents::EmplaceType(fType);
			}
			else {
				if (c.tok != RecognizedToken::Symbol) {
					throw_not_a_name_error(c);
				}

				PrimitiveType pType;
				pType.name = c;
				c.move();

				if (c.tok == RecognizedToken::DoubleColon) {
					pType.package = pType.name.data;
					c.move();
					pType.name = c;
					c.move();
				}

				if (c.tok == RecognizedToken::LessThan) {
					TemplateContext tps;

					c.move();
					while (true) {
						if (c.tok == RecognizedToken::GreaterThan) {
							c.move(); break;
						}

						tps.push_back(Type::parse(c));

						if (c.tok == RecognizedToken::Comma) c.move(); else if (c.tok != RecognizedToken::GreaterThan) {
							throw_wrong_token_error(c, "',' or '>'");
						}
					}
					pType.templates = Contents::register_generic_array(std::move(tps));
				}


				if ((c.tok == RecognizedToken::Symbol && c.data == "ref") || c.tok == RecognizedToken::And) {
					pType.ref = true;
					c.move();
				}

				rType = Contents::EmplaceType(pType);
			}
		}


		while (true) {
			if (c.tok == RecognizedToken::OpenParenthesis) {
				FunctionType fType;
				std::vector<const Type*> tps;
				fType.returns = rType;

				c.move();
				while (true) {
					if (c.tok == RecognizedToken::CloseParenthesis) {
						c.move(); break;
					}

					if (argnames != nullptr)
					{
						argnames->push_back(c);
						c.move();
						if (c.tok != RecognizedToken::Colon) {
							throw_wrong_token_error(c, "':'");
						}
						c.move();
					}

					tps.push_back(Type::parse(c));

					if (c.tok == RecognizedToken::Comma) c.move(); else if (c.tok != RecognizedToken::CloseParenthesis) {
						throw_wrong_token_error(c, "',' or ')'");
					}
				}

				if ((c.tok == RecognizedToken::Symbol && c.data == "ref") || c.tok == RecognizedToken::And) {
					fType.ref = true;
					c.move();
				}

				fType.arguments = Contents::register_type_array(std::move(tps));
				rType = Contents::EmplaceType(fType);
			}
			else if (c.tok == RecognizedToken::OpenBracket) {
				ArrayType aType;
				aType.base = rType;

				c.move();

				aType.size = c;
				Cursor c1 = c;
				c1.move();
				if (c1.tok == RecognizedToken::CloseBracket) {
					aType.has_simple_size = true;
					if (c.tok == RecognizedToken::Number)
						aType.actual_size = (unsigned int)svtoi(c.data);
					else if (c.tok == RecognizedToken::UnsignedNumber)
						aType.actual_size = (unsigned int)svtoi(c.data.substr(0,c.data.size()-1));
					else if (c.tok == RecognizedToken::LongNumber)
						aType.actual_size = (unsigned int)svtoi(c.data.substr(0, c.data.size() - 1));
					else if (c.tok == RecognizedToken::UnsignedLongNumber)
						aType.actual_size = (unsigned int)svtoi(c.data.substr(0, c.data.size() - 2));

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
								throw_specific_error(ce, "Array cannot be created with negative or zero size");
							}
							aType.actual_size = (unsigned int)cv;
						}
						else if (v.t == t_u8 || v.t == t_u16 || v.t == t_u32 || v.t == t_u64) {
							unsigned long long cv = LLVMConstIntGetZExtValue(v.v);
							if (cv == 0) {
								throw_specific_error(ce, "Array cannot be created with zero size");
							}
							aType.actual_size = (unsigned int)cv;
						}
						else {
							throw_specific_error(ce, "Array type must have constant integer size");
						}
					}

					if (c.tok != RecognizedToken::CloseBracket) {
						throw_wrong_token_error(c, "']'");
					}
				}
				c.move();

				if ((c.tok == RecognizedToken::Symbol && c.data == "ref") || c.tok == RecognizedToken::And) {
					aType.ref = true;
					c.move();
				}

				rType = Contents::EmplaceType(aType);
			}
			else break;
		}

		return rType;
	}
}