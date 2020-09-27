#pragma once
#include <Windows.h>
#include <vector>
#include <tuple>

namespace Retcheck
{
	struct packed_arg
	{
		uint32_t small_value; // +0
		uint64_t large_value; // +8
		bool is_large;		  // +16

		packed_arg(int data);
		packed_arg(uint32_t data);
		packed_arg(uint64_t data);
		packed_arg(const char* data);
	};

	extern void init();
	extern std::tuple<uint32_t, uint64_t> call(void* func, const char* convention, std::vector<packed_arg>args);
}
