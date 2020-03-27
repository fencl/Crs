#include "PredefinedTypes.h"
#include "Contents.h"

namespace Corrosive {

	const char* PredefinedNamespace = "corrosive";

	void DefaultTypes::setup_type(CompileContext& ctx,std::string_view name,Type*& into,unsigned int runtime_size,unsigned int runtime_alignment,ILDataType ildt) {
		std::unique_ptr<StructureTemplate> s = std::make_unique<StructureTemplate>();
		Cursor c;
		c.buffer = name;
		//s->namespace_type = NamespaceType::t_struct_template;

		s->parent = ctx.global;
		s->template_parent = nullptr;
		s->rvalue_stacked = false;
		s->name = c;
		s->parent = ctx.global;


		s->compile_state = 2;
		s->singe_instance = std::make_unique<StructureInstance>();
		s->singe_instance->parent = ctx.global;
		s->singe_instance->name = c;
		s->singe_instance->namespace_type = NamespaceType::t_struct_instance;
		s->singe_instance->size = runtime_size;
		s->singe_instance->alignment = runtime_alignment;
		s->singe_instance->generator = s.get();
		s->singe_instance->key = nullptr;
		s->singe_instance->compile_state = 2;
		s->singe_instance->type = std::make_unique<TypeInstance>();
		s->singe_instance->type->owner = s->singe_instance.get();
		s->singe_instance->type->rvalue = ildt;

		into = s->singe_instance->type.get();

		ctx.global->subtemplates[c.buffer] = std::move(s);
	}

	void DefaultTypes::setup(CompileContext& ctx) {
		setup_type(ctx, "void", t_void, 0,0, ILDataType::none);
		setup_type(ctx, "i8", t_i8, 1,1, ILDataType::i8);
		setup_type(ctx, "bool", t_bool, 1, 1, ILDataType::ibool);
		setup_type(ctx, "i16", t_i16, 2, 2, ILDataType::i16);
		setup_type(ctx, "i32", t_i32, 4, 4, ILDataType::i32);
		setup_type(ctx, "u8",  t_u8, 1, 1, ILDataType::u8);
		setup_type(ctx, "u16", t_u16, 2, 2, ILDataType::u16);
		setup_type(ctx, "u32", t_u32, 4, 4, ILDataType::u32);
		setup_type(ctx, "f32", t_f32, 4, 4, ILDataType::f32);
		setup_type(ctx, "f64", t_f64, 8, 8, ILDataType::f64);

		if (ctx.module->architecture == ILArchitecture::i386) {
			setup_type(ctx, "i64", t_i64, 8, 4, ILDataType::u64);
			setup_type(ctx, "u64", t_u64, 8, 4, ILDataType::i64);
			setup_type(ctx, "ptr", t_ptr, 4, 4, ILDataType::ptr);
			setup_type(ctx, "type", t_type, sizeof(void*), sizeof(void*), ILDataType::type);
		}
		else if (ctx.module->architecture == ILArchitecture::x86_64) {
			setup_type(ctx, "i64", t_i64, 8, 8, ILDataType::u64);
			setup_type(ctx, "u64", t_u64, 8, 8, ILDataType::i64);
			setup_type(ctx, "ptr", t_ptr, 8, 8, ILDataType::ptr);
			setup_type(ctx, "type", t_type, sizeof(void*), sizeof(void*), ILDataType::type);
		}
	}

}