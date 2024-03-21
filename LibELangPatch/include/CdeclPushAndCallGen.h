#pragma once

#include <cstdint>
#include <vector>

/**
  * 00401545 | 68 48000152 | push 52010048     <- this should not be relocatable
  * 0040154A | E8 11000000 | call exe.401560   <- call_delta = 0x11
  * 0040154F | 83C4 04     | add esp,4         <- stack_adjustment
  */
std::vector<uint8_t> GenerateCdeclPushAndCall(uint32_t push_value, uint32_t call_delta, uint32_t ret_delta);
