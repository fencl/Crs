#include "Ast.h"
#include "Error.h"
#include "Compiler.h"
#include "Declaration.h"

namespace Corrosive {
	std::unique_ptr<AstRootNode> AstRootNode::parse(Source* src) {
		auto root = std::make_unique<AstRootNode>();
		root->parent = src;
		RecognizedToken tok;
		Cursor c = src->read_first(tok);

		auto global = std::make_unique<AstNamespaceNode>();
		global->parent = root.get();

		while (tok!=RecognizedToken::Eof) {

			auto buf = c.buffer();
			if (buf == "namespace") {
				global->namespaces.push_back(AstNamedNamespaceNode::parse(c, tok, global.get()));
			}
			else if (buf == "struct") {
				global->structures.push_back(AstStructureNode::parse(c, tok, global.get()));
			}
			else if (buf == "fn") {
				global->functions.push_back(AstFunctionNode::parse(c, tok, global.get()));
			}
			else if (buf == "trait") {
				global->traits.push_back(AstTraitNode::parse(c, tok, global.get()));
			}
			else if (buf == "require") {
				c.move(tok);
				root->require.push_back(c.offset);
				c.move(tok);

				if (tok != RecognizedToken::Semicolon) {
					throw_wrong_token_error(c, "';'");
				}
				c.move(tok);
			}
			else {
				throw_wrong_token_error(c, "namespace, fn, struct, trait or require");
			}

		}

		root->global_namespace = std::move(global);
		return std::move(root);
	}

	std::unique_ptr<AstNamedNamespaceNode> AstNamedNamespaceNode::parse(Cursor& c, RecognizedToken& tok, AstNode* parent) {
		auto space = std::make_unique<AstNamedNamespaceNode>();
		space->parent = parent;
		c.move(tok);
		space->name = c.offset;
		space->name_string = c.buffer();
		c.move(tok);
		if (tok != RecognizedToken::OpenBrace) {
			throw_wrong_token_error(c, "'{'");
		}
		c.move(tok);

		while (tok != RecognizedToken::CloseBrace) {
			if (tok == RecognizedToken::Eof) {
				throw_eof_error(c, "parsing of namespace");
			}

			auto buf = c.buffer();
			if (buf == "namespace") {
				space->namespaces.push_back(AstNamedNamespaceNode::parse(c, tok, space.get()));
			}
			else if (buf == "struct") {
				space->structures.push_back(AstStructureNode::parse(c, tok, space.get()));
			}
			else if (buf == "fn") {
				space->functions.push_back(AstFunctionNode::parse(c, tok, space.get()));
			}
			else if (buf == "trait") {
				space->traits.push_back(AstTraitNode::parse(c, tok,space.get()));
			}
			else {
				throw_wrong_token_error(c, "namespace, fn, struct or trait");
			}
		}

		c.move(tok);


		return std::move(space);
	}

	std::unique_ptr<AstStructureNode> AstStructureNode::parse(Cursor& c, RecognizedToken& tok, AstNode* parent) {
		auto structure = std::make_unique<AstStructureNode>();
		structure->parent = parent;
		c.move(tok);
		if (tok == RecognizedToken::OpenParenthesis) {
			structure->is_generic = true;
			structure->annotation = c.offset;
			c.move_matching(tok);
			c.move(tok);
		}
		else {
			structure->is_generic = false;
		}

		auto buf = c.buffer();
		if (buf == "compile") {
			structure->context = ILContext::compile;
			c.move(tok);
		}
		else if (buf == "runtime") {
			structure->context = ILContext::runtime;
			c.move(tok);
		}
		else {
			structure->context = ILContext::both;
		}

		structure->name = c.offset;
		structure->name_string = c.buffer();
		c.move(tok);
		if (tok != RecognizedToken::OpenBrace) {
			throw_wrong_token_error(c, "'{'");
		}
		c.move(tok);

		while (tok != RecognizedToken::CloseBrace) {
			if (tok == RecognizedToken::Eof) {
				throw_eof_error(c, "parsing of structure");
			}

			auto buf = c.buffer();
			if (buf == "struct") {
				structure->structures.push_back(AstStructureNode::parse(c, tok, structure.get()));
			}
			else if (buf == "fn") {
				Cursor err = c;
				auto fun = AstFunctionNode::parse(c, tok, structure.get());
				if (!fun->has_body()) {
					throw_specific_error(err, "Functions inside structures are supposed to have a body");
				}

				structure->functions.push_back(std::move(fun));
			}
			else if (buf == "var") {
				structure->variables.push_back(AstVariableNode::parse(c, tok, structure.get()));
			}
			else if (buf == "trait") {
				structure->traits.push_back(AstTraitNode::parse(c, tok,structure.get()));
			}
			else if (buf == "impl") {
				structure->implementations.push_back(AstImplementationNode::parse(c, tok, structure.get()));
			}
			else {
				throw_wrong_token_error(c, "var, fn, struct, impl or trait");
			}
		}

		c.move(tok);


		return std::move(structure);
	}


	std::unique_ptr<AstFunctionDeclarationNode> AstFunctionNode::parse(Cursor& c, RecognizedToken& tok, AstNode* parent) {
		c.move(tok);
		bool is_generic = false;
		AstCursor annotation = 0;
		if (tok == RecognizedToken::OpenParenthesis) {
			is_generic = true;
			annotation = c.offset;
			c.move_matching(tok);
			c.move(tok);
		}

		bool ext = false;
		ILCallingConvention convention = ILCallingConvention::bytecode;
		ILContext context = ILContext::both;
		while (true) {
			auto buf = c.buffer();
			if (buf == "compile") {
				context = ILContext::compile;
				c.move(tok);
			}
			else if (buf == "runtime") {
				context = ILContext::runtime;
				c.move(tok);
			}
			else if (buf == "ext") {
				ext = true;
				c.move(tok);
			}
			else if (buf == "native") {
				convention = ILCallingConvention::native;
				c.move(tok);
			}
			else if (buf == "stdcall") {
				convention = ILCallingConvention::stdcall;
				c.move(tok);
			}
			else break;
		}
		
		AstCursor name = c.offset;
		std::string_view name_string = c.buffer();
		c.move(tok);
		if (tok != RecognizedToken::Colon) {
			throw_wrong_token_error(c, "':'");
		}
		c.move(tok);
		if (tok != RecognizedToken::OpenParenthesis) {
			throw_wrong_token_error(c, "'('");
		}
		AstCursor type = c.offset;
		c.move_matching(tok);
		c.move(tok);
		// TODO: this is dumb
		while (tok != RecognizedToken::OpenBrace && tok != RecognizedToken::Semicolon) {
			if (tok == RecognizedToken::Eof) {
				throw_eof_error(c, "parsing of function type");
			}
			c.move(tok);
		}

		if (tok == RecognizedToken::OpenBrace) {
			auto fun = std::make_unique<AstFunctionNode>();
			fun->annotation = annotation;
			fun->is_generic = is_generic;
			fun->block = c.offset;
			fun->context = context;
			fun->convention = convention;
			fun->type = type;
			fun->name_string = name_string;
			fun->parent = parent;
			c.move_matching(tok);
			c.move(tok);
			return std::move(fun);
		}
		else {
			auto fun = std::make_unique<AstFunctionDeclarationNode>();
			fun->context = context;
			fun->convention = convention;
			fun->type = type;
			fun->name_string = name_string;
			fun->parent = parent;
			c.move(tok);
			return std::move(fun);
		}
	}


	std::unique_ptr<AstVariableNode> AstVariableNode::parse(Cursor& c, RecognizedToken& tok, AstNode* parent) {
		auto var = std::make_unique<AstVariableNode>();
		
		c.move(tok);
		
		if (c.buffer() == "alias") {
			var->alias = true;
			c.move(tok);
		}
		else {
			var->alias = false;
		}

		var->name = c.offset;
		var->name_string = c.buffer();
		c.move(tok);

		if (tok != RecognizedToken::Colon) {
			throw_wrong_token_error(c, "':'");
		}
		c.move(tok);
		var->type = c.offset;
		// TODO this is maybe also dumb
		while (tok != RecognizedToken::Semicolon) {
			switch (tok)
			{
				case RecognizedToken::Eof:
					throw_eof_error(c, "parsing variable type"); break;
				case RecognizedToken::OpenParenthesis:
				case RecognizedToken::OpenBrace:
					c.move_matching(tok);
				default:
					c.move(tok);
					break;
			}
		}
		c.move(tok);
		return std::move(var);
	}


	std::unique_ptr<AstTraitNode> AstTraitNode::parse(Cursor& c, RecognizedToken& tok, AstNode* parent) {
		auto trait = std::make_unique<AstTraitNode>();
		trait->parent = parent;
		c.move(tok);
		if (tok == RecognizedToken::OpenParenthesis) {
			trait->is_generic = true;
			trait->annotation = c.offset;
			c.move_matching(tok);
			c.move(tok);
		}
		else {
			trait->is_generic = false;
		}

		auto buf = c.buffer();
		if (buf == "compile") {
			trait->context = ILContext::compile;
			c.move(tok);
		}
		else if (buf == "runtime") {
			trait->context = ILContext::runtime;
			c.move(tok);
		}
		else {
			trait->context = ILContext::both;
		}


		trait->name = c.offset;
		trait->name_string = c.buffer();
		c.move(tok);

		if (tok != RecognizedToken::OpenBrace) {
			throw_wrong_token_error(c, "'{'");
		}
		c.move(tok);


		while (tok != RecognizedToken::CloseBrace) {
			if (tok == RecognizedToken::Eof) {
				throw_eof_error(c, "parsing of trait");
			}

			auto buf = c.buffer();
			if (buf == "fn") {
				trait->declarations.push_back(AstFunctionNode::parse(c, tok, trait.get()));
			}
			else {
				throw_wrong_token_error(c, "fn");
			}
		}

		c.move(tok);


		return std::move(trait);
	}


	std::unique_ptr<AstImplementationNode> AstImplementationNode::parse(Cursor& c, RecognizedToken& tok, AstNode* parent) {
		auto impl = std::make_unique<AstImplementationNode>();
		impl->parent = parent;
		c.move(tok);
		impl->trait = c.offset;
		// TODO maybe put type into parenthesies?
		while (tok != RecognizedToken::OpenBrace && tok!= RecognizedToken::Colon) {
			if (tok == RecognizedToken::Eof) {
				throw_eof_error(c, "parsing of implementation type");
			}

			c.move(tok);
		}
		if (tok == RecognizedToken::OpenBrace) {
			impl->fast = false;
			c.move(tok);
		


			while (tok != RecognizedToken::CloseBrace) {
				if (tok == RecognizedToken::Eof) {
					throw_eof_error(c, "parsing of trait");
				}

				auto buf = c.buffer();
				if (buf == "fn") {
					Cursor err = c;
					auto fun = AstFunctionNode::parse(c, tok,impl.get());
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

			c.move(tok);
		}
		else {
			impl->fast = true;
			c.move(tok);
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
				c.move(tok);
			}
			else if (buf == "runtime") {
				fundecl->context = ILContext::runtime;
				c.move(tok);
			}
			else {
				fundecl->context = ILContext::both;
			}

			c.move_matching(tok);
			c.move(tok);
			while (tok != RecognizedToken::OpenBrace) {
				if (tok == RecognizedToken::Eof) {
					throw_eof_error(c, "parsing of function declaration");
				}
				c.move(tok);
			}
			fundecl->block = c.offset;
			c.move_matching(tok);
			c.move(tok);
			impl->functions.push_back(std::move(fundecl));
		}

		return std::move(impl);
	}


	void AstRootNode::populate(Compiler* compiler) {
		global_namespace->populate(compiler,compiler->global_namespace());
	}


	void AstNamespaceNode::populate(Compiler* compiler, Namespace* into) {
		for (auto&& n : namespaces) {
			std::unique_ptr<Namespace> nspc = std::make_unique<Namespace>();
			nspc->ast_node = n.get();
			nspc->parent = into;

			into->name_table[n->name_string] = std::make_pair<uint8_t,uint32_t>(0, (uint32_t)into->subnamespaces.size());
			n->populate(compiler, nspc.get());
			into->subnamespaces.push_back(std::move(nspc));

		}

		for (auto&& n : structures) {
			std::unique_ptr<StructureTemplate> structure = std::make_unique<StructureTemplate>();
			structure->ast_node = n.get();
			structure->parent = into;
			structure->compiler = compiler;

			into->name_table[n->name_string] = std::make_pair<uint8_t, uint32_t>(1, (uint32_t)into->subtemplates.size());
			into->subtemplates.push_back(std::move(structure));
		}

		for (auto&& n : functions) {
			std::unique_ptr<FunctionTemplate> function = std::make_unique<FunctionTemplate>();
			function->ast_node = n.get();
			function->parent = into;
			function->compiler = compiler;

			into->name_table[n->name_string] = std::make_pair<uint8_t, uint32_t>(2, (uint32_t)into->subfunctions.size());
			into->subfunctions.push_back(std::move(function));
		}

		for (auto&& n : traits) {
			std::unique_ptr<TraitTemplate> trait = std::make_unique<TraitTemplate>();
			trait->ast_node = n.get();
			trait->parent = into;
			trait->compiler = compiler;

			into->name_table[n->name_string] = std::make_pair<uint8_t, uint32_t>(3, (uint32_t)into->subtraits.size());
			into->subtraits.push_back(std::move(trait));
		}
	}
}