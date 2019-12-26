
#include "eyestep_util.h"
#include "eyestep.h"

namespace eyestep {
	namespace util {
		PROTECT_INFO::PROTECT_INFO() {
			address = 0;
			size = 1;
			oldProtect = 0x20;
		}

		extern const int MAX_STR_READ = 1024;

		extern DWORD bytesWritten = 0;
		extern DWORD bytesRead = 0;

		extern conv conv_cdecl = 0;
		extern conv conv_stdcall = 1;
		extern conv conv_thiscall = 2;
		extern conv conv_fastcall = 3;

		bool find_in_table(BYTE* t, BYTE b) {
			for (int i = 0; i < sizeof(t) / sizeof(BYTE); i++) {
				if (b == t[i]) {
					return true;
				}
			}
			return false;
		};

		bool isgood(int addr) {
			return (addr > base && addr < 0x3FFFFFFF && addr % 4 == 0);
		}

		
		cbyte::cbyte() {
			bytes = std::vector<BYTE>();
		}

		cbyte::cbyte(const char* str) {
			bytes = std::vector<BYTE>();
			std::string newstr = "";
			for (int i = 0; i < lstrlenA(str); i++) {
				if (str[i] != 0x20) {
					newstr += str[i];
				}
			}
			if ((newstr.length() / 2) > 0 && (newstr.length() % 2) == 0) {
				for (int i = 0; i < newstr.length(); i += 2) {
					char s[2];
					s[0] = newstr[i];
					s[1] = newstr[i + 1];
					add(convert::to_hex(s));
				}
			}
		}

		cbyte::cbyte(BYTE* byte_array, size_t count) {
			bytes = std::vector<BYTE>();
			if (sizeof(byte_array) > 0) {
				for (int i = 0; i < count; i++) {
					add(byte_array[i]);
				}
			}
		}

		cbyte::~cbyte() {}

		BYTE cbyte::at(int index) {
			if (index < bytes.size() && index >= 0) {
				return bytes[index];
			}
			else {
				throw std::exception("BAD CBYTE INDEX\n");
				return 0;
			}
		}

		void cbyte::add(BYTE b) {
			bytes.push_back(b);
		}

		size_t cbyte::size() {
			return bytes.size();
		}

		std::string cbyte::to_string() {
			std::string str = "";
			for (int i = 0; i < bytes.size(); i++) {
				str += convert::to_str(bytes[i]);
				if (i != (size() - 1))
					str += ", ";
				else
					str += ".";
			}
			return str;
		};


		int valloc(int addr, size_t size, DWORD alloc_type, DWORD page_flags){
			if (DLL_MODE) {
				return reinterpret_cast<int>(VirtualAlloc(reinterpret_cast<void*>(addr), size, alloc_type, page_flags));
			} else {
				return reinterpret_cast<int>(VirtualAllocEx(handle, reinterpret_cast<void*>(addr), size, alloc_type, page_flags));
			}
		}

		bool vfree(int addr, size_t size){
			if (DLL_MODE) {
				return VirtualFree(reinterpret_cast<void*>(addr), size, MEM_RELEASE);
			} else {
				return VirtualFreeEx(handle, reinterpret_cast<void*>(addr), size, MEM_RELEASE);
			}
		}

		// makes setting page protection really easy
		namespace edit {
			PROTECT_INFO unprotect(int address, size_t size) {
				PROTECT_INFO x;
				x.address = address;
				x.size = size;
				if (DLL_MODE){
					VirtualProtect(reinterpret_cast<void*>(address), size, 0x40, &x.oldProtect);
				} else {
					VirtualProtectEx(handle, reinterpret_cast<void*>(address), size, 0x40, &x.oldProtect);
				}
				return x;
			}

			// restores page protection after calling unprotect()
			void restore(PROTECT_INFO x) {
				DWORD nothing;
				if (DLL_MODE) {
					VirtualProtect(reinterpret_cast<void*>(x.address), x.size, x.oldProtect, &nothing);
				} else {
					VirtualProtectEx(handle, reinterpret_cast<void*>(x.address), x.size, x.oldProtect, &nothing);
				}
			}
		}

		// referencing arr (&) isn't required
		void readb(int addr, void* arr, size_t count) {
			if (DLL_MODE) {
				memcpy(arr, reinterpret_cast<void*>(addr), count);
			} else {
				ReadProcessMemory(handle, reinterpret_cast<void*>(addr), arr, count, &bytesRead);
			}
		}

		BYTE* readb(int addr, size_t count){
			BYTE* x = new BYTE[count];
			if (DLL_MODE) {
				memcpy(reinterpret_cast<void*>(x), reinterpret_cast<void*>(addr), count);
			} else {
				ReadProcessMemory(handle, reinterpret_cast<void*>(addr), x, count, &bytesRead);
				return x;
			}
		}

		cbyte readcb(int addr, size_t count){
			cbyte cb = cbyte();
			BYTE* data = readb(addr, count);
			for (int i = 0; i < count; i++) {
				cb.add(data[i]);
			}
			delete data;
			return cb;
		}

		BYTE readb(int addr){
			if (DLL_MODE) return *(BYTE*)addr;
			BYTE x = 0;
			ReadProcessMemory(handle, reinterpret_cast<void*>(addr), &x, 1, &bytesRead);
			return x;
		}

		USHORT readus(int addr){
			if (DLL_MODE) return *(USHORT*)addr;
			USHORT x = 0;
			ReadProcessMemory(handle, reinterpret_cast<void*>(addr), &x, 2, &bytesRead);
			return x;
		}

		UINT readui(int addr){
			if (DLL_MODE) return *(UINT*)addr;
			UINT x = 0;
			ReadProcessMemory(handle, reinterpret_cast<void*>(addr), &x, 4, &bytesRead);
			return x;
		}

		ULONGLONG readull(int addr){
			if (DLL_MODE) return *(ULONGLONG*)addr;
			ULONGLONG x = 0;
			ReadProcessMemory(handle, reinterpret_cast<void*>(addr), &x, 8, &bytesRead);
			return x;
		}

		// simplicity :p
		char readc(int addr){
			return static_cast<char>(readb(addr));
		}

		int16_t reads(int addr){
			return static_cast<int16_t>(readus(addr));
		}

		int32_t readi(int addr){
			return static_cast<int32_t>(readui(addr));
		}

		int64_t readll(int addr){
			return static_cast<int64_t>(readull(addr));
		}

		float readf(int addr){
			if (DLL_MODE) return *(float*)addr;
			float x = 0;
			ReadProcessMemory(handle, reinterpret_cast<void*>(addr), &x, 4, &bytesRead);
			return x;
		}

		double readd(int addr){
			if (DLL_MODE) return *(double*)addr;
			double x = 0;
			ReadProcessMemory(handle, reinterpret_cast<void*>(addr), &x, 8, &bytesRead);
			return x;
		}

		void write(int addr, void* v, size_t count){
			if (DLL_MODE) {
				memcpy(reinterpret_cast<void*>(addr), v, count);
			} else {
				WriteProcessMemory(handle, reinterpret_cast<void*>(addr), v, count, &bytesWritten);
			}
		}

		void write(int addr, BYTE v){
			if (DLL_MODE) {
				*(BYTE*)addr = v;
			} else {
				WriteProcessMemory(handle, reinterpret_cast<void*>(addr), &v, 1, &bytesWritten);
			}
		}

		void write(int addr, USHORT v){
			if (DLL_MODE) {
				*(USHORT*)addr = v;
			} else {
				WriteProcessMemory(handle, reinterpret_cast<void*>(addr), &v, 2, &bytesWritten);
			}
		}

		void write(int addr, UINT v){
			if (DLL_MODE){
				*(UINT*)addr = v;
			} else {
				WriteProcessMemory(handle, reinterpret_cast<void*>(addr), &v, 4, &bytesWritten);
			}
		}

		void write(int addr, ULONGLONG v){
			if (DLL_MODE) {
				*(ULONGLONG*)addr = v;
			} else {
				WriteProcessMemory(handle, reinterpret_cast<void*>(addr), &v, 8, &bytesWritten);
			}
		}

		void write(int addr, char v){
			write(addr, static_cast<BYTE>(v));
		}

		void write(int addr, int16_t v){
			write(addr, static_cast<uint16_t>(v));
		}

		void write(int addr, int32_t v){
			write(addr, static_cast<uint32_t>(v));
		}

		void write(int addr, int64_t v){
			write(addr, static_cast<uint64_t>(v));
		}

		void write(int addr, float v){
			if (DLL_MODE) {
				*(BYTE*)addr = v;
			} else {
				WriteProcessMemory(handle, reinterpret_cast<void*>(addr), &v, 4, &bytesWritten);
			}
		}
		void write(int addr, double v){
			if (DLL_MODE) {
				*(BYTE*)addr = v;
			} else {
				WriteProcessMemory(handle, reinterpret_cast<void*>(addr), &v, 8, &bytesWritten);
			}
		}

		std::string readstring(int addr) {
			int ptr = addr;
			std::string str = "";
			if (isgood(ptr)) {
				if (isgood(*(int*)(ptr))) {
					ptr = *(int*)ptr;
				}
				char c[MAX_STR_READ];
				memcpy(&c, reinterpret_cast<void*>(ptr), MAX_STR_READ);
				int i = 0;
				while (i < MAX_STR_READ && c[i] >= 0x20 && c[i] <= 0x7E) {
					str += c[i++];
				}
			}
			return str;
		}

		bool isprologue(int address) {
			BYTE b[3];
			readb(address, &b, 3);

			// push ebp
			// mov ebp,esp
			if (address % 16 == 0 && b[0] == 0x55 && b[1] == 0x8B && b[2] == 0xEC)
				return true;
			
			// push ebx
			// mov ebx,esp
			if (address % 16 == 0 && b[0] == 0x53 && b[1] == 0x8B && b[2] == 0xDC)
				return true;

			// some prologues use ESI, but this can rarely occur in a function
			if (address % 16 == 0 && b[0] == 0x56 && b[1] == 0x8B && b[2] == 0xF1) {
				return true;
				/*for (int i = 0; i < 0xFFFF; i++) {
					readb(address + i, &b, 2);
					if (b[0] == 0x5E && (b[1] == 0xC3 || b[1] == 0xC2)) {
						return true;
					}
				}*/
			}
			return false;
		}

		bool isepilogue(int address) {
			BYTE b[2];
			readb(address, &b, 2);

			return ((b[0] == 0x5D || b[0] == 0x5E) && // pop ebp, or pop esi,
				(b[1] == 0xC2 || b[1] == 0xC3));  // with a retn or ret XX
		}

		int nextprologue(int address, direction d, bool aligned) {
			int at = address, count = 0;
			// Skip this prologue if we're already at one
			if (isprologue(at)) {
				if (d == behind) at -= 16;
				if (d == ahead)  at += 16;
			}
			// go until the next prologue, but don't go into zero memory/end of .text
			// (simple check as otherwise it could go on forever)
			while (!isprologue(at) && !(at > base + base_size && readull(at) == 0 && readull(at + 8) == 0)) {
				if (count++ > 0xFFFF) break;
				if (d == ahead)  if (!aligned) at++; else at += 16;
				if (d == behind) if (!aligned) at--; else at -= 16;
			}
			return at;
		}

		int nextepilogue(int address, direction d) {
			int at = address, count = 0;
			while (!isepilogue(at)) {
				if (count++ > 0xFFFF) break;
				if (d == ahead)  at++;
				if (d == behind) at--;
			}
			return (at + 1); // Return the location of the functions `retn` address
		}

		int fsize(int func) {
			int eof = nextprologue(func, ahead, true);
			return (eof - func);
		}

		int getprologue(int addr) {
			return nextprologue(addr, behind, false);
		}

		std::vector<int> getprologues(int func, direction d, int count) {
			std::vector<int> result_list = std::vector<int>();
			int addr = func, current = 0;
			while (current < count) {
				addr = nextprologue(addr, d, true);
				result_list.push_back(addr);
				current++;
			}
			return result_list;
		}

		int getepilogue(int func) {
			return nextepilogue(nextprologue(func, ahead, true), behind);
		}

		std::vector<int> getepilogues(int func) {
			std::vector<int> result_list = std::vector<int>();
			int start = func;
			int end = (start + fsize(func));
			while (start < end) {
				if (isepilogue(start)) {
					result_list.push_back(start + 1);
				}
				start++;
			}
			return result_list;
		}

		short fretn(int func) {
			for (int addr : getepilogues(func)) {
				if (readb(addr) == 0xC2) {
					return reads(addr + 1);
				}
			}
			return 0;
		}

		std::vector<int> getcalls(int func) {
			std::vector<int> result_list = std::vector<int>();
			int start = func;
			int end = (func + fsize(func));
			while (start < end) {
				if (readb(start) == 0xE8) {
					int o = (start + readi(start + 1) + 5);
					if (o % 16 == 0 && o > base && o < base + base_size) {
						result_list.push_back(o);
					}
				}
				start++;
			}
			return result_list;
		}

		// returns all IMM32/DISP32 offsets/pointers used in
		// the instructions in the given function
		// 
		std::vector<int> getpointers(int func) {
			std::vector<int> result_list = std::vector<int>();
			int start = func;
			int end = (func + fsize(func));
			while (start < end) {
				inst i = read(start);
				int p = 0;
				if (i.flags & Fl_src_disp32)	p = i.src.disp32;
				if (i.flags & Fl_dest_disp32)	p = i.dest.disp32;
				if (i.flags & Fl_src_imm32)		p = i.src.imm32;
				if (i.flags & Fl_dest_imm32)	p = i.dest.imm32;
				if (p != 0) {
					if (p % 4 == 0 && p > base && p < base + base_size) result_list.push_back(p);
					if (p % 4 == 0 && p > base && p < base + base_size) result_list.push_back(p);
				}
				start += i.len;
			}
			return result_list;
		}

		// start at the E8 byte
		int getrel(int addr) {
			int o = (addr + readi(addr + 1) + 5);
			if (o % 16 == 0 && o > base && o < base + base_size)
				return o;
			return 0;
		}

		int nextcall(int func, direction d, bool loc) {
			int start = func;

			// opcodes to ignore before the E8 byte for more
			// accuracy on specifically the call instructions
			BYTE ignore[] = { 0x83, 0x89, 0x8B };

			// Skip current call if we're already at one
			if (readb(start) == 0xE8) {
				if (d == ahead)  start++;
				if (d == behind) start--;
			}

			while (!(readb(start) == 0xE8 && !find_in_table(ignore, readb(start - 1)))) {
				if (d == ahead)  start++;
				if (d == behind) start--;
			}

			int o = getrel(start);
			if (o != 0) if (loc) o = start;
			return o;
		}

		int nextxref(int start, direction d, int func, bool parentfunc) {
			int at = start;
			while (getrel(at) != func) {
				int old_at = at;
				at = nextcall(at, d, true);
				if (at == 0) {
					if (d == behind){
						at = --old_at;
					} else {
						at = ++old_at;
					}
				}
			}
			if (!parentfunc) return at;
			return nextprologue(at, behind, false);
		}

		std::vector<int> __fastcall scan(const char* aob, const char* _mask, int begin, int to, int stopAtResult) {
			int size = 0, at = 0, start = begin, end = to;
			if (!(start && end)) {
				start = base;
				end = base + base_size;
			}

			int chunksize = ((end - start) < (64 * 64 * 32)) ? (end - start) : (64 * 64 * 32);
			BYTE* buffer = new BYTE[chunksize];
			char match = 1;
			char wildchar = 'x'; // 'x' means it can be any byte; put '.' for the rest

			std::vector<int>results_list = std::vector<int>();
			std::vector<BYTE>bytes = cbyte(aob).bytes;
			size = bytes.size();

			// create mask for AOB if string is empty
			char mask[256];
			mask[0] = '\0';
			if (lstrlenA(_mask) == 0) {
				for (int i = 0; i < size; i++)
					strcat(mask, ".");
			}
			else strcpy(mask, _mask);

			// every (chunksize) bytes, we start a new buffer so there
			// is a 1/100000 chance the AOB we're looking for is between
			// two buffers
			int padding = (size - 1);

			// start scanning...
			while (start < end) {
				try {
					readb(start, reinterpret_cast<void*>(buffer), chunksize);
				} catch (std::exception e) { printf("Exception: %s\n", e.what()); }

				__asm push edi
				__asm xor edi, edi
				__asm jmp L2
		L1 :	__asm inc edi
				__asm mov at, edi
				match = 1;
				for (int x = 0; x < size; x++)
					if (buffer[at + x] != bytes[x] && mask[x] != wildchar)
						match = 0;
				if (match) results_list.push_back(start + at);
		L2:	__asm cmp edi, chunksize
				__asm jb L1
				__asm sub edi, padding
				__asm add start, edi
				__asm pop edi

				// faster scans
				if (stopAtResult != 0 && results_list.size() >= stopAtResult) {
					break;
				}
			}

			free(buffer);
			return results_list;
		}

		std::vector<int> __fastcall scanpointer(int address) {
			return scan(convert::to_bytes(address).c_str(), "....");
		}

		// strings are located onward from .rdata
		// so this wont necessarily work unless you adjust
		// things and bypass the scan check for .rdata section.
		// Otherwise, use this in an external application and
		// it'll work just fine.
		std::vector<int> __fastcall scanxrefs(const char* str, int result_number) {
			char* mask = new char[lstrlenA(str)];
			for (int i = 0; i < lstrlenA(str); i++) {
				mask[i] = '.';
			}
			std::vector<int>strlocs = scan(convert::to_bytes(str).c_str(), mask);
			free(mask);
			if (strlocs.size() == 0) return strlocs;
			return scanpointer(strlocs[result_number]);
		}

		// gets xrefs made to a function
		std::vector<int> __fastcall scanxrefs(int func, int begin, int to) {
			int size = 0, at = 0, start = begin, end = to;
			if (!(start && end)) {
				start = base;
				end = base + base_size;
			}

			int chunksize = ((end - begin) < (64 * 64 * 32)) ? (end - begin) : (64 * 64 * 32);
			BYTE* buffer = new BYTE[chunksize];
			bool found = false;
			std::vector<int>	xrefs = std::vector<int>();

			while (start < end && start < base + base_size) {
				bool read = true;
				readb(start, buffer, chunksize);
				if (read) {
					for (int i = 0; i < chunksize; i++) {
						if (readb(start + i) == 0xE8) { // call instruction
							// calculate relative offset
							int o = readi(start + i + 1);
							if ((start + i + 5 + o) == func) {
								xrefs.push_back(start + i);
								found = true;
								break;
							}
							i += 4;
						}
					}
				}
				if (found) break;
				start += chunksize - 5;
			}

			free(buffer);
			return xrefs;
		}

		// places a hook that removes itself, after
		// reading an offset of a register
		int debug(int address, BYTE r32, int offset) {
			int size = 5, nop, isize = 0, value = 0, at = 0;
			int code_loc = valloc(0, 128);
			int trace_loc = code_loc + 120;
			int hit_loc = trace_loc + 4;

			// Interpret size of original instruction(s)
			inst i = read(address);
			while (i.address < (address + size)) {
				isize += i.len;
				nop = ((i.address + i.len) - (address + size));
				i = read(address + isize);
			}

			// store original bytes
			BYTE* old_bytes = new BYTE[size + nop];
			readb(address, old_bytes, size + nop);

			// Make a JMP from the address to our own code
			BYTE* inject = new BYTE[5];
			*(BYTE*)(inject) = 0xE9;
			*(int*)(inject + 1) = (code_loc - (address + 5));

			BYTE code_data[128];
			if (offset == 0) { // simply place one instruction to capture 
				// the value of the register to our readout location
				*(BYTE*)(code_data + at++) = (0x50 + r32); // push (r32)
				switch (r32) {
				case reg_32::eax:
					*(BYTE*)(code_data + at++) = 0xA3;
					break;
				default:
					*(BYTE*)(code_data + at++) = 0x89; // ecx-edi (0xD,0x15,0x1D,0x25,0x2D . . .)
					*(BYTE*)(code_data + at++) = (0x5 + (r32 * 8));
					break;
				}
				// trace the register to our location
				*(int*)(code_data + at) = trace_loc; at += 4;
				// hit detected
				*(BYTE*)(code_data + at++) = 0xC7;
				*(BYTE*)(code_data + at++) = 0x5;
				*(int*)(code_data + at) = hit_loc;	at += 4;
				*(int*)(code_data + at) = 1;		at += 4;
				// pop (r32)
				*(BYTE*)(code_data + at++) = (0x58 + r32);
			}
			else { // or, if we want an offset of a register ...
				// move the offset into EAX and show EAX
				*(BYTE*)(code_data + at++) = 0x50; // push eax
				*(BYTE*)(code_data + at++) = 0x8B;
				if (offset > UCHAR_MAX) {	// 32 bit offset
					if (r32 != reg_32::esp)
						*(BYTE*)(code_data + at++) = (0x80 + r32);
					else {
						*(BYTE*)(code_data + at++) = 0x84;
						*(BYTE*)(code_data + at++) = 0x24;
					}
					*(int*)(code_data + at++) = offset;
				}
				else {					// 8 bit offset
					if (r32 != reg_32::esp)
						*(BYTE*)(code_data + at++) = (0x40 + r32);
					else {
						*(BYTE*)(code_data + at++) = 0x44;
						*(BYTE*)(code_data + at++) = 0x24;
					}
					*(BYTE*)(code_data + at++) = offset;
				}
				// Trace register to our trace location
				*(BYTE*)(code_data + at++) = 0xA3;
				*(int*)(code_data + at) = trace_loc; at += 4;
				// hit detected
				*(BYTE*)(code_data + at++) = 0xC7;
				*(BYTE*)(code_data + at++) = 0x5;
				*(int*)(code_data + at) = hit_loc;	at += 4;
				*(int*)(code_data + at) = 1;		at += 4;
				// pop eax
				*(BYTE*)(code_data + at++) = 0x58;
			}

			write(code_loc, &code_data, at);

			// Put overwritten bytes back (full instruction(s))
			write(code_loc + at, old_bytes, size + nop);
			at += (size + nop);
			// Place our JMP back
			write(code_loc + at++, (BYTE)0xE9);
			write(code_loc + at, ((address + 5) - (code_loc + at + 4)));
			at += 4;

			util::PROTECT_INFO p = edit::unprotect(address, size + nop);
			// Inject the JMP to our own code
			write(address, inject, size);
			for (int i = 0; i < nop; i++) {
				write(address + size + i, (BYTE)0x90);
			}
			edit::restore(p);
			delete inject;

			// wait for our masked value to be modified
			// this means something wrote to our location
			int d = 0;
			while (readb(hit_loc) != 1) {
				Sleep(1); if (d++ > 0xFFFF) break;
			}
			value = readi(trace_loc);

			p = edit::unprotect(address, size + nop);
			write(address, old_bytes, size + nop);
			edit::restore(p);
			delete old_bytes;

			vfree(code_loc, 128);
			return value;
		}


		// Use on an active function to dump [count] offsets of the register [reg].
		// neg determines whether the offsets are negative.
		// for example, dumpreg(location, ecx, 3, true);
		// returns the values of [ecx-0], [ecx-4], [ecx-8],
		// at that location in a function
		// dumpreg(location, ebp, 10, false); will give you a readout of all(well technically 10) args
		// presently used in the function (ignore the first two results/args start at ebp+8)
		// dumpreg(location, ebp, 10, true); will give you a readout of all local function-scope variables
		// presently used in the function
		std::vector<int> dumpreg(int address, BYTE reg, int count, bool neg) {
			int size = 5, nop, isize = 0, at = 0;
			int trace_loc = valloc(0, 128);
			int code_loc = trace_loc + 120;
			int hit_loc = code_loc + 4;
			std::vector<int> values = std::vector<int>();

			// Interpret size of original instruction(s)
			inst i = read(address);
			while (i.address < (address + size)) {
				isize += i.len;
				nop = ((i.address + i.len) - (address + size));
				i = read(address + isize);
			}

			// store original bytes
			BYTE* old_bytes = new BYTE[size + nop];
			readb(address, old_bytes, size + nop);

			BYTE dist = 19;
			if (neg) dist += 2;

			BYTE code_data[128];
			// loop/read +4 of the reg each time until we reach count*4
			// place it along trace_loc
			*(BYTE*)(code_data + at++) = 0x60; // pushad
			*(BYTE*)(code_data + at++) = 0x50; // push eax
			*(BYTE*)(code_data + at++) = 0x33; // xor eax,eax
			*(BYTE*)(code_data + at++) = 0xC0;
			*(BYTE*)(code_data + at++) = 0x53; // push ebx
			*(BYTE*)(code_data + at++) = 0x33; // xor ebx,ebx
			*(BYTE*)(code_data + at++) = 0xDB;
			*(BYTE*)(code_data + at++) = 0x51; // push ecx
			// loop:
			*(BYTE*)(code_data + at++) = 0x8B; // mov ebx,eax
			*(BYTE*)(code_data + at++) = 0xD8;
			if (neg) {
				*(BYTE*)(code_data + at++) = 0xF7; // neg ebx
				*(BYTE*)(code_data + at++) = 0xDB;
			}
			*(BYTE*)(code_data + at++) = 0x8B; // mov ecx,[ebx+ebp]
			*(BYTE*)(code_data + at++) = 0x0C;
			*(BYTE*)(code_data + at++) = ebx + (reg * 8);
			*(BYTE*)(code_data + at++) = 0x89; // mov [eax+trace_loc],ecx
			*(BYTE*)(code_data + at++) = 0x88;
			*(int*)(code_data + at) = trace_loc; at += 4;
			*(BYTE*)(code_data + at++) = 0x83; // add eax,04
			*(BYTE*)(code_data + at++) = 0xC0;
			*(BYTE*)(code_data + at++) = 0x04;
			*(BYTE*)(code_data + at++) = 0x83; // cmp eax,0x40 (64)
			*(BYTE*)(code_data + at++) = 0xF8;
			*(BYTE*)(code_data + at++) = static_cast<BYTE>(count * 4);
			*(BYTE*)(code_data + at++) = 0x72; // jb loop
			*(BYTE*)(code_data + at++) = static_cast<BYTE>(-dist);
			*(BYTE*)(code_data + at++) = 0xC7; // mov hit_loc, 1
			*(BYTE*)(code_data + at++) = 0x5;
			*(int*)(code_data + at) = hit_loc;	 at += 4;
			*(int*)(code_data + at) = 1;		 at += 4;
			// end
			*(BYTE*)(code_data + at++) = 0x59; // pop ecx
			*(BYTE*)(code_data + at++) = 0x5B; // pop ebx
			*(BYTE*)(code_data + at++) = 0x58; // pop eax
			*(BYTE*)(code_data + at++) = 0x61; // popad

			write(code_loc, &code_data, at);

			// Make a JMP from the address to our own code
			BYTE* inject = new BYTE[5];
			*(BYTE*)(inject) = 0xE9;
			*(int*)(inject + 1) = (code_loc - (address + 5));

			// Put overwritten bytes back (full instruction(s))
			write(code_loc + at, old_bytes, size + nop);
			at += (size + nop);

			// Place our JMP back
			write(code_loc + at++, (BYTE)0xE9);
			write((code_loc + at), ((address + 5) - (code_loc + at + 4)));
			at += 4;

			util::PROTECT_INFO p = edit::unprotect(address, size + nop);
			// Inject the JMP to our own code
			write(address, inject, size);
			for (int i = 0; i < nop; i++) {
				write(address + size + i, (BYTE)0x90);
			}
			edit::restore(p);
			delete inject;

			// wait for our masked value to be modified
			// this means something wrote to our location
			int d = 0;
			while (readb(hit_loc) != 1) {
				Sleep(1); if (d++ > 0xFFFF) break;
			}

			p = edit::unprotect(address, size + nop);
			write(address, old_bytes, size + nop);
			edit::restore(p);
			delete old_bytes;

			for (int i = 0; i < count; i++) {
				values.push_back(readi(trace_loc + (i * 4)));
			}

			vfree(code_loc, 128);
			return values;
		}


		conv getconv(int func) {
			conv c = conv_cdecl;
			if (fretn(func)) c = conv_stdcall;

			// try to determine the convention based on the instructions used
			int eof = getepilogue(func);
			int at = func;
			bool not_fastcall = false;
			bool neither = false;

			while (at < eof) {
				inst i = read(at);
				at += i.len;

				//if (func == aslr(0x7605E0)){ // for debugging
				//	printf("%s, %i, %i.\n", i.data, i.src.r32, i.dest.r32);
				//}
				// if edx or ecx was pushed thats an instant indication
				if (strcmp(i.opcode, "push") == 0) {
					if (i.src.r32 == ecx) break;//neither = true;
					if (i.src.r32 == edx) not_fastcall = true;
				}
				// completely ignore a test instruction, with ecx,ecx or edx,edx
				if (strcmp(i.opcode, "test") == 0) {
					if ((i.src.r32 == ecx && i.dest.r32 == ecx) || (i.src.r32 == edx && i.dest.r32 == edx)) {
						continue;
					}
				}
				// DONT ignore a xor instruction, with ecx,ecx or edx,edx
				if (strcmp(i.opcode, "xor") == 0) {
					if (i.src.r32 == ecx && i.dest.r32 == ecx) {
						neither = true;
						continue;
					}
					else if (i.src.r32 == edx && i.dest.r32 == edx) {
						not_fastcall = true;
						continue;
					}
				}

				if (i.flags & Fl_src_r32 || i.flags & Fl_src_rm32 || (i.flags & Fl_src_rxmm && i.src.pref && PRE_DWORD_PTR)) {
					if (strcmp(i.opcode, "mov") == 0 || strcmp(i.opcode, "lea") == 0 ||
						strcmp(i.opcode, "movzx") == 0 || strcmp(i.opcode, "or") == 0 ||
						strcmp(i.opcode, "xor") == 0 || strcmp(i.opcode, "xorps") == 0 ||
						strcmp(i.opcode, "movd") == 0 || strcmp(i.opcode, "cvt") != -1
						) {
						// because something like a mov ecx,edx
						// can also throw things off
						if (i.dest.r32 == edx && /*!neither && */!not_fastcall) {
							// if !neither is included,
							// it wont work if there's a push ecx prior to mov ???, edx
							// even though some __fastcall's/__thiscall's push ecx, fuck knows why
							return conv_fastcall;
							break;
						}
						// it will take a mov or lea instruction with ecx or edx
						// to indicate it is NOT a fastcall or thiscall.
						// But, something like "or ecx,1" has ecx in the src operand,
						// and its using it as if it was passed to the function
						if (i.src.r32 == ecx) neither = true;
						if (i.src.r32 == edx) not_fastcall = true;
					}
					else {
						if (strcmp(i.opcode, "cmp") == 0 || strcmp(i.opcode, "add") == 0) {
							if (i.src.r32 == edx && !not_fastcall && !neither) {
								return conv_fastcall;
								break;
							}
						}
					}
				}
				// now, we check if an instruction USES edx or ecx after
				// all that has passed, now that it's determined whether
				// it was or wasn't passed to this function
				if (i.flags & Fl_dest_r32 || i.flags & Fl_dest_rm32) {
					if (i.dest.r32 == edx) {
						if (!neither && !not_fastcall) {
							return conv_fastcall;
							break;
						}
					}
					if (i.dest.r32 == ecx) {
						if (c != conv_fastcall && !neither) {
							c = conv_thiscall;
						}
					}
					if (not_fastcall && c == conv_thiscall) {
						c = conv_thiscall;
						break;
					}
				}
			}

			// look forward and backwards for an XREF to this function
			at = func;
			for (int i = 0; i < 0xFFFF; i++) {
				if (readb(at - i) == 0xE8) {
					if (getrel(at - i) == func) {
						at = at - i;
						break;
					}
				} else if (readb(at + i) == 0xE8){
					if (getrel(at + i) == func) {
						at = at + i;
						break;
					}
				}
			}

			if (at != func) {
				int pos = at;
				// back up to a decent point
				// to start properly disassembling
				int i = 0;
				while (i < 24) {
					at--, i++;
					if (isprologue(at)) {
						at += 3;
						break;
					}
					if (i > 4) {
						if (readb(at) == 0xE8 || readb(at) == 0xE9) {
							at += 5;
							break;
						}
					}
				}

				std::vector<eyestep::inst> p = std::vector<eyestep::inst>();
				i = 0;
				while (at < pos) {
					eyestep::inst j = eyestep::read(at);
					p.push_back(j);
					at += j.len;
				}

				std::reverse(p.begin(), p.end());

				// Check the first 3 instructions...
				// This should be improved but 99% of the fastcall
				// functions are 2-3 args anyway.
				for (i = 0; i < 3; i++) {
					if (strcmp(p[i].data, "push edx") == 0) break;
					if (strcmp(p[i].data, "push ecx") == 0) break;
					if (p[i].rel32 > 0 || p[i].rel16 > 0 || p[i].rel8 > 0) break;

					if (p[i].flags & Fl_src_dest) {
						if (p[i].src.r32 == reg_32::edx) {
							c = conv_fastcall;
							break;
						}
						if (p[i].src.r32 == reg_32::ecx) {
							c = conv_thiscall;
						}
					}
				}
			}

			return c;
		}

		// converts a calling convention ID to a string
		const char* getsconv(conv c) {
			switch (c) {
				case 0:
					return "cdecl";
					break;
				case 1:
					return "stdcall";
					break;
				case 2:
					return "thiscall";
					break;
				case 3:
					return "fastcall";
					break;
				default:
					return "";
				break;
			}
		}

		// gets a calling convention of a function as a string
		const char* getsconv(int func) {
			return getsconv(getconv(func));
		}

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
		int transform_cdecl(int func, std::vector<int> args, conv _conv) {
			BYTE cc = _conv;
			if (cc == 0xF) cc = getconv(func);
			if (cc == conv_cdecl) return func;

			int routine = valloc(0, 128);
			int at = routine;
			int arg = 0;

			at += eyestep::write(at, "push ebp").len;
			at += eyestep::write(at, "mov ebp,esp").len;
			at += eyestep::write(at, "xor eax,eax").len;

			if ((cc == conv_fastcall || cc == conv_thiscall) && args.size() >= 1) {
				at += eyestep::write(at, "mov ecx,[ebp+08]").len;
				arg = 1;
			}
			if (cc == conv_fastcall && args.size() >= 2) {
				at += eyestep::write(at, "mov edx,[ebp+0C]").len;
				arg = 2;
			}

			int top = args.size();
			while (top > arg) {
				top--;
				BYTE bits = args[top];
				BYTE o = (8 + (top * 4));
				if (bits == 32) {
					// pushes a 32-bit arg
					char c_arg[8];
					sprintf_s(c_arg, "%02X", o);
					at += eyestep::write(at, "push [ebp+" + std::string(c_arg) + "]").len;
				}
				else if (bits == 64) {
					// handles pushing a 64-bit arg
					*(BYTE*)(at++) = 0xF2; // movsd xmm0, qword ptr [ebp+o]
					*(BYTE*)(at++) = 0x0F;
					*(BYTE*)(at++) = 0x10;
					*(BYTE*)(at++) = 0x45;
					*(BYTE*)(at++) = o;
					*(BYTE*)(at++) = 0x83; // sub esp,8
					*(BYTE*)(at++) = 0xEC;
					*(BYTE*)(at++) = 0x08;
					*(BYTE*)(at++) = 0xF2; // movsd qword ptr [esp],xmm0
					*(BYTE*)(at++) = 0x0F;
					*(BYTE*)(at++) = 0x11;
					*(BYTE*)(at++) = 0x04;
					*(BYTE*)(at++) = 0x24;
				}
			}

			at += eyestep::write(at, "call " + convert::to_str(func)).len;
			at += eyestep::write(at, "pop ebp").len;
			at += eyestep::write(at, "retn").len;

			int size = at - routine;
			for (int i = 0; i < (size + 16) % 16; i++) {
				at += eyestep::write(at, "int3").len;
			}

			return routine;
		}

		// cleans up routines generated from transform_cdecl
		int free_routine(int func) {
			vfree(func, 128);
		}

		// Get process PEB
		PEB::PEB* getPEB() {
			if (DLL_MODE) {
				int x = 0;
				__asm push eax;
				__asm mov eax, fs:[0x30];
				__asm mov x, eax;
				__asm pop eax;
				return reinterpret_cast<PEB::PEB*>(x);
			} else {
				PEB::PEB* peb = nullptr;
				int code_loc = valloc(0, 64, 0x1000);
				int output_loc = code_loc + 60;

				BYTE PEB_offset = 8;
				BYTE data[] = { 0x55, 0x8B, 0xEC, 0x50, 0x64, 0x8B, 0x5, 0x30, 0, 0, 0, 0x8B, 0x40, PEB_offset, 0xA3, 0xFF, 0xFF, 0xFF, 0xFF, 0x58, 0x5D, 0xC3 };
				*(int*)(data + 15) = output_loc;

				write(code_loc, &data, sizeof(data) / sizeof(BYTE));
				HANDLE thread = CreateRemoteThread(handle, 0, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(code_loc), 0, 0, NULL);
				WaitForSingleObject(thread, 1000);
				CloseHandle(thread);

				peb = reinterpret_cast<PEB::PEB*>(readi(output_loc));

				vfree(code_loc, 64);
				return peb;
			}
		}
	}
}
