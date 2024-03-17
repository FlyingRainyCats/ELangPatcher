#include "../SearchMatcher.h"
#include "ELangPatcher.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include <Windows.h>

namespace fs = std::filesystem;

void print_help() {
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: \n");
    fprintf(stderr, "  ELangPatcher \"<path>\" \"[output_path=path]\"\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "  <path>   path to ELang compiled EXE file\n");
    fprintf(stderr, "\n");
}

int main_unicode(int argc, wchar_t *argv[]) {
    fwprintf(stderr, L"ELang Patcher v0.1 by FlyingRainyCats (爱飞的猫 @52pojie.cn)");
    fprintf(stderr, "\n");

    if (argc <= 1) {
        print_help();
        return 0;
    }
    fs::path exe_file(argv[1]);
    fs::path output_file(argv[argc > 2 ? 2: 1]);
    if (!fs::exists(exe_file)) {
        std::cerr << "File does not exist." << std::endl;
        return 1;
    }

    if (fs::exists(output_file)) {
        auto bak_file{output_file};
        bak_file.replace_extension(output_file.extension().u8string() + u8".bak");
        if (!fs::exists(bak_file)) {
            std::error_code ec_backup{};
            fs::copy_file(exe_file, bak_file, ec_backup);
            if (ec_backup) {
                std::cerr << "backup failed: " << ec_backup.message() << std::endl;
                return 2;
            }
        }
    }

    auto exe_file_size = static_cast<std::streamsize>(fs::file_size(exe_file));
    std::vector<uint8_t> exe_data(exe_file_size);
    {
        std::ifstream ifs(exe_file, std::ifstream::binary);
        ifs.read(reinterpret_cast<char *>(exe_data.data()), exe_file_size);
        ifs.close();
    }

    ELangPatcher patcher(exe_data);
    patcher.PatchEWndV02();
    patcher.PatchEWndUltimate();
    patcher.PatchWndEventHandlerMain();
    patcher.PatchWndEventHandlerSecondary();
//    patcher.AddEWndStub();
    {
        std::ofstream ofs(output_file, std::ifstream::binary);
        if (!ofs.is_open()) {
            fprintf(stderr, "ERR: could not open file for output!\n");
        } else {
           ofs.write(reinterpret_cast<char*>(exe_data.data()), static_cast<std::streamsize>(exe_data.size()));
        }
        ofs.close();
    }

    return 0;
}

int main() {
    int argc{0};
    auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    return main_unicode(argc, argv);
}
