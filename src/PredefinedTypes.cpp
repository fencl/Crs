#include "PredefinedTypes.h"

namespace Corrosive {

	const char* PredefinedNamespace = "corrosive";

	void DefaultTypes::setup_type(CompileContext& ctx,std::string_view name,Type*& into, uint32_t runtime_size, uint32_t runtime_alignment, uint32_t compile_size, uint32_t compile_alignment,ILDataType ildt) {
		std::unique_ptr<StructureTemplate> s = std::make_unique<StructureTemplate>();
		Cursor c;
		c.buffer = name;
		//s->namespace_type = NamespaceType::t_struct_template;

		s->parent = ctx.global;
		s->template_parent = nullptr;
		s->rvalue_stacked = false;
		s->name = c;
		s->parent = ctx.global;

		s->compile(ctx);
		StructureInstance* sinst;
		s->generate(ctx, nullptr, sinst);
		sinst->compile(ctx);
		
		sinst->size = runtime_size;
		sinst->alignment = runtime_alignment;
		sinst->compile_size = compile_size;
		sinst->compile_alignment = compile_alignment;
		s->singe_instance->type->rvalue = ildt;

		into = sinst->type.get();

		ctx.global->subtemplates[c.buffer] = std::move(s);
	}

	void DefaultTypes::setup(CompileContext& ctx) {
		setup_type(ctx, "void", t_void, 0,0,0,0, ILDataType::none);
		setup_type(ctx, "i8", t_i8, 1,1,1,1, ILDataType::i8);
		setup_type(ctx, "bool", t_bool, 1, 1, 1, 1, ILDataType::ibool);
		setup_type(ctx, "i16", t_i16, 2, 2,2, 2, ILDataType::i16);
		setup_type(ctx, "i32", t_i32, 4, 4,4, 4, ILDataType::i32);
		setup_type(ctx, "u8",  t_u8, 1, 1,1, 1, ILDataType::u8);
		setup_type(ctx, "u16", t_u16, 2, 2,2, 2, ILDataType::u16);
		setup_type(ctx, "u32", t_u32, 4, 4,4, 4, ILDataType::u32);
		setup_type(ctx, "f32", t_f32, 4, 4, sizeof(float), alignof(float), ILDataType::f32);
		setup_type(ctx, "f64", t_f64, 8, 8, sizeof(double), alignof(double), ILDataType::f64);

		if (ctx.module->architecture == ILArchitecture::i386) {
			setup_type(ctx, "i64", t_i64, 8, 4, 8, alignof(int64_t), ILDataType::i64);
			setup_type(ctx, "u64", t_u64, 8, 4, 8, alignof(uint64_t), ILDataType::u64);
			setup_type(ctx, "ptr", t_ptr, 4, 4,sizeof(void*),alignof(void*), ILDataType::ptr);
			setup_type(ctx, "type", t_type, sizeof(void*), alignof(void*), sizeof(void*), alignof(void*), ILDataType::type);
		}
		else if (ctx.module->architecture == ILArchitecture::x86_64) {
			setup_type(ctx, "i64", t_i64, 8, 8, 8, alignof(int64_t), ILDataType::i64);
			setup_type(ctx, "u64", t_u64, 8, 8, 8, alignof(uint64_t), ILDataType::u64);
			setup_type(ctx, "ptr", t_ptr, 8, 8, sizeof(void*), alignof(void*), ILDataType::ptr);
			setup_type(ctx, "type", t_type, sizeof(void*), alignof(void*), sizeof(void*), alignof(void*), ILDataType::type);
		}
	}




	size_t DefaultTypes::load_or_register_argument_array(std::vector<Type*> arg_array) {
		return argument_array_storage.register_or_load(std::move(arg_array));
	}

	TypeFunction* DefaultTypes::load_or_register_function_type(std::vector<Type*> arg_array, Type* return_type) {
		size_t arg_id = load_or_register_argument_array(std::move(arg_array));

		std::pair<size_t, Type*> key = std::make_pair(arg_id, return_type);
		auto t_find = function_types_storage.find(key);
		if (t_find != function_types_storage.end()) {
			return t_find->second.get();
		}
		else {
			std::unique_ptr<TypeFunction> tf = std::make_unique<TypeFunction>();
			tf->argument_array_id = arg_id;
			tf->owner = this;
			tf->return_type = return_type;
			TypeFunction* ret = tf.get();
			function_types_storage[key] = std::move(tf);
			return ret;
		}
	}
}