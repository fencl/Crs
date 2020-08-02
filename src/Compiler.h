#pragma once
#ifndef _compiler_crs_h
#define _compiler_crs_h

#include "IL/IL.h"
#include "PredefinedTypes.h"
#include "StackManager.h"
#include "ConstantManager.h"
#include "Declaration.h"

namespace Corrosive {
	class Compiler {
	public:
		bool initialized = false;

		ILEvaluator* evaluator() { return e.get(); }
		ILBlock* scope() { return s.back(); }
		ILBlock* loop_break() { return lb.back().first; }
		ILBlock* loop_continue() { return lb.back().second; }
		bool has_loop() { return lb.size() > 0; }
		DefaultTypes* types() { return &dt; }
		ILContext scope_context() { return sc.back(); }
		Namespace* workspace() { return nm.back(); }
		StackManager* stack() { return &rts; }
		StackManager* compiler_stack() { return &cps; }
		StackManager* temp_stack() { return &tms; }
		Namespace* global_namespace() { return &gn; }
		ILModule* global_module() { return &m; }
		ConstantManager* constant_manager() { return &cmgr; }

		ILBytecodeFunction* target() { return wf.back(); };
		Type* return_type() { return rt.back(); }

		void push_scope_context(ILContext ctx) { sc.push_back(ctx); }
		void pop_scope_context() { sc.pop_back(); }

		void push_scope(ILBlock* sc) { s.push_back(sc); }
		void pop_scope() { s.pop_back(); }

		void push_workspace(Namespace* nspc) { nm.push_back(nspc); }
		void pop_workspace() { nm.pop_back(); }

		void push_function(ILBytecodeFunction* t, Type* rtt) { wf.push_back(t); rt.push_back(rtt); }
		void pop_function() { wf.pop_back(); rt.pop_back(); }

		void push_loop_blocks(ILBlock* break_b, ILBlock* continue_b) { lb.push_back(std::make_pair(break_b, continue_b)); }
		void pop_loop_blocks() { lb.pop_back(); }

		void switch_scope(ILBlock* sblock) { s.back() = sblock; }

		void push_source(Source* s) { src.push_back(s); }
		void pop_source() { src.pop_back(); }
		Source* source() { return src.back(); }
		void setup();

		std::vector<ILBlock*> s;
		std::vector<Type*> rt;
		std::vector<std::pair<ILBlock*,ILBlock*>> lb;
		std::vector<ILContext> sc;
		std::vector<Namespace*> nm;
		std::vector<ILBytecodeFunction*> wf;
		std::vector<Source*> src;

		std::map<std::filesystem::path, std::unique_ptr<Source>> included_sources;

		ILModule m;
		DefaultTypes dt;
		Namespace gn;
		std::unique_ptr<ILEvaluator> e = std::make_unique<ILEvaluator>();
		StackManager rts;
		StackManager cps;
		StackManager tms;
		ConstantManager cmgr;

		FunctionInstance* register_ext_function(std::initializer_list<const char*> path, void(*ptr)(ILEvaluator*));

		static thread_local std::vector<Compiler*> c;
		static void push_compiler(Compiler* c);
		static void pop_compiler();
		static Compiler* current();
		static std::unique_ptr<Compiler> create();
	};
}
#endif