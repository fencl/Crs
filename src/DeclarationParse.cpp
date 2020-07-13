#include "Declaration.h"
#include "Error.h"
#include <iostream>
#include <string>
#include "PredefinedTypes.h"


namespace Corrosive {



	void Namespace::parse(Cursor& c, std::unique_ptr<Namespace>& into) {
		std::unique_ptr<Namespace> result = std::make_unique<Namespace>();
		result->namespace_type = NamespaceType::t_namespace; 
		if (c.tok != RecognizedToken::Symbol)
		{
			throw_not_a_name_error(c);
		}
		result->name = c;
		c.move();

		if (c.tok != RecognizedToken::OpenBrace) {
			throw_wrong_token_error(c, "'{'");
		}
		c.move();

		Namespace::parse_inner(c, result.get(), nullptr);

		if (c.tok != RecognizedToken::CloseBrace) {
			throw_wrong_token_error(c, "'}'");
		}
		c.move();

		into = std::move(result);
	}


	void Namespace::parse_inner(Cursor& c, Namespace* into, GenericInstance* gen_inst) {
		
		while (c.tok != RecognizedToken::CloseBrace && c.tok!=RecognizedToken::Eof) {
			if (c.buffer == "require") {
				c.move();
				if (c.tok != RecognizedToken::String) {
					throw_specific_error(c, "Expected string"); // TODO better message
				}
				std::cout <<"require: "<< c.buffer << "\n";

				c.move();
				if (c.tok != RecognizedToken::Semicolon) {
					throw_wrong_token_error(c, "';'");
				}
				c.move();
			}else if (c.buffer == "struct") {
				c.move();
				Cursor nm = c;

				std::unique_ptr<StructureTemplate> decl;
				StructureTemplate::parse(c, into, nullptr, decl);

				decl->parent = into;
				if (into->subtemplates.find(decl->name.buffer) != into->subtemplates.end()) {
					throw_specific_error(nm, "this name already exists in current namespace");
				}

				into->subtemplates[decl->name.buffer] = std::move(decl);

			} else if (c.buffer == "trait") {
				c.move();
				Cursor nm = c;

				std::unique_ptr<TraitTemplate> decl;
				TraitTemplate::parse(c, into, nullptr, decl);

				decl->parent = into;
				if (into->subtraits.find(decl->name.buffer) != into->subtraits.end()) {
					throw_specific_error(nm, "this name already exists in current namespace");
				}

				into->subtraits[decl->name.buffer] = std::move(decl);

			}
			else if (c.buffer == "namespace") {
				c.move();
				Cursor nm = c;

				std::unique_ptr<Namespace> decl;
				Namespace::parse(c, decl);
				decl->parent = into;
				if (into->subnamespaces.find(decl->name.buffer) != into->subnamespaces.end()) {
					throw_specific_error(nm, "this name already exists in current namespace");
				}
				into->subnamespaces[decl->name.buffer] = std::move(decl);
			}
			else if (c.buffer == "fn") {
				StructureTemplateMemberFunc member;
				bool ext = false;

				c.move();
				member.context = ILContext::both;

				while (true) {
					if (c.buffer == "compile" && member.context == ILContext::both) {
						member.context = ILContext::compile;
						c.move();
					}
					else if (c.buffer == "runtime" && member.context == ILContext::both) {
						member.context = ILContext::runtime;
						c.move();
					}
					else if (c.buffer == "ext") {
						ext = true;
						c.move();
					}
					else break;
				}

				if (c.tok != RecognizedToken::Symbol) {
					throw_not_a_name_error(c);
				}

				member.name = c;
				c.move();

				if (!ext && c.tok == RecognizedToken::OpenParenthesis) {
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
				}
				c.move();
				member.type = c;

				if (!ext) {
					while (c.tok != RecognizedToken::OpenBrace) {
						if (c.tok == RecognizedToken::Eof) {
							throw_eof_error(c, "parsing of structure member type");
						}
						c.move();
					}
					member.block = c;
					member.block.move();
					c.move_matching();
				}
				else {
					while (c.tok != RecognizedToken::Semicolon) {
						if (c.tok == RecognizedToken::Eof) {
							throw_eof_error(c, "parsing of structure member type");
						}
						c.move();
					}

					member.block.src = nullptr;
				}

				c.move();

				std::unique_ptr<FunctionTemplate> ft = std::make_unique<FunctionTemplate>();
				ft->name = member.name;
				ft->annotation = member.annotation;
				ft->is_generic = member.annotation.tok != RecognizedToken::Eof;
				ft->parent = into;
				ft->generic_ctx.generator = gen_inst;
				//ft->template_parent = nullptr;
				ft->decl_type = member.type;
				ft->block = member.block;
				ft->context = member.context;

				ft->type = std::make_unique<TypeFunctionTemplate>();
				ft->type->owner = ft.get();

				if (into->subfunctions.find(member.name.buffer) != into->subfunctions.end()) {
					throw_specific_error(member.name, "Function with the same name already exists in the namespace");
				}

				into->subfunctions[member.name.buffer] = std::move(ft);

			}
			else {
				throw_specific_error(c, "Unexpected keyword found during parsing of namespace");
			}
		}
	}

	void StructureTemplate::parse(Cursor& c, Namespace* parent, GenericInstance* gen_inst, std::unique_ptr<StructureTemplate>& into) {
		std::unique_ptr<StructureTemplate> result = std::make_unique<StructureTemplate>();

		result->parent = parent;
		result->generic_ctx.generator = gen_inst;

		//result->template_parent = dynamic_cast<StructureInstance*>(parent);

		if (c.tok != RecognizedToken::Symbol)
		{
			throw_not_a_name_error(c);
			
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
						
				}
				c.move();
			}
		}

		if (c.tok != RecognizedToken::OpenBrace) {
			throw_wrong_token_error(c, "'{'");
			
		}
		c.move();

		while (c.tok == RecognizedToken::Symbol) {
			if (c.buffer == "var") {
				StructureTemplateMemberVar member;
				c.move();
				if (c.buffer == "alias") {
					member.composite = true;
					c.move();
				}
				else {
					member.composite = false;
				}
				member.name = c;
				c.move();
				if (c.tok != RecognizedToken::Colon) {
					throw_wrong_token_error(c, "':'");
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

				result->member_vars.push_back(member);
			}
			else if (c.buffer == "fn") {
				StructureTemplateMemberFunc member;

				c.move();
				if (c.buffer == "compile") {
					member.context = ILContext::compile;
					c.move();
				}else if (c.buffer == "runtime") {
					member.context = ILContext::runtime;
					c.move();
				}
				else member.context = ILContext::both;

				if (c.tok != RecognizedToken::Symbol) {
					throw_not_a_name_error(c);
					
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
					
				}
				c.move();
				member.type = c;

				while (c.tok != RecognizedToken::OpenBrace) {
					if (c.tok == RecognizedToken::Eof) {
						throw_eof_error(c, "parsing of structure member type");
						
					}
					c.move();
				}
				//c.move();
				member.block = c;
				member.block.move();
				c.move_matching();
				c.move();
				/*int lvl = 1;
				while (lvl > 0) {
					switch (c.tok)
					{
						case RecognizedToken::OpenBrace: lvl++; c.move(); break;
						case RecognizedToken::CloseBrace: lvl--; c.move(); break;
						case RecognizedToken::Eof: {
								throw_eof_error(c, "parsing of function block");
								
							}
						default: c.move(); break;
					}
				}*/

				result->member_funcs.push_back(member);
			}
			else if (c.buffer == "struct") {
				StructureTemplateSubtemplate member;
				c.move();
				member.cursor = c;
				if (c.tok != RecognizedToken::Symbol) {
					throw_not_a_name_error(c);
					
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
									
								}
							default: c.move(); break;
						}
					}
				}
				
				if (c.tok != RecognizedToken::OpenBrace) {
					throw_wrong_token_error(c, "'{'");
					
				}
				c.move_matching();
				c.move();

				/*int lvl = 1;
				while (lvl > 0) {
					switch (c.tok)
					{
						case RecognizedToken::OpenBrace: lvl++; c.move(); break;
						case RecognizedToken::CloseBrace: lvl--; c.move(); break;
						case RecognizedToken::Eof: {
								throw_eof_error(c, "parsing of function block");
								
							}
						default: c.move(); break;
					}
				}*/

				result->member_templates.push_back(member);
			}
			else if (c.buffer == "impl") {
				StructureTemplateImpl impl;
				c.move();
				impl.type = c;

				while (c.tok != RecognizedToken::OpenBrace && c.tok != RecognizedToken::Colon) {
					if (c.tok == RecognizedToken::Eof) {
						throw_eof_error(c, "parsing of structure member type");
						
					}
					c.move();
				}
				bool single = c.tok == RecognizedToken::Colon;
				c.move();

				if (!single) {
					if (c.tok != RecognizedToken::CloseBrace) {
						while (c.tok != RecognizedToken::CloseBrace) {
							if (c.buffer == "fn") {

								StructureTemplateImplFunc member;
								c.move();
								/*if (c.buffer == "compile") {
									member.ctx = ILContext::compile;
									c.move();
								}
								else if (c.buffer == "runtime") {
									member.ctx = ILContext::runtime;
									c.move();
								}*/

								if (c.tok != RecognizedToken::Symbol) {
									throw_not_a_name_error(c);

								}

								member.name = c;
								c.move();

								if (c.tok != RecognizedToken::Colon) {
									throw_wrong_token_error(c, "':'");

								}
								c.move();
								member.type = c;

								while (c.tok != RecognizedToken::OpenBrace) {
									if (c.tok == RecognizedToken::Eof) {
										throw_eof_error(c, "parsing of structure member type");

									}
									c.move();
								}
								//c.move();
								member.block = c;
								member.block.move();
								c.move_matching();
								c.move();
								/*int lvl = 1;
								while (lvl > 0) {
									switch (c.tok)
									{
										case RecognizedToken::OpenBrace: lvl++; c.move(); break;
										case RecognizedToken::CloseBrace: lvl--; c.move(); break;
										case RecognizedToken::Eof: {
											throw_eof_error(c, "parsing of function block");

										}
										default: c.move(); break;
									}
								}*/

								impl.functions.push_back(member);
							}
						}
					}
					c.move();
				}
				else {
					StructureTemplateImplFunc member;
					Cursor noname;		
					noname.buffer = "<s>";
					member.name = noname;

					member.type = c;

					while (c.tok != RecognizedToken::OpenBrace) {
						if (c.tok == RecognizedToken::Eof) {
							throw_eof_error(c, "parsing of structure member type");

						}
						c.move();
					}
					//c.move();
					member.block = c;
					member.block.move();
					c.move_matching();
					c.move();
					/*int lvl = 1;
					while (lvl > 0) {
						switch (c.tok)
						{
							case RecognizedToken::OpenBrace: lvl++; c.move(); break;
							case RecognizedToken::CloseBrace: lvl--; c.move(); break;
							case RecognizedToken::Eof: {
								throw_eof_error(c, "parsing of function block");

							}
							default: c.move(); break;
						}
					}*/

					impl.functions.push_back(member);
				}

				result->member_implementation.push_back(impl);
			}
			else {
				throw_specific_error(c,"unexpected keyword found during parsing of structure");
				
			}
		}

		if (c.tok != RecognizedToken::CloseBrace) {
			throw_wrong_token_error(c, "'}'");
			
		}
		c.move();


		into = std::move(result);
	}
	


	void TraitTemplate::parse(Cursor& c, Namespace* parent, GenericInstance* gen_inst, std::unique_ptr<TraitTemplate>& into) {
		std::unique_ptr<TraitTemplate> result = std::make_unique<TraitTemplate>();

		result->parent = parent;
		result->generic_ctx.generator = gen_inst;
		//result->template_parent = dynamic_cast<StructureInstance*>(parent);

		if (c.tok != RecognizedToken::Symbol)
		{
			throw_not_a_name_error(c);
			
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
						
				}
				c.move();
			}
		}

		if (c.tok != RecognizedToken::OpenBrace) {
			throw_wrong_token_error(c, "'{'");
			
		}
		c.move();

		while (c.tok == RecognizedToken::Symbol) {
			if (c.buffer == "fn") {
				TraitTemplateMemberFunc member;

				c.move();

				if (c.buffer == "compile") {
					member.ctx = ILContext::compile;
					c.move();
				}
				else if (c.buffer == "runtime") {
					member.ctx = ILContext::runtime;
					c.move();
				}

				if (c.tok != RecognizedToken::Symbol) {
					throw_not_a_name_error(c);
					
				}

				member.name = c;
				c.move();

				if (c.tok != RecognizedToken::Colon) {
					throw_wrong_token_error(c, "':'");
					
				}
				c.move();
				member.type = c;

				while (c.tok != RecognizedToken::Semicolon) {
					if (c.tok == RecognizedToken::Eof) {
						throw_eof_error(c, "parsing of trait function");
						
					}
					c.move();
				}
				c.move();

				result->member_funcs.push_back(member);
			}
			else {
				throw_specific_error(c, "unexpected keyword found during parsing of trait");
				
			}
		}

		if (c.tok != RecognizedToken::CloseBrace) {
			throw_wrong_token_error(c, "'}'");
			
		}
		c.move();


		into = std::move(result);
	}


	void Declaration::parse_global(Cursor &c, Namespace* global_namespace) {
		
		Namespace::parse_inner(c, global_namespace, nullptr);

		if (c.tok != RecognizedToken::Eof) {
			throw_wrong_token_error(c, "end of file");	
		}
	}
}