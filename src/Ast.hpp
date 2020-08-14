#ifndef  _ast_crs_h
#define  _ast_crs_h
#include "Source.hpp"
#include <memory>
#include <vector>
#include <string_view>

namespace Corrosive {
	using AstCursor = size_t;

	inline Cursor load_cursor(AstCursor c, Source* src) {
		Cursor cr;
		cr.offset = c;
		cr.length = 0;
		cr.src = src;
		cr.move();
		return cr;
	}

	class AstNode {
	public:
		virtual ~AstNode() {}
		virtual Source* get_source() { return nullptr; }
	};

	class AstRegularNode : public AstNode {
	public:
		AstNode* parent;
		virtual Source* get_source() override { return parent->get_source(); }
	};

	class AstVariableNode : public AstRegularNode {
	public:
		std::string_view name_string;
		AstCursor name;
		AstCursor type;
		bool alias;

		static std::unique_ptr<AstVariableNode> parse(Cursor& c, AstNode* parent);
	};

	class AstFunctionDeclarationNode : public AstRegularNode {
	public:
		std::string_view name_string;
		AstCursor name;
		AstCursor type;
		ILContext context;
		ILCallingConvention convention;
		bool exported;
		virtual bool has_body() { return false; }
	};

	class AstFunctionNode : public AstFunctionDeclarationNode {
	public:
		AstCursor annotation;
		bool is_generic;
		AstCursor block;

		std::vector<std::pair<AstCursor, std::string_view>> argument_names;
		AstCursor return_type;

		virtual bool has_body() override { return true; }

		static std::unique_ptr<AstFunctionDeclarationNode> parse(Cursor& c, AstNode* parent, ILContext force_context);
	};

	class AstTraitNode : public AstRegularNode {
	public:
		std::string_view name_string;
		AstCursor name;
		AstCursor annotation;
		bool is_generic;
		ILContext context;
		std::vector<std::unique_ptr<AstFunctionDeclarationNode>> declarations;

		static std::unique_ptr<AstTraitNode> parse(Cursor& c, AstNode* parent);
	};

	class AstImplementationNode : public AstRegularNode {
	public:
		bool fast;
		AstCursor trait;
		std::vector<std::unique_ptr<AstFunctionNode>> functions;

		static std::unique_ptr<AstImplementationNode> parse(Cursor& c, AstNode* parent);
	};


	class AstStaticNode : public AstRegularNode {
	public:
		std::string_view name_string;
		AstCursor name;
		AstCursor type;
		ILContext context;
		bool has_value;

		static std::unique_ptr<AstStaticNode> parse(Cursor& c, AstNode* parent);
	};


	class AstStructureNode : public AstRegularNode {
	public:
		std::string_view name_string;
		AstCursor name;
		
		AstCursor annotation;
		bool is_generic;
		ILContext context;

		std::vector<std::unique_ptr<AstStructureNode>> structures;
		std::vector<std::unique_ptr<AstVariableNode>> variables;
		std::vector<std::unique_ptr<AstTraitNode>> traits;
		std::vector<std::unique_ptr<AstFunctionDeclarationNode>> functions;
		std::vector<std::unique_ptr<AstImplementationNode>> implementations;
		std::vector<std::unique_ptr<AstStaticNode>> statics;
		std::vector<AstCursor> compile_blocks;

		static std::unique_ptr<AstStructureNode> parse(Cursor& c, AstNode* parent);
	};

	class Compiler;
	class Namespace;
	class AstNamedNamespaceNode;
	class AstNamespaceNode : public AstRegularNode {
	public:
		std::vector<std::unique_ptr<AstNamedNamespaceNode>> namespaces;
		std::vector<std::unique_ptr<AstStructureNode>> structures;
		std::vector<std::unique_ptr<AstTraitNode>> traits;
		std::vector<std::unique_ptr<AstFunctionDeclarationNode>> functions;
		std::vector<std::unique_ptr<AstStaticNode>> statics;

		void populate(Namespace* into);
	};

	class AstNamedNamespaceNode : public AstNamespaceNode {
	public:
		std::string_view name_string;
		AstCursor name;
		static std::unique_ptr<AstNamedNamespaceNode> parse(Cursor& c, AstNode* parent);
	};

	class AstRootNode : public AstNode {
	public:
		std::vector<AstCursor> compile;
		Source* parent;
		virtual Source* get_source() override { return parent; }
		std::unique_ptr<AstNamespaceNode> global_namespace;
		static std::unique_ptr<AstRootNode> parse(Source* src);

		void populate();
	};

	
}

#endif
