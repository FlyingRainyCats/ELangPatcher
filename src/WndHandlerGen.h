#pragma once

#include <cstdint>
#include <vector>
#include <random>

std::vector<uint8_t> GenerateWndHandlerCode(std::random_device &rd);
