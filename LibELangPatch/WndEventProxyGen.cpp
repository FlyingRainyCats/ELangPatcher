#include "include/WndEventProxyGen.h"

#include "CodeGenHelper.h"

class WndEventProxyStubGen : public CodeGenHelper {
public:
    explicit WndEventProxyStubGen(uint32_t ecx_value, uint32_t call_delta) : ecx_value_(ecx_value), call_delta_(call_delta) {
        fillWithJunk(15, {eax, ecx, edx});
        mov(ecx, ecx_value_);
        db(0xE9);
        dd(call_delta_);

        std::vector<uint8_t> dummy(3);
        std::generate(dummy.begin(), dummy.end(), mt_);
        db(dummy.data(), dummy.size());
    }

private:
    uint32_t ecx_value_{};
    uint32_t call_delta_{};
};

std::vector<uint8_t> GenerateWndEventProxySnippet(uint32_t ecx_value, uint32_t call_delta) {
    return WndEventProxyStubGen{ecx_value, call_delta}.vec();
}
