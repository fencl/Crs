#include "Declaration.h"
#include "Error.h"
#include <iostream>
#include "Contents.h"
#include <string>
#include "PredefinedTypes.h"


namespace Corrosive {



	bool Namespace::parse(Cursor& c, CompileContext& ctx, std::unique_ptr<Namespace>& into) {
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

		while (c.tok == RecognizedToken::Symbol) {
			if (c.buffer == "struct") {
				c.move();
				Cursor nm = c;

				std::unique_ptr<StructureTemplate> decl;
				if (!StructureTemplate::parse(c, ctx,result.get(), decl)) return false;

				decl->parent = result.get();
				if (result->subtemplates.find(decl->name.buffer) != result->subtemplates.end()) {
					throw_specific_error(nm, "this name already exists in current namespace");
					return false;
				}

				result->subtemplates[decl->name.buffer] = std::move(decl);

			} else if (c.buffer == "namespace") {
				c.move();
				Cursor nm = c;

				std::unique_ptr<Namespace> decl;
				if (!Namespace::parse(c, ctx, decl)) return false;
				decl->parent = result.get();
				if (result->subnamespaces.find(decl->name.buffer) != result->subnamespaces.end()) {
					throw_specific_error(nm, "this name already exists in current namespace");
					return false;
				}
				result->subnamespaces[decl->name.buffer] = std::move(decl);
			}
			else {
				throw_specific_error(c, "unexpected keyword found during parsing of namespace");
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

	bool StructureTemplate::parse(Cursor& c, CompileContext& ctx, Namespace* parent, std::unique_ptr<StructureTemplate>& into) {
		std::unique_ptr<StructureTemplate> result = std::make_unique<StructureTemplate>();

		result->type = std::make_unique<TypeStructure>();
		result->type->rvalue = ILDataType::ptr;
		result->type->owner = result.get();

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
			else if (c.buffer == "function") {
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
	



	bool Declaration::parse_global(Cursor &c, CompileContext& ctx, Namespace& global_namespace) {
		while (c.tok == RecognizedToken::Symbol) {
			if (c.buffer == "struct") {
				c.move();
				Cursor nm = c;
				std::unique_ptr<StructureTemplate> decl;
				if (!StructureTemplate::parse(c, ctx,&global_namespace, decl)) return false;
				decl->parent = &global_namespace;
				if (global_namespace.subnamespaces.find(decl->name.buffer) != global_namespace.subnamespaces.end()) {
					throw_specific_error(nm, "this name already exists in global namespace");
					return false;
				}
				global_namespace.subtemplates[decl->name.buffer] = std::move(decl);
			}
			else if(c.buffer == "namespace") {
				c.move();
				Cursor nm = c;

				std::unique_ptr<Namespace> decl;
				if (!Namespace::parse(c, ctx, decl)) return false;
				decl->parent = &global_namespace;
				if (global_namespace.subnamespaces.find(decl->name.buffer) != global_namespace.subnamespaces.end()) {
					throw_specific_error(nm, "this name already exists in global namespace");
					return false;
				}
				global_namespace.subnamespaces[decl->name.buffer] = std::move(decl);
			}
			else {
				throw_wrong_token_error(c, "'struct'");
				return false;
			}
		}

		if (c.tok != RecognizedToken::Eof) {
			throw_wrong_token_error(c, "end of file");
			return false;
		}

		return true;
	}
}