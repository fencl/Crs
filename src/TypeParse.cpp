#include "Type.h"
#include "Error.h"
#include <iostream>
#include "Declaration.h"
#include "Contents.h"
#include "PredefinedTypes.h"
#include "svtoi.h"

namespace Corrosive {
	const Type* Type::Parse(Cursor& c, std::vector<Cursor>* argnames) {
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
					std::vector<std::variant<unsigned int, const Type*>> tps;

					c.Move();
					while (true) {
						if (c.Tok() == RecognizedToken::GreaterThan) {
							c.Move(); break;
						}

						if (c.Tok() == RecognizedToken::Number) {
							tps.push_back((unsigned int)svtoi(c.Data()));
							c.Move();
						}
						else
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

				if (c.Tok() == RecognizedToken::Number || c.Tok() == RecognizedToken::Symbol) {
					aType.Size(c);
					c.Move();
				}

				if (c.Tok() != RecognizedToken::CloseBracket) {
					ThrowWrongTokenError(c, "']'");
				}
				c.Move();

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