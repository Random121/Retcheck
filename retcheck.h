#pragma once
#include <Windows.h>
#include <vector>

namespace Retcheck 
{
	struct func_data 
	{
		uint32_t address;
		uint32_t size;
	};

	auto functions = std::vector<func_data>();
	
	uint32_t patch(uint32_t func_start)
	{
		auto func_end = func_start + 16;
		
		// very optimized (though this looks ugly)
		while (!(*reinterpret_cast<uint8_t*>(func_end) == 0x55 && *reinterpret_cast<uint16_t*>(func_end + 1) == 0xEC8B))
		{
			func_end += 16;
		}

		auto func_size = (func_end - func_start) + 4;
		auto new_func = reinterpret_cast<uint32_t>(VirtualAlloc(nullptr, func_size, 0x1000 | 0x2000, PAGE_EXECUTE_READWRITE));
		
		if (new_func == NULL)
		{
			return func_start;
		}

		auto func_at = func_start;
		auto new_func_at = new_func;
		auto spoof_address = *reinterpret_cast<uint32_t*>(__readfsdword(0x30) + 8) + 0x2800000;

		// scan through the function bytes
		while (func_at < func_end)
		{

			// look for a mov ???,[ebp+4] instruction
			if (*reinterpret_cast<uint8_t*>(func_at) == 0x8B			// is it mov instruction?
			 && *reinterpret_cast<uint8_t*>(func_at + 1) % 8 == 5		// second register == ebp?
			 && *reinterpret_cast<uint8_t*>(func_at + 1) / 0x40 == 1	// uses a 1-byte (imm8) offset?
			 && *reinterpret_cast<uint8_t*>(func_at + 2) == 0x04		// offset is +4?
			){
				auto first_register = (*reinterpret_cast<uint8_t*>(func_at + 1) - 0x40) / 8;

				// replace the instruction with a mov ???,spoof_address
				*reinterpret_cast<uint8_t*>(new_func_at++) = 0xB8 + first_register;
				*reinterpret_cast<uint32_t*>(new_func_at) = spoof_address;
				new_func_at += sizeof(uint32_t);

				func_at += 3;

				continue;
			}

			// fix relative calls
			if (*reinterpret_cast<uint8_t*>(func_at) == 0xE8
			 && 
				// check if it's aligned properly so it must call a function
			    (func_at + 5 + *reinterpret_cast<signed int*>(func_at + 1)) % 16 == 0
			){
				auto oldrel = *reinterpret_cast<uint32_t*>(func_at + 1);
				uint32_t relfunc = (func_at + oldrel) + 5;
				uint32_t newrel = relfunc - (new_func_at + 5); // the new distance

				*reinterpret_cast<uint8_t*>(new_func_at++) = 0xE8;
				*reinterpret_cast<uint32_t*>(new_func_at) = newrel;
				new_func_at += sizeof(uint32_t);

				func_at += 5;

				continue;
			}


			// add original byte to our function
			*reinterpret_cast<uint8_t*>(new_func_at++) = *reinterpret_cast<uint8_t*>(func_at++);
		}

		functions.push_back(func_data({ new_func, func_size }));

		return new_func;
	}

	// cleans up the patched functions
	void flush() 
	{
		for (size_t i = 0; i < functions.size(); i++) 
		{
			VirtualFree(reinterpret_cast<void*>(functions.at(i).address), 0, MEM_RELEASE);
		}

		functions.clear();
	}
}
