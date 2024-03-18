#pragma once

#include <cstdint>
#include <vector>

/**
 * 00419260 | 55          | push ebp
 * 00419261 | 8BEC        | mov ebp,esp
 * 00419263 | 8B45 08     | mov eax,dword ptr ss:[ebp+8]
 * 00419266 | 50          | push eax
 * 00419267 | B9 18264B00 | mov ecx, exe.4B2618             <- key = 0x004B2618
 * 0041926C | E8 BF7DFFFF | call exe.411030                 <- call_delta = 0xFFFF7DBF
 * 00419271 | 5D          | pop ebp
 * 00419272 | C3          | ret
 * @param ecx_value
 * @param call_delta
 * @return
 */
std::vector<uint8_t> GenerateDllFunctionCallStub(uint32_t ecx_value, uint32_t call_delta);
