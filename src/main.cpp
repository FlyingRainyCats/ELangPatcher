#include <algorithm>
#include <iostream>
#include <random>

#include "WndHandlerGen.h"

std::random_device rd;

int main() {
    auto shellcode = GenerateWndHandlerCode(rd);
    for (auto bytecode: shellcode) {
        printf("%02x ", bytecode);
    }
    printf("\n %zu bytes\n", shellcode.size());
    return 0;
}
