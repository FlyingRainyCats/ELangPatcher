#include "ELangPatchFile.h"
#include "ELangPatcher.h"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

template<typename T>
const char* get_stream_error(const T& file) {
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

bool ELangPatchFile(const fs::path &file_path, bool backup, bool fake_stub) {
    if (!fs::exists(file_path)) {
        fprintf(stderr, "  ERR: file does not exist.\n");
        return false;
    }

    if (backup) {
        auto bak_file{file_path};
        bak_file.replace_extension(file_path.extension().u8string() + u8".bak");
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
    {
        std::ifstream ifs(file_path, std::ifstream::binary);
        if (!ifs.is_open()) {
            fprintf(stderr, "  ERR: could not open file for read: %s\n", get_stream_error(ifs));
            return false;
        }
        ifs.read(reinterpret_cast<char *>(exe_data.data()), file_size);
    }

    ELangPatcher patcher(exe_data);
    patcher.PatchEWndV02();
    patcher.PatchEWndUltimate();
    patcher.PatchWndEventHandlerMain();
    patcher.PatchWndEventHandlerSecondary();
    patcher.PatchKernelInvokeCall();
    patcher.PatchDllInvokeCall();
    if (fake_stub) patcher.AddFakeEWndStub();

    {
        std::ofstream ofs(file_path, std::ifstream::binary);
        if (!ofs.is_open()) {
            fprintf(stderr, "  ERR: could not open file for write: %s\n", get_stream_error(ofs));
            return false;
        }
        ofs.write(reinterpret_cast<char *>(exe_data.data()), static_cast<std::streamsize>(exe_data.size()));
    }
    return true;
}
