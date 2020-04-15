#include "Declaration.h"
#include "Error.h"
#include <iostream>
#include <string>
#include "PredefinedTypes.h"


namespace Corrosive {



	bool Namespace::parse(Cursor& c, std::unique_ptr<Namespace>& into) {
		std::unique_ptr<Namespace> result = std::make_unique<Namespace>();
		result->namespace_type = NamespaceType::t_namespace; 
		if (c.tok != RecognizedToken::Symbol)
		{
			throw_not_a_name_error(c);
			return false;
		}
		result->name = c;
		c.move();

		if (c.tok != RecognizedToken::OpenBrace) {
			throw_wrong_token_error(c, "'{'");
			return false;
		}
		c.move();

		if (!parse_inner(c, result.get())) return false;

		if (c.tok != RecognizedToken::CloseBrace) {
			throw_wrong_token_error(c, "'}'");
			return false;
		}
		c.move();

		into = std::move(result);
		return true;
	}


	bool Namespace::parse_inner(Cursor& c, Namespace* into) {
		
		while (c.tok != RecognizedToken::CloseBrace && c.tok!=RecognizedToken::Eof) {

			if (c.buffer == "struct") {
				c.move();
				Cursor nm = c;

				std::unique_ptr<StructureTemplate> decl;
				if (!StructureTemplate::parse(c, into, decl)) return false;

				decl->parent = into;
				if (into->subtemplates.find(decl->name.buffer) != into->subtemplates.end()) {
					throw_specific_error(nm, "this name already exists in current namespace");
					return false;
				}

				into->subtemplates[decl->name.buffer] = std::move(decl);

			} else if (c.buffer == "trait") {
				c.move();
				Cursor nm = c;

				std::unique_ptr<TraitTemplate> decl;
				if (!TraitTemplate::parse(c, into, decl)) return false;

				decl->parent = into;
				if (into->subtraits.find(decl->name.buffer) != into->subtraits.end()) {
					throw_specific_error(nm, "this name already exists in current namespace");
					return false;
				}

				into->subtraits[decl->name.buffer] = std::move(decl);

			}
			else if (c.buffer == "namespace") {
				c.move();
				Cursor nm = c;

				std::unique_ptr<Namespace> decl;
				if (!Namespace::parse(c, decl)) return false;
				decl->parent = into;
				if (into->subnamespaces.find(decl->name.buffer) != into->subnamespaces.end()) {
					throw_specific_error(nm, "this name already exists in current namespace");
					return false;
				}
				into->subnamespaces[decl->name.buffer] = std::move(decl);
			}
			else if (c.buffer == "fn") {
				StructureTemplateMemberFunc member;
				c.move();
				if (c.tok != RecognizedToken::Symbol) {
					throw_not_a_name_error(c);
					return false;
				}

				member.name = c;
				c.move();

				if (c.tok == RecognizedToken::OpenParenthesis) {
					c.move();
					member.annotation = c;
					int lvl = 1;
					while (lvl > 0) {
						switch (c.tok)
						{
							case RecognizedToken::OpenParenthesis: lvl++; c.move(); break;
							case RecognizedToken::CloseParenthesis: lvl--; c.move(); break;
							case RecognizedToken::Eof: {
									throw_eof_error(c, "parsing of function generic annotation");
									return false;
								}
							default: c.move(); break;
						}
					}
				}
				else {
					member.annotation.tok = RecognizedToken::Eof;
				}

				if (c.tok != RecognizedToken::Colon) {
					throw_wrong_token_error(c, "':'");
					return false;
				}
				c.move();
				member.type = c;

				while (c.tok != RecognizedToken::OpenBrace) {
					if (c.tok == RecognizedToken::Eof) {
						throw_eof_error(c, "parsing of structure member type");
						return false;
					}
					c.move();
				}
				c.move();
				member.block = c;
				int lvl = 1;
				while (lvl > 0) {
					switch (c.tok)
					{
						case RecognizedToken::OpenBrace: lvl++; c.move(); break;
						case RecognizedToken::CloseBrace: lvl--; c.move(); break;
						case RecognizedToken::Eof: {
								throw_eof_error(c, "parsing of function block");
								return false;
							}
						default: c.move(); break;
					}
				}

				std::unique_ptr<FunctionTemplate> ft = std::make_unique<FunctionTemplate>();
				ft->name = member.name;
				ft->annotation = member.annotation;
				ft->is_generic = member.annotation.tok != RecognizedToken::Eof;
				ft->parent = into;
				ft->template_parent = nullptr;
				ft->decl_type = member.type;
				ft->block = member.block;

				ft->type = std::make_unique<TypeFunctionTemplate>();
				ft->type->owner = ft.get();
				ft->type->rvalue = ILDataType::type;

				if (into->subfunctions.find(member.name.buffer) != into->subfunctions.end()) {
					throw_specific_error(member.name, "Function with the same name already exists in the namespace");
					return false;
				}

				into->subfunctions[member.name.buffer] = std::move(ft);

			}
			else {
				throw_specific_error(c, "Unexpected keyword found during parsing of namespace");
				return false;
			}
		}


		return true;
	}

	bool StructureTemplate::parse(Cursor& c, Namespace* parent, std::unique_ptr<StructureTemplate>& into) {
		std::unique_ptr<StructureTemplate> result = std::make_unique<StructureTemplate>();

		result->parent = parent;
		result->template_parent = dynamic_cast<StructureInstance*>(parent);

		if (c.tok != RecognizedToken::Symbol)
		{
			throw_not_a_name_error(c);
			return false;
		} 
		result->name = c;
		c.move();


		if (c.tok == RecognizedToken::OpenParenthesis) {
			c.move();
			result->is_generic = true;
			result->annotation = c;
			int lvl = 1;
			while (lvl > 0) {
				switch (c.tok)
				{
					case RecognizedToken::OpenParenthesis: lvl++; break;
					case RecognizedToken::CloseParenthesis: lvl--; break;
					case RecognizedToken::Eof:
						throw_eof_error(c, "parsing generic declaration header");
						return false;
				}
				c.move();
			}
		}

		if (c.tok != RecognizedToken::OpenBrace) {
			throw_wrong_token_error(c, "'{'");
			return false;
		}
		c.move();

		while (c.tok == RecognizedToken::Symbol) {
			if (c.buffer == "var") {
				StructureTemplateMemberVar member;
				c.move();
				member.name = c;
				c.move();
				if (c.tok != RecognizedToken::Colon) {
					throw_wrong_token_error(c, "':'");
					return false;
				}
				c.move();
				member.type = c;

				while (c.tok != RecognizedToken::Semicolon) {
					if (c.tok == RecognizedToken::Eof) {
						throw_eof_error(c, "parsing of structure member type");
						return false;
					}
					c.move();
				}
				c.move();
				result->member_vars.push_back(member);
			}
			else if (c.buffer == "fn") {
				StructureTemplateMemberFunc member;
				c.move();
				if (c.tok != RecognizedToken::Symbol) {
					throw_not_a_name_error(c);
					return false;
				}

				member.name = c;
				c.move();

				if (c.tok == RecognizedToken::OpenParenthesis) {
					c.move();
					member.annotation = c;
					int lvl = 1;
					while (lvl > 0) {
						switch (c.tok)
						{
							case RecognizedToken::OpenParenthesis: lvl++; c.move(); break;
							case RecognizedToken::CloseParenthesis: lvl--; c.move(); break;
							case RecognizedToken::Eof: {
									throw_eof_error(c, "parsing of function generic annotation");
									return false;
								}
							default: c.move(); break;
						}
					}
				}
				else {
					member.annotation.tok = RecognizedToken::Eof;
				}

				if (c.tok != RecognizedToken::Colon) {
					throw_wrong_token_error(c, "':'");
					return false;
				}
				c.move();
				member.type = c;

				while (c.tok != RecognizedToken::OpenBrace) {
					if (c.tok == RecognizedToken::Eof) {
						throw_eof_error(c, "parsing of structure member type");
						return false;
					}
					c.move();
				}
				c.move();
				member.block = c;
				int lvl = 1;
				while (lvl > 0) {
					switch (c.tok)
					{
						case RecognizedToken::OpenBrace: lvl++; c.move(); break;
						case RecognizedToken::CloseBrace: lvl--; c.move(); break;
						case RecognizedToken::Eof: {
								throw_eof_error(c, "parsing of function block");
								return false;
							}
						default: c.move(); break;
					}
				}

				result->member_funcs.push_back(member);
			}
			else if (c.buffer == "struct") {
				StructureTemplateSubtemplate member;
				c.move();
				member.cursor = c;
				if (c.tok != RecognizedToken::Symbol) {
					throw_not_a_name_error(c);
					return false;
				}

				c.move();


				if (c.tok == RecognizedToken::OpenParenthesis) {
					c.move();

					int lvl = 1;
					while (lvl > 0) {
						switch (c.tok)
						{
							case RecognizedToken::OpenParenthesis: lvl++; c.move(); break;
							case RecognizedToken::CloseParenthesis: lvl--; c.move(); break;
							case RecognizedToken::Eof: {
									throw_eof_error(c, "parsing of function block");
									return false;
								}
							default: c.move(); break;
						}
					}
				}
				
				if (c.tok != RecognizedToken::OpenBrace) {
					throw_wrong_token_error(c, "'{'");
					return false;
				}
				c.move();

				int lvl = 1;
				while (lvl > 0) {
					switch (c.tok)
					{
						case RecognizedToken::OpenBrace: lvl++; c.move(); break;
						case RecognizedToken::CloseBrace: lvl--; c.move(); break;
						case RecognizedToken::Eof: {
								throw_eof_error(c, "parsing of function block");
								return false;
							}
						default: c.move(); break;
					}
				}

				result->member_templates.push_back(member);
			}
			else if (c.buffer == "impl") {
				StructureTemplateImpl impl;
				c.move();
				impl.type = c;

				while (c.tok != RecognizedToken::OpenBrace) {
					if (c.tok == RecognizedToken::Eof) {
						throw_eof_error(c, "parsing of structure member type");
						return false;
					}
					c.move();
				}
				c.move();

				if (c.tok != RecognizedToken::CloseBrace) {
					while (c.tok != RecognizedToken::CloseBrace) {
						if (c.buffer == "fn") {
							StructureTemplateImplFunc member;
							c.move();
							if (c.tok != RecognizedToken::Symbol) {
								throw_not_a_name_error(c);
								return false;
							}

							member.name = c;
							c.move();

							if (c.tok != RecognizedToken::Colon) {
								throw_wrong_token_error(c, "':'");
								return false;
							}
							c.move();
							member.type = c;

							while (c.tok != RecognizedToken::OpenBrace) {
								if (c.tok == RecognizedToken::Eof) {
									throw_eof_error(c, "parsing of structure member type");
									return false;
								}
								c.move();
							}
							c.move();
							member.block = c;
							int lvl = 1;
							while (lvl > 0) {
								switch (c.tok)
								{
									case RecognizedToken::OpenBrace: lvl++; c.move(); break;
									case RecognizedToken::CloseBrace: lvl--; c.move(); break;
									case RecognizedToken::Eof: {
										throw_eof_error(c, "parsing of function block");
										return false;
									}
									default: c.move(); break;
								}
							}

							impl.functions.push_back(member);
						}
					}
				}
				c.move();

				result->member_implementation.push_back(impl);
			}
			else {
				throw_specific_error(c,"unexpected keyword found during parsing of structure");
				return false;
			}
		}

		if (c.tok != RecognizedToken::CloseBrace) {
			throw_wrong_token_error(c, "'}'");
			return false;
		}
		c.move();


		into = std::move(result);
		return true;
	}
	


	bool TraitTemplate::parse(Cursor& c, Namespace* parent, std::unique_ptr<TraitTemplate>& into) {
		std::unique_ptr<TraitTemplate> result = std::make_unique<TraitTemplate>();

		result->parent = parent;
		result->template_parent = dynamic_cast<StructureInstance*>(parent);

		if (c.tok != RecognizedToken::Symbol)
		{
			throw_not_a_name_error(c);
			return false;
		}
		result->name = c;
		c.move();


		if (c.tok == RecognizedToken::OpenParenthesis) {
			c.move();
			result->is_generic = true;
			result->annotation = c;
			int lvl = 1;
			while (lvl > 0) {
				switch (c.tok)
				{
					case RecognizedToken::OpenParenthesis: lvl++; break;
					case RecognizedToken::CloseParenthesis: lvl--; break;
					case RecognizedToken::Eof:
						throw_eof_error(c, "parsing generic declaration header");
						return false;
				}
				c.move();
			}
		}

		if (c.tok != RecognizedToken::OpenBrace) {
			throw_wrong_token_error(c, "'{'");
			return false;
		}
		c.move();

		while (c.tok == RecognizedToken::Symbol) {
			if (c.buffer == "fn") {
				TraitTemplateMemberFunc member;
				c.move();
				if (c.tok != RecognizedToken::Symbol) {
					throw_not_a_name_error(c);
					return false;
				}

				member.name = c;
				c.move();

				if (c.tok != RecognizedToken::Colon) {
					throw_wrong_token_error(c, "':'");
					return false;
				}
				c.move();
				member.type = c;

				while (c.tok != RecognizedToken::Semicolon) {
					if (c.tok == RecognizedToken::Eof) {
						throw_eof_error(c, "parsing of trait function");
						return false;
					}
					c.move();
				}
				c.move();

				result->member_funcs.push_back(member);
			}
			else {
				throw_specific_error(c, "unexpected keyword found during parsing of trait");
				return false;
			}
		}

		if (c.tok != RecognizedToken::CloseBrace) {
			throw_wrong_token_error(c, "'}'");
			return false;
		}
		c.move();


		into = std::move(result);
		return true;
	}


	bool Declaration::parse_global(Cursor &c, Namespace* global_namespace) {
		
		if (!Namespace::parse_inner(c, global_namespace)) return false; 

		if (c.tok != RecognizedToken::Eof) {
			throw_wrong_token_error(c, "end of file");
			return false;
		}

		return true;

	}
}