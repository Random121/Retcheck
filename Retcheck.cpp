// written by static 9/27/2020
//

#include "retcheck.h"
#include <cstdarg>

namespace Retcheck
{
	//private variables defined outside of the function scope
	//so our assembling is more predictable and accurate
	uint32_t routine = 0;
	uint32_t redirect = 0;
	uint32_t r_func = 0;
	uint8_t is_thiscall = 0;
	uint8_t is_fastcall = 0;
	uint32_t arg_ecx = 0;
	uint32_t arg_edx = 0;
	uint32_t _return32 = 0;
	uint64_t _return64 = 0;
	uint32_t stack_cleanup = 0;
	size_t n_args = 0;
	packed_arg* arg_data = nullptr;

	packed_arg::packed_arg(int data)
	{
		small_value = data;
		large_value = 0;
		is_large = false;
	};

	packed_arg::packed_arg(uint32_t data)
	{
		small_value = data;
		large_value = 0;
		is_large = false;
	};

	packed_arg::packed_arg(uint64_t data)
	{
		small_value = 0;
		large_value = data;
		is_large = true;
	};

	packed_arg::packed_arg(const char* data)
	{
		small_value = reinterpret_cast<uint32_t>(data);
		large_value = 0;
		is_large = false;
	};

	void init()
	{
		// scan for a jmp dword ptr[xxxxxxxx]
		// this provides an escape outlet within the .text section
		// that the function will return to, and then jmp immediately
		// back after our initial jmp, to continue execution flow.
		routine = *reinterpret_cast<uint32_t*>(__readfsdword(0x30) + 8);

		// AOB scan for this signature:
		// FF25????????55
		while (!(
			*reinterpret_cast<uint16_t*>(routine) == 0x25FF
		 && 
			*reinterpret_cast<uint8_t *>(routine + 6) == 0x55
		))
		{
			routine++;
		}

		// Get the ???????? pointer
		redirect = *reinterpret_cast<uint32_t*>(routine + 2);
	}

	// Emulate the call -- this uses a jmp instead of a call,
	// in which we set up our own prologue and skip
	// the original. The goal is to have [ebp+4]
	// set to a particular type of `jmp` instruction
	// in roblox's .text section.
	// We have to pick one that jumps to a pointer
	// that we're able to edit without triggering memcheck.
	// It passes retcheck because this jmp instruction
	// is in the .text section.
	// We rig the pointer of the jmp to jump back to our own
	// code, continuing flow of execution without ever
	// modifying any of roblox's code.
	// 
	std::tuple<uint32_t, uint64_t> call(void* pfunc, const char* conv, std::vector<packed_arg>args)
	{
		n_args = 0;
		arg_data = nullptr;
		stack_cleanup = 0;
		is_thiscall = FALSE;
		is_fastcall = FALSE;
		_return32 = 0;
		_return64 = 0;
		arg_ecx = 0;
		arg_edx = 0;
		arg_data = nullptr;
		r_func = reinterpret_cast<uint32_t>(pfunc);

		// ensure that the prologue of this ROBLOX
		// function is skipped, so that we can set
		// up our own (rigged)
		if (*reinterpret_cast<uint8_t*>(r_func) == 0x55)
		{
			r_func += 3;
		}

		if (strcmp(conv, "fastcall") == 0)
		{
			is_fastcall = TRUE;

			arg_edx = args[1].small_value;
			args.erase(args.begin() + 1);

			arg_ecx = args[0].small_value;
			args.erase(args.begin() + 0);
		}
		else if (strcmp(conv, "thiscall") == 0)
		{
			is_thiscall = TRUE;

			arg_ecx = args[0].small_value;
			args.erase(args.begin() + 0);
		}
		else if (strcmp(conv, "cdecl") == 0)
		{
			stack_cleanup = args.size() * 4;
		}

		std::reverse(args.begin(), args.end());

		arg_data = args.data();
		n_args = args.size();

		__asm
		{
			push esi;
			push eax;
			push edi;
			xor edi, edi;
			xor eax, eax;
		iterate_args:
			mov esi, arg_data;
			add esi, eax; // esi = current arg struct
			cmp edi, [n_args];
			jge args_loaded;
			// first iteration
			cmp byte ptr[esi + 0x10], 1; // ->is_large
			je push_large_arg;
			// ... PUSH SMALL ARG ONTO STACK
			push dword ptr[esi + 0x00];
			// ...
			inc edi;
			add eax, 0x18;
			jmp iterate_args;
		push_large_arg:
			// ... PUSH LARGE ARG ONTO STACK
			push qword ptr[esi + 0x08];
			// ...
			inc edi;
			add eax, 0x18;
			jmp iterate_args;
		args_loaded:

			// Set ECX and/or EDX if necessary.
			// We have already removed the first one or two args
			// from the list being pushed normally
			// and have placed it into arg_ecx/arg_edx
			cmp byte ptr[is_thiscall], 0;
			je check2;
			mov esi, arg_data;
			mov ecx, dword ptr[arg_ecx];
			jmp ready;
		check2:
			cmp byte ptr[is_fastcall], 0;
			je ready;
			mov esi, arg_data;
			mov ecx, dword ptr[arg_ecx];
			mov edx, dword ptr[arg_edx];
		ready:
			call retcheck_crusher;
		retcheck_crusher:
			push ebp;
			mov ebp, esp;
			// carefully set the return location
			// to the instruction immediately after our
			// `jmp dword ptr`.
			// we need to literally check the distance from
			// `retcheck_crusher` to `func_return`
			push edi;
			mov edi, [ebp + 4]; // address of `retcheck_crusher` (during run-time)
			add edi, 0x24; // this is the correct distance to get to `func_return`
			push esi;
			mov esi, [redirect];
			mov dword ptr[esi], edi;
			pop esi;
			mov edi, routine;
			mov[ebp + 4], edi; // <---- spoof return
			pop edi;
			// jmp to the function now...which
			// we've skipped the prologue of 
			jmp dword ptr[r_func];
		func_return:

			// store return value into our variable
			mov [_return32], eax;
			add esp, [stack_cleanup];

			// support for 64-bit return values / (untested)
			movq xmm0, qword ptr[esp];
			movq [_return64], xmm0;
			pop eax;
			pop edi;
			pop esi;
		}

		return std::make_tuple(_return32, _return64);
	}
}
