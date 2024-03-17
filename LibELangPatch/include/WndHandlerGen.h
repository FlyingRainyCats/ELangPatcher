#pragma once

#include <cstdint>
#include <vector>
#include <random>

std::vector<uint8_t> GenerateWndHandlerCode(uint32_t call_inst_address, uint32_t call_inst_delta);
// 0041883A | E8 B19AFFFF              | call 测试2_5.1_静态编译_q1.4122F0                        |
// call_inst_address: 0x0041883A
// call_inst_delta: 0xFFFF9AB1
