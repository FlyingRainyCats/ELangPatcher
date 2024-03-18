#include "DlLFunctionCallGen.h"
#include "CodeGenHelper.h"

class DllFunctionCallStubGen : public CodeGenHelper {
public:
    explicit DllFunctionCallStubGen(uint32_t ecx_value, uint32_t call_delta) {
        fillWithJunk(7, {eax, ecx, edx});
        mov(ecx, ecx_value);
        db(0xE9);
        dd(call_delta);

        std::vector<uint8_t> dummy(2);
        std::generate(dummy.begin(), dummy.end(), mt_);
        db(dummy.data(), dummy.size());
    }
};

std::vector<uint8_t> GenerateDllFunctionCallStub(uint32_t ecx_value, uint32_t call_delta) {
    return DllFunctionCallStubGen{ecx_value, call_delta}.vec();
}