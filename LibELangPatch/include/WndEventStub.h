#pragma once
#include <cstdint>
#include <vector>
#include <random>

std::vector<uint8_t> GenerateWndEventStubSnippet(uint32_t ecx_value, uint32_t call_delta);
// 15 bytes => junk nop
// mov ecx, ...
// call => jmp

// 00411540 | 8B4424 0C   | mov eax,dword ptr ss:[esp+C]
// 00411544 | 8B4C24 08   | mov ecx,dword ptr ss:[esp+8]
// 00411548 | 8B5424 04   | mov edx,dword ptr ss:[esp+4]
// 0041154C | 50          | push eax
// 0041154D | 51          | push ecx
// 0041154E | 52          | push edx
// 0041154F | B9 08F64A00 | mov ecx,测试2_5.1_静态编译.4AF608
// 00411554 | E8 87C7FFFF | call 测试2_5.1_静态编译.40DCE0
// 00411559 | C2 0C00     | ret C
