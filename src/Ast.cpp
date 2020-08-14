#include "Ast.hpp"
#include "Error.hpp"
#include "Compiler.hpp"
#include "Declaration.hpp"

namespace Corrosive {
	std::unique_ptr<AstRootNode> AstRootNode::parse(Source* src) {
		auto root = std::make_unique<AstRootNode>();
		root->parent = src;
		
		Cursor c = src->read_first();

		auto global = std::make_unique<AstNamespaceNode>();
		global->parent = root.get();

		while (c.tok!=RecognizedToken::Eof) {

			auto buf = c.buffer();
			if (buf == "namespace") {
				global->namespaces.push_back(AstNamedNamespaceNode::parse(c, global.get()));
			}
			else if (buf == "struct") {
				global->structures.push_back(AstStructureNode::parse(c, global.get()));
			}
			else if (buf == "fn") {
				global->functions.push_back(AstFunctionNode::parse(c, global.get(),ILContext::both));
			}
			else if (buf == "trait") {
				global->traits.push_back(AstTraitNode::parse(c, global.get()));
			}
			else if (buf == "compile") {
				c.move();
				root->compile.push_back(c.offset);

				if (c.tok != RecognizedToken::OpenBrace) {
					throw_wrong_token_error(c, "'{'");
				}
				c.move_matching();
				c.move();
			}
			else if (buf == "static") {
				global->statics.push_back(AstStaticNode::parse(c, global.get()));
			}
			else {
				throw_wrong_token_error(c, "namespace, fn, struct, trait, compile or static");
			}

		}

		root->global_namespace = std::move(global);
		return std::move(root);
	}

	std::unique_ptr<AstNamedNamespaceNode> AstNamedNamespaceNode::parse(Cursor& c, AstNode* parent) {
		auto space = std::make_unique<AstNamedNamespaceNode>();
		space->parent = parent;
		c.move();
		space->name = c.offset;
		space->name_string = c.buffer();
		c.move();
		if (c.tok != RecognizedToken::OpenBrace) {
			throw_wrong_token_error(c, "'{'");
		}
		c.move();

		while (c.tok != RecognizedToken::CloseBrace) {
			if (c.tok == RecognizedToken::Eof) {
				throw_eof_error(c, "parsing of namespace");
			}

			auto buf = c.buffer();
			if (buf == "namespace") {
				space->namespaces.push_back(AstNamedNamespaceNode::parse(c, space.get()));
			}
			else if (buf == "struct") {
				space->structures.push_back(AstStructureNode::parse(c, space.get()));
			}
			else if (buf == "fn") {
				space->functions.push_back(AstFunctionNode::parse(c, space.get(),ILContext::both));
			}
			else if (buf == "trait") {
				space->traits.push_back(AstTraitNode::parse(c,space.get()));
			}
			else if (buf == "static") {
				space->statics.push_back(AstStaticNode::parse(c, space.get()));
			}
			else {
				throw_wrong_token_error(c, "namespace, fn, struct, trait or static");
			}
		}

		c.move();


		return std::move(space);
	}

	std::unique_ptr<AstStructureNode> AstStructureNode::parse(Cursor& c, AstNode* parent) {
		auto structure = std::make_unique<AstStructureNode>();
		structure->parent = parent;
		c.move();
		if (c.tok == RecognizedToken::OpenParenthesis) {
			structure->is_generic = true;
			structure->annotation = c.offset;
			c.move_matching();
			c.move();
		}
		else {
			structure->is_generic = false;
		}

		auto buf = c.buffer();
		if (buf == "compile") {
			structure->context = ILContext::compile;
			c.move();
		}
		else if (buf == "runtime") {
			structure->context = ILContext::runtime;
			c.move();
		}
		else {
			structure->context = ILContext::both;
		}

		structure->name = c.offset;
		structure->name_string = c.buffer();
		c.move();
		if (c.tok != RecognizedToken::OpenBrace) {
			throw_wrong_token_error(c, "'{'");
		}
		c.move();

		while (c.tok != RecognizedToken::CloseBrace) {
			if (c.tok == RecognizedToken::Eof) {
				throw_eof_error(c, "parsing of structure");
			}

			auto buf = c.buffer();
			if (buf == "struct") {
				structure->structures.push_back(AstStructureNode::parse(c, structure.get()));
			}
			else if (buf == "fn") {
				Cursor err = c;
				auto fun = AstFunctionNode::parse(c, structure.get(), ILContext::both);
				if (!fun->has_body()) {
					throw_specific_error(err, "Functions inside structures are supposed to have a body");
				}

				structure->functions.push_back(std::move(fun));
			}
			else if (buf == "var") {
				structure->variables.push_back(AstVariableNode::parse(c, structure.get()));
			}
			else if (buf == "trait") {
				structure->traits.push_back(AstTraitNode::parse(c,structure.get()));
			}
			else if (buf == "impl") {
				structure->implementations.push_back(AstImplementationNode::parse(c, structure.get()));
			}
			else if (buf == "static") {
				structure->statics.push_back(AstStaticNode::parse(c, structure.get()));
			}
			else if (buf == "compile") {
				c.move();
				structure->compile_blocks.push_back(c.offset);
				if (c.tok == RecognizedToken::OpenBrace) {
					c.move_matching();
					c.move();
				}else {
					while(c.tok != RecognizedToken::Semicolon) {
						if (c.tok == RecognizedToken::Eof) {
							throw_eof_error(c,"parsing of compile expression");
						}
						c.move();
					}
					c.move();
				}
			}
			else {
				throw_wrong_token_error(c, "var, fn, struct, impl, trait, static or compile");
			}
		}

		c.move();


		return std::move(structure);
	}


	std::unique_ptr<AstFunctionDeclarationNode> AstFunctionNode::parse(Cursor& c, AstNode* parent, ILContext force_context) {
		c.move();
		bool is_generic = false;
		AstCursor annotation = 0;
		if (c.tok == RecognizedToken::OpenParenthesis) {
			is_generic = true;
			annotation = c.offset;
			c.move_matching();
			c.move();
		}

		bool exported = false;
		bool ext = false;
		ILCallingConvention convention = ILCallingConvention::bytecode;
		ILContext context = force_context;

		while (true) {
			auto buf = c.buffer();
			if (buf == "compile") {
				if (force_context == ILContext::both) {
					context = ILContext::compile;
				}
				else {
					throw_specific_error(c, "Context was forced by parent declaration");
				}

				c.move();
			}
			else if (buf == "runtime") {
				if (force_context == ILContext::both) {
					context = ILContext::runtime;
				}
				else {
					throw_specific_error(c, "Context was forced by parent declaration");
				}

				c.move();
			}
			else if (buf == "native") {
				convention = ILCallingConvention::native;
				ext = true;
				c.move();
			}
			else if (buf == "stdcall") {
				convention = ILCallingConvention::stdcall;
				ext = true;
				c.move();
			}
			else if (buf == "export") {
				exported = true;
				c.move();
			}
			else break;
		}
		
		AstCursor name = c.offset;
		std::string_view name_string = c.buffer();
		c.move();
		if (c.tok != RecognizedToken::Colon) {
			throw_wrong_token_error(c, "':'");
		}
		c.move();
		if (c.tok != RecognizedToken::OpenParenthesis) {
			throw_wrong_token_error(c, "'('");
		}
		AstCursor type = c.offset;
		Cursor ct = c;
		c.move_matching();
		c.move();


		// TODO: this is dumb
		while (c.tok != RecognizedToken::OpenBrace && c.tok != RecognizedToken::Semicolon) {
			if (c.tok == RecognizedToken::Eof) {
				throw_eof_error(c, "parsing of function type");
			}
			c.move();
		}

		if (c.tok == RecognizedToken::OpenBrace) {
			auto fun = std::make_unique<AstFunctionNode>();
			fun->annotation = annotation;
			fun->is_generic = is_generic;
			fun->block = c.offset;
			fun->context = context;
			fun->convention = convention;
			fun->type = type;
			fun->name = name;
			fun->name_string = name_string;
			fun->parent = parent;
			fun->exported = exported;

			ct.move();
			if (ct.tok != RecognizedToken::CloseParenthesis) {
				while (ct.tok != RecognizedToken::Eof) {
					if (ct.tok != RecognizedToken::Symbol) {
						throw_not_a_name_error(ct);
					}
					fun->argument_names.push_back(std::make_pair(ct.offset, ct.buffer()));

					ct.move();
					if (ct.tok != RecognizedToken::Colon) {
						throw_wrong_token_error(ct, "':'");
					}

					ct.move();
					while (ct.tok != RecognizedToken::CloseParenthesis && ct.tok != RecognizedToken::Comma) {
						if (ct.tok == RecognizedToken::OpenParenthesis || ct.tok == RecognizedToken::OpenBrace) {
							ct.move_matching();
						}
						ct.move();
					}

					if (ct.tok == RecognizedToken::Comma) {
						ct.move();
					}
					else if (ct.tok == RecognizedToken::CloseParenthesis) {
						break;
					}
					else {
						throw_wrong_token_error(ct, "',' or ')'");
					}
				}
			}
			ct.move();
			fun->return_type = ct.offset;

			c.move_matching();
			c.move();
			return std::move(fun);
		}
		else {
			auto fun = std::make_unique<AstFunctionDeclarationNode>();
			fun->context = context;
			fun->convention = convention;
			fun->type = type;
			fun->name = name;
			fun->name_string = name_string;
			fun->parent = parent;
			fun->exported = exported;
			c.move();
			return std::move(fun);
		}
	}


	std::unique_ptr<AstVariableNode> AstVariableNode::parse(Cursor& c, AstNode* parent) {
		auto var = std::make_unique<AstVariableNode>();
		
		c.move();
		
		if (c.buffer() == "alias") {
			var->alias = true;
			c.move();
		}
		else {
			var->alias = false;
		}

		var->name = c.offset;
		var->name_string = c.buffer();
		c.move();

		if (c.tok != RecognizedToken::Colon) {
			throw_wrong_token_error(c, "':'");
		}
		c.move();
		var->type = c.offset;
		// TODO this is maybe also dumb
		while (c.tok != RecognizedToken::Semicolon) {
			switch (c.tok)
			{
				case RecognizedToken::Eof:
					throw_eof_error(c, "parsing variable type"); break;
				case RecognizedToken::OpenParenthesis:
				case RecognizedToken::OpenBrace:
					c.move_matching();
				default:
					c.move();
					break;
			}
		}
		c.move();
		return std::move(var);
	}


	std::unique_ptr<AstTraitNode> AstTraitNode::parse(Cursor& c, AstNode* parent) {
		auto trait = std::make_unique<AstTraitNode>();
		trait->parent = parent;
		c.move();
		if (c.tok == RecognizedToken::OpenParenthesis) {
			trait->is_generic = true;
			trait->annotation = c.offset;
			c.move_matching();
			c.move();
		}
		else {
			trait->is_generic = false;
		}

		auto buf = c.buffer();
		if (buf == "compile") {
			trait->context = ILContext::compile;
			c.move();
		}
		else if (buf == "runtime") {
			trait->context = ILContext::runtime;
			c.move();
		}
		else {
			trait->context = ILContext::both;
		}


		trait->name = c.offset;
		trait->name_string = c.buffer();
		c.move();

		if (c.tok != RecognizedToken::OpenBrace) {
			throw_wrong_token_error(c, "'{'");
		}
		c.move();


		while (c.tok != RecognizedToken::CloseBrace) {
			if (c.tok == RecognizedToken::Eof) {
				throw_eof_error(c, "parsing of trait");
			}

			auto buf = c.buffer();
			if (buf == "fn") {
				trait->declarations.push_back(AstFunctionNode::parse(c, trait.get(), trait->context));
			}
			else {
				throw_wrong_token_error(c, "fn");
			}
		}

		c.move();


		return std::move(trait);
	}


	std::unique_ptr<AstImplementationNode> AstImplementationNode::parse(Cursor& c, AstNode* parent) {
		auto impl = std::make_unique<AstImplementationNode>();
		impl->parent = parent;
		c.move();
		impl->trait = c.offset;
		// TODO maybe put type into parenthesies?
		while (c.tok != RecognizedToken::OpenBrace && c.tok!= RecognizedToken::Colon) {
			if (c.tok == RecognizedToken::Eof) {
				throw_eof_error(c, "parsing of implementation type");
			}

			c.move();
		}
		if (c.tok == RecognizedToken::OpenBrace) {
			impl->fast = false;
			c.move();
		


			while (c.tok != RecognizedToken::CloseBrace) {
				if (c.tok == RecognizedToken::Eof) {
					throw_eof_error(c, "parsing of trait");
				}

				auto buf = c.buffer();
				if (buf == "fn") {
					Cursor err = c;
					auto fun = AstFunctionNode::parse(c,impl.get(), ILContext::both);
					if (fun->has_body()) {
						impl->functions.push_back((std::unique_ptr<AstFunctionNode>&&)std::move(fun));
					}
					else {
						throw_specific_error(err, "Functions inside implementations are supposed to have a body");
					}
				}
				else {
					throw_wrong_token_error(c, "fn");
				}
			}

			c.move();
		}
		else {
			impl->fast = true;
			c.move();
			std::unique_ptr<AstFunctionNode> fundecl = std::make_unique<AstFunctionNode>();
			fundecl->parent = impl.get();
			fundecl->convention = ILCallingConvention::bytecode;
			fundecl->type = c.offset;
			fundecl->name = impl->trait;
			fundecl->name_string = "single";
			fundecl->is_generic = false;
			fundecl->annotation = 0;

			auto buf = c.buffer();
			if (buf == "compile") {
				fundecl->context = ILContext::compile;
				c.move();
			}
			else if (buf == "runtime") {
				fundecl->context = ILContext::runtime;
				c.move();
			}
			else {
				fundecl->context = ILContext::both;
			}

			c.move_matching();
			c.move();
			while (c.tok != RecognizedToken::OpenBrace) {
				if (c.tok == RecognizedToken::Eof) {
					throw_eof_error(c, "parsing of function declaration");
				}
				c.move();
			}
			fundecl->block = c.offset;
			c.move_matching();
			c.move();
			impl->functions.push_back(std::move(fundecl));
		}

		return std::move(impl);
	}


	void AstRootNode::populate() {
		global_namespace->populate(Compiler::current()->global_namespace());
	}


	void AstNamespaceNode::populate(Namespace* into) {
		for (auto&& n : namespaces) {
			std::unique_ptr<Namespace> nspc = std::make_unique<Namespace>();
			nspc->ast_node = n.get();
			nspc->parent = into;

			if (into->name_table.find(n->name_string)!= into->name_table.end()) {
				
				Cursor c = load_cursor(n->name, n->get_source());
				throw_specific_error(c, "Name already exists in the namespace");
			}

			into->name_table[n->name_string] = std::make_pair<uint8_t,uint32_t>(0, (uint32_t)into->subnamespaces.size());
			n->populate(nspc.get());
			into->subnamespaces.push_back(std::move(nspc));

		}

		for (auto&& n : structures) {
			std::unique_ptr<StructureTemplate> structure = std::make_unique<StructureTemplate>();
			structure->ast_node = n.get();
			structure->parent = into;

			if (into->name_table.find(n->name_string) != into->name_table.end()) {
				
				Cursor c = load_cursor(n->name, n->get_source());
				throw_specific_error(c, "Name already exists in the namespace");
			}
			into->name_table[n->name_string] = std::make_pair<uint8_t, uint32_t>(1, (uint32_t)into->subtemplates.size());
			into->subtemplates.push_back(std::move(structure));
		}

		for (auto&& n : functions) {
			std::unique_ptr<FunctionTemplate> function = std::make_unique<FunctionTemplate>();
			function->ast_node = n.get();
			function->parent = into;
			if (function->ast_node->exported) {
				Compiler::current()->exported_functions.push_back(function.get());
			}

			if (into->name_table.find(n->name_string) != into->name_table.end()) {
				
				Cursor c = load_cursor(n->name, n->get_source());
				throw_specific_error(c, "Name already exists in the namespace");
			}
			into->name_table[n->name_string] = std::make_pair<uint8_t, uint32_t>(2, (uint32_t)into->subfunctions.size());
			into->subfunctions.push_back(std::move(function));
		}

		for (auto&& n : traits) {
			std::unique_ptr<TraitTemplate> trait = std::make_unique<TraitTemplate>();
			trait->ast_node = n.get();
			trait->parent = into;

			if (into->name_table.find(n->name_string) != into->name_table.end()) {
				Cursor c = load_cursor(n->name, n->get_source());
				throw_specific_error(c, "Name already exists in the namespace");
			}
			into->name_table[n->name_string] = std::make_pair<uint8_t, uint32_t>(3, (uint32_t)into->subtraits.size());
			into->subtraits.push_back(std::move(trait));
		}

		for (auto&& n : statics) {
			std::unique_ptr<StaticInstance> substatic = std::make_unique<StaticInstance>();
			substatic->ast_node = n.get();
			substatic->parent = into;

			if (into->name_table.find(n->name_string) != into->name_table.end()) {
				Cursor c = load_cursor(n->name, n->get_source());
				throw_specific_error(c, "Name already exists in the namespace");
			}
			into->name_table[n->name_string] = std::make_pair<uint8_t, uint32_t>(4, (uint32_t)into->substatics.size());
			into->substatics.push_back(std::move(substatic));
		}
	}


	std::unique_ptr<AstStaticNode> AstStaticNode::parse(Cursor& c, AstNode* parent) {
		auto svar = std::make_unique<AstStaticNode>();
		svar->parent = parent;
		c.move();

		auto buf = c.buffer();
		if (buf == "compile") {
			svar->context = ILContext::compile;
			c.move();
		}
		else if (buf == "runtime") {
			svar->context = ILContext::runtime;
			c.move();
		}
		else {
			svar->context = ILContext::both;
		}

		if (c.tok != RecognizedToken::Symbol) {
			throw_not_a_name_error(c);
		}
		svar->name = c.offset;
		svar->name_string = c.buffer();
		c.move();


		if (c.tok != RecognizedToken::Colon && c.tok != RecognizedToken::Equals) {
			throw_wrong_token_error(c, "':' or '='");
		}
		svar->has_value = c.tok == RecognizedToken::Equals;
		c.move();
		svar->type = c.offset;

		while (c.tok != RecognizedToken::Semicolon) {
			if (c.tok == RecognizedToken::Eof) {
				throw_eof_error(c, "parsing of static declaration");
			}
			c.move();
		}
		c.move();

		return std::move(svar);
	}
}