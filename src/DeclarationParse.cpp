#include "Declaration.h"
#include "Error.h"
#include <iostream>
#include "Contents.h"
#include <string>
#include "PredefinedTypes.h"


namespace Corrosive {



	bool Namespace::parse(Cursor& c, CompileContext& ctx, std::unique_ptr<Namespace>& into) {
		std::unique_ptr<Namespace> result = std::make_unique<Namespace>();
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

				std::unique_ptr<Structure> decl;
				Structure::parse(c, ctx, decl);
				decl->parent = result.get();
				if (result->subnamespaces.find(decl->name.buffer) != result->subnamespaces.end()) {
					throw_specific_error(nm, "this name already exists in current namespace");
					return false;
				}

				result->subnamespaces[decl->name.buffer] = std::move(decl);
			} else if (c.buffer == "namespace") {
				c.move();
				Cursor nm = c;

				std::unique_ptr<Namespace> decl;
				Namespace::parse(c, ctx, decl);
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

	bool Structure::parse(Cursor& c, CompileContext& ctx, std::unique_ptr<Structure>& into) {
		std::unique_ptr<Structure> result = std::make_unique<Structure>();
		std::unique_ptr<DirectType> result_type = std::make_unique<DirectType>();
		result_type->rvalue = ILDataType::ptr;
		result_type->owner = result.get();
		result->type = std::move(result_type);

		if (c.tok != RecognizedToken::Symbol)
		{
			throw_not_a_name_error(c);
			return false;
		} 
		result->name = c;
		c.move();


		if (c.tok == RecognizedToken::Colon) {
			c.move();
			if (c.tok != RecognizedToken::OpenParenthesis) {
				throw_wrong_token_error(c, "'('");
				return false;
			}
			c.move();
			result->is_generic = true;
			result->generic_types = c;
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
				StructureMember member;
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
					}
					c.move();
				}
				c.move();
				result->members.push_back(member);
			}
			else if (c.buffer == "struct") {
				c.move();
				Cursor nm = c;

				std::unique_ptr<Structure> decl;
				Structure::parse(c, ctx, decl);
				decl->parent = result.get();
				if (result->subnamespaces.find(decl->name.buffer) != result->subnamespaces.end()) {
					throw_specific_error(nm, "this name already exists in current structure");
					return false;
				}

				result->subnamespaces[decl->name.buffer] = std::move(decl);
			}
			else if (c.buffer == "namespace") {
				c.move();
				Cursor nm = c;

				std::unique_ptr<Namespace> decl;
				Namespace::parse(c, ctx, decl);
				decl->parent = result.get();
				if (result->subnamespaces.find(decl->name.buffer) != result->subnamespaces.end()) {
					throw_specific_error(nm, "this name already exists in current structure");
					return false;
				}
				result->subnamespaces[decl->name.buffer] = std::move(decl);
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
				std::unique_ptr<Structure> decl;
				Structure::parse(c, ctx, decl);
				decl->parent = &global_namespace;
				if (global_namespace.subnamespaces.find(decl->name.buffer) != global_namespace.subnamespaces.end()) {
					throw_specific_error(nm, "this name already exists in global namespace");
					return false;
				}
				global_namespace.subnamespaces[decl->name.buffer] = std::move(decl);
			}
			else if(c.buffer == "namespace") {
				c.move();
				Cursor nm = c;

				std::unique_ptr<Namespace> decl;
				Namespace::parse(c, ctx, decl);
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