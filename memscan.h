#pragma once
#include <Windows.h>
#include <vector>


enum scancheck_type {
    scancheck_byte_equal,
    scancheck_word_equal,
    scancheck_int_equal,
    scancheck_byte_notequal,
    scancheck_word_notequal,
    scancheck_int_notequal
};

namespace memscan {
    class scancheck {
    public:
        scancheck(scancheck_type _type, int _offset, long long _v) {
            type = _type;
            offset = _offset;
            v = _v;
        };
        ~scancheck() {};

        scancheck_type type;
        int offset;
        long long v;
    };

    BOOL compare(const BYTE* location, const BYTE* aob, const char* mask) {
        for (; *mask; ++aob, ++mask, ++location) {
            __try {
                if (*mask == '.' && *location != *aob)
                    return 0;
            }
            __except (EXCEPTION_EXECUTE_HANDLER) {
                return 0;
            }
        }
        return 1;
    }
    
    std::vector<int> find_pattern(char* strscan, bool vmscan, int alignment, int stopatresult,     std::vector<scancheck>scanchecks) {
        std::vector<int> results;

        // variables to ensure region can be scanned
        MEMORY_BASIC_INFORMATION mbi;
        BYTE protection = PAGE_EXECUTE_READ;
        DWORD start = 0;
        DWORD end = 0;
        DWORD rdata = 0;

        __asm {
            push eax;
            mov eax, fs:[0x30];
            mov eax, [eax + 8];
            mov start, eax;
            mov rdata, eax;
            pop eax;
        }

        // look for ".rda" marker
        while (*(int*)rdata != 0x6164722E) rdata++;
        rdata += 12;
        rdata = *(int*)rdata + start;
        end = rdata - 0x4000; // avoid the NOACCESS region before .rdata

        //printf("RobloxPlayerBeta.exe+%08X.\n", end - start);

        if (vmscan) {
            protection = PAGE_EXECUTE_READWRITE | PAGE_READWRITE;
            // Restrict scan to virtual memory
            start = end + 0x400000; // skip past (hopefully) all of .rdata
            end = 0x3FFFFFFF;
        }

        BYTE* pattern = new BYTE[128];
        char* mask = new char[128];
        RtlZeroMemory(pattern, 128);
        
        // reinterpret AOB string as BYTE array
        // '??' bytes will be skipped automatically
        for (int i = 0, j = 0; i < lstrlenA(strscan); i++) {
            if (strscan[i] == 0x20) continue;
            char x[2];
            x[0] = strscan[i];
            x[1] = strscan[1 + i++];
  
            if (x[0] == '?' && x[1] == '?') {
                pattern[j] = 0;
                mask[j++] = '?';
            } else {
                // convert 2 chars representing a byte to a byte
                int id = 0, n = 0;
convert:        if (x[id] > 0x60) n = x[id] - 0x57; // n = A-F (10-16)
                else if (x[id] > 0x40) n = x[id] - 0x37; // n = a-f (10-16)
                else if (x[id] >= 0x30) n = x[id] - 0x30; // number chars
                if (id != 0) pattern[j] += n; else {
                    pattern[j] += (n * 16);
                    id++;
                    goto convert;
                }
                mask[j++] = '.';
            }
        }

        while (start < end && VirtualQuery((void*)start, &mbi, sizeof(mbi))) {
        // Make sure the memory is committed, matches our protection, and isn't PAGE_GUARD.
            if ((mbi.State & MEM_COMMIT) && (mbi.Protect & protection) && !(mbi.Protect & PAGE_GUARD) && !(mbi.Protect & PAGE_NOACCESS)) {
                // Scan all the memory in the region.
                for (DWORD i = (DWORD)mbi.BaseAddress; i < (DWORD)mbi.BaseAddress + mbi.RegionSize; i += alignment) {
                    if (compare((BYTE*)i, pattern, mask)) {
                        if (scanchecks.size() == 0) {
                            results.push_back(i);
                        } else {
                            // Go through a series of extra checks,
                            // make sure all are passed before it's a valid result
                            int checks_pass = 1;
                            for (scancheck check : scanchecks) {
                                switch (check.type) {
                                    case scancheck_byte_equal:
                                        if (*(BYTE*)(i + check.offset) == (BYTE)check.v) checks_pass++;
                                        break;
                                    case scancheck_word_equal:
                                        if (*(WORD*)(i + check.offset) == (WORD)check.v) checks_pass++;
                                        break;
                                    case scancheck_int_equal:
                                        if (*(INT*)(i + check.offset) == (INT)check.v) checks_pass++;
                                        break;
                                    case scancheck_byte_notequal:
                                        if (*(BYTE*)(i + check.offset) != (BYTE)check.v) checks_pass++;
                                        break;
                                    case scancheck_word_notequal:
                                        if (*(WORD*)(i + check.offset) != (WORD)check.v) checks_pass++;
                                        break;
                                    case scancheck_int_notequal:
                                        if (*(INT*)(i + check.offset) != (INT)check.v) checks_pass++;
                                        break;
                                }
                            }
                            if (checks_pass == scanchecks.size()) {
                                results.push_back(i);
                            }
                        }
                        if (stopatresult > 0 && results.size() >= stopatresult) {
                            break;
                        }
                    }
                }
            }
            // Move onto the next region of memory.
            start += mbi.RegionSize;
        }

        delete[] mask;
        delete[] pattern;

        return results;
    }
  
    // ---------------------------------------------------------------------------
    // - if stopatresult is set to a number, scanning will end at that result.
    // 
    // - if `scanchecks` are present, it will go through a series of
    // checks after the initial scan, pinpointing it to a single result.
    // Basically, you can scan a tiny AOB and then simply add a
    // "check" to ensure that, for example, +8 of the result would be
    // a 0x8B byte.
    // However, because it does this check after the initial AOB scan,
    // for each result, /while/ it's scanning, the scan will be
    // extremely fast and effective.
    // Especially if you have it stop at the first result.
    // 
    // - if vmscan is set to true, it will scan virtual memory.
    // Otherwise, it is restricted to the .text section of the process
    // 
    // - if alignment is set to 4 for example, it will check for
    // the aob every +4 bytes.
    // this contributes to 4x the scan speed if you're scanning an
    // integer(which are always 4 byte aligned)
    // 
    std::vector<int> scan(const char* content, bool vmscan = false, int alignment = 1, int stopatresult = 0, std::vector<scancheck>scanchecks = std::vector<scancheck>()) {
        return find_pattern((char*)content, vmscan, alignment, stopatresult, scanchecks);
    }
    // Also, please note that the "scan mask" is automatic.
    // just put '??' as the byte in the aob string.
    // e.g. "558BEC8B??8D????5D"
    // 
}