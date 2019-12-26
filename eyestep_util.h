#ifndef H_EYESTEP_UTIL
#define H_EYESTEP_UTIL
#include <Windows.h>
#include <vector>
#include <string>
#include "peb.h"

typedef BYTE conv; // calling `convention` identifier

namespace eyestep {
	namespace util {
		// Direction that a function should go.
		// For example: nextcall(. . ., direction::ahead)
		// will go +1 addresses until it reaches a call instruction.
		// and direction::behind will go -1 from the given address
		enum direction {
			behind,
			ahead
		};

		// stores data for the previous page protection
		// which can be restored with edit::restore();
		struct PROTECT_INFO {
			int address;
			size_t size;
			DWORD oldProtect;
			PROTECT_INFO();
		};

		// Globals needed for use with utility functions
		extern const int MAX_STR_READ;

		extern DWORD bytesWritten;
		extern DWORD bytesRead;

		extern conv conv_cdecl;
		extern conv conv_stdcall;
		extern conv conv_thiscall;
		extern conv conv_fastcall;

		// Used for a few things
		bool find_in_table(BYTE* t, BYTE b);
		bool isgood(int addr);

		struct cbyte {
			cbyte();
			cbyte(const char* str);
			cbyte(BYTE* byte_array, size_t count);
			~cbyte();

			std::vector<BYTE>bytes;

			void add(BYTE b);
			BYTE at(int index);

			std::string to_string();
			size_t size();
		};

		int valloc(int addr, size_t size, DWORD alloc_type = MEM_COMMIT | MEM_RESERVE, DWORD page_flags = PAGE_EXECUTE_READWRITE);
		bool vfree(int addr, size_t size);

		// referencing arr (&) isn't required
		void readb(int addr, void* arr, size_t count);
		BYTE* readb(int addr, size_t count);
		cbyte readcb(int addr, size_t count);
		BYTE readb(int addr);
		USHORT readus(int addr);
		UINT readui(int addr);
		ULONGLONG readull(int addr);
		char readc(int addr);
		int16_t reads(int addr);
		int32_t readi(int addr);
		int64_t readll(int addr);
		float readf(int addr);
		double readd(int addr);

		void write(int addr, void* v, size_t count);
		void write(int addr, BYTE v);
		void write(int addr, USHORT v);
		void write(int addr, UINT v);
		void write(int addr, ULONGLONG v);
		void write(int addr, char v);
		void write(int addr, int16_t v);
		void write(int addr, int32_t v);
		void write(int addr, int64_t v);
		void write(int addr, float v);
		void write(int addr, double v);

		// makes setting page protection really easy
		namespace edit {
			PROTECT_INFO unprotect(int address, size_t size);
			void restore(PROTECT_INFO); // restores page protection after calling unprotect()
		}

		std::string readstring(int addr);

		bool isprologue(int address);
		bool isepilogue(int address);
		int nextprologue(int address, direction d, bool aligned = true);
		int nextepilogue(int address, direction d);
		int fsize(int func);
		short fretn(int func);

		int getprologue(int addr);
		int getepilogue(int func);
		std::vector<int> getprologues(int func, direction d, int count);
		std::vector<int> getepilogues(int func);
		std::vector<int> getcalls(int func);
		std::vector<int> getpointers(int func); // returns list of IMM32/DISP32 offsets/pointers used in the function
		int getrel(int addr);
		int nextcall(int func, direction d, bool loc = false);
		int nextxref(int start, direction d, int func, bool parentfunc = false);

		std::vector<int> __fastcall scan(const char* aob, const char* _mask, int begin = 0, int to = 0, int stopAtResult = 0);
		std::vector<int> __fastcall scanpointer(int address);

		// strings are located onward from .rdata
		// so this wont necessarily work unless you adjust
		// things and bypass the scan check for .rdata section.
		// Otherwise, use this in an external application and
		// it'll work just fine.
		std::vector<int> __fastcall scanxrefs(const char* str, int result_number = 0);
		std::vector<int> __fastcall scanxrefs(int func, int begin = 0, int to = 0); // gets xrefs made to a function

		// places a hook that removes itself, after
		// reading an offset of a register
		int debug(int address, BYTE r32, int offset = 0);

		// Use on an active function to dump [count] offsets of the register [reg].
		// neg determines whether the offsets are negative.
		// for example, dumpreg(location, ecx, 3, true);
		// returns the values of [ecx-0], [ecx-4], [ecx-8],
		// at that location in a function
		// dumpreg(location, ebp, 10, false); will give you a readout of all(well technically 10) args
		// presently used in the function (ignore the first two results/args start at ebp+8)
		// dumpreg(location, ebp, 10, true); will give you a readout of all local function-scope variables
		// presently used in the function
		std::vector<int> dumpreg(int address, BYTE reg, int count, bool neg = false);

		conv getconv(int func);

		// converts a calling convention ID to a string
		const char* getsconv(conv c);
		// gets a calling convention of a function as a string
		const char* getsconv(int func);

		// -#-#-#-#- transform_cdecl -#-#-#-#-
		//
		// 1st Arg is your function, 2nd Arg put a table of int values
		// (for each 32-bit arg put 32, and for each 64-bit arg for 64)
		// (for byte/char/short/int/long you would just put 32)
		// 
		// This returns a new routine for that function,
		// which is a __cdecl.
		// You can now keep your typedef a __cdecl for that function
		// and this will ensure that it is ALWAYS called as a __cdecl
		// as long as the accuracy of the calling convention determiner
		// is at 100%.
		// 
		// Every time a calling convention is wrong, I've only
		// improved the code for determining it.
		// 
		// If, for example, roblox's lua_pushvalue was a __fastcall,
		// you can now do this:
		// 
		// typedef void(__cdecl* T_lua_pushvalue)(int rL, int id);
		// T_lua_pushvalue r_pushvalue = (T_lua_pushvalue)transform_cdecl(retcheck::patch(aslr(addr)), {32,32}, getconv(addr));
		// 
		// Leave out the _conv arg when using this function
		// unless you want to specify the convention manually.
		// 
		int transform_cdecl(int func, std::vector<int> args, conv _conv = 0xF);

		// cleans up routines generated from transform_cdecl
		int free_routine(int func);

		// Get process PEB
		PEB::PEB* getPEB();
	}
}

#endif
