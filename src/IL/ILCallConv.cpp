#include "IL.h"
#include <unordered_map>
#include <vector>

#ifdef WINDOWS
#include <Windows.h>
#endif

namespace Corrosive {
	std::vector<std::pair<uint8_t*, uint8_t*>> memory_pages;
	struct call_wrapper_hash {
		size_t operator() (const  std::tuple<Corrosive::ILDataType, uint32_t, Corrosive::ILDataType*>& k) const {
			size_t h = std::hash<Corrosive::ILDataType>()(std::get<0>(k)) ^ (std::hash<uint32_t>()(std::get<1>(k)) << 1);
			for (size_t a = 0; a < std::get<1>(k); a++) {
				h ^= std::hash<Corrosive::ILDataType>()(std::get<2>(k)[a]) << a;
			}
			return h;
		}
	};

	struct call_wrapper_compare {
		bool operator() (const std::tuple<Corrosive::ILDataType, uint32_t, Corrosive::ILDataType*>& l, const std::tuple<Corrosive::ILDataType, uint32_t, Corrosive::ILDataType*>& r) const {
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
	};

	std::unordered_map < std::tuple<ILDataType, uint32_t, ILDataType*>, void*, call_wrapper_hash, call_wrapper_compare> call_wrappers[(uint8_t)ILCallingConvention::__max];

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

	void release_jit_code() {
		for (auto&& p : memory_pages) {
			VirtualFree(p.first, 0, MEM_RELEASE);
		}

		for (size_t i = 0; i < (uint8_t)ILCallingConvention::__max; ++i) {
			call_wrappers[i].clear();
		}

		memory_pages.clear();
	}

	


#ifdef X86
	uint32_t b4_stack[256];
	uint32_t* b4_stack_ptr = b4_stack;
	size_t return_storage_1;
	size_t return_storage_2;

	void push_32bit_temp_stack(ILEvaluator* eval, std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>>& decl,void* pointer) {
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
		b4_stack[off++] = (uint32_t)pointer;
	}

	void pop_32bit_temp_stack(ILEvaluator* eval, std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>>& decl) {
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

			size_t abs_v = (size_t)(b4_stack_ptr);
			call_wrapper.push_back(0xB8);
			call_wrapper.push_back((uint8_t)(abs_v));
			call_wrapper.push_back((uint8_t)(abs_v >> 8));
			call_wrapper.push_back((uint8_t)(abs_v >> 16));
			call_wrapper.push_back((uint8_t)(abs_v >> 24)); // mov eax, b4_stack_ptr


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
			call_wrapper.push_back(0x10); //call [eax]

			size_t saligned = _align_up(stack + 4, 16) - 4;
			call_wrapper[2] = (uint8_t)(saligned - stack);

			call_wrapper.push_back(0x83);
			call_wrapper.push_back(0xC4);
			if (conv == ILCallingConvention::native) {
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

		void* asm_call_wrapper = build_win_x86_cdecl_stdcall_call_wrapper(ILCallingConvention::native,std::make_tuple(std::get<1>(decl), (uint32_t)std::get<2>(decl).size(), std::get<2>(decl).data()));
		push_32bit_temp_stack(eval, decl, pointer);

		switch (std::get<1>(decl)) {
			case ILDataType::i8:
			case ILDataType::u8:
				eval->write_register_value(((uint8_t(*)())asm_call_wrapper)()); break;
			case ILDataType::i16:
			case ILDataType::u16:
				eval->write_register_value(((uint16_t(*)())asm_call_wrapper)()); break;
			case ILDataType::i32:
			case ILDataType::u32:
			case ILDataType::word:
				eval->write_register_value(((uint32_t(*)())asm_call_wrapper)()); break;
			case ILDataType::i64:
			case ILDataType::u64:
				eval->write_register_value(((uint64_t(*)())asm_call_wrapper)()); break;
			case ILDataType::f32:
				eval->write_register_value(((float(*)())asm_call_wrapper)()); break;
			case ILDataType::f64:
				eval->write_register_value(((double(*)())asm_call_wrapper)()); break;
			case ILDataType::none:
				((void(*)())asm_call_wrapper)(); break;
		}
		
	}

	void call_x86_stdcall(ILEvaluator* eval, void* pointer, std::tuple<ILCallingConvention,ILDataType, std::vector<ILDataType>>& decl) {

		void* asm_call_wrapper = build_win_x86_cdecl_stdcall_call_wrapper(ILCallingConvention::stdcall, std::make_tuple(std::get<1>(decl), (uint32_t)std::get<2>(decl).size(), std::get<2>(decl).data()));
		push_32bit_temp_stack(eval, decl,pointer);

		switch (std::get<1>(decl)) {
			case ILDataType::i8:
			case ILDataType::u8:
				eval->write_register_value(((uint8_t(*)())asm_call_wrapper)()); break;
			case ILDataType::i16:
			case ILDataType::u16:
				eval->write_register_value(((uint16_t(*)())asm_call_wrapper)()); break;
			case ILDataType::i32:
			case ILDataType::u32:
			case ILDataType::word:
				eval->write_register_value(((uint32_t(*)())asm_call_wrapper)()); break;
			case ILDataType::i64:
			case ILDataType::u64:
				eval->write_register_value(((uint64_t(*)())asm_call_wrapper)()); break;
			case ILDataType::f32:
				eval->write_register_value(((float(*)())asm_call_wrapper)()); break;
			case ILDataType::f64:
				eval->write_register_value(((double(*)())asm_call_wrapper)()); break;
			case ILDataType::none:
				((void(*)())asm_call_wrapper)(); break;
		}
	}
#endif
#endif




#ifdef X64
	uint64_t b8_stack[256];
	uint64_t* b8_stack_ptr = b8_stack;
	size_t return_storage_1;

	void push_64bit_temp_stack(ILEvaluator* eval, std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>>& decl, void* pointer) {
		size_t off = 0;
		for (size_t i = std::get<2>(decl).size(); i > 0; --i) {
			switch (std::get<2>(decl)[i - 1])
			{
				case ILDataType::i8:
				case ILDataType::u8:
					b8_stack[off++] = eval->pop_register_value<uint8_t>(); break;
				case ILDataType::i16:
				case ILDataType::u16:
					b8_stack[off++] = eval->pop_register_value<uint16_t>(); break;
				case ILDataType::i32:
				case ILDataType::u32:
				case ILDataType::f32:
					b8_stack[off++] = eval->pop_register_value<uint32_t>(); break;
				case ILDataType::word:
				case ILDataType::f64:
				case ILDataType::i64:
				case ILDataType::u64:
					b8_stack[off++] = eval->pop_register_value<uint64_t>(); break;
				case ILDataType::dword: {
					auto dw = eval->pop_register_value<dword_t>(); 
					b8_stack[off++] = (size_t)dw.p1;
					b8_stack[off++] = (size_t)dw.p2;
				} break;

				default:
					break;
			}
		}

		b8_stack[off++] = (uint64_t)pointer;
	}


#ifdef WINDOWS

	void* build_win_x64_call_wrapper(std::tuple<ILDataType, uint32_t, ILDataType*> decl) {
		auto& cwrps = call_wrappers[(uint8_t)ILCallingConvention::native];
		auto f = cwrps.find(decl);
		if (f != cwrps.end()) {
			return f->second;
		}
		else {
			std::vector<uint8_t> call_wrapper;

			uint32_t argc = std::get<1>(decl);
			ILDataType* argv = std::get<2>(decl);
			size_t stack = 0;

			size_t abs_v = (size_t)(b8_stack_ptr);
			call_wrapper.push_back(0x48);
			call_wrapper.push_back(0xB8);
			call_wrapper.push_back((uint8_t)(abs_v));
			call_wrapper.push_back((uint8_t)(abs_v>>8));
			call_wrapper.push_back((uint8_t)(abs_v>>16));
			call_wrapper.push_back((uint8_t)(abs_v>>24));
			call_wrapper.push_back((uint8_t)(abs_v>>32));
			call_wrapper.push_back((uint8_t)(abs_v>>40));
			call_wrapper.push_back((uint8_t)(abs_v>>48));
			call_wrapper.push_back((uint8_t)(abs_v>>56)); // movabs rax, b8_stack_ptr

			for (uint32_t i = argc; i > 0; --i) {
				ILDataType dt = argv[i - 1];

				uint8_t raxadd = 8;

				switch (dt) {
					case ILDataType::f32:
						switch (i - 1) {
							case 0:
								call_wrapper.push_back(0xF3);
								call_wrapper.push_back(0x0F);
								call_wrapper.push_back(0x10);
								call_wrapper.push_back(0x00); // movss xmm0, [rax]
								break;
							case 1:
								call_wrapper.push_back(0xF3);
								call_wrapper.push_back(0x0F);
								call_wrapper.push_back(0x10);
								call_wrapper.push_back(0x08); // movss xmm1, [rax]
								break;
							case 2:
								call_wrapper.push_back(0xF3);
								call_wrapper.push_back(0x0F);
								call_wrapper.push_back(0x10);
								call_wrapper.push_back(0x10); // movss xmm2, [rax]
								break;
							case 3:
								call_wrapper.push_back(0xF3);
								call_wrapper.push_back(0x0F);
								call_wrapper.push_back(0x10);
								call_wrapper.push_back(0x18); // movss xmm3, [rax]
								break;

							default:
								call_wrapper.push_back(0xFF);
								call_wrapper.push_back(0x30); // push [rax]
								stack += 8;
						}
						break;
					case ILDataType::f64:
						switch (i - 1) {
							case 0:
								call_wrapper.push_back(0xF2);
								call_wrapper.push_back(0x0F);
								call_wrapper.push_back(0x10);
								call_wrapper.push_back(0x00); // movss xmm0, [rax]
								break;
							case 1:
								call_wrapper.push_back(0xF2);
								call_wrapper.push_back(0x0F);
								call_wrapper.push_back(0x10);
								call_wrapper.push_back(0x08); // movss xmm1, [rax]
								break;
							case 2:
								call_wrapper.push_back(0xF2);
								call_wrapper.push_back(0x0F);
								call_wrapper.push_back(0x10);
								call_wrapper.push_back(0x10); // movss xmm2, [rax]
								break;
							case 3:
								call_wrapper.push_back(0xF2);
								call_wrapper.push_back(0x0F);
								call_wrapper.push_back(0x10);
								call_wrapper.push_back(0x18); // movss xmm3, [rax]
								break;

							default:
								call_wrapper.push_back(0xFF);
								call_wrapper.push_back(0x30); // push [rax]
								stack += 8;
						}
						break;
					case ILDataType::dword: 
						switch (i - 1) {
							case 0:
								call_wrapper.push_back(0x48);
								call_wrapper.push_back(0x89);
								call_wrapper.push_back(0xC1); // mov rcx, rax
								break;
							case 1:
								call_wrapper.push_back(0x48);
								call_wrapper.push_back(0x89);
								call_wrapper.push_back(0xC2); // mov rdx, rax
								break;
							case 2:
								call_wrapper.push_back(0x49);
								call_wrapper.push_back(0x89);
								call_wrapper.push_back(0xC0); // mov r8, rax
								break;
							case 3:
								call_wrapper.push_back(0x49);
								call_wrapper.push_back(0x89);
								call_wrapper.push_back(0xC1); // mov r9, rax
								break;

							default:
								call_wrapper.push_back(0x50); // push rax
								stack += 16;
						}
						raxadd = 16;
						break;

					default:
						switch (i-1) {
							case 0:
								call_wrapper.push_back(0x48);
								call_wrapper.push_back(0x8B);
								call_wrapper.push_back(0x08); // mov rcx, [rax]
								break;
							case 1:
								call_wrapper.push_back(0x48);
								call_wrapper.push_back(0x8B);
								call_wrapper.push_back(0x10); // mov rdx, [rax]
								break;
							case 2:
								call_wrapper.push_back(0x4C);
								call_wrapper.push_back(0x8B);
								call_wrapper.push_back(0x00); // mov r8, [rax]
								break;
							case 3:
								call_wrapper.push_back(0x4C);
								call_wrapper.push_back(0x8B);
								call_wrapper.push_back(0x08); // mov r9, [rax]
								break;

							default:
								call_wrapper.push_back(0xFF);
								call_wrapper.push_back(0x30); // push [rax]
								stack += 8;
						}

						break;
				}

				call_wrapper.push_back(0x48);
				call_wrapper.push_back(0x83);
				call_wrapper.push_back(0xC0);
				call_wrapper.push_back(raxadd); // add rax, raxadd
			}

			if (stack > 0) {
				size_t saligned = _align_up(stack + 8, 32) - 8;

				call_wrapper.insert(call_wrapper.begin(),0x48);
				call_wrapper.insert(call_wrapper.begin()+1,0x83);
				call_wrapper.insert(call_wrapper.begin()+2, 0xEC);
				call_wrapper.insert(call_wrapper.begin()+3, (uint8_t)(saligned - stack)); // sub rsp, saligned-stack


				call_wrapper.push_back(0xFF);
				call_wrapper.push_back(0x10); // call [rax]


				call_wrapper.push_back(0x48);
				call_wrapper.push_back(0x83);
				call_wrapper.push_back(0xC4);
				call_wrapper.push_back((uint8_t)(saligned)); // add rsp, saligned


				call_wrapper.push_back(0xC3); // ret
			}
			else {
				// tail call optimization. without this optimization msvc release version fails sometimes
				call_wrapper.push_back(0xFF);
				call_wrapper.push_back(0x20); // jmp [rax]
			}


			void* mem = exec_alloc(call_wrapper.size());
			memcpy(mem, call_wrapper.data(), call_wrapper.size());
			cwrps[decl] = mem;
			return mem;
		}

	}



	void call_x64_call(ILEvaluator* eval, void* pointer, std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>>& decl) {

		auto asm_call_wrapper = build_win_x64_call_wrapper(std::make_tuple(std::get<1>(decl), (uint32_t)std::get<2>(decl).size(), std::get<2>(decl).data()));

		push_64bit_temp_stack(eval, decl, pointer);
		
		switch (std::get<1>(decl)) {
			case ILDataType::i8:
			case ILDataType::u8:
				eval->write_register_value(((uint8_t(*)())asm_call_wrapper)()); break;
			case ILDataType::i16:
			case ILDataType::u16:
				eval->write_register_value(((uint16_t(*)())asm_call_wrapper)()); break;
			case ILDataType::i32:
			case ILDataType::u32:
				eval->write_register_value(((uint32_t(*)())asm_call_wrapper)()); break;
			case ILDataType::i64:
			case ILDataType::u64:
			case ILDataType::word:
				eval->write_register_value(((uint64_t(*)())asm_call_wrapper)()); break;
			case ILDataType::f32:
				eval->write_register_value(((float(*)())asm_call_wrapper)()); break;
			case ILDataType::f64:
				eval->write_register_value(((double(*)())asm_call_wrapper)()); break;
			case ILDataType::dword:
				eval->write_register_value(((dword_t(*)())asm_call_wrapper)()); break;
			case ILDataType::none:
				((void(*)())asm_call_wrapper)(); break;
		}
	}

#endif

#endif

	void abi_dynamic_call(ILEvaluator* eval, ILCallingConvention conv, void* ptr, std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>>& decl) {
		switch (conv)
		{
#ifdef X86
			case Corrosive::ILCallingConvention::native:
				call_x86_cdecl(eval,ptr, decl); break;
			case Corrosive::ILCallingConvention::stdcall:
				call_x86_stdcall(eval, ptr, decl); break;
#endif

#ifdef X64
			case Corrosive::ILCallingConvention::stdcall:
			case Corrosive::ILCallingConvention::native:
				call_x64_call(eval, ptr, decl); break;
#endif
				break;
			default:
				break;
		}
	}
}