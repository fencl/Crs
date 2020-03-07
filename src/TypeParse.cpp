#include "Type.h"
#include "Error.h"
#include <iostream>
#include "Declaration.h"
#include "Contents.h"
#include "PredefinedTypes.h"
#include "Utilities.h"
#include "Expression.h"

namespace Corrosive {
	bool Type::parse(Cursor& c,const Type*& into,std::vector<Cursor>* argnames) {
		CompileContext cctx;
		cctx.parent_namespace = nullptr;
		cctx.parent_struct = nullptr;
		return parse_direct(cctx, c,into, argnames);
	}

	bool Type::parse_direct(CompileContext& ctx,Cursor& c, const Type*& into, std::vector<Cursor>* argnames) {
		const Type* rType = nullptr;


		if (c.tok == RecognizedToken::Eof) {
			throw_eof_error(c, "an attempt to parse type");
			return false;
		}

		switch (c.tok) {
			case RecognizedToken::OpenParenthesis: {
					PrimitiveType pType;
					Cursor voidc;
					voidc.buffer = "void";
					pType.name = voidc;
					pType.package = PredefinedNamespace;
					rType = Contents::emplace_type(pType);
				} break;

			case RecognizedToken::OpenBracket: {
					TupleType fType;
					std::vector<const Type*> tps;
					c.move();

					while (true) {
						if (c.tok == RecognizedToken::CloseBracket) {
							c.move(); break;
						}

						const Type* res;
						if (!Type::parse(c, res)) return false;
						tps.push_back(res);

						if (c.tok == RecognizedToken::Comma) c.move(); else if (c.tok != RecognizedToken::CloseBracket) {
							throw_wrong_token_error(c, "',' or ']'");
							return false;
						}
					}

					if ((c.tok == RecognizedToken::Symbol && c.buffer == "ref") || c.tok == RecognizedToken::And) {
						fType.ref = true;
						c.move();
					}

					fType.types = Contents::register_type_array(std::move(tps));
					rType = Contents::emplace_type(fType);

				} break;

			case RecognizedToken::LessThan: {
					InterfaceType fType;
					std::vector<const Type*> tps;
					c.move();

					while (true) {
						if (c.tok == RecognizedToken::GreaterThan) {
							c.move(); break;
						}

						const Type* res;
						if (!Type::parse(c, res)) return false;
						tps.push_back(res);


						if (c.tok == RecognizedToken::Comma) c.move(); else if (c.tok != RecognizedToken::GreaterThan) {
							throw_wrong_token_error(c, "',' or '>'");
							return false;
						}
					}

					if ((c.tok == RecognizedToken::Symbol && c.buffer == "ref") || c.tok == RecognizedToken::And) {
						fType.ref = true;
						c.move();
					}

					fType.types = Contents::register_type_array(std::move(tps));
					rType = Contents::emplace_type(fType);
				} break;

			default: {
					if (c.tok != RecognizedToken::Symbol) {
						throw_not_a_name_error(c);
					}

					PrimitiveType pType;
					pType.name = c;
					c.move();

					if (c.tok == RecognizedToken::DoubleColon) {
						pType.package = pType.name.buffer;
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

							const Type* res;
							if (!Type::parse(c, res)) return false;
							tps.push_back(res);

							if (c.tok == RecognizedToken::Comma) c.move(); else if (c.tok != RecognizedToken::GreaterThan) {
								throw_wrong_token_error(c, "',' or '>'");

								return false;
							}
						}
						pType.templates = Contents::register_generic_array(std::move(tps));
					}


					if ((c.tok == RecognizedToken::Symbol && c.buffer == "ref") || c.tok == RecognizedToken::And) {
						pType.ref = true;
						c.move();
					}

					rType = Contents::emplace_type(pType);
				} break;
		}


		while (true) {
			switch (c.tok) {
				case RecognizedToken::OpenParenthesis: {
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
									return false;
								}
								c.move();
							}

							const Type* res;
							if (!Type::parse(c, res)) return false;
							tps.push_back(res);

							if (c.tok == RecognizedToken::Comma) c.move(); else if (c.tok != RecognizedToken::CloseParenthesis) {
								throw_wrong_token_error(c, "',' or ')'");
								return false;
							}
						}

						if ((c.tok == RecognizedToken::Symbol && c.buffer == "ref") || c.tok == RecognizedToken::And) {
							fType.ref = true;
							c.move();
						}

						fType.arguments = Contents::register_type_array(std::move(tps));
						rType = Contents::emplace_type(fType);
					}break;
				case RecognizedToken::OpenBracket: {
						ArrayType aType;
						aType.base = rType;

						c.move();

						Cursor c1 = c;
						c1.move();
						if (c1.tok == RecognizedToken::CloseBracket) {
							switch (c.tok) {
								case RecognizedToken::Number: aType.size = (unsigned int)svtoi(c.buffer); break;
								case RecognizedToken::UnsignedNumber: aType.size = (unsigned int)svtoi(c.buffer.substr(0, c.buffer.size() - 1)); break;
								case RecognizedToken::LongNumber: aType.size = (unsigned int)svtoi(c.buffer.substr(0, c.buffer.size() - 1)); break;
								case RecognizedToken::UnsignedLongNumber: aType.size = (unsigned int)svtoi(c.buffer.substr(0, c.buffer.size() - 2)); break;
							}

							c = c1;
						}
						else {
							throw_wrong_token_error(c, "']'");
							return false;
						}
						c.move();

						if ((c.tok == RecognizedToken::Symbol && c.buffer == "ref") || c.tok == RecognizedToken::And) {
							aType.ref = true;
							c.move();
						}

						rType = Contents::emplace_type(aType);
					} break;
				default: goto while_break;
			}

			continue;
		while_break:
			break;
		}

		into = rType;
		return true;
	}
}