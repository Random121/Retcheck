#ifndef H_EYESTEP_RBX
#define H_EYESTEP_RBX
#include "eyestep_util.h"
#include <fstream>
#include <tchar.h>

using namespace eyestep;
using namespace eyestep::dllutil;

namespace rbx {
	char bFreeConsole;

	// loads console with the title, and prevents from being closed
	void open_console(LPCSTR title) {
		unsigned long ignore = 0;
		VirtualProtect(&FreeConsole, 1, PAGE_EXECUTE_READWRITE, &ignore);
		bFreeConsole = *(char*)&FreeConsole;
		*(char*)&FreeConsole = 0xC3;
		VirtualProtect(&FreeConsole, 1, ignore, &ignore);
	
		AllocConsole();
		freopen("conin$", "r", stdin); // Enable input
		freopen("conout$", "w", stdout); // Display output
		freopen("conout$", "w", stderr); // std error handling
		SetConsoleTitleA(title);
	}

	// allows console to close again
	void patch_console() {
		unsigned long ignore = 0;
		VirtualProtect(&FreeConsole, 1, PAGE_EXECUTE_READWRITE, &ignore);
		*(char*)&FreeConsole = bFreeConsole;
		VirtualProtect(&FreeConsole, 1, ignore, &ignore);
	}

	void patch_raiseexception() {
		uint32_t pRaiseException = reinterpret_cast<uint32_t>(GetProcAddress(GetModuleHandleA("kernelbase.dll"), "RaiseException"));
		unsigned long RE_oldprotect = 0;
		VirtualProtect(vcast(pRaiseException), 0x100, 0x40, &RE_oldprotect);
			*(uint8_t*)(pRaiseException + 5) = 0x8B; // mov esp,ebp
			*(uint8_t*)(pRaiseException + 6) = 0xE5;
			*(uint8_t*)(pRaiseException + 7) = 0x5D; // pop ebp
			*(uint8_t*)(pRaiseException + 8) = 0xC2; // ret 0010
			*(uint8_t*)(pRaiseException + 9) = 0x10;
			*(uint8_t*)(pRaiseException + 10) = 0x00;
		VirtualProtect(vcast(pRaiseException), 0x100, RE_oldprotect, &RE_oldprotect);
	}

	namespace instances {
		uint32_t replicator			= 0;
		uint32_t networkclient		= 0;
		uint32_t game				= 0;
		uint32_t script_context		= 0;
		uint32_t workspace			= 0;
		uint32_t players			= 0;

		namespace offsets {
			uint8_t name			= 44;
			uint8_t parent			= 56;
			uint8_t children_start	= 48;
			uint8_t children_end	= 4;
		}

		std::string getClass(uint32_t i){ return std::string((const char*)(*(int(**)(void))(*(DWORD*)i+16))()); }
		std::string getName(uint32_t i) { return readstring(i + offsets::name); }
		int	getParent(uint32_t i)		{ return *(uint32_t*)(i + offsets::parent); }

		std::vector<uint32_t>getChildren(uint32_t i) {
			std::vector<uint32_t> children;
			int CHILD_START = *(uint32_t*)(i + offsets::children_start);
			int CHILD_END = *(uint32_t*)(CHILD_START + offsets::children_end);
			for (int j=*(uint32_t*)(CHILD_START); j!=CHILD_END; j+=8)
				children.push_back(*(uint32_t*)j);
			return children;
		}

		uint32_t findFirstChild(uint32_t i, std::string name) {
			for (uint32_t v : getChildren(i))
				if (getName(v).find(name) != std::string::npos)
					return v;
			return 0;
		}

		void _checklog() {
			if (!instances::replicator){
				char c_fpath[MAX_PATH];
				GetModuleFileNameA(GetModuleHandleA(0), c_fpath, sizeof(c_fpath));
				std::string fpath(c_fpath), logspath="", filepath="";
				for (int i=0; i<fpath.length(); i++){
					if (fpath[i]<0x20 || fpath[i]>0x7E) break;
					if ((i+8)<fpath.length()){
						if (fpath.substr(i, 8) == "Versions") break; else {
							logspath += fpath[i];
						}
					} else break;
				}
				logspath += "logs"; // check logs directory

				WIN32_FIND_DATA FindFileData;
				HANDLE hFind;
				char patter[MAX_PATH];

				FILETIME bestDate = { 0, 0 };
				FILETIME curDate;

				memset(patter, 0x00, MAX_PATH);
				_stprintf(patter, TEXT("%s\\*.txt"), logspath.c_str());
				hFind = FindFirstFile(patter, &FindFileData);
				if (hFind != INVALID_HANDLE_VALUE){ 
					do { //ignore current and parent directories
						if (_tcscmp(FindFileData.cFileName,TEXT("."))==0 || _tcscmp(FindFileData.cFileName,TEXT(".."))==0)
							continue;
						if (!(FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
							std::string temppath = (logspath + "\\" + FindFileData.cFileName);
							curDate = FindFileData.ftCreationTime;
							if (CompareFileTime(&curDate, &bestDate) > 0){
								bestDate = curDate;
								filepath = temppath;
							}
						}
					} while (FindNextFile(hFind, &FindFileData));
					FindClose(hFind);
				}

				std::string fdata = "";
				std::ifstream ifs(filepath, std::ifstream::in);
				char c=ifs.get();
				while (ifs.good()) fdata+=c,c=ifs.get();
				ifs.close();

				size_t pos = fdata.find("Replicator created: ");
				if (pos != std::string::npos) {
					instances::replicator = convert::to_addr((char*)fdata.substr(pos + 20, 8).c_str());
				}
			}
		}

		// automatically resolves all vftables, and their offsets
		bool fetch() {
			//_checklog(); // Some ppl have weirdass path to their roblox logs
			//if (!instances::replicator) return false;
			if (!instances::replicator) {
				std::vector<uint32_t> results = dllutil::scan("74??8B??????000085??0F85", ".x.xxx...x..");
				if (results.size() != 0) {
					uint32_t RakPeerHook = results[0];
					uint8_t count = 0;
					while (*(uint32_t*)(RakPeerHook) != 0x57F18B56 && count <= 0xFF) RakPeerHook--, count++;
					if (count < 0xFF) {
						instances::replicator = dllutil::debug(RakPeerHook, eyestep::reg_32::ecx);
					}
				}
			}

			printf("Replicator: %08X.\n", instances::replicator);
			/*
			uint16_t i;
			i = 0;
			while (i < 0x80){
				i += 4;
				uint32_t x = *(uint32_t*)(instances::replicator + i);
				if (readstring(x).find("ClientReplicator") != std::string::npos && offsets::name == 0){
					offsets::name = i;
					i = 0;
				}
				if (offsets::parent == 0 && offsets::name != 0) {
					if (getName(x).find("NetworkClient") != std::string::npos) {
						instances::networkclient = x;
						offsets::parent = i;
					}
				}
			}
			*/
			instances::networkclient = getParent(instances::replicator);
			if (instances::networkclient){
				instances::game = getParent(instances::networkclient);
				printf("DataModel: %08X.\n", instances::game);
				/*
				i = 0;
				while (i < 0x80){
					i += 4;
					uint32_t o = *(uint32_t*)(instances::networkclient + i);
					if (!isgood(o)) continue;
					//if (isgood(*(uint32_t*)o) && isgood(*(uint32_t*)(o+4))){
						uint32_t space = (*(uint32_t*)(o+4) - *(uint32_t*)o);
						if (space % 8 == 0 && space < 0x1000){
							uint32_t start_location = *(uint32_t*)o;
							uint32_t first_child = *(uint32_t*)start_location;
							if (first_child == instances::replicator){
								offsets::children_start = i;
								offsets::children_end = 4;
								break;
							}
						}
					//}
				}
				*/
				if (instances::game) {
					instances::script_context = findFirstChild(instances::game, "Script Context");
					return true;
				} else {
					printf("Failed to find Game\n");
				}
			} else {
				printf("Failed to find NetworkClient\n");
			}

			return false;
		}
	}

	// use once for the active lua state from script context
	uint32_t get_lua_state(uint32_t script_context) {
		uint32_t lua_state = 0;

		std::vector<uint32_t>result_list = scan("B9000000000F45CA", "........");
		if (result_list.size() == 0)
			printf("No results for lua state\n"); 
		else {
			uint32_t at = result_list[0] + 8;
			uint8_t data[64];
			uint8_t size = 0;
			uint8_t b[2];

			while (size<64){
				memcpy(&b, vcast(at), 2);
				if (b[0]==0x56 && b[1]==0xE8) break;
				data[size++]=*(uint8_t*)(at++);
			}

			uint32_t func = reinterpret_cast<uint32_t>(VirtualAlloc(nullptr, 64, 0x1000|0x2000, 0x40));
			uint8_t func_data1[] = { 0x55, 0x8B, 0xEC, 0x56, 0xBE, 0xFF, 0xFF, 0xFF, 0xFF, 0x52, 0x51, 0xBA, 0x1, 0, 0, 0, 0xB9, 0, 0, 0, 0 };
			uint8_t func_data2[] = { 0x8B, 0xC6, 0x59, 0x5A, 0x5E, 0x5D, 0xC2, 0x4, 0 };
			*(uint32_t*)(func_data1 + 5) = script_context;

			memcpy(vcast(func), func_data1, 21);
			memcpy(vcast(func+21), data, size);
			memcpy(vcast(func+21+size), func_data2, 9);
				
			__asm {
				push eax
				call func
				mov lua_state, eax
				pop eax
			}

			VirtualFree(vcast(func), 64, MEM_RELEASE);
		}
		return lua_state;
	}

	namespace retcheck {
		union func_data {
			uint32_t address;
			uint32_t size;
		};

		std::vector<func_data> functions;

		uint32_t patch(uint32_t func_start) {
			uint32_t func_end = nextprologue(func_start, ahead);
			uint32_t func_size = func_end - func_start;
			uint32_t retcheck_at = func_start;

			uint8_t check[8];
			while (retcheck_at < func_end) {
				memcpy(&check, vcast(retcheck_at), 8);
				// find retcheck signature
				if (check[0] == 0x72 && check[2] == 0xA1 && check[7] == 0x8B) {
					retcheck_at = retcheck_at - func_start;
					break;
				}
				retcheck_at++;
			}

			if (retcheck_at == 0){
				return func_start;
			} else {
				// if there is retcheck . . .
				func_size = func_end - func_start;
				uint32_t func = reinterpret_cast<uint32_t>(VirtualAlloc(nullptr, func_size, 0x1000|0x2000, 0x40));
				uint32_t jmpstart = func + retcheck_at + 2;
				uint32_t newjmpdist = 0;

				uint8_t* data = new uint8_t[func_size];
				memcpy(data, vcast(func_start), func_size);

				uint32_t i = 0;
				while (i < func_size) {
					// Fix relative calls
					if (data[i] == 0xE8){
						uint32_t oldrel = *(uint32_t*)(func_start + i + 1);
						uint32_t relfunc = (func_start + i + oldrel) + 5;
					
						if (relfunc % 0x10 == 0){
							uint32_t newrel = relfunc - (func + i + 5);
							*(uint32_t*)(data + i + 1) = newrel;
							i += 4;
						}
					}
					i++;
				}

				// jump to the epilogue
				data[retcheck_at] = 0xEB;

				// write modified bytes to our new function
				memcpy(vcast(func), data, func_size);
				delete[] data;

				// store information about this de-retcheck'd function
				func_data rdata;
				rdata.address = func;
				rdata.size = func_size;
				functions.push_back(rdata);

				return func;
			}
		}

		// clean up for all retcheck functions
		void flush() {
			for (int i=0; i<functions.size(); i++) {
				VirtualFree(vcast(functions.at(i).address), functions.at(i).size, MEM_RELEASE);
			}
		}
	}
}

#endif
