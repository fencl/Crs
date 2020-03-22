#include "PredefinedTypes.h"
#include "Contents.h"

namespace Corrosive {

	const char* PredefinedNamespace = "corrosive";

	void DefaultTypes::setup_type(CompileContext& ctx,std::string_view name,Type& into,ILType* t,ILDataType ildt) {
		std::unique_ptr<StructureTemplate> s = std::make_unique<StructureTemplate>();
		Cursor c;
		c.buffer = name;
		//s->namespace_type = NamespaceType::t_struct_template;

		s->parent = ctx.global;
		s->template_parent = nullptr;
		s->rvalue_stacked = false;
		//s->name = c;
		//s->parent = ctx.global;
		s->compile_state = 2;
		s->singe_instance = std::make_unique<StructureInstance>();
		s->singe_instance->parent = ctx.global;
		s->singe_instance->name = c;
		s->singe_instance->namespace_type = NamespaceType::t_struct_instance;
		s->singe_instance->iltype = t;
		s->singe_instance->generator = s.get();
		s->singe_instance->key = nullptr;
		s->singe_instance->compile_state = 2;
		s->singe_instance->type = std::make_unique<TypeInstance>();
		s->singe_instance->type->type = TypeInstanceType::type_instance;
		s->singe_instance->type->owner_ptr = (void*)s->singe_instance.get();
		s->singe_instance->type->rvalue = ildt;

		into.type = s->singe_instance->type.get();
		into.ref_count = 0;

		ctx.global->subtemplates[c.buffer] = std::move(s);
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
		setup_type(ctx, "ptr", t_ptr,ctx.module->t_ptr,ILDataType::ptr);
		setup_type(ctx, "type", t_type,ctx.module->t_type,ILDataType::ctype);
	}

}