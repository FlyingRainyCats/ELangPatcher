#include "ELangPatchFile.h"
#include "ELangPatcher.h"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

template<typename T>
const char *get_stream_error(const T &file) {
    if (file.bad()) {
        return "Unrecoverable I/O error occurred.";
    } else if (file.eof()) {
        return "End-of-File reached.";
    } else if (file.fail()) {
        return "Non-fatal I/O error occurred.";
    } else if (file.good()) {
        return "No error occurred.";
    }
    return "Unknown error.";
}

bool ELangPatchFile(const fs::path &file_path, const std::u8string &suffix, bool backup, bool fake_stub) {
    if (!fs::exists(file_path)) {
        fprintf(stderr, "  ERR: file does not exist.\n");
        return false;
    }

    fs::path output_path{file_path};
    auto output_file_name = output_path.stem().u8string() + suffix + output_path.extension().u8string();
    output_path.replace_filename(output_file_name);

    if (backup && fs::exists(output_path)) {
        auto bak_file{output_path};
        bak_file.replace_extension(output_path.extension().u8string() + u8".bak");

        if (!fs::exists(bak_file)) {
            std::error_code ec_backup{};
            fs::copy_file(file_path, bak_file, ec_backup);
            if (ec_backup) {
                fprintf(stderr, "  ERR: backup failed: %s\n", ec_backup.message().c_str());
                return false;
            }
        }
    }

    auto file_size = static_cast<std::streamsize>(fs::file_size(file_path));
    std::vector<uint8_t> exe_data(file_size);
    exe_data.reserve(file_size * 2 + 0x4000);
    {
        std::ifstream ifs(file_path, std::ifstream::binary);
        if (!ifs.is_open()) {
            fprintf(stderr, "  ERR: could not open file for read: %s\n", get_stream_error(ifs));
            return false;
        }
        ifs.read(reinterpret_cast<char *>(exe_data.data()), file_size);
    }

    auto patcher = MakeELangPatcher(exe_data);
    patcher->PatchEWndV02();
    patcher->PatchEWndUltimate();
    patcher->PatchWndEventHandlerMain();
    patcher->PatchKernelInvokeCall();
    patcher->PatchProxyStub();
    patcher->PatchELangLoaderInitStub();
    if (fake_stub) patcher->PatchAddFakeEWndStub();

    {
        std::ofstream ofs(output_path, std::ifstream::binary);
        if (!ofs.is_open()) {
            fprintf(stderr, "  ERR: could not open file for write: %s\n", get_stream_error(ofs));
            return false;
        }
        ofs.write(reinterpret_cast<char *>(exe_data.data()), static_cast<std::streamsize>(exe_data.size()));
    }
    return true;
}
