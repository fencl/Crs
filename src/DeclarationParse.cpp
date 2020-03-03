#include "Declaration.h"
#include "Error.h"
#include <iostream>
#include "Contents.h"
#include <string>
#include "PredefinedTypes.h"


namespace Corrosive {
	void Declaration::parse(Cursor& c, std::vector<std::unique_ptr<Declaration>>& into, Declaration* parent, NamespaceDeclaration* pack) {
		if (c.Data() == "var") {
			if (dynamic_cast<StructDeclaration*>(parent) == nullptr) {
				ThrowSpecificError(c, "Variable has to be a member of struct or class");
			}

			c.Move();

			std::vector<Cursor> names;
			while (true) {
				if (c.Tok() != RecognizedToken::Symbol) {
					ThrowNotANameError(c);
				}
				names.push_back(c);
				c.Move();

				if (c.Tok() != RecognizedToken::Comma) {
					break;
				}
				else c.Move();
			}


			if (c.Tok() != RecognizedToken::Colon) {
				ThrowWrongTokenError(c, "':'");
			}
			c.Move();

			const Corrosive::Type* tp = Type::Parse(c);

			if (c.Tok() != RecognizedToken::Semicolon) {
				ThrowWrongTokenError(c, "';'");
			}
			c.Move();

			for (int i = 0; i < names.size(); i++) {
				std::unique_ptr<VariableDeclaration> vd = std::make_unique<VariableDeclaration>();
				vd->name = names[i];
				vd->type = tp;
				if (parent != nullptr) {
					vd->package = parent->package;
					vd->parent = parent;
				}

				vd->parent_pack = pack;

				into.push_back(std::move(vd));
			}
		}
		else if (c.Data() == "type") {
			if (dynamic_cast<StructDeclaration*>(parent) != nullptr) {
				ThrowSpecificError(c, "Type cannot be a member of struct or class");
			}

			c.Move();

			std::vector<Cursor> names;
			while (true) {
				if (c.Tok() != RecognizedToken::Symbol) {
					ThrowNotANameError(c);
				}
				names.push_back(c);
				c.Move();

				if (c.Tok() != RecognizedToken::Comma) {
					break;
				}
				else c.Move();
			}

			if (c.Tok() != RecognizedToken::Colon) {
				ThrowWrongTokenError(c, "':'");
			}
			c.Move();

			const Corrosive::Type* tp = Type::Parse(c);

			if (c.Tok() != RecognizedToken::Semicolon) {
				ThrowWrongTokenError(c, "';'");
			}
			c.Move();

			for (int i = 0; i < names.size(); i++) {

				std::unique_ptr<TypedefDeclaration> vd = std::make_unique<TypedefDeclaration>();
				vd->name = names[i];
				vd->Type(tp);


				if (parent != nullptr) {
					vd->package = parent->package;
					vd->parent = parent;
				}
				vd->parent_pack = pack;

				if (Contents::FindTypedef(vd->package, vd->name.Data()) != nullptr) {
					ThrowSpecificError(vd->name, "Typedef with the same name already exist's it this package");
				}
				else {
					Contents::RegisterTypedef(vd->package, vd->name.Data(), vd.get());
				}

				into.push_back(std::move(vd));
			}
		}
		else if (c.Data() == "function") {
			bool iss = false;
			c.Move();

			if (c.Tok() == RecognizedToken::Symbol && c.Data() == "static") {
				iss = true;
				c.Move();
			}

			if (c.Tok() != RecognizedToken::Symbol) {
				ThrowNotANameError(c);
			}

			Cursor name = c;
			std::vector<std::string_view> gen_names;
			c.Move();

			if (c.Tok() == RecognizedToken::LessThan) {
				c.Move();
				while (true) {
					if (c.Tok() != RecognizedToken::Symbol) {
						ThrowNotANameError(c);
					}

					gen_names.push_back(c.Data());
					c.Move();
					if (c.Tok() == RecognizedToken::Comma) {
						c.Move();
					}
					else break;
				}

				if (c.Tok() != RecognizedToken::GreaterThan) {
					ThrowWrongTokenError(c, "',' or '>'");
				}
				c.Move();
			}

			if (c.Tok() != RecognizedToken::Colon) {
				ThrowWrongTokenError(c, "':'");
			}
			c.Move();

			std::unique_ptr<FunctionDeclaration> fd;
			if (gen_names.size() > 0) {
				std::unique_ptr<GenericFunctionDeclaration> gfd = std::make_unique<GenericFunctionDeclaration>();

				for (auto&& it : gen_names) {
					unsigned int gs = (unsigned int)gfd->Generics().size();
					gfd->Generics()[it] = gs;
				}

				fd = std::move(gfd);
			}
			else
				fd = std::make_unique<FunctionDeclaration>();

			const Corrosive::Type* tp = Type::Parse(c, fd->Argnames());

			fd->name = name;
			fd->Type(tp);
			fd->Static(iss);
			if (name.Data() == "main") {
				Contents::entry_point = fd.get();
			}

			if (parent != nullptr) {
				fd->package = parent->package;
				fd->parent = parent;
			}

			fd->parent_pack = pack;

			if (c.Tok() == RecognizedToken::OpenBrace) {
				fd->HasBlock(true);
				c.Move();
				fd->Block(c);

				int level = 1;
				while (true) {
					if (c.Tok() == RecognizedToken::CloseBrace) {
						level -= 1;
						if (level == 0) { c.Move();  break; }
					}
					else if (c.Tok() == RecognizedToken::OpenBrace) {
						level += 1;
					}
					else if (c.Tok() == RecognizedToken::Eof) {
						break;
					}

					c.Move();
				}
			}
			else if (c.Tok() == RecognizedToken::Semicolon) {
				StructDeclaration* ps = nullptr;
				if (parent == nullptr || ((ps = dynamic_cast<StructDeclaration*>(parent)) != nullptr && !ps->is_trait)) {
					ThrowSpecificError(name, "Global functions and functions inside structures must have body");
				}
				c.Move();
			}
			else {
				ThrowWrongTokenError(c, "'{' or ';'");
			}

			into.push_back(std::move(fd));
		}
		else if (c.Data() == "struct" || c.Data() == "trait") {
			bool is_trait = c.Data() == "trait";
			bool isext = false;
			std::string_view overpack = "";

			if (dynamic_cast<StructDeclaration*>(parent) != nullptr) {
				ThrowSpecificError(c, "Structures cannot be nested");
			}

			c.Move();
			if (c.Tok() == RecognizedToken::Symbol && c.Data() == "extension") {
				isext = true;
				c.Move();
			}

			if (c.Tok() != RecognizedToken::Symbol) {
				ThrowNotANameError(c);
			}
			Cursor name = c;
			std::vector<std::string_view> gen_names;
			c.Move();
			if (c.Tok() == RecognizedToken::DoubleColon) {
				overpack = name.Data();
				c.Move();
				name = c;
				c.Move();


				if (!isext)
					ThrowSpecificError(name, "Only struct/class extensions can have cross-package identificator");
				else
					Contents::RegisterNamespace(overpack);
			}

			if (c.Tok() == RecognizedToken::LessThan) {
				c.Move();
				while (true) {
					if (c.Tok() != RecognizedToken::Symbol) {
						ThrowNotANameError(c);
					}

					gen_names.push_back(c.Data());
					c.Move();
					if (c.Tok() == RecognizedToken::Comma) {
						c.Move();
					}
					else break;
				}

				if (c.Tok() != RecognizedToken::GreaterThan) {
					ThrowWrongTokenError(c, "',' or '>'");
				}
				c.Move();
			}

			std::string_view pkg = overpack;

			if (parent != nullptr && pkg == "") {
				pkg = parent->package;
			}

			std::unique_ptr<StructDeclaration> sd;
			if (gen_names.size() == 0) {
				sd = std::make_unique<StructDeclaration>();
			}
			else
			{
				std::unique_ptr<GenericStructDeclaration> gsd = std::make_unique<GenericStructDeclaration>();
				for (auto&& it : gen_names) {
					unsigned int gs = (unsigned int)gsd->Generics().size();
					gsd->Generics()[it] = gs;
				}
				sd = std::move(gsd);
			}

			sd->name = name;
			sd->is_trait = is_trait;
			sd->is_extending = isext;
			sd->package = pkg;

			if (parent != nullptr) {
				sd->parent = parent;
			}

			sd->parent_pack = pack;



			if (auto existing = Contents::FindStruct(pkg, name.Data())) {
				if (!isext && !existing->is_extending) {
					ThrowSpecificError(name, "There already exist's class/structure with the same name");
				}


				if (isext) {
					if (is_trait && !existing->is_trait)
						ThrowSpecificError(name, "Cannot extend structure with class");
					if (!is_trait && existing->is_trait)
						ThrowSpecificError(name, "Cannot extend class with structure");
				}
				else {
					if (is_trait && !existing->is_trait)
						ThrowSpecificError(existing->name, "Cannot extend class with structure");
					if (!is_trait && existing->is_trait)
						ThrowSpecificError(existing->name, "Cannot extend structure with class");
				}

				if (gen_names.size() == 0) {
					if (existing->is_generic())
						ThrowSpecificError(isext ? name : existing->name, "Cannot extend generic struct/class with non-generic struct/class");
				}
				else {
					GenericStructDeclaration* gsd = (GenericStructDeclaration*)existing;
					bool gen_nm_ok = true;
					if (gsd->Generics().size() != gen_names.size()) {
						gen_nm_ok = false;
					}

					if (!gen_nm_ok)
						ThrowSpecificError(name, "Generic typenames do not match extended structure/class");
				}



				if (c.Tok() == RecognizedToken::Colon) {
					if (is_trait) {
						ThrowSpecificError(c, "Classes cannot implement other classes");
					}

					c.Move();
					while (true) {
						existing->implements.push_back(std::make_pair(sd.get(), Type::Parse(c)));

						if (c.Tok() == RecognizedToken::Comma) {
							c.Move();
						}
						else break;
					}
				}

				if (c.Tok() != RecognizedToken::OpenBrace) {
					ThrowWrongTokenError(c, "'{'");
				}
				c.Move();
				while (true) {
					if (c.Tok() == RecognizedToken::CloseBrace) {
						c.Move();
						break;
					}
					else if (c.Tok() == RecognizedToken::Eof) {
						break;
					}
					else if (c.Tok() == RecognizedToken::Symbol && c.Data() == "alias") {
						c.Move();
						bool specific = false;
						Cursor alias_from = c;
						c.Move();

						if (c.Tok() == RecognizedToken::DoubleColon) {
							c.Move();

							while (true) {
								if (c.Tok() != RecognizedToken::Symbol) {
									ThrowNotANameError(c);
								}
								specific = true;
								existing->aliases.push_back(std::make_pair(c, alias_from));
								c.Move();

								if (c.Tok() == RecognizedToken::Semicolon) {
									break;
								}
								else if (c.Tok() != RecognizedToken::Comma) {
									ThrowWrongTokenError(c, "',' or ';'");
								}
								else {
									c.Move();
								}
							}
						}

						if (c.Tok() != RecognizedToken::Semicolon) {
							ThrowWrongTokenError(c, "';'");
						}
						c.Move();

						if (!specific) {
							Cursor empty;
							existing->aliases.push_back(std::make_pair(empty, alias_from));
						}
					}
					else {
						Declaration::parse(c, existing->members, sd.get(), pack);
						if (auto varmember = dynamic_cast<VariableDeclaration*>(existing->members.back().get())) {
							if (existing->decl_type!= StructDeclarationType::Declared)
								ThrowSpecificError(varmember->name, "Cannot add new members into this structure");
						}
					}
				}
			}
			else {

				// from here is normal, non-extending declaration

				if (c.Tok() == RecognizedToken::Colon) {
					if (is_trait) {
						ThrowSpecificError(c, "Classes cannot implement other classes");
					}

					c.Move();
					while (true) {
						sd->implements.push_back(std::make_pair(sd.get(), Type::Parse(c)));

						if (c.Tok() == RecognizedToken::Comma) {
							c.Move();
						}
						else break;
					}
				}

				Contents::RegisterStruct(sd->package, sd->name.Data(), sd.get());

				if (c.Tok() != RecognizedToken::OpenBrace) {
					ThrowWrongTokenError(c, "'{'");
				}
				c.Move();

				while (true) {
					if (c.Tok() == RecognizedToken::CloseBrace) {
						c.Move();
						break;
					}
					else if (c.Tok() == RecognizedToken::Eof) {
						break;
					}
					else if (c.Tok() == RecognizedToken::Symbol && c.Data() == "alias") {
						c.Move();
						bool specific = false;
						Cursor alias_from = c;
						c.Move();

						if (c.Tok() == RecognizedToken::DoubleColon) {
							c.Move();

							while (true) {
								if (c.Tok() != RecognizedToken::Symbol) {
									ThrowNotANameError(c);
								}
								specific = true;
								sd->aliases.push_back(std::make_pair(c, alias_from));
								c.Move();

								if (c.Tok() == RecognizedToken::Semicolon) {
									break;
								}
								else if (c.Tok() != RecognizedToken::Comma) {
									ThrowWrongTokenError(c, "',' or ';'");
								}
								else {
									c.Move();
								}
							}
						}

						if (c.Tok() != RecognizedToken::Semicolon) {
							ThrowWrongTokenError(c, "';'");
						}
						c.Move();

						if (!specific) {
							Cursor empty;
							sd->aliases.push_back(std::make_pair(empty,alias_from));
						}

					} else {
						Declaration::parse(c, sd->members, sd.get(), pack);
					}
				}
			}


			into.push_back(std::move(sd));
		}
		else if (c.Data() == "package") {
			if (dynamic_cast<NamespaceDeclaration*>(parent) != nullptr) {
				ThrowSpecificError(c, "Packages cannot be nested");
			}
			c.Move();
			if (c.Tok() != RecognizedToken::Symbol) {
				ThrowNotANameError(c);
			}
			Cursor name = c;
			c.Move();
			Contents::RegisterNamespace(name.Data());


			std::unique_ptr<NamespaceDeclaration> nd = std::make_unique<NamespaceDeclaration>();
			if (c.Tok() == RecognizedToken::Colon) {
				c.Move();
				while (true) {
					nd->queue.push_back(c.Data());
					c.Move();
					if (c.Tok() == RecognizedToken::Comma) {
						c.Move();
					}
					else break;
				}
			}

			nd->package = name.Data();
			nd->name = name;
			nd->parent_pack = nd.get();

			if (c.Tok() != RecognizedToken::OpenBrace) {
				ThrowWrongTokenError(c, "'{'");
			}
			c.Move();

			while (true) {
				if (c.Tok() == RecognizedToken::CloseBrace) {
					c.Move();
					break;
				}
				else if (c.Tok() == RecognizedToken::Eof) {
					break;
				}
				Declaration::parse(c, nd->members, nd.get(), nd.get());
			}

			into.push_back(std::move(nd));
		}
		else {
			ThrowWrongTokenError(c, "'namespace', 'struct', 'class', 'function' or 'var'");
		}
	}

}