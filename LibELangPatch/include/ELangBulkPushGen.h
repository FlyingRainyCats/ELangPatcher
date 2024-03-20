#pragma once

#include <cstdint>
#include <vector>

std::vector<uint8_t> GenerateBulkPushInstruction(uint32_t ret_delta, std::vector<uint32_t> values_to_push);
