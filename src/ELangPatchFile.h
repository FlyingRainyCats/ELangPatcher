#pragma once

#include <filesystem>

bool ELangPatchFile(const std::filesystem::path &file_path, bool backup, bool fake_stub);
