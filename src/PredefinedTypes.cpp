#include "PredefinedTypes.h"
#include "Contents.h"

namespace Corrosive {

	const char* PredefinedNamespace = "corrosive";

	void DefaultTypes::setup_type(CompileContext& ctx,std::string_view name,Type& into,ILType* t,ILDataType ildt) {
		std::unique_ptr<Structure> s = std::make_unique<Structure>();
		Cursor c;
		c.buffer = name;
		s->name = c;
		s->parent = ctx.global;
		s->compile_state = 2;
		s->singe_instance = std::make_unique<StructureInstance>();
		s->singe_instance->iltype = t;
		s->singe_instance->generator = s.get();
		s->singe_instance->compile_state = 2;
		s->singe_instance->type = std::make_unique<InstanceType>();
		s->singe_instance->type->owner = s->singe_instance.get();
		s->singe_instance->type->rvalue = ildt;

		into.type = s->singe_instance->type.get();
		into.ref_count = 0;

		ctx.global->subnamespaces[c.buffer] = std::move(s);
	}

	void DefaultTypes::setup(CompileContext& ctx) {
		setup_type(ctx, "i32", t_i32,ctx.module->t_i32,ILDataType::i32);
		setup_type(ctx, "type", t_type,ctx.module->t_type,ILDataType::ctype);
	}

}