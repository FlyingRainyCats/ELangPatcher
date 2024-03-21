#include "ELangPatchFile.h"
#include "ELangPatcher.h"

#include <filesystem>
#include <string>

#include <cxxopts.hpp>

#include <Windows.h>
#include <shellapi.h>
#include <tlhelp32.h>

namespace fs = std::filesystem;

std::string ConvertWideToMultibyte(UINT CodePage, const std::wstring &wstr) {
    if (wstr.empty()) return {};
    int sizeNeeded = WideCharToMultiByte(CodePage, 0, wstr.data(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(sizeNeeded, 0);
    WideCharToMultiByte(CodePage, 0, wstr.data(), -1, &result[0], sizeNeeded, nullptr, nullptr);
    return result;
}

bool checkCallingFromE() {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    PROCESSENTRY32 pe{};
    pe.dwSize = sizeof(PROCESSENTRY32);

    DWORD pid = GetCurrentProcessId();
    DWORD ppid{};
    std::vector<DWORD> elang_pids{};
    elang_pids.reserve(8);
    if (Process32First(hSnapshot, &pe)) {
        do {
            if (pe.th32ProcessID == pid) {
                ppid = pe.th32ParentProcessID;
            } else if (_stricmp(pe.szExeFile, "e.exe") == 0) {
                elang_pids.push_back(pe.th32ProcessID);
            }
        } while (Process32Next(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);

    return std::find(elang_pids.cbegin(), elang_pids.cend(), ppid) != elang_pids.cend();
}

int main_unicode(int argc, wchar_t *argv[]) {
    cxxopts::Options options("ELang-Patcher", "ELang AntiEWnd by FlyingRainyCats (爱飞的猫 @52pojie.cn)");
    options.add_options()                                                                   //
            ("b,backup", "Enable backup", cxxopts::value<bool>()->default_value("true"))    //
            ("fake-stub", "Insert fake stub", cxxopts::value<bool>()->default_value("true"))//
            ("suffix", "Write to a different file with suffix, if specified.",
             cxxopts::value<std::string>()->default_value(""))//
            ("h,help", "Show help")                           //
            ("files", "The file(s) to process", cxxopts::value<std::vector<std::string>>());

    options.parse_positional({"files"});

    std::vector<std::string> args_utf8(argc);
    std::vector<char *> argv_utf8(argc);
    for (int i = 0; i < argc; i++) {
        args_utf8[i] = ConvertWideToMultibyte(CP_UTF8, argv[i]);
        argv_utf8[i] = args_utf8[i].data();
    }
    auto result = options.parse(argc, argv_utf8.data());
    if (result.count("help")) {
        fputs(options.help().c_str(), stderr);
        return 0;
    }
    bool useGBK = checkCallingFromE();
    fprintf(stderr, "ELang Patcher v0.1 by FlyingRainyCats (%s @52pojie.cn)\n", useGBK ? "\xB0\xAE\xB7\xC9\xB5\xC4\xC3\xA8" : "爱飞的猫");
    const auto file_count = result.count("files");
    if (file_count == 0) {
        fprintf(stderr, "ERROR: no input files specified\n");
        return 999;
    }
    auto files = result["files"].as<std::vector<std::string>>();
    auto output_suffix_str = result["suffix"].as<std::string>();
    std::u8string output_suffix{output_suffix_str.cbegin(), output_suffix_str.cend()};

    auto error_count{0};
    auto fake_stub = result["fake-stub"].as<bool>();
    auto backup = result["backup"].as<bool>();
    for (auto &file_path: files) {
        std::u8string temp_path(file_path.cbegin(), file_path.cend());
        fs::path exe_path{temp_path};

        std::string exe_path_str{};
        if (useGBK) {
            exe_path_str = ConvertWideToMultibyte(936, exe_path.wstring());
        } else {
            exe_path_str = exe_path.string();
        }
        fprintf(stderr, "INFO: processing: %s\n", exe_path_str.c_str());

        if (!ELangPatchFile(exe_path, output_suffix, backup, fake_stub)) {
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
