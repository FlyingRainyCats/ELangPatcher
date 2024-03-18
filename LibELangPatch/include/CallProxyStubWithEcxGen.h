#pragma once

#include <cstdint>
#include <vector>

/**
 * @desc Replace the following function
 * 15 bytes => junk nop
 * mov ecx, ...
 * call => jmp
 * 00411540 | 8B4424 0C   | mov eax,dword ptr ss:[esp+C]
 * 00411544 | 8B4C24 08   | mov ecx,dword ptr ss:[esp+8]
 * 00411548 | 8B5424 04   | mov edx,dword ptr ss:[esp+4]
 * 0041154C | 50          | push eax
 * 0041154D | 51          | push ecx
 * 0041154E | 52          | push edx
 * 0041154F | B9 08F64A00 | mov ecx, exe.4AF608      <- `ecx_value`
 * 00411554 | E8 87C7FFFF | call exe.40DCE0          <- `call_delta`
 * 00411559 | C2 0C00     | ret C
 * @param pre_junk_len The number of bytes until mov ecx. e.g. `15`.
 * @param post_junk_len The number of bytes available afterwards, .e.g `3`.
 * @param ecx_value
 * @param call_delta
 * @return
 */
std::vector<uint8_t> GenerateCallProxyStubWithEcx(int pre_junk_len, int post_junk_len, uint32_t ecx_value, uint32_t call_delta);
