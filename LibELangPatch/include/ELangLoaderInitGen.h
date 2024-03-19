#pragma once

#include <cstdint>
#include <vector>
#include <optional>

/**
 * 0040116D | FC          | cld             <-- addr start
 * 0040116E | DBE3        | fninit
 * 00401170 | E8 ECFFFFFF | call exe.401161 <-- call_delta = 0x0xFFFFFFEC
 * @param call_delta This can be `{}` if the call is empty.
 * @return
 */
std::vector<uint8_t> GenerateELangLoaderInit(std::optional<uint32_t> call_delta);
