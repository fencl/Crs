#include "PredefinedTypes.h"
#include "Contents.h"

namespace Corrosive {

	const char* PredefinedNamespace = "corrosive";

	void DefaultTypes::setup_type(CompileContext& ctx,std::string_view name,Type& into,ILType* t,ILDataType ildt) {
		std::unique_ptr<Structure> s = std::make_unique<Structure>();
		Cursor c;
		c.buffer = name;
		s->rvalue_stacked = false;
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
		setup_type(ctx, "i8", t_i8, ctx.module->t_i8, ILDataType::i8);
		setup_type(ctx, "bool", t_bool, ctx.module->t_bool, ILDataType::ibool);
		setup_type(ctx, "i16", t_i16, ctx.module->t_i16, ILDataType::i16);
		setup_type(ctx, "i32", t_i32, ctx.module->t_i32, ILDataType::i32);
		setup_type(ctx, "i64", t_i64, ctx.module->t_i64, ILDataType::i64);
		setup_type(ctx, "u8",  t_u8, ctx.module->t_u8, ILDataType::u8);
		setup_type(ctx, "u16", t_u16, ctx.module->t_u16, ILDataType::u16);
		setup_type(ctx, "u32", t_u32, ctx.module->t_u32, ILDataType::u32);
		setup_type(ctx, "u64", t_u64, ctx.module->t_u64, ILDataType::u64);
		setup_type(ctx, "f32", t_f32, ctx.module->t_f32, ILDataType::f32);
		setup_type(ctx, "f64", t_f64, ctx.module->t_f64, ILDataType::f64);

		setup_type(ctx, "type", t_type,ctx.module->t_type,ILDataType::ctype);
	}

}