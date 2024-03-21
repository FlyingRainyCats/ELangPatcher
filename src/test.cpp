#include <algorithm>
#include <iostream>
#include <random>

#include "../LibELangPatch/include/CallProxyStubWithEcxGen.h"
#include "../LibELangPatch/include/ELangInitFnGen.h"
#include "../LibELangPatch/include/WndHandlerGen.h"
#include "CdeclPushAndCallGen.h"
#include "ELangBulkPushGen.h"

void print_shellcode(const std::vector<uint8_t> &code) {
    for (auto bytecode: code) {
        printf("%02x ", bytecode);
    }
    printf("\n");
}

int main() {
    auto wnd_handler_code = GenerateWndHandlerCode(0x0041883A, 0xFFFF9AB1);
    printf("WndHandler: %zu bytes\n", wnd_handler_code.size());
    print_shellcode(wnd_handler_code);

    uint32_t data[3]{0, 1, 2};
    auto elang_init_code = GenerateELangInitSnippet(0x004365A5, 0x47E6A0, data);
    printf("ELangInitSnippet: %zu bytes\n", elang_init_code.size());
    print_shellcode(elang_init_code);

    auto elang_wnd_proc_stub = GenerateCallProxyStubWithEcx(15, 3, 0x4AF608, 0xFFFFC787);
    printf("ELangWndProcStub: %zu bytes\n", elang_wnd_proc_stub.size());
    print_shellcode(elang_wnd_proc_stub);

    auto elang_bulkPushInstruction = GenerateBulkPushInstruction(0x00401036 - 0x00401007, {0x80000002, 0, 1, 0x10001, 0x6010000, 0x52010001, 0x10001, 0x601000E, 0x5201000F, 3});
    printf("elang_bulkPushInstruction: %zu bytes\n", elang_bulkPushInstruction.size());
    print_shellcode(elang_bulkPushInstruction);

    // uint32_t push_value, uint32_t call_delta, uint32_t ret_delta
    auto elang_GenerateCdeclPushAndCall = GenerateCdeclPushAndCall(0x52010048, 0x11 + 5, 5);
    printf("elang_GenerateCdeclPushAndCall: %zu bytes\n", elang_GenerateCdeclPushAndCall.size());
    print_shellcode(elang_GenerateCdeclPushAndCall);
    return 0;
}
