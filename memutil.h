#pragma once
#include <Windows.h>
#include <stdio.h>
#include <string>
#include "retcheck.h"
#include "eyestep.h"

namespace memutil {
	int rebase(int addr) {
		__asm {
			mov eax, fs:[0x30]
			mov eax, [eax+0x8]
			add eax, addr
		}
	}
  
	// start at the E8 byte
	int getrel(int addr) {
		int o = (addr + *(DWORD*)(addr + 1) + 5);
		if (o % 16 == 0) {
			return o;
		} else {
			return NULL;
		}
	}
  
	bool isprologue(int addr) {
		if (addr % 16 == 0) {
			// push ebp
			// mov ebp,esp
			if (*(BYTE*)addr == 0x55 && *(WORD*)(addr + 1) == 0xEC8B) {
				return true;
			}
			// push ebx
			// mov ebx,esp
			if (*(BYTE*)addr == 0x53 && *(WORD*)(addr + 1) == 0xDC8B) {
				return true;
			}
			// push esi
			// mov esi,esp
			if (*(BYTE*)addr == 0x56 && *(WORD*)(addr + 1) == 0xF48B) {
				return true;
			}
		}
		return false;
	}

	int getprologue(int addr) {
		while (!isprologue(addr)) addr--;
		return addr;
	}

	int nextprologue(int addr) {
		if (isprologue(addr)) addr += 0x10;
		while (!isprologue(addr)) {
			addr += 0x10;
			if (*(LONGLONG*)addr == 0 && *(LONGLONG*)(addr + 8) == 0) break;
		}
		return addr;
	}

	int prevcall(int addr, bool get_callinstruction_address = false) {
		int start = addr;

		// Skip current call if we're already at one
		if (*(BYTE*)start == 0xE8)
			start--;

		while (*(BYTE*)start != 0xE8 || getrel(start) == 0)
			start--;

		if (!get_callinstruction_address)
			start = getrel(start);

		return start;
	}

	int nextcall(int addr, bool get_call_location = false) {
		int start = addr;

		// Skip current call if we're already at one
		if (*(BYTE*)start == 0xE8)
			start++;

		while (*(BYTE*)start != 0xE8 || getrel(start) == 0)
			start++;

		if (!get_call_location)
			start = getrel(start);

		return start;
	}

	int fsize(int func) {
		return memutil::nextprologue(func) - func;
	}

	std::vector<int> getcalls(int func) {
		std::vector<int>calls;
		int start = func;
		int end = memutil::nextprologue(func);
		while (true) {
			start++;
			start = nextcall(start, true);
			if (start > end) {
				break;
			} else {
				calls.push_back(getrel(start));
			}
		}
		return calls;
	}

	bool isepilogue(int at) {
		if (!(*(BYTE*)(at - 1) == 0x5D)) return false;
		return (*(BYTE*)at == 0xC3 || (*(BYTE*)at == 0xC2 && *(BYTE*)(at + 2) == 0x0));
	}

	WORD getretn(int func) {
		int at = nextprologue(func);
		while (!isepilogue(at)) at--;
		if (*(BYTE*)at == 0xC2 && *(BYTE*)(at + 2) == 0x00) return *(WORD*)(at + 1);
		return 0;
	}

	int nextxref(int start_address, int func_look_for) {
		int at = start_address;
		while (memutil::nextcall(at) != func_look_for) {
			at = memutil::nextcall(at, true);
		}
		return memutil::getprologue(memutil::nextcall(at, true));
	}

	namespace conventions {
		const char* get(int func, int n_Expected_Args) {
			const char* conv = nullptr;

			if (getretn(func) > 0) {
				conv = "__stdcall";
			} else {
				conv = "__cdecl";
			}

			// search for the highest ebp offset,
			// which will easily indicate the number of
			// args that were pushed onto the stack
			// rather than placed in ECX/EDX
			int args = 0;
			int func_start = func;
			int func_end = nextprologue(func_start);

			int at = func_start;
			while (at < func_end) {
				eyestep::inst i = eyestep::read(at);

				if (i.flags & Fl_dest_imm8 && i.dest.r32 == eyestep::reg_32::ebp && i.dest.imm8 != 4 && i.dest.imm8 < 0x7F){
					//printf("arg offset: %02X\n", i.dest.imm8);
					if (i.dest.imm8 > args) {
						args = i.dest.imm8;
					}
				} else if (i.flags & Fl_src_imm8 && i.src.r32 == eyestep::reg_32::ebp && i.src.imm8 != 4 && i.src.imm8 < 0x7F){
					//printf("arg offset: %02X\n", i.src.imm8);
					if (i.src.imm8 > args) {
						args = i.src.imm8;
					}
				}

				at += i.len;
			}

			// no pushed args were used, but we know there
			// must be 1 or 2 args so it is either a fastcall
			// or a thiscall!
			if (args == 0) {
				switch (n_Expected_Args) {
				case 1:
					return "__thiscall";
					break;
				case 2:
					return "__fastcall";
					break;
				}
			}

			args -= 8;
			args = (args / 4) + 1;
			
			if (args == n_Expected_Args - 1) {
				conv = "__thiscall";
			} else if (args == n_Expected_Args - 2) {
				conv = "__fastcall";
			}
			
			return conv;
		}
	}
}


