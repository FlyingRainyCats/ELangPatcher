#pragma once

#include <filesystem>
#include <string>

bool ELangPatchFile(const std::filesystem::path &file_path, const std::u8string& suffix, bool backup, bool fake_stub);
