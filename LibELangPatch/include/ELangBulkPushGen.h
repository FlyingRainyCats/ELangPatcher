#pragma once

#include <cstdint>
#include <vector>

std::vector<uint8_t> GenerateBulkPushInstruction(uint32_t ret_delta, std::vector<uint32_t> values_to_push);
std::vector<uint8_t> GenerateBulkPushInstructionWithEBXCall(
        uint32_t ret_delta, std::vector<uint32_t> values_to_push,
        uint32_t ebx_delta_to_ret_addr, uint32_t call_delta_to_ret_addr);
