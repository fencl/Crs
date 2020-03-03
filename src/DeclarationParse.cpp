#include "Declaration.h"
#include "Error.h"
#include <iostream>
#include "Contents.h"
#include <string>
#include "PredefinedTypes.h"


namespace Corrosive {
	void Declaration::parse(Cursor& c, std::vector<std::unique_ptr<Declaration>>& into, Declaration* parent, NamespaceDeclaration* pack) {
		if (c.buffer == "var") {
			if (dynamic_cast<StructDeclaration*>(parent) == nullptr) {
				throw_specific_error(c, "Variable has to be a member of struct or class");
			}

			c.move();

			std::vector<Cursor> names;
			while (true) {
				if (c.tok != RecognizedToken::Symbol) {
					throw_not_a_name_error(c);
				}
				names.push_back(c);
				c.move();

				if (c.tok != RecognizedToken::Comma) {
					break;
				}
				else c.move();
			}


			if (c.tok != RecognizedToken::Colon) {
				throw_wrong_token_error(c, "':'");
			}
			c.move();

			const Corrosive::Type* tp = Type::parse(c);

			if (c.tok != RecognizedToken::Semicolon) {
				throw_wrong_token_error(c, "';'");
			}
			c.move();

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
		else if (c.buffer == "type") {
			if (dynamic_cast<StructDeclaration*>(parent) != nullptr) {
				throw_specific_error(c, "Type cannot be a member of struct or class");
			}

			c.move();

			std::vector<Cursor> names;
			while (true) {
				if (c.tok != RecognizedToken::Symbol) {
					throw_not_a_name_error(c);
				}
				names.push_back(c);
				c.move();

				if (c.tok != RecognizedToken::Comma) {
					break;
				}
				else c.move();
			}

			if (c.tok != RecognizedToken::Colon) {
				throw_wrong_token_error(c, "':'");
			}
			c.move();

			const Corrosive::Type* tp = Type::parse(c);

			if (c.tok != RecognizedToken::Semicolon) {
				throw_wrong_token_error(c, "';'");
			}
			c.move();

			for (int i = 0; i < names.size(); i++) {

				std::unique_ptr<TypedefDeclaration> vd = std::make_unique<TypedefDeclaration>();
				vd->name = names[i];
				vd->type = tp;


				if (parent != nullptr) {
					vd->package = parent->package;
					vd->parent = parent;
				}
				vd->parent_pack = pack;

				if (Contents::find_typedef(vd->package, vd->name.buffer) != nullptr) {
					throw_specific_error(vd->name, "Typedef with the same name already exist's it this package");
				}
				else {
					Contents::register_typedef(vd->package, vd->name.buffer, vd.get());
				}

				into.push_back(std::move(vd));
			}
		}
		else if (c.buffer == "function") {
			bool is_static = false;
			c.move();

			if (c.tok == RecognizedToken::Symbol && c.buffer == "static") {
				is_static = true;
				c.move();
			}

			if (c.tok != RecognizedToken::Symbol) {
				throw_not_a_name_error(c);
			}

			Cursor name = c;
			std::vector<std::string_view> gen_names;
			c.move();

			if (c.tok == RecognizedToken::LessThan) {
				c.move();
				while (true) {
					if (c.tok != RecognizedToken::Symbol) {
						throw_not_a_name_error(c);
					}

					gen_names.push_back(c.buffer);
					c.move();
					if (c.tok == RecognizedToken::Comma) {
						c.move();
					}
					else break;
				}

				if (c.tok != RecognizedToken::GreaterThan) {
					throw_wrong_token_error(c, "',' or '>'");
				}
				c.move();
			}

			if (c.tok != RecognizedToken::Colon) {
				throw_wrong_token_error(c, "':'");
			}
			c.move();

			std::unique_ptr<FunctionDeclaration> fd;
			if (gen_names.size() > 0) {
				std::unique_ptr<GenericFunctionDeclaration> gfd = std::make_unique<GenericFunctionDeclaration>();

				for (auto&& it : gen_names) {
					unsigned int gs = (unsigned int)gfd->generic_typenames.size();
					gfd->generic_typenames[it] = gs;
				}

				fd = std::move(gfd);
			}
			else
				fd = std::make_unique<FunctionDeclaration>();

			const Corrosive::Type* type = Type::parse(c, &fd->argnames);

			fd->name = name;
			fd->type = type;
			fd->is_static = is_static;
			if (name.buffer == "main") {
				Contents::entry_point = fd.get();
			}

			if (parent != nullptr) {
				fd->package = parent->package;
				fd->parent = parent;
			}

			fd->parent_pack = pack;

			if (c.tok == RecognizedToken::OpenBrace) {
				fd->has_block = true;
				c.move();
				fd->block = c;

				int level = 1;
				while (true) {
					if (c.tok == RecognizedToken::CloseBrace) {
						level -= 1;
						if (level == 0) { c.move();  break; }
					}
					else if (c.tok == RecognizedToken::OpenBrace) {
						level += 1;
					}
					else if (c.tok == RecognizedToken::Eof) {
						break;
					}

					c.move();
				}
			}
			else if (c.tok == RecognizedToken::Semicolon) {
				StructDeclaration* ps = nullptr;
				if (parent == nullptr || ((ps = dynamic_cast<StructDeclaration*>(parent)) != nullptr && !ps->is_trait)) {
					throw_specific_error(name, "Global functions and functions inside structures must have body");
				}
				c.move();
			}
			else {
				throw_wrong_token_error(c, "'{' or ';'");
			}

			into.push_back(std::move(fd));
		}
		else if (c.buffer == "struct" || c.buffer == "trait") {
			bool is_trait = c.buffer == "trait";
			bool isext = false;
			std::string_view overpack = "";

			if (dynamic_cast<StructDeclaration*>(parent) != nullptr) {
				throw_specific_error(c, "Structures cannot be nested");
			}

			c.move();
			if (c.tok == RecognizedToken::Symbol && c.buffer == "extension") {
				isext = true;
				c.move();
			}

			if (c.tok != RecognizedToken::Symbol) {
				throw_not_a_name_error(c);
			}
			Cursor name = c;
			std::vector<std::string_view> gen_names;
			c.move();
			if (c.tok == RecognizedToken::DoubleColon) {
				overpack = name.buffer;
				c.move();
				name = c;
				c.move();


				if (!isext)
					throw_specific_error(name, "Only struct/class extensions can have cross-package identificator");
				else
					Contents::register_namespace(overpack);
			}

			if (c.tok == RecognizedToken::LessThan) {
				c.move();
				while (true) {
					if (c.tok != RecognizedToken::Symbol) {
						throw_not_a_name_error(c);
					}

					gen_names.push_back(c.buffer);
					c.move();
					if (c.tok == RecognizedToken::Comma) {
						c.move();
					}
					else break;
				}

				if (c.tok != RecognizedToken::GreaterThan) {
					throw_wrong_token_error(c, "',' or '>'");
				}
				c.move();
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
					unsigned int gs = (unsigned int)gsd->generic_typenames.size();
					gsd->generic_typenames[it] = gs;
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



			if (auto existing = Contents::find_struct(pkg, name.buffer)) {
				if (!isext && !existing->is_extending) {
					throw_specific_error(name, "There already exist's class/structure with the same name");
				}


				if (isext) {
					if (is_trait && !existing->is_trait)
						throw_specific_error(name, "Cannot extend structure with class");
					if (!is_trait && existing->is_trait)
						throw_specific_error(name, "Cannot extend class with structure");
				}
				else {
					if (is_trait && !existing->is_trait)
						throw_specific_error(existing->name, "Cannot extend class with structure");
					if (!is_trait && existing->is_trait)
						throw_specific_error(existing->name, "Cannot extend structure with class");
				}

				if (gen_names.size() == 0) {
					if (existing->is_generic())
						throw_specific_error(isext ? name : existing->name, "Cannot extend generic struct/class with non-generic struct/class");
				}
				else {
					GenericStructDeclaration* gsd = (GenericStructDeclaration*)existing;
					bool gen_nm_ok = true;
					if (gsd->generic_typenames.size() != gen_names.size()) {
						gen_nm_ok = false;
					}

					if (!gen_nm_ok)
						throw_specific_error(name, "Generic typenames do not match extended structure/class");
				}



				if (c.tok == RecognizedToken::Colon) {
					if (is_trait) {
						throw_specific_error(c, "Classes cannot implement other classes");
					}

					c.move();
					while (true) {
						existing->implements.push_back(std::make_pair(sd.get(), Type::parse(c)));

						if (c.tok == RecognizedToken::Comma) {
							c.move();
						}
						else break;
					}
				}

				if (c.tok != RecognizedToken::OpenBrace) {
					throw_wrong_token_error(c, "'{'");
				}
				c.move();
				while (true) {
					if (c.tok == RecognizedToken::CloseBrace) {
						c.move();
						break;
					}
					else if (c.tok == RecognizedToken::Eof) {
						break;
					}
					else if (c.tok == RecognizedToken::Symbol && c.buffer == "alias") {
						c.move();
						bool specific = false;
						Cursor alias_from = c;
						c.move();

						if (c.tok == RecognizedToken::DoubleColon) {
							c.move();

							while (true) {
								if (c.tok != RecognizedToken::Symbol) {
									throw_not_a_name_error(c);
								}
								specific = true;
								existing->aliases.push_back(std::make_pair(c, alias_from));
								c.move();

								if (c.tok == RecognizedToken::Semicolon) {
									break;
								}
								else if (c.tok != RecognizedToken::Comma) {
									throw_wrong_token_error(c, "',' or ';'");
								}
								else {
									c.move();
								}
							}
						}

						if (c.tok != RecognizedToken::Semicolon) {
							throw_wrong_token_error(c, "';'");
						}
						c.move();

						if (!specific) {
							Cursor empty;
							existing->aliases.push_back(std::make_pair(empty, alias_from));
						}
					}
					else {
						Declaration::parse(c, existing->members, sd.get(), pack);
						if (auto varmember = dynamic_cast<VariableDeclaration*>(existing->members.back().get())) {
							if (existing->decl_type!= StructDeclarationType::Declared)
								throw_specific_error(varmember->name, "Cannot add new members into this structure");
						}
					}
				}
			}
			else {

				// from here is normal, non-extending declaration

				if (c.tok == RecognizedToken::Colon) {
					if (is_trait) {
						throw_specific_error(c, "Classes cannot implement other classes");
					}

					c.move();
					while (true) {
						sd->implements.push_back(std::make_pair(sd.get(), Type::parse(c)));

						if (c.tok == RecognizedToken::Comma) {
							c.move();
						}
						else break;
					}
				}

				Contents::register_struct(sd->package, sd->name.buffer, sd.get());

				if (c.tok != RecognizedToken::OpenBrace) {
					throw_wrong_token_error(c, "'{'");
				}
				c.move();

				while (true) {
					if (c.tok == RecognizedToken::CloseBrace) {
						c.move();
						break;
					}
					else if (c.tok == RecognizedToken::Eof) {
						break;
					}
					else if (c.tok == RecognizedToken::Symbol && c.buffer == "alias") {
						c.move();
						bool specific = false;
						Cursor alias_from = c;
						c.move();

						if (c.tok == RecognizedToken::DoubleColon) {
							c.move();

							while (true) {
								if (c.tok != RecognizedToken::Symbol) {
									throw_not_a_name_error(c);
								}
								specific = true;
								sd->aliases.push_back(std::make_pair(c, alias_from));
								c.move();

								if (c.tok == RecognizedToken::Semicolon) {
									break;
								}
								else if (c.tok != RecognizedToken::Comma) {
									throw_wrong_token_error(c, "',' or ';'");
								}
								else {
									c.move();
								}
							}
						}

						if (c.tok != RecognizedToken::Semicolon) {
							throw_wrong_token_error(c, "';'");
						}
						c.move();

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
		else if (c.buffer == "package") {
			if (dynamic_cast<NamespaceDeclaration*>(parent) != nullptr) {
				throw_specific_error(c, "Packages cannot be nested");
			}
			c.move();
			if (c.tok != RecognizedToken::Symbol) {
				throw_not_a_name_error(c);
			}
			Cursor name = c;
			c.move();
			Contents::register_namespace(name.buffer);


			std::unique_ptr<NamespaceDeclaration> nd = std::make_unique<NamespaceDeclaration>();
			if (c.tok == RecognizedToken::Colon) {
				c.move();
				while (true) {
					nd->queue.push_back(c.buffer);
					c.move();
					if (c.tok == RecognizedToken::Comma) {
						c.move();
					}
					else break;
				}
			}

			nd->package = name.buffer;
			nd->name = name;
			nd->parent_pack = nd.get();

			if (c.tok != RecognizedToken::OpenBrace) {
				throw_wrong_token_error(c, "'{'");
			}
			c.move();

			while (true) {
				if (c.tok == RecognizedToken::CloseBrace) {
					c.move();
					break;
				}
				else if (c.tok == RecognizedToken::Eof) {
					break;
				}
				Declaration::parse(c, nd->members, nd.get(), nd.get());
			}

			into.push_back(std::move(nd));
		}
		else {
			throw_wrong_token_error(c, "'namespace', 'struct', 'class', 'function' or 'var'");
		}
	}

}