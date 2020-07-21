#include "IL.h"
#include <unordered_map>
#include <vector>

#ifdef WINDOWS
#include <Windows.h>
#endif

namespace std {

	template <>
	struct hash<std::tuple<Corrosive::ILDataType, uint32_t, Corrosive::ILDataType*>>
	{
		std::size_t operator()(const std::tuple<Corrosive::ILDataType, uint32_t, Corrosive::ILDataType*>& k) const
		{
			size_t h = hash<Corrosive::ILDataType>()(std::get<0>(k)) ^ (hash<uint32_t>()(std::get<1>(k)) << 1);
			for (size_t a = 0; a < std::get<1>(k); a++) {
				h ^= hash<Corrosive::ILDataType>()(std::get<2>(k)[a]) << a;
			}
			return h;
		}
	};

	bool operator==(const std::tuple<Corrosive::ILDataType, uint32_t, Corrosive::ILDataType*>& l, const std::tuple<Corrosive::ILDataType, uint32_t, Corrosive::ILDataType*>& r) {
		if (std::get<0>(l) != std::get<0>(r)) return false;
		if (std::get<1>(l) != std::get<1>(r)) return false;

		Corrosive::ILDataType* lp = std::get<2>(l);
		Corrosive::ILDataType* rp = std::get<2>(r);

		for (size_t i = 0; i < std::get<1>(l); ++i) {
			if (*lp != *rp) return false;
			lp++;
			rp++;
		}
		return true;
	}

}

namespace Corrosive {
	std::vector<std::pair<uint8_t*, uint8_t*>> memory_pages;

	void* exec_alloc(size_t bytes) {
		if (memory_pages.size() == 0) {
			uint8_t* mem = (uint8_t*)VirtualAlloc(nullptr, 4096, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
			memory_pages.push_back(std::make_pair(mem,mem));
		}

		auto& lmp = memory_pages.back();
		if (4096 - (size_t)(lmp.second-lmp.first) >= bytes ) {
			auto r = lmp.second;
			lmp.second += bytes;
			return r;
		}
		else {
			uint8_t* mem = (uint8_t*)VirtualAlloc(nullptr, 4096, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
			memory_pages.push_back(std::make_pair(mem, mem + bytes));
			return mem;
		}
	}

	std::unordered_map<std::tuple<ILDataType, uint32_t, ILDataType*>, void*> call_wrappers[(uint8_t)ILCallingConvention::__max];

	uint32_t b4_stack[256];
	uint32_t* b4_stack_ptr = b4_stack;
	size_t return_storage_1;
	size_t return_storage_2;

	void push_32bit_temp_stack(ILEvaluator* eval,std::tuple<ILCallingConvention,ILDataType, std::vector<ILDataType>>& decl) {
		size_t off = 0;
		for (size_t i = std::get<2>(decl).size(); i > 0; --i) {
			switch (std::get<2>(decl)[i - 1])
			{
				case ILDataType::i8:
				case ILDataType::u8:
					b4_stack[off++] = eval->pop_register_value<uint8_t>(); break;
				case ILDataType::i16:
				case ILDataType::u16:
					b4_stack[off++] = eval->pop_register_value<uint16_t>(); break;
				case ILDataType::i32:
				case ILDataType::u32:
				case ILDataType::f32:
				case ILDataType::word:
					b4_stack[off++] = eval->pop_register_value<uint32_t>(); break;
				case ILDataType::f64:
				case ILDataType::i64:
				case ILDataType::u64: {
					uint64_t v = eval->pop_register_value<uint64_t>();
					b4_stack[off++] = (uint32_t)(v >> 32);
					b4_stack[off++] = (uint32_t)(v);
					break;
				}

				default:
					break;
			}
		}
	}

	void pop_32bit_temp_stack(ILEvaluator* eval, std::tuple<ILCallingConvention,ILDataType, std::vector<ILDataType>>& decl) {
		switch (std::get<1>(decl)) {
			case ILDataType::i8:
			case ILDataType::u8:
				eval->write_register_value_indirect(sizeof(uint8_t), &return_storage_1); break;

			case ILDataType::i16:
			case ILDataType::u16:
				eval->write_register_value_indirect(sizeof(uint16_t), &return_storage_1); break;

			case ILDataType::i32:
			case ILDataType::u32:
			case ILDataType::f32:
				eval->write_register_value_indirect(sizeof(uint32_t), &return_storage_1); break;

			case ILDataType::i64:
			case ILDataType::u64: {
				uint64_t v = (uint64_t)return_storage_1 | (((uint64_t)return_storage_2) << 32);
				eval->write_register_value_indirect(sizeof(uint64_t), &v);
			} break;
		}
	}

#ifdef WINDOWS


	void* build_win_x86_cdecl_stdcall_call_wrapper(ILCallingConvention conv, std::tuple<ILDataType, uint32_t, ILDataType*> decl) {
		auto& cwrps = call_wrappers[(uint8_t)conv];
		auto f = cwrps.find(decl);
		if (f != cwrps.end()) {
			return f->second;
		}
		else {
			std::vector<uint8_t> call_wrapper;

			uint32_t argc = std::get<1>(decl);
			ILDataType* argv = std::get<2>(decl);
			size_t stack = 0;

			call_wrapper.push_back(0x83);
			call_wrapper.push_back(0xEC);
			call_wrapper.push_back(0); // sub esp, 0


			for (uint32_t i = argc; i > 0; --i) {
				ILDataType dt = argv[i - 1];

				switch (dt) {
					case ILDataType::i64:
					case ILDataType::u64:
					case ILDataType::f64:
						call_wrapper.push_back(0xFF);
						call_wrapper.push_back(0x30); // push [eax]
						call_wrapper.push_back(0x83);
						call_wrapper.push_back(0xC0);
						call_wrapper.push_back(0x04); // add eax, 4
						stack += 4;
						// fallthrough, second byte lower
					case ILDataType::i8:
					case ILDataType::u8:
					case ILDataType::i16:
					case ILDataType::u16:
					case ILDataType::i32:
					case ILDataType::u32:
					case ILDataType::word:
					case ILDataType::f32:
						call_wrapper.push_back(0xFF);
						call_wrapper.push_back(0x30); // push [eax]
						call_wrapper.push_back(0x83);
						call_wrapper.push_back(0xC0);
						call_wrapper.push_back(0x04); // add eax, 4

						stack += 4;
						break;

				}
			}

			call_wrapper.push_back(0xFF);
			call_wrapper.push_back(0xD1); //call ecx

			size_t saligned = _align_up(stack, 12);
			call_wrapper[2] = (uint8_t)(saligned - stack);

			call_wrapper.push_back(0x83);
			call_wrapper.push_back(0xC4);
			if (conv == ILCallingConvention::x86_cdecl) {
				call_wrapper.push_back((uint8_t)saligned); //add esp, saligned
			}
			else {
				call_wrapper.push_back((uint8_t)(saligned - stack)); //add esp, alignment
			}

			call_wrapper.push_back(0xC3); //ret



			void* mem = exec_alloc(call_wrapper.size());
			memcpy(mem, call_wrapper.data(), call_wrapper.size());
			cwrps[decl] = mem;
			return mem;
		}

	}
	

	void call_x86_cdecl(ILEvaluator* eval,void* pointer, std::tuple<ILCallingConvention,ILDataType, std::vector<ILDataType>>& decl) {

		void* wrapper = build_win_x86_cdecl_stdcall_call_wrapper(ILCallingConvention::x86_cdecl,std::make_tuple(std::get<1>(decl), (uint32_t)std::get<2>(decl).size(), std::get<2>(decl).data()));
		push_32bit_temp_stack(eval, decl);

		__asm {
			mov eax, b4_stack_ptr
			mov ecx, pointer
			mov edx, wrapper

			push ebp
			mov ebp, esp
			call edx

			mov [return_storage_1], eax
			mov [return_storage_2], edx
			pop ebp
		}

		pop_32bit_temp_stack(eval, decl);
		
	}

	void call_x86_stdcall(ILEvaluator* eval, void* pointer, std::tuple<ILCallingConvention,ILDataType, std::vector<ILDataType>>& decl) {

		void* wrapper = build_win_x86_cdecl_stdcall_call_wrapper(ILCallingConvention::x86_stdcall, std::make_tuple(std::get<1>(decl), (uint32_t)std::get<2>(decl).size(), std::get<2>(decl).data()));
		push_32bit_temp_stack(eval, decl);

		__asm {
			mov eax, b4_stack_ptr
			mov ecx, pointer
			mov edx, wrapper

			push ebp
			mov ebp, esp
			call edx

			mov[return_storage_1], eax
			mov[return_storage_2], edx
			pop ebp
		}

		pop_32bit_temp_stack(eval, decl);
	}
#endif

	void abi_dynamic_call(ILEvaluator* eval, ILCallingConvention conv, void* ptr, std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>>& decl) {
		switch (conv)
		{
			case Corrosive::ILCallingConvention::x86_cdecl:
				call_x86_cdecl(eval,ptr, decl);
			case Corrosive::ILCallingConvention::x86_stdcall:
				call_x86_stdcall(eval, ptr, decl);
				break;
			default:
				break;
		}
	}
}