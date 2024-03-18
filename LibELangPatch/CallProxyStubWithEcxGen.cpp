#include "include/CallProxyStubWithEcxGen.h"

#include "CodeGenHelper.h"

class CallProxyStubWithEcxGen : public CodeGenHelper {
public:
    explicit CallProxyStubWithEcxGen(int pre_junk_len, int post_junk_len, uint32_t ecx_value, uint32_t call_delta) : ecx_value_(ecx_value), call_delta_(call_delta) {
        fillWithJunk(pre_junk_len, {eax, ecx, edx});
        auto slide_junk_len = rand_int(0, post_junk_len);
        post_junk_len -= slide_junk_len;

        mov(ecx, ecx_value_);
        fillWithJunkSlideInst(slide_junk_len, {eax, edx});
        db(0xE9);
        dd(call_delta_ - slide_junk_len);

        if (post_junk_len != 0) {
           std::vector<uint8_t> dummy(post_junk_len);
           std::generate(dummy.begin(), dummy.end(), mt_);
           db(dummy.data(), dummy.size());
        }
    }

private:
    uint32_t ecx_value_{};
    uint32_t call_delta_{};
};

std::vector<uint8_t> GenerateCallProxyStubWithEcx(int pre_junk_len, int post_junk_len, uint32_t ecx_value, uint32_t call_delta) {
    return CallProxyStubWithEcxGen{pre_junk_len, post_junk_len, ecx_value, call_delta}.vec();
}
