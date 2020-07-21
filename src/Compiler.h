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
		Compiler();

		ILEvaluator* evaluator() { return e.get(); }
		ILBlock* scope() { return s.back(); }
		ILBlock* scope_exit() { return se.back(); }
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

		void push_scope_exit(ILBlock* scexit) { se.push_back(scexit); }
		void pop_scope_exit() { se.pop_back(); }

		void push_workspace(Namespace* nspc) { nm.push_back(nspc); }
		void pop_workspace() { nm.pop_back(); }

		void push_function(ILBytecodeFunction* t, Type* rtt) { wf.push_back(t); rt.push_back(rtt); }
		void pop_function() { wf.pop_back(); rt.pop_back(); }

		std::vector<ILBlock*> s;
		std::vector<ILBlock*> se;
		std::vector<Type*> rt;
		std::vector<ILContext> sc;
		std::vector<Namespace*> nm;
		std::vector<ILBytecodeFunction*> wf;

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
	};
}
#endif