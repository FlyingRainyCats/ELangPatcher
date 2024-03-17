#pragma once

#include <cstdint>
#include <vector>
#include <random>

std::vector<uint8_t> GenerateELangInitSnippet(uint32_t offset_process_heap, uint32_t offset_has_ole, uint32_t* header_data);
// 0043659F | 8983 18040000 | mov dword ptr ds:[ebx+418],eax | <-- start address offset_process_heap: 0x418
// 004365A5 | A1 A0E64700   | mov eax,dword ptr ds:[47E6A0]  | <-- header_data[0]
//                                             (inst_address=0x004365A5, wnd_data_address=0x47E6A0)
// 004365AA | 8983 C4000000 | mov dword ptr ds:[ebx+C4],eax  |
// 004365B0 | 8B0D A4E64700 | mov ecx,dword ptr ds:[47E6A4]  | <-- header_data[1]
// 004365B6 | 8B83 10040000 | mov eax,dword ptr ds:[ebx+410] | <-- offset_has_ole: 0x410
// 004365BC | 898B C8000000 | mov dword ptr ds:[ebx+C8],ecx  |
// 004365C2 | 8B15 A8E64700 | mov edx,dword ptr ds:[47E6A8]  | <-- header_data[2]
// 004365C8 | 42            | inc edx                        |
// 004365C9 | 85C0          | test eax,eax                   |
// 004365CB | 8993 CC000000 | mov dword ptr ds:[ebx+CC],edx  | <-- end address (size = 44)
