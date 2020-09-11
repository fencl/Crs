#include "IL.hpp"
#include <unordered_map>
#include <vector>
#include <cstring>

#ifdef WINDOWS
#include <Windows.h>
#endif

#ifdef LINUX
#include <sys/mman.h>
#endif

namespace Crs {
	

	std::vector<std::pair<std::uint8_t*, std::uint8_t*>> memory_pages;
	struct call_wrapper_hash {
		std::size_t operator() (const  std::tuple<Crs::ILDataType, std::uint32_t, Crs::ILDataType*>& k) const {
			std::size_t h = std::hash<Crs::ILDataType>()(std::get<0>(k)) ^ (std::hash<std::uint32_t>()(std::get<1>(k)) << 1);
			for (std::size_t a = 0; a < std::get<1>(k); a++) {
				h ^= std::hash<Crs::ILDataType>()(std::get<2>(k)[a]) << a;
			}
			return h;
		}
	};

	struct call_wrapper_compare {
		bool operator() (const std::tuple<Crs::ILDataType, std::uint32_t, Crs::ILDataType*>& l, const std::tuple<Crs::ILDataType, std::uint32_t, Crs::ILDataType*>& r) const {
			if (std::get<0>(l) != std::get<0>(r)) return false;
			if (std::get<1>(l) != std::get<1>(r)) return false;

			Crs::ILDataType* lp = std::get<2>(l);
			Crs::ILDataType* rp = std::get<2>(r);

			for (std::size_t i = 0; i < std::get<1>(l); ++i) {
				if (*lp != *rp) return false;
				lp++;
				rp++;
			}
			return true;
		}
	};

	std::unordered_map < std::tuple<ILDataType, std::uint32_t, ILDataType*>, void*, call_wrapper_hash, call_wrapper_compare> call_wrappers[(std::uint8_t)ILCallingConvention::__max] = {
		std::unordered_map < std::tuple<ILDataType, std::uint32_t, ILDataType*>, void*, call_wrapper_hash, call_wrapper_compare>(256),
		std::unordered_map < std::tuple<ILDataType, std::uint32_t, ILDataType*>, void*, call_wrapper_hash, call_wrapper_compare>(256),
		std::unordered_map < std::tuple<ILDataType, std::uint32_t, ILDataType*>, void*, call_wrapper_hash, call_wrapper_compare>(256)
	};

	std::uint8_t* virtual_alloc_page() {
	#ifdef WINDOWS
		return (std::uint8_t*)VirtualAlloc(nullptr, 4096, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	#endif

	#ifdef LINUX
		return (std::uint8_t*)mmap(nullptr, 4096, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, 0, 0);
	#endif
	}

	void virtual_free_page(std::uint8_t* mem) {
	#ifdef WINDOWS
		VirtualFree((void*)mem, 0, MEM_RELEASE);
	#endif
	#ifdef LINUX
		munmap((void*)mem, 4096);
	#endif
	}

	void* exec_alloc(std::size_t bytes) {
		if (memory_pages.size() == 0) {
			std::uint8_t* mem = virtual_alloc_page();
			memory_pages.push_back(std::make_pair(mem,mem));
		}

		auto& lmp = memory_pages.back();
		if (4096 - (std::size_t)(lmp.second-lmp.first) >= bytes ) {
			auto r = lmp.second;
			lmp.second += bytes;
			return r;
		}
		else {
			std::uint8_t* mem = virtual_alloc_page();
			memory_pages.push_back(std::make_pair(mem, mem + bytes));
			return mem;
		}
	}

	void release_jit_code() {
		invalidate_sandbox();

		for (auto&& p : memory_pages) {
			virtual_free_page(p.first);
		}

		for (std::size_t i = 0; i < (std::uint8_t)ILCallingConvention::__max; ++i) {
			call_wrappers[i].clear();
		}

		memory_pages.clear();
	}

	


#ifdef X86
	std::uint32_t b4_stack[256];
	std::uint32_t* b4_stack_ptr = b4_stack;
	std::size_t return_storage_1;
	std::size_t return_storage_2;

	errvoid push_32bit_temp_stack(ILEvaluator* eval, std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>>& decl,void* pointer) {
		std::size_t off = 0;
		for (std::size_t i = std::get<2>(decl).size(); i > 0; --i) {
			switch (std::get<2>(decl)[i - 1])
			{
				case ILDataType::i8:
				case ILDataType::u8: {
					std::uint8_t v;
					if (!eval->pop_register_value<std::uint8_t>(v)) return err::fail;
					b4_stack[off++] = v;
				} break;

				case ILDataType::i16:
				case ILDataType::u16: {
					std::uint16_t v;
					if (!eval->pop_register_value<std::uint16_t>(v)) return err::fail;
					b4_stack[off++] = v;
				} break;

				case ILDataType::i32:
				case ILDataType::u32:
				case ILDataType::f32:
				case ILDataType::word: {
					std::uint32_t v;
					if (!eval->pop_register_value<std::uint32_t>(v)) return err::fail;
					b4_stack[off++] = v;
				} break;

				case ILDataType::f64:
				case ILDataType::i64:
				case ILDataType::dword: 
				case ILDataType::u64: {
					std::uint64_t v;
					if (!eval->pop_register_value<std::uint64_t>(v)) return err::fail;;
					b4_stack[off++] = (std::uint32_t)(v >> 32);
					b4_stack[off++] = (std::uint32_t)(v);
					break;
				}

				default:
					break;
			}
		}
		b4_stack[off++] = (std::uint32_t)pointer;
		return err::ok;
	}

#ifdef WINDOWS
	struct state
	{
	    std::uint32_t ebx;
	    std::uint32_t esi;
	    std::uint32_t edi;
	    std::uint32_t ebp;
	    std::uint32_t esp;
	    std::uint32_t fp0;
		std::uint32_t rip;
	};
#endif

#ifdef LINUX
	struct state
	{
	    std::uint32_t ebx;
	    std::uint32_t esi;
	    std::uint32_t edi;
	    std::uint32_t ebp;
	    std::uint32_t esp;
		std::uint32_t rip;
	};
#endif

	state sandbox_state;
	void* sandbox = &sandbox_state;
	int (*wrap)(void*) = nullptr;
	void (*longjmp_func)(void*, int) = nullptr;


#ifdef WINDOWS
	unsigned char setjmp_data[] ={ 0x8B, 0x44, 0x24, 0x04, 0x89, 0x18, 0x89, 0x70, 0x04, 0x89, 0x78, 0x08, 0x89, 0x68, 0x0C, 0x8D, 0x4C, 0x24, 0x04, 0x89, 0x48, 0x10, 0x64, 0x8B, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x89, 0x48, 0x14, 0x8B, 0x0C, 0x24, 0x89, 0x48, 0x18, 0x31, 0xC0, 0xC3 };
	/*
		mov    eax,DWORD PTR [esp+0x4]
		mov    DWORD PTR [eax],ebx
		mov    DWORD PTR [eax+0x4],esi
		mov    DWORD PTR [eax+0x8],edi
		mov    DWORD PTR [eax+0xc],ebp
		lea    ecx,[esp+0x4]
		mov    DWORD PTR [eax+0x10],ecx
		mov    ecx,DWORD PTR fs:0x0
		mov    DWORD PTR [eax+0x14],ecx
		mov    ecx,DWORD PTR [esp]
		mov    DWORD PTR [eax+0x18],ecx
		xor    eax,eax
		ret
	*/


	unsigned char longjmp_data[] ={ 0x8B, 0x54, 0x24, 0x04, 0x8B, 0x44, 0x24, 0x08, 0x8B, 0x1A, 0x8B, 0x72, 0x04, 0x8B, 0x7A, 0x08, 0x8B, 0x6A, 0x0C, 0x8B, 0x4A, 0x10, 0x89, 0xCC, 0x8B, 0x4A, 0x14, 0x64, 0x89, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x8B, 0x4A, 0x18, 0xFF, 0xE1 };
	/*
		mov    edx,DWORD PTR [esp+0x4]
		mov    eax,DWORD PTR [esp+0x8]
		mov    ebx,DWORD PTR [edx]
		mov    esi,DWORD PTR [edx+0x4]
		mov    edi,DWORD PTR [edx+0x8]
		mov    ebp,DWORD PTR [edx+0xc]
		mov    ecx,DWORD PTR [edx+0x10]
		mov    esp,ecx
		mov    ecx,DWORD PTR [edx+0x14]
		mov    DWORD PTR fs:0x0,ecx
		mov    ecx,DWORD PTR [edx+0x18]
		jmp    ecx
	*/
#endif


#ifdef LINUX
	unsigned char setjmp_data[] ={ 0x8B, 0x44, 0x24, 0x04, 0x89, 0x18, 0x89, 0x70, 0x04, 0x89, 0x78, 0x08, 0x89, 0x68, 0x0C, 0x8D, 0x4C, 0x24, 0x04, 0x89, 0x48, 0x10, 0x8B, 0x0C, 0x24, 0x89, 0x48, 0x14, 0x31, 0xC0, 0xC3 };
	
	/*
		mov    eax,DWORD PTR [esp+0x4]
		mov    DWORD PTR [eax],ebx
		mov    DWORD PTR [eax+0x4],esi
		mov    DWORD PTR [eax+0x8],edi
		mov    DWORD PTR [eax+0xc],ebp
		lea    ecx,[esp+0x4]
		mov    DWORD PTR [eax+0x10],ecx
		mov    ecx,DWORD PTR [esp]
		mov    DWORD PTR [eax+0x14],ecx
		xor    eax,eax
		ret
	*/


	unsigned char longjmp_data[] = { 0x8B, 0x54, 0x24, 0x04, 0x8B, 0x44, 0x24, 0x08, 0x8B, 0x1A, 0x8B, 0x72, 0x04, 0x8B, 0x7A, 0x08, 0x8B, 0x6A, 0x0C, 0x8B, 0x4A, 0x10, 0x89, 0xCC, 0x8B, 0x4A, 0x14, 0xFF, 0xE1 };
	
	/*
		mov    edx,DWORD PTR [esp+0x4]
		mov    eax,DWORD PTR [esp+0x8]
		mov    ebx,DWORD PTR [edx]
		mov    esi,DWORD PTR [edx+0x4]
		mov    edi,DWORD PTR [edx+0x8]
		mov    ebp,DWORD PTR [edx+0xc]
		mov    ecx,DWORD PTR [edx+0x10]
		mov    esp,ecx
		mov    ecx,DWORD PTR [edx+0x14]
		jmp    ecx
	*/
#endif
	void build_sandbox() {
		if (!wrap) {
			void* mem = exec_alloc(sizeof(setjmp_data));
			std::memcpy(mem, setjmp_data, sizeof(setjmp_data));
			wrap = (int(*)(void*)) mem;
		}

		if (!longjmp_func) {
			void* mem = exec_alloc(sizeof(longjmp_data));
			std::memcpy(mem, longjmp_data, sizeof(longjmp_data));
			longjmp_func = (void(*)(void*,int)) mem;
		}
	}


	void* build_x86_cdecl_stdcall_call_wrapper(ILCallingConvention conv, std::tuple<ILDataType, std::uint32_t, ILDataType*> decl) {
		auto& cwrps = call_wrappers[(std::uint8_t)conv];
		auto f = cwrps.find(decl);
		if (f != cwrps.end()) {
			return f->second;
		}
		else {
			std::vector<std::uint8_t> call_wrapper;

			std::uint32_t argc = std::get<1>(decl);
			ILDataType* argv = std::get<2>(decl);
			std::size_t stack = 0;

			call_wrapper.push_back(0x83);
			call_wrapper.push_back(0xEC);
			call_wrapper.push_back(0); // sub esp, 0

			std::size_t abs_v = (std::size_t)(b4_stack_ptr);
			call_wrapper.push_back(0xB8);
			call_wrapper.push_back((std::uint8_t)(abs_v));
			call_wrapper.push_back((std::uint8_t)(abs_v >> 8));
			call_wrapper.push_back((std::uint8_t)(abs_v >> 16));
			call_wrapper.push_back((std::uint8_t)(abs_v >> 24)); // mov eax, b4_stack_ptr


			for (std::uint32_t i = argc; i > 0; --i) {
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
					case ILDataType::dword:
						call_wrapper.push_back(0xFF);
						call_wrapper.push_back(0x30); // push [eax]
						call_wrapper.push_back(0x83);
						call_wrapper.push_back(0xC0);
						call_wrapper.push_back(0x04); // add eax, 4
						call_wrapper.push_back(0xFF);
						call_wrapper.push_back(0x30); // push [eax]
						call_wrapper.push_back(0x83);
						call_wrapper.push_back(0xC0);
						call_wrapper.push_back(0x04); // add eax, 4
						stack += 8;
						break;

				}
			}

			call_wrapper.push_back(0xFF);
			call_wrapper.push_back(0x10); //call [eax]

			std::size_t saligned = align_up(stack + 4, 16) - 4;
			call_wrapper[2] = (std::uint8_t)(saligned - stack);

			call_wrapper.push_back(0x83);
			call_wrapper.push_back(0xC4);
			if (conv == ILCallingConvention::native) {
				call_wrapper.push_back((std::uint8_t)saligned); //add esp, saligned
			}
			else {
				call_wrapper.push_back((std::uint8_t)(saligned - stack)); //add esp, alignment
			}

			call_wrapper.push_back(0xC3); //ret



			void* mem = exec_alloc(call_wrapper.size());
			std::memcpy(mem, call_wrapper.data(), call_wrapper.size());
			cwrps[decl] = mem;
			return mem;
		}

	}
	

	errvoid call_x86_cdecl(ILEvaluator* eval,void* pointer, std::tuple<ILCallingConvention,ILDataType, std::vector<ILDataType>>& decl) {

		void* asm_call_wrapper = build_x86_cdecl_stdcall_call_wrapper(ILCallingConvention::native,std::make_tuple(std::get<1>(decl), (std::uint32_t)std::get<2>(decl).size(), std::get<2>(decl).data()));
		if (!push_32bit_temp_stack(eval, decl, pointer)) return err::fail;

		switch (std::get<1>(decl)) {
			case ILDataType::i8:
			case ILDataType::u8:
				eval->write_register_value(((std::uint8_t(*)())asm_call_wrapper)()); break;
			case ILDataType::i16:
			case ILDataType::u16:
				eval->write_register_value(((std::uint16_t(*)())asm_call_wrapper)()); break;
			case ILDataType::i32:
			case ILDataType::u32:
			case ILDataType::word:
				eval->write_register_value(((std::uint32_t(*)())asm_call_wrapper)()); break;
			case ILDataType::i64:
			case ILDataType::u64:
				eval->write_register_value(((std::uint64_t(*)())asm_call_wrapper)()); break;
			case ILDataType::f32:
				eval->write_register_value(((float(*)())asm_call_wrapper)()); break;
			case ILDataType::f64:
				eval->write_register_value(((double(*)())asm_call_wrapper)()); break;
			case ILDataType::dword:
				eval->write_register_value(((dword_t(*)())asm_call_wrapper)()); break;
			case ILDataType::none:
				((void(*)())asm_call_wrapper)(); break;
		}

		return err::ok;
	}

	errvoid call_x86_stdcall(ILEvaluator* eval, void* pointer, std::tuple<ILCallingConvention,ILDataType, std::vector<ILDataType>>& decl) {

		void* asm_call_wrapper = build_x86_cdecl_stdcall_call_wrapper(ILCallingConvention::stdcall, std::make_tuple(std::get<1>(decl), (std::uint32_t)std::get<2>(decl).size(), std::get<2>(decl).data()));
		if (!push_32bit_temp_stack(eval, decl,pointer)) return err::fail;

		switch (std::get<1>(decl)) {
			case ILDataType::i8:
			case ILDataType::u8:
				eval->write_register_value(((std::uint8_t(*)())asm_call_wrapper)()); break;
			case ILDataType::i16:
			case ILDataType::u16:
				eval->write_register_value(((std::uint16_t(*)())asm_call_wrapper)()); break;
			case ILDataType::i32:
			case ILDataType::u32:
			case ILDataType::word:
				eval->write_register_value(((std::uint32_t(*)())asm_call_wrapper)()); break;
			case ILDataType::i64:
			case ILDataType::u64:
				eval->write_register_value(((std::uint64_t(*)())asm_call_wrapper)()); break;
			case ILDataType::f32:
				eval->write_register_value(((float(*)())asm_call_wrapper)()); break;
			case ILDataType::f64:
				eval->write_register_value(((double(*)())asm_call_wrapper)()); break;
			case ILDataType::dword:
				eval->write_register_value(((dword_t(*)())asm_call_wrapper)()); break;
			case ILDataType::none:
				((void(*)())asm_call_wrapper)(); break;
		}

		return err::ok;
	}
#endif




#ifdef X64
	std::uint64_t b8_stack[256];
	std::uint64_t* b8_stack_ptr = b8_stack;
	std::size_t return_storage_1;

	


#ifdef WINDOWS

	errvoid push_64bit_temp_stack(ILEvaluator* eval, std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>>& decl, void* pointer) {
		std::size_t off = 0;
		for (std::size_t i = std::get<2>(decl).size(); i > 0; --i) {
			switch (std::get<2>(decl)[i - 1])
			{
				case ILDataType::i8:
				case ILDataType::u8: {
					std::uint8_t v;
					if (!eval->pop_register_value<std::uint8_t>(v)) return err::fail;
					b8_stack[off++] = v;
				} break;
				case ILDataType::i16:
				case ILDataType::u16: {
					std::uint16_t v;
					if (!eval->pop_register_value<std::uint16_t>(v)) return err::fail; 
					b8_stack[off++] = v;
				} break;
				case ILDataType::i32:
				case ILDataType::u32:
				case ILDataType::f32: {
					std::uint32_t v;
					if (!eval->pop_register_value<std::uint32_t>(v)) return err::fail;
					b8_stack[off++] = v;
				} break;
				case ILDataType::word:
				case ILDataType::f64:
				case ILDataType::i64:
				case ILDataType::u64: {
					std::uint64_t v;
					if (!eval->pop_register_value<std::uint64_t>(v)) return err::fail;
					b8_stack[off++] = v;
				} break;
				case ILDataType::dword: {
					dword_t dw;
					if (!eval->pop_register_value<dword_t>(dw)) return err::fail; 
					b8_stack[off++] = (std::uint64_t)dw.p1;
					b8_stack[off++] = (std::uint64_t)dw.p2;
				} break;

				default:
					break;
			}
		}

		b8_stack[off++] = (std::uint64_t)pointer;
		return err::ok;
	}

	struct state
	{
	    std::uint64_t fp;
	    std::uint64_t rbx;
	    std::uint64_t rsp;
	    std::uint64_t rbp;
	    std::uint64_t rsi;
	    std::uint64_t rdi;
	    std::uint64_t r12;
	    std::uint64_t r13;
	    std::uint64_t r14;
	    std::uint64_t r15;
	    std::uint64_t rip;
	    std::uint64_t __align;
	    uint128_t xmm6;
	    uint128_t xmm7;
	    uint128_t xmm8;
	    uint128_t xmm9;
	    uint128_t xmm10;
	    uint128_t xmm11;
	    uint128_t xmm12;
	    uint128_t xmm13;
	    uint128_t xmm14;
	    uint128_t xmm15;
	};

	state sandbox_state;
	void* sandbox = &sandbox_state;
	int (*wrap)(void*) = nullptr;
	void (*longjmp_func)(void*, int) = nullptr;

	unsigned char setjmp_data[] = { 0x48, 0x89, 0x11, 0x48, 0x89, 0x59, 0x08, 0x4C, 0x8D, 0x44, 0x24, 0x08, 0x4C, 0x89, 0x41, 0x10, 0x48, 0x89, 0x69, 0x18, 0x48, 0x89, 0x71, 0x20, 0x48, 0x89, 0x79, 0x28, 0x4C, 0x89, 0x61, 0x30, 0x4C, 0x89, 0x69, 0x38, 0x4C, 0x89, 0x71, 0x40, 0x4C, 0x89, 0x79, 0x48, 0x4C, 0x8B, 0x04, 0x24, 0x4C, 0x89, 0x41, 0x50, 0x66, 0x0F, 0x7F, 0x71, 0x60, 0x66, 0x0F, 0x7F, 0x79, 0x70, 0x66, 0x44, 0x0F, 0x7F, 0x81, 0x80, 0x00, 0x00, 0x00, 0x66, 0x44, 0x0F, 0x7F, 0x89, 0x90, 0x00, 0x00, 0x00, 0x66, 0x44, 0x0F, 0x7F, 0x91, 0xA0, 0x00, 0x00, 0x00, 0x66, 0x44, 0x0F, 0x7F, 0x99, 0xB0, 0x00, 0x00, 0x00, 0x66, 0x44, 0x0F, 0x7F, 0xA1, 0xC0, 0x00, 0x00, 0x00, 0x66, 0x44, 0x0F, 0x7F, 0xA9, 0xD0, 0x00, 0x00, 0x00, 0x66, 0x44, 0x0F, 0x7F, 0xB1, 0xE0, 0x00, 0x00, 0x00, 0x66, 0x44, 0x0F, 0x7F, 0xB9, 0xF0, 0x00, 0x00, 0x00, 0x31, 0xC0, 0xC3 };

	/*
	mov [rcx], rdx
 	mov [rcx+8], rbx
 	lea r8, [rsp+8]
 	mov [rcx + 0x10], r8
 	mov [rcx + 0x18], rbp
 	mov [rcx + 0x20], rsi
 	mov [rcx + 0x28], rdi
 	mov [rcx + 0x30], r12
 	mov [rcx + 0x38], r13
 	mov [rcx + 0x40], r14
 	mov [rcx + 0x48], r15
 	mov r8, [rsp]
 	mov [rcx + 0x50], r8
 	movdqa [rcx + 0x60], xmm6
 	movdqa [rcx + 0x70], xmm7
 	movdqa [rcx + 0x80], xmm8
 	movdqa [rcx + 0x90], xmm9
 	movdqa [rcx + 0x0a0], xmm10
 	movdqa [rcx + 0x0b0], xmm11
 	movdqa [rcx + 0x0c0], xmm12
 	movdqa [rcx + 0x0d0], xmm13
 	movdqa [rcx + 0x0e0], xmm14
 	movdqa [rcx + 0x0f0], xmm15
 	xor eax, eax
 	ret  
	*/

	unsigned char longjmp_data[] = { 0x48, 0x89, 0xD0, 0x48, 0x8B, 0x11, 0x48, 0x8B, 0x59, 0x08, 0x48, 0x8B, 0x61, 0x10, 0x48, 0x8B, 0x69, 0x18, 0x48, 0x8B, 0x71, 0x20, 0x48, 0x8B, 0x79, 0x28, 0x4C, 0x8B, 0x61, 0x30, 0x4C, 0x8B, 0x69, 0x38, 0x4C, 0x8B, 0x71, 0x40, 0x4C, 0x8B, 0x79, 0x48, 0x4C, 0x8B, 0x41, 0x50, 0x66, 0x0F, 0x6F, 0x71, 0x60, 0x66, 0x0F, 0x6F, 0x79, 0x70, 0x66, 0x44, 0x0F, 0x6F, 0x81, 0x80, 0x00, 0x00, 0x00, 0x66, 0x44, 0x0F, 0x6F, 0x89, 0x90, 0x00, 0x00, 0x00, 0x66, 0x44, 0x0F, 0x6F, 0x91, 0xA0, 0x00, 0x00, 0x00, 0x66, 0x44, 0x0F, 0x6F, 0x99, 0xB0, 0x00, 0x00, 0x00, 0x66, 0x44, 0x0F, 0x6F, 0xA1, 0xC0, 0x00, 0x00, 0x00, 0x66, 0x44, 0x0F, 0x6F, 0xA9, 0xD0, 0x00, 0x00, 0x00, 0x66, 0x44, 0x0F, 0x6F, 0xB1, 0xE0, 0x00, 0x00, 0x00, 0x66, 0x44, 0x0F, 0x6F, 0xB9, 0xF0, 0x00, 0x00, 0x00, 0x41, 0xFF, 0xE0 };

	/*
	mov rax, rdx
    mov rdx, [rcx]
    mov rbx, [rcx+8]
    mov rsp, [rcx + 0x10]
    mov rbp, [rcx + 0x18]
    mov rsi, [rcx + 0x20]
    mov rdi, [rcx + 0x28]
    mov r12, [rcx + 0x30]
    mov r13, [rcx + 0x38]
    mov r14, [rcx + 0x40]
    mov r15, [rcx + 0x48]
    mov r8, [rcx + 0x50]
    movdqa xmm6,  [rcx + 0x60]
    movdqa xmm7,  [rcx + 0x70]
    movdqa xmm8,  [rcx + 0x80]
    movdqa xmm9,  [rcx + 0x90]
    movdqa xmm10, [rcx + 0x0a0]
    movdqa xmm11, [rcx + 0x0b0]
    movdqa xmm12, [rcx + 0x0c0]
    movdqa xmm13, [rcx + 0x0d0]
    movdqa xmm14, [rcx + 0x0e0]
    movdqa xmm15, [rcx + 0x0f0]
    jmp r8
	*/

	void build_sandbox() {
		if (!wrap) {
			void* mem = exec_alloc(sizeof(setjmp_data));
			std::memcpy(mem, setjmp_data, sizeof(setjmp_data));
			wrap = (int(*)(void*)) mem;
		}

		if (!longjmp_func) {
			void* mem = exec_alloc(sizeof(longjmp_data));
			std::memcpy(mem, longjmp_data, sizeof(longjmp_data));
			longjmp_func = (void(*)(void*,int)) mem;
		}
	}

	void* build_x64_call_wrapper(std::tuple<ILDataType, std::uint32_t, ILDataType*> decl) {
		auto& cwrps = call_wrappers[(std::uint8_t)ILCallingConvention::native];
		auto f = cwrps.find(decl);
		if (f != cwrps.end()) {
			return f->second;
		}
		else {
			std::vector<std::uint8_t> call_wrapper;

			std::uint32_t argc = std::get<1>(decl);
			ILDataType* argv = std::get<2>(decl);
			std::size_t stack = 0;

			std::size_t abs_v = (std::size_t)(b8_stack_ptr);
			call_wrapper.push_back(0x48);
			call_wrapper.push_back(0xB8);
			call_wrapper.push_back((std::uint8_t)(abs_v));
			call_wrapper.push_back((std::uint8_t)(abs_v>>8));
			call_wrapper.push_back((std::uint8_t)(abs_v>>16));
			call_wrapper.push_back((std::uint8_t)(abs_v>>24));
			call_wrapper.push_back((std::uint8_t)(abs_v>>32));
			call_wrapper.push_back((std::uint8_t)(abs_v>>40));
			call_wrapper.push_back((std::uint8_t)(abs_v>>48));
			call_wrapper.push_back((std::uint8_t)(abs_v>>56)); // movabs rax, b8_stack_ptr

			for (std::uint32_t i = argc; i > 0; --i) {
				ILDataType dt = argv[i - 1];

				std::uint8_t raxadd = 8;

				switch (dt) {
					
					case ILDataType::f64:
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
								stack += 8;
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
				std::size_t saligned = align_up(stack + 8, 32) - 8;

				call_wrapper.insert(call_wrapper.begin(),0x48);
				call_wrapper.insert(call_wrapper.begin()+1,0x83);
				call_wrapper.insert(call_wrapper.begin()+2, 0xEC);
				call_wrapper.insert(call_wrapper.begin()+3, (std::uint8_t)(saligned - stack)); // sub rsp, saligned-stack


				call_wrapper.push_back(0xFF);
				call_wrapper.push_back(0x10); // call [rax]


				call_wrapper.push_back(0x48);
				call_wrapper.push_back(0x83);
				call_wrapper.push_back(0xC4);
				call_wrapper.push_back((std::uint8_t)(saligned)); // add rsp, saligned


				call_wrapper.push_back(0xC3); // ret
			}
			else {
				// tail call optimization. without this optimization msvc release version fails sometimes
				call_wrapper.push_back(0xFF);
				call_wrapper.push_back(0x20); // jmp [rax]
			}


			void* mem = exec_alloc(call_wrapper.size());
			std::memcpy(mem, call_wrapper.data(), call_wrapper.size());
			cwrps[decl] = mem;
			return mem;
		}

	}

#endif

#ifdef LINUX

	errvoid push_64bit_temp_stack(ILEvaluator* eval, std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>>& decl, void* pointer) {
		std::size_t off = 0;
		for (std::size_t i = std::get<2>(decl).size(); i > 0; --i) {
			switch (std::get<2>(decl)[i - 1])
			{
				case ILDataType::i8:
				case ILDataType::u8: {
					std::uint8_t v;
					if (!eval->pop_register_value<std::uint8_t>(v)) return err::fail;
					b8_stack[off++] = v;
				} break;
				case ILDataType::i16:
				case ILDataType::u16: {
					std::uint16_t v;
					if (!eval->pop_register_value<std::uint16_t>(v)) return err::fail; 
					b8_stack[off++] = v;
				} break;
				case ILDataType::i32:
				case ILDataType::u32:
				case ILDataType::f32: {
					std::uint32_t v;
					if (!eval->pop_register_value<std::uint32_t>(v)) return err::fail;
					b8_stack[off++] = v;
				} break;
				case ILDataType::word:
				case ILDataType::f64:
				case ILDataType::i64:
				case ILDataType::u64: {
					std::uint64_t v;
					if (!eval->pop_register_value<std::uint64_t>(v)) return err::fail;
					b8_stack[off++] = v;
				} break;
				case ILDataType::dword: {
					dword_t dw;
					if (!eval->pop_register_value<dword_t>(dw)) return err::fail; 
					b8_stack[off++] = (std::uint64_t)dw.p2;
					b8_stack[off++] = (std::uint64_t)dw.p1;
				} break;

				default:
					break;
			}
		}

		b8_stack[off++] = (std::uint64_t)pointer;
		return err::ok;
	}

	struct state
	{
	    std::uint64_t rbx;
	    std::uint64_t rsp;
	    std::uint64_t rbp;
	    std::uint64_t r12;
	    std::uint64_t r13;
	    std::uint64_t r14;
	    std::uint64_t r15;
	    std::uint64_t rip;

	    uint128_t xmm8;
	    uint128_t xmm9;
	    uint128_t xmm10;
	    uint128_t xmm11;
	    uint128_t xmm12;
	    uint128_t xmm13;
	    uint128_t xmm14;
	    uint128_t xmm15;
	};

	state sandbox_state;
	void* sandbox = &sandbox_state;
	int (*wrap)(void*) = nullptr;
	void (*longjmp_func)(void*, int) = nullptr;

	unsigned char setjmp_data[] = { 0x48, 0x89, 0x1F, 0x4C, 0x8D, 0x44, 0x24, 0x08, 0x4C, 0x89, 0x47, 0x08, 0x48, 0x89, 0x6F, 0x10, 0x4C, 0x89, 0x67, 0x18, 0x4C, 0x89, 0x6F, 0x20, 0x4C, 0x89, 0x77, 0x28, 0x4C, 0x89, 0x7F, 0x30, 0x4C, 0x8B, 0x04, 0x24, 0x4C, 0x89, 0x47, 0x38, 0x66, 0x44, 0x0F, 0x7F, 0x47, 0x40, 0x66, 0x44, 0x0F, 0x7F, 0x4F, 0x50, 0x66, 0x44, 0x0F, 0x7F, 0x57, 0x60, 0x66, 0x44, 0x0F, 0x7F, 0x5F, 0x70, 0x66, 0x44, 0x0F, 0x7F, 0xA7, 0x80, 0x00, 0x00, 0x00, 0x66, 0x44, 0x0F, 0x7F, 0xAF, 0x90, 0x00, 0x00, 0x00, 0x66, 0x44, 0x0F, 0x7F, 0xB7, 0xA0, 0x00, 0x00, 0x00, 0x66, 0x44, 0x0F, 0x7F, 0xBF, 0xB0, 0x00, 0x00, 0x00, 0x31, 0xC0, 0xC3 };

	/*
 	mov [rdi], rbx
 	lea r8, [rsp+8]
 	mov [rdi + 8], r8
 	mov [rdi + 0x10], rbp
 	mov [rdi + 0x18], r12
 	mov [rdi + 0x20], r13
 	mov [rdi + 0x28], r14
 	mov [rdi + 0x30], r15
 	mov r8, [rsp]
 	mov [rdi + 0x38], r8
 	movdqa [rdi + 0x40], xmm8
 	movdqa [rdi + 0x50], xmm9
 	movdqa [rdi + 0x60], xmm10
 	movdqa [rdi + 0x70], xmm11
 	movdqa [rdi + 0x80], xmm12
 	movdqa [rdi + 0x90], xmm13
 	movdqa [rdi + 0x0a0], xmm14
 	movdqa [rdi + 0x0b0], xmm15
 	xor eax, eax
 	ret
	*/

	unsigned char longjmp_data[] = { 0x48, 0x89, 0xF0, 0x48, 0x8B, 0x1F, 0x48, 0x8B, 0x67, 0x08, 0x48, 0x8B, 0x6F, 0x10, 0x4C, 0x8B, 0x67, 0x18, 0x4C, 0x8B, 0x6F, 0x20, 0x4C, 0x8B, 0x77, 0x28, 0x4C, 0x8B, 0x7F, 0x30, 0x4C, 0x8B, 0x47, 0x38, 0x66, 0x44, 0x0F, 0x6F, 0x47, 0x40, 0x66, 0x44, 0x0F, 0x6F, 0x4F, 0x50, 0x66, 0x44, 0x0F, 0x6F, 0x57, 0x60, 0x66, 0x44, 0x0F, 0x6F, 0x5F, 0x70, 0x66, 0x44, 0x0F, 0x6F, 0xA7, 0x80, 0x00, 0x00, 0x00, 0x66, 0x44, 0x0F, 0x6F, 0xAF, 0x90, 0x00, 0x00, 0x00, 0x66, 0x44, 0x0F, 0x6F, 0xB7, 0xA0, 0x00, 0x00, 0x00, 0x66, 0x44, 0x0F, 0x6F, 0xBF, 0xB0, 0x00, 0x00, 0x00, 0x41, 0xFF, 0xE0 };

	/*
	mov rax, rsi
    mov rbx, [rdi]
    mov rsp, [rdi + 8]
    mov rbp, [rdi + 0x10]
    mov r12, [rdi + 0x18]
    mov r13, [rdi + 0x20]
    mov r14, [rdi + 0x28]
    mov r15, [rdi + 0x30]
    mov r8, [rdi + 0x38]
    movdqa xmm8,  [rdi + 0x40]
    movdqa xmm9,  [rdi + 0x50]
    movdqa xmm10, [rdi + 0x60]
    movdqa xmm11, [rdi + 0x70]
    movdqa xmm12, [rdi + 0x80]
    movdqa xmm13, [rdi + 0x90]
    movdqa xmm14, [rdi + 0x0a0]
    movdqa xmm15, [rdi + 0x0b0]
    jmp r8
	*/

	void build_sandbox() {
		if (!wrap) {
			void* mem = exec_alloc(sizeof(setjmp_data));
			std::memcpy(mem, setjmp_data, sizeof(setjmp_data));
			wrap = (int(*)(void*)) mem;
		}

		if (!longjmp_func) {
			void* mem = exec_alloc(sizeof(longjmp_data));
			std::memcpy(mem, longjmp_data, sizeof(longjmp_data));
			longjmp_func = (void(*)(void*,int)) mem;
		}
	}

	void* build_x64_call_wrapper(std::tuple<ILDataType, std::uint32_t, ILDataType*> decl) {
		auto& cwrps = call_wrappers[(std::uint8_t)ILCallingConvention::native];
		auto f = cwrps.find(decl);
		if (f != cwrps.end()) {
			return f->second;
		}
		else {
			std::vector<std::uint8_t> call_wrapper;

			std::uint32_t argc = std::get<1>(decl);
			ILDataType* argv = std::get<2>(decl);
			std::size_t stack = 0;

			std::size_t abs_v = (std::size_t)(b8_stack_ptr);
			call_wrapper.push_back(0x48);
			call_wrapper.push_back(0xB8);
			call_wrapper.push_back((std::uint8_t)(abs_v));
			call_wrapper.push_back((std::uint8_t)(abs_v>>8));
			call_wrapper.push_back((std::uint8_t)(abs_v>>16));
			call_wrapper.push_back((std::uint8_t)(abs_v>>24));
			call_wrapper.push_back((std::uint8_t)(abs_v>>32));
			call_wrapper.push_back((std::uint8_t)(abs_v>>40));
			call_wrapper.push_back((std::uint8_t)(abs_v>>48));
			call_wrapper.push_back((std::uint8_t)(abs_v>>56)); // movabs rax, b8_stack_ptr

			std::size_t ints = 0;
			std::size_t floats = 0;
			for (std::uint32_t i = 0; i < argc; ++i) {
				ILDataType dt = argv[i];

				switch (dt) {
					case ILDataType::f64:
					case ILDataType::f32:
						floats++;
						break;
					case ILDataType::dword:
						ints++;
						ints++;
						break;
					default:
						ints++;
						break;
				}
			}


			for (std::uint32_t i = argc; i > 0; --i) {
				ILDataType dt = argv[i - 1];

				std::uint8_t raxadd = 8;

				switch (dt) {
					
					case ILDataType::f64:
					case ILDataType::f32:
						switch (floats - 1) {
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
								
							case 4:
								call_wrapper.push_back(0xF3);
								call_wrapper.push_back(0x0F);
								call_wrapper.push_back(0x10);
								call_wrapper.push_back(0x20); // movss xmm4, [rax]
								break;

							case 5:
								call_wrapper.push_back(0xF3);
								call_wrapper.push_back(0x0F);
								call_wrapper.push_back(0x10);
								call_wrapper.push_back(0x28); // movss xmm5, [rax]
								break;


							case 6:
								call_wrapper.push_back(0xF3);
								call_wrapper.push_back(0x0F);
								call_wrapper.push_back(0x10);
								call_wrapper.push_back(0x30); // movss xmm6, [rax]
								break;


							case 7:
								call_wrapper.push_back(0xF3);
								call_wrapper.push_back(0x0F);
								call_wrapper.push_back(0x10);
								call_wrapper.push_back(0x38); // movss xmm7, [rax]
								break;


							default:
								call_wrapper.push_back(0xFF);
								call_wrapper.push_back(0x30); // push [rax]
								stack += 8;
						}

						--floats;
						break;
					case ILDataType::dword:
						switch (ints-1) {
							case 3:
								call_wrapper.push_back(0x48);
								call_wrapper.push_back(0x8B);
								call_wrapper.push_back(0x08); // mov rcx, [rax]
								break;
							case 2:
								call_wrapper.push_back(0x48);
								call_wrapper.push_back(0x8B);
								call_wrapper.push_back(0x10); // mov rdx, [rax]
								break;
							case 4:
								call_wrapper.push_back(0x4C);
								call_wrapper.push_back(0x8B);
								call_wrapper.push_back(0x00); // mov r8, [rax]
								break;
							case 5:
								call_wrapper.push_back(0x4C);
								call_wrapper.push_back(0x8B);
								call_wrapper.push_back(0x08); // mov r9, [rax]
								break;
							case 0:
								call_wrapper.push_back(0x48);
								call_wrapper.push_back(0x8B);
								call_wrapper.push_back(0x38); // mov rdi, [rax]
								break;
							case 1:
								call_wrapper.push_back(0x48);
								call_wrapper.push_back(0x8B);
								call_wrapper.push_back(0x30); // mov rsi, [rax]
								break;

							default:
								call_wrapper.push_back(0xFF);
								call_wrapper.push_back(0x30); // push [rax]
								stack += 8;
						}

						call_wrapper.push_back(0x48);
						call_wrapper.push_back(0x83);
						call_wrapper.push_back(0xC0);
						call_wrapper.push_back(8);
						--ints;

						//FALLTHROUGH
					default:
						switch (ints-1) {
							case 3:
								call_wrapper.push_back(0x48);
								call_wrapper.push_back(0x8B);
								call_wrapper.push_back(0x08); // mov rcx, [rax]
								break;
							case 2:
								call_wrapper.push_back(0x48);
								call_wrapper.push_back(0x8B);
								call_wrapper.push_back(0x10); // mov rdx, [rax]
								break;
							case 4:
								call_wrapper.push_back(0x4C);
								call_wrapper.push_back(0x8B);
								call_wrapper.push_back(0x00); // mov r8, [rax]
								break;
							case 5:
								call_wrapper.push_back(0x4C);
								call_wrapper.push_back(0x8B);
								call_wrapper.push_back(0x08); // mov r9, [rax]
								break;
							case 0:
								call_wrapper.push_back(0x48);
								call_wrapper.push_back(0x8B);
								call_wrapper.push_back(0x38); // mov rdi, [rax]
								break;
							case 1:
								call_wrapper.push_back(0x48);
								call_wrapper.push_back(0x8B);
								call_wrapper.push_back(0x30); // mov rsi, [rax]
								break;

							default:
								call_wrapper.push_back(0xFF);
								call_wrapper.push_back(0x30); // push [rax]
								stack += 8;
						}
						
						--ints;
						break;
				}

				call_wrapper.push_back(0x48);
				call_wrapper.push_back(0x83);
				call_wrapper.push_back(0xC0);
				call_wrapper.push_back(raxadd); // add rax, raxadd
			}

			if (stack > 0) {
				std::size_t saligned = align_up(stack + 8, 32) - 8;

				call_wrapper.insert(call_wrapper.begin(),0x48);
				call_wrapper.insert(call_wrapper.begin()+1,0x83);
				call_wrapper.insert(call_wrapper.begin()+2, 0xEC);
				call_wrapper.insert(call_wrapper.begin()+3, (std::uint8_t)(saligned - stack)); // sub rsp, saligned-stack


				call_wrapper.push_back(0xFF);
				call_wrapper.push_back(0x10); // call [rax]


				call_wrapper.push_back(0x48);
				call_wrapper.push_back(0x83);
				call_wrapper.push_back(0xC4);
				call_wrapper.push_back((std::uint8_t)(saligned)); // add rsp, saligned


				call_wrapper.push_back(0xC3); // ret
			}
			else {
				// tail call optimization.
				call_wrapper.push_back(0xFF);
				call_wrapper.push_back(0x20); // jmp [rax]
			}


			void* mem = exec_alloc(call_wrapper.size());
			std::memcpy(mem, call_wrapper.data(), call_wrapper.size());
			cwrps[decl] = mem;
			return mem;
		}

	}

#endif

	errvoid call_x64_call(ILEvaluator* eval, void* pointer, std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>>& decl) {
		auto asm_call_wrapper = build_x64_call_wrapper(std::make_tuple(std::get<1>(decl), (std::uint32_t)std::get<2>(decl).size(), std::get<2>(decl).data()));

		if (!push_64bit_temp_stack(eval, decl, pointer)) return err::fail;
		
		switch (std::get<1>(decl)) {
			case ILDataType::i8:
			case ILDataType::u8:
				eval->write_register_value(((std::uint8_t(*)())asm_call_wrapper)()); break;
			case ILDataType::i16:
			case ILDataType::u16:
				eval->write_register_value(((std::uint16_t(*)())asm_call_wrapper)()); break;
			case ILDataType::i32:
			case ILDataType::u32:
				eval->write_register_value(((std::uint32_t(*)())asm_call_wrapper)()); break;
			case ILDataType::i64:
			case ILDataType::u64:
			case ILDataType::word:
				eval->write_register_value(((std::uint64_t(*)())asm_call_wrapper)()); break;
			case ILDataType::f32:
				eval->write_register_value(((float(*)())asm_call_wrapper)()); break;
			case ILDataType::f64:
				eval->write_register_value(((double(*)())asm_call_wrapper)()); break;
			case ILDataType::dword:
				eval->write_register_value(((dword_t(*)())asm_call_wrapper)()); break;
			case ILDataType::none:
				((void(*)())asm_call_wrapper)(); break;
		}

		return err::ok;
	}



#endif

	errvoid abi_dynamic_call(ILEvaluator* eval, ILCallingConvention conv, void* ptr, std::tuple<ILCallingConvention, ILDataType, std::vector<ILDataType>>& decl) {
		switch (conv)
		{
#ifdef X86
			case Corrosive::ILCallingConvention::native:
				if (!call_x86_cdecl(eval,ptr, decl)) return err::fail;
				break;
			case Corrosive::ILCallingConvention::stdcall:
			#ifdef WINDOWS
				if (!call_x86_stdcall(eval, ptr, decl)) return err::fail;
			#else 
				if (!call_x86_cdecl(eval,ptr, decl)) return err::fail;
			#endif
				break;
#endif

#ifdef X64
			case Crs::ILCallingConvention::stdcall:
			case Crs::ILCallingConvention::native:
				if (!call_x64_call(eval, ptr, decl)) return err::fail; 
				break;
#endif
				break;
			default:
				break;
		}
		
		return err::ok;
	}

	void invalidate_sandbox() {
		wrap = nullptr;
		longjmp_func = nullptr;
	}
}


	