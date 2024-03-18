#include "ELangPatchFile.h"
#include "ELangPatcher.h"

#include <filesystem>
#include <string>

#include <cxxopts.hpp>

#include <Windows.h>
#include <shellapi.h>

namespace fs = std::filesystem;

std::string ConvertWideToUtf8(const std::wstring &wstr) {
    if (wstr.empty()) return {};
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), -1, &result[0], sizeNeeded, nullptr, nullptr);
    return result;
}

int main_unicode(int argc, wchar_t *argv[]) {
    cxxopts::Options options("ELang-Patcher", "ELang AntiEWnd by FlyingRainyCats (爱飞的猫 @52pojie.cn)");
    options.add_options()                                                                   //
            ("b,backup", "Enable backup", cxxopts::value<bool>()->default_value("true"))    //
            ("fake-stub", "Insert fake stub", cxxopts::value<bool>()->default_value("true"))//
            ("h,help", "Show help")                                                         //
            ("files", "The file(s) to process", cxxopts::value<std::vector<std::string>>());

    options.parse_positional({"files"});

    std::vector<std::string> args_utf8(argc);
    std::vector<char *> argv_utf8(argc);
    for (int i = 0; i < argc; i++) {
        args_utf8[i] = ConvertWideToUtf8(argv[i]);
        argv_utf8[i] = args_utf8[i].data();
    }
    auto result = options.parse(argc, argv_utf8.data());
    if (result.count("help")) {
        fputs(options.help().c_str(), stderr);
        return 0;
    }
    fprintf(stderr, "ELang Patcher v0.1 by FlyingRainyCats (爱飞的猫 @52pojie.cn)\n");
    const auto file_count = result.count("files");
    if (file_count == 0) {
        fprintf(stderr, "ERROR: no input files specified\n");
        return 999;
    }
    auto files = result["files"].as<std::vector<std::string>>();

    auto error_count{0};
    auto fake_stub = result["fake-stub"].as<bool>();
    auto backup = result["backup"].as<bool>();
    for (auto &file_path: files) {
        std::u8string temp_path(file_path.cbegin(), file_path.cend());
        fs::path exe_path{temp_path};
        fprintf(stderr, "INFO: processing: %s\n", exe_path.string().c_str());
        if (!ELangPatchFile(exe_path, backup, fake_stub)) {
            error_count++;
        }
    }
    return error_count;
}

int main() {
    setlocale(LC_ALL, ".UTF8");

    int argc{0};
    auto argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    return main_unicode(argc, argv);
}
