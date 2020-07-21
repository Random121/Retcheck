#pragma once
#include "disassembler.h"


#define isPrologue(x)	((x % 16 == 0) && \
						((*(BYTE*)x == 0x55 && *(WORD*)(x + 1) == 0xEC8B) || \
						(*(BYTE*)x == 0x53 && *(WORD*)(x + 1) == 0xDC8B) || \
						(*(BYTE*)x == 0x56 && *(WORD*)(x + 1) == 0xF48B)))


#define isEpilogue(x)	(*(WORD*)(x - 1) == 0xC35D || \
						(*(WORD*)(x - 1) == 0xC25D && *(WORD*)(x + 1) >= 0 && *(WORD*)(x + 1) % 4 == 0))


#define isValidCode(x)	!(*(ULONGLONG*)x == NULL && *(ULONGLONG*)(x + 8) == NULL)


auto savedRoutines = std::vector<DWORD>();


const char* getConvention(DWORD func, size_t n_Expected_Args) 
{
	const char* conv = nullptr;

	DWORD epilogue = func + 16;
	while (!isPrologue(epilogue) && isValidCode(epilogue)) epilogue += 16;

	DWORD args = 0;
	DWORD func_start = func;
	DWORD func_end = epilogue;
	while (!isEpilogue(epilogue)) epilogue--;

	if (*(BYTE*)epilogue == 0xC2) 
	{
		conv = "__stdcall";
	} else 
	{
		conv = "__cdecl";
	}

	// search for the highest ebp offset, which will 
	// indicate the number of args that were pushed
	// on the stack, rather than placed in ECX/EDX
	DWORD at = func_start;
	while (at < func_end) 
	{
		disassembler::inst i = disassembler::read(at);

		if (i.flags & Fl_dest_imm8 && i.dest.r32 == disassembler::reg_32::ebp && i.dest.imm8 != 4 && i.dest.imm8 < 0x7F) 
		{
			//printf("arg offset: %02X\n", i.dest.imm8);

			if (i.dest.imm8 > args) 
			{
				args = i.dest.imm8;
			}
		}
		else if (i.flags & Fl_src_imm8 && i.src.r32 == disassembler::reg_32::ebp && i.src.imm8 != 4 && i.src.imm8 < 0x7F) 
		{
			//printf("arg offset: %02X\n", i.src.imm8);

			if (i.src.imm8 > args) 
			{
				args = i.src.imm8;
			}
		}

		at += i.len;
	}

	// no pushed args were used, but we know there
	// is a 1 or 2 EBP-arg difference, so it is either
	// a fastcall or a thiscall
	if (args == 0) 
	{
		switch (n_Expected_Args) 
		{
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
	}
	else if (args == n_Expected_Args - 2) {
		conv = "__fastcall";
	}

	return conv;
}


// Usage: 
/*
 * // lua_getfield could be a fastcall
 * int new_lua_getfield = createRoutine(lua_getfield, 3); // we know that it ALWAYS takes 3 args
 * 
 * `new_lua_getfield` is now lua_getfield, but as a __cdecl function. 
 * You can now use __cdecl for your typedef ALL THE TIME now, by
 * using new_lua_getfield in place of the original lua_getfield address.
 * You never need to update the calling convention, ever again.*
 * 
 * *Unless the number of args changed, in which case
 *  you need to update your exploit regardless.
 * 
 * This function is highly optimized in its output
 * and it isn't going to affect the speed of your
 * exploit in any shape or form.
*/
int createRoutine(int function, int n_expected_args) 
{
	DWORD func = function;
	DWORD n_Args = n_expected_args;
	DWORD size = 0;
	BYTE data[128];

	const char* convention = getConvention(func, n_Args);
	if (strcmp(convention, "__cdecl") == 0) 
	{
		return func;
	}

	DWORD new_func = reinterpret_cast<DWORD>(VirtualAlloc(nullptr, 128, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
	if (new_func == NULL) 
	{
		printf("Error while allocating memory\n");
		return func;
	}

	data[size++] = 0x55; // push ebp

	data[size++] = 0x8B; // mov ebp,esp
	data[size++] = 0xEC;

	/*if (strcmp(convention, "__cdecl") == 0) 
	{
		for (int i = (n_Args * 4) + 8; i > 8; i -= 4) 
		{
			data[size++] = 0xFF; // push [ebp+??]
			data[size++] = 0x75;
			data[size++] = i - 4;
		}

		data[size++] = 0xE8; // call func
		*(DWORD*)(data + size) = func - (new_func + size + 4);
		size += 4;

		data[size++] = 0x83;
		data[size++] = 0xC4;
		data[size++] = n_Args * 4;

	else */if (strcmp(convention, "__stdcall") == 0) 
	{
		for (int i = (n_Args * 4) + 8; i > 8; i -= 4) 
		{
			data[size++] = 0xFF; // push [ebp+??]
			data[size++] = 0x75;
			data[size++] = i - 4;
		}

		data[size++] = 0xE8; // call func
		*(DWORD*)(data + size) = func - (new_func + size + 4);
		size += 4;
	}
	else if (strcmp(convention, "__thiscall") == 0) 
	{
		data[size++] = 0x51; // push ecx

		for (int i = n_Args; i > 1; i--) 
		{
			data[size++] = 0xFF; // push [ebp+??]
			data[size++] = 0x75;
			data[size++] = (i + 1) * 4;
		}

		data[size++] = 0x8B; // mov ecx,[ebp+08]
		data[size++] = 0x4D;
		data[size++] = 0x08;

		data[size++] = 0xE8; // call func
		*(DWORD*)(data + size) = func - (new_func + size + 4);
		size += 4;

		data[size++] = 0x59; // pop ecx
	}
	else if (strcmp(convention, "__fastcall") == 0) 
	{
		data[size++] = 0x51; // push ecx
		data[size++] = 0x52; // push edx

		for (int i = n_Args; i > 2; i--) 
		{
			data[size++] = 0xFF; // push [ebp+??]
			data[size++] = 0x75;
			data[size++] = (i + 1) * 4;
		}

		data[size++] = 0x8B; // mov ecx,[ebp+08]
		data[size++] = 0x4D;
		data[size++] = 0x08;
		data[size++] = 0x8B; // mov edx,[ebp+0C]
		data[size++] = 0x55;
		data[size++] = 0x0C;

		data[size++] = 0xE8; // call func
		*(DWORD*)(data + size) = func - (new_func + size + 4);
		size += 4;

		data[size++] = 0x59; // pop ecx
		data[size++] = 0x5A; // pop edx
	}

	data[size++] = 0x5D; // pop ebp
	data[size++] = 0xC3; // retn
	//data[size++] = 0xC2; // ret xx
	//data[size++] = n_Args * 4;
	//data[size++] = 0x00;

	memcpy_s(reinterpret_cast<void*>(new_func), size, &data, size);
	savedRoutines.push_back(new_func);

	return new_func;
}


