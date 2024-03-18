#pragma once

#include <cstdint>
#include <vector>

/**
  * 00418C90 | 8D4424 08 | lea eax,dword ptr ss:[esp+8]  |
  * 00418C94 | 83EC 0C   | sub esp,C                     |
  * 00418C97 | 50        | push eax                      |
  * 00418C98 | FF7424 14 | push dword ptr ss:[esp+14]    |
  * 00418C9C | 33C0      | xor eax,eax                   |
  * 00418C9E | 894424 08 | mov dword ptr ss:[esp+8],eax  |
  * 00418CA2 | 894424 0C | mov dword ptr ss:[esp+C],eax  |
  * 00418CA6 | 894424 10 | mov dword ptr ss:[esp+10],eax |
  * 00418CAA | 8D5424 08 | lea edx,dword ptr ss:[esp+8]  |
  * 00418CAE | 52        | push edx                      |
  * 00418CAF | FFD3      | call ebx                      | <-- core: call to the function
  * 00418CB1 | 8B4424 0C | mov eax,dword ptr ss:[esp+C]  |
  * 00418CB5 | 8B5424 10 | mov edx,dword ptr ss:[esp+10] |
  * 00418CB9 | 8B4C24 14 | mov ecx,dword ptr ss:[esp+14] |
  * 00418CBD | 83C4 18   | add esp,18                    |
  * 00418CC0 | C3        | ret                           |
  */
std::vector<uint8_t> GenerateVArgsProxyCode();
