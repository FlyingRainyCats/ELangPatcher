#include <algorithm>
#include <iostream>
#include <random>

#include "../LibELangPatch/include/ELangInitFnGen.h"
#include "../LibELangPatch/include/WndEventProxyGen.h"
#include "../LibELangPatch/include/WndHandlerGen.h"

void print_shellcode(const std::vector<uint8_t>& code) {
    for (auto bytecode: code) {
        printf("%02x ", bytecode);
    }
    printf("\n");
}

int main() {
    auto wnd_handler_code = GenerateWndHandlerCode(0x0041883A, 0xFFFF9AB1);
    printf("WndHandler: %zu bytes\n", wnd_handler_code.size());
    print_shellcode(wnd_handler_code);

    auto elang_init_code = GenerateELangInitSnippet(0x004365A5, 0x47E6A0);
    printf("ELangInitSnippet: %zu bytes\n", elang_init_code.size());
    print_shellcode(elang_init_code);

    auto elang_wnd_proc_stub = GenerateWndEventProxySnippet(0x4AF608, 0xFFFFC787);
    printf("ELangWndProcStub: %zu bytes\n", elang_wnd_proc_stub.size());
    print_shellcode(elang_wnd_proc_stub);

    return 0;
}
