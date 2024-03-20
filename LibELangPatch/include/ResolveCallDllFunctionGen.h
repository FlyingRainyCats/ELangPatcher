#pragma once

#include <cstdint>
#include <vector>

/**
 * .text:00419280 50             | push    eax
 * .text:00419281 E8 DA FF FF FF | call    sub_419260  <-- call_delta: 0xFFFFFFDA
 * .text:00419286 83 C4 04       | add     esp, 4
 * .text:00419289 FF E0          | jmp     eax
 * @param call_delta
 * @return
 */
std::vector<uint8_t> GenerateResolveCallDllFunction(uint32_t call_delta);
