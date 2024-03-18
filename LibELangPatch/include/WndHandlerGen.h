#pragma once

#include <cstdint>
#include <vector>

/**
 * 0041883A | E8 B19AFFFF  | call exe.4122F0            <-- CALL to get event handler
 * 0041887D | FF55 FC      | call dword ptr ss:[ebp-4]  <-- ELang Button Event Handler call
 * call_inst_address: 0x0041883A
 * call_inst_delta:   0xFFFF9AB1
 * @param call_inst_address
 * @param call_inst_delta
 * @return
 */
std::vector<uint8_t> GenerateWndHandlerCode(uint32_t call_inst_address, uint32_t call_inst_delta);
