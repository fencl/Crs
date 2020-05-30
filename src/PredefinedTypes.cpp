#include "PredefinedTypes.h"

namespace Corrosive {

	const char* PredefinedNamespace = "corrosive";

	bool DefaultTypes::priv_debug_cursor(ILEvaluator* eval_ctx) {
		CompileContext& nctx = CompileContext::get();
		uint64_t c_id = eval_ctx->pop_register_value<uint64_t>();
		nctx.default_types->debug_info = nctx.default_types->debug_cursor_storage.get((size_t)c_id);
		return true;
	}

	void DefaultTypes::setup_type(CompileContext& ctx,std::string_view name,Type*& into, ILSize size, ILSize alignment,ILDataType ildt,ILContext context) {
		std::unique_ptr<StructureTemplate> s = std::make_unique<StructureTemplate>();
		Cursor c;
		c.buffer = name;

		s->parent = ctx.global;
		s->template_parent = nullptr;
		s->name = c;
		s->parent = ctx.global;

		s->compile();
		StructureInstance* sinst;
		s->generate(nullptr, sinst);
		sinst->compile();

		sinst->context = context;
		sinst->size = size;
		sinst->alignment = alignment;
		sinst->rvalue = ildt;
		sinst->structure_type = StructureInstanceType::primitive_structure;

		into = sinst->type.get();

		ctx.global->subtemplates[c.buffer] = std::move(s);
	}


	Type* DefaultTypes::get_type_from_rvalue(ILDataType rval) {
		return primitives[(unsigned char)rval];
	}

	bool DefaultTypes::setup(CompileContext& ctx) {
		setup_type(ctx, "void", t_void, { 0,0 }, { 0, 0 }, ILDataType::none, ILContext::both);
		setup_type(ctx, "i8", t_i8, { 1,0 }, { 1, 0 }, ILDataType::i8, ILContext::both);
		setup_type(ctx, "bool", t_bool, { 1,0 }, { 1, 0 }, ILDataType::ibool, ILContext::both);
		setup_type(ctx, "i16", t_i16, { 2,0 }, { 2, 0 }, ILDataType::i16, ILContext::both);
		setup_type(ctx, "i32", t_i32, { 4,0 }, { 4, 0 }, ILDataType::i32, ILContext::both);
		setup_type(ctx, "u8",  t_u8, { 1,0 }, { 1, 0 }, ILDataType::u8, ILContext::both);
		setup_type(ctx, "u16", t_u16, { 2,0 }, { 2, 0 }, ILDataType::u16, ILContext::both);
		setup_type(ctx, "u32", t_u32, { 4,0 }, { 4, 0 }, ILDataType::u32, ILContext::both);
		setup_type(ctx, "f32", t_f32, { 4,0 }, { 4, 0 }, ILDataType::f32, ILContext::both);

		setup_type(ctx, "f64", t_f64, { 8,0 }, { 0, 1 }, ILDataType::f64, ILContext::both);
		setup_type(ctx, "i64", t_i64, { 8,0 }, { 0, 1 }, ILDataType::i64, ILContext::both);
		setup_type(ctx, "u64", t_u64, { 8,0 }, { 0, 1 }, ILDataType::u64, ILContext::both);
		setup_type(ctx, "ptr", t_ptr, { 0,1 }, { 0, 1 }, ILDataType::ptr, ILContext::both);
		setup_type(ctx, "size", t_size, { 0,1 }, { 0, 1 }, ILDataType::size, ILContext::both);
		setup_type(ctx, "type", t_type, { 0,1 }, { 0, 1 }, ILDataType::ptr, ILContext::compile);

		primitives[(unsigned char)ILDataType::ibool] = t_bool;
		primitives[(unsigned char)ILDataType::u8] = t_u8;
		primitives[(unsigned char)ILDataType::u16] = t_u16;
		primitives[(unsigned char)ILDataType::u32] = t_u32;
		primitives[(unsigned char)ILDataType::u64] = t_u64;
		primitives[(unsigned char)ILDataType::i8] = t_i8;
		primitives[(unsigned char)ILDataType::i16] = t_i16;
		primitives[(unsigned char)ILDataType::i32] = t_i32;
		primitives[(unsigned char)ILDataType::i64] = t_i64;
		primitives[(unsigned char)ILDataType::f32] = t_f32;
		primitives[(unsigned char)ILDataType::f64] = t_f64;
		primitives[(unsigned char)ILDataType::size] = t_size;

		auto f_malloc_template = std::make_unique<FunctionTemplate>();
		f_malloc_template->name.buffer = "malloc";
		f_malloc_template->is_generic = false;
		f_malloc_template->parent = ctx.global;
		f_malloc_template->singe_instance = std::make_unique<FunctionInstance>();
		f_malloc_template->singe_instance->parent = ctx.global;
		f_malloc_template->singe_instance->returns.second = t_u8->generate_slice();
		Cursor arg;
		arg.buffer = "size";
		f_malloc_template->singe_instance->arguments.push_back(std::make_pair(arg,t_size));

		f_malloc_template->compile_state = 2;
		f_malloc_template->singe_instance->compile_state = 3;
		f_malloc_template->singe_instance->func = ctx.module->create_function();
		f_malloc_template->singe_instance->func->alias = "malloc";
		std::vector<Type*> arg_array = {t_size};

		TypeFunction* tf = load_or_register_function_type(std::move(arg_array), f_malloc_template->singe_instance->returns.second,ILContext::both);
		f_malloc_template->singe_instance->type = tf;

		ILBlock* f_malloc_block = f_malloc_template->singe_instance->func->create_and_append_block(ILDataType::none);
		ILBuilder::build_insintric(f_malloc_block, ILInsintric::malloc);
		ILBuilder::build_ret(f_malloc_block, ILDataType::none);

		ctx.global->subfunctions["malloc"] = std::move(f_malloc_template);


		std_lib.load_data("trait Copy(T:type) {fn Copy: (&T);}\ntrait Move(T:type) {fn Move: (&T);}\ntrait Compare(T:type) {fn Compare: (&T) i8;}");
		Cursor c = std_lib.read_first();
		if (!Declaration::parse_global(c, ctx.global)) return false;

		tr_copy = ctx.global->subtraits["Copy"].get();
		tr_move = ctx.global->subtraits["Move"].get();
		tr_compare = ctx.global->subtraits["Compare"].get();

		return true;
	}




	size_t DefaultTypes::load_or_register_argument_array(std::vector<Type*> arg_array) {
		return argument_array_storage.register_or_load(std::move(arg_array));
	}

	size_t DefaultTypes::load_or_register_debug_cursor(Cursor c) {
		return debug_cursor_storage.register_or_load(c);
	}

	TypeFunction* DefaultTypes::load_or_register_function_type(std::vector<Type*> arg_array, Type* return_type, ILContext ctx) {
		size_t arg_id = load_or_register_argument_array(std::move(arg_array));

		std::tuple<size_t, Type*,ILContext> key = std::make_tuple(arg_id, return_type, ctx);
		auto t_find = function_types_storage.find(key);
		if (t_find != function_types_storage.end()) {
			return t_find->second.get();
		}
		else {
			std::unique_ptr<TypeFunction> tf = std::make_unique<TypeFunction>();
			tf->argument_array_id = arg_id;
			tf->owner = this;
			tf->return_type = return_type;
			tf->ptr_context = ctx;
			TypeFunction* ret = tf.get();
			function_types_storage[key] = std::move(tf);
			return ret;
		}
	}

	TypeTemplate* DefaultTypes::load_or_register_template_type(std::vector<Type*> arg_array) {
		size_t arg_id = load_or_register_argument_array(std::move(arg_array));

		size_t key = arg_id;

		auto t_find = template_types_storage.find(key);
		if (t_find != template_types_storage.end()) {
			return t_find->second.get();
		}
		else {
			std::unique_ptr<TypeTemplate> tf = std::make_unique<TypeTemplate>();
			tf->argument_array_id = arg_id;
			tf->owner = this;
			TypeTemplate* ret = tf.get();
			template_types_storage[key] = std::move(tf);
			return ret;
		}
	}
}