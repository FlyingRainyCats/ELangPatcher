#include "include/CallProxyStubWithEcxGen.h"

#include "CodeGenHelper.h"

class CallProxyStubWithEcxGen : public CodeGenHelper {
public:
    explicit CallProxyStubWithEcxGen(bool is_cdecl, int arg_count, int pre_junk_len, int post_junk_len, uint32_t ecx_value, uint32_t call_delta) {
        if (is_cdecl) {
            generate_cdecl(arg_count, pre_junk_len, post_junk_len, ecx_value, call_delta);
        } else {
            generate_stdcall(pre_junk_len, post_junk_len, ecx_value, call_delta);
        }
    }

    void generate_cdecl(int arg_count, int pre_junk_len, int post_junk_len, uint32_t ecx_value, uint32_t call_delta) {
        int max_junk_per_inst{};
        if (arg_count <= 2) {
            max_junk_per_inst = 1;
        } else if (arg_count <= 4) {
            max_junk_per_inst = 2;
        } else if (arg_count <= 6) {
            max_junk_per_inst = 3;
        } else {
            max_junk_per_inst = 4;
        }

        auto regs = shuffled<Reg32>({eax, ecx, edx});

        int prefix_junk_left {pre_junk_len - arg_count * 3 - 2/* mov reg, esp */};
        auto junk_start = rand_int(0, std::min(max_junk_per_inst, prefix_junk_left));
        getJunkInstByLen(junk_start, {edx, eax, ecx});
        prefix_junk_left -= junk_start;

        auto reg_stack = pop_last_item(regs);
        mov(reg_stack, esp);

        int esp_offset = 4 * arg_count;
        for(int i = 0; i < arg_count; i++) {
            auto junk_this_round = rand_int(0, std::min(max_junk_per_inst, prefix_junk_left));
            getJunkInstByLen(junk_this_round, regs);
            prefix_junk_left -= junk_this_round;

            push(dword[reg_stack + esp_offset]);
            esp_offset -= 4;
        }
        fillWithJunk(prefix_junk_left, {eax, ecx, edx});

        post_junk_len -= 1;
        auto slide_junk_len = rand_int(0, post_junk_len);
        post_junk_len -= slide_junk_len;

        mov(ecx, ecx_value);
        fillWithJunkSlideInst(slide_junk_len, {eax, edx});
        db(0xE8);
        dd(call_delta - slide_junk_len);
        ret();

        if (post_junk_len != 0) {
            std::vector<uint8_t> dummy(post_junk_len);
            std::generate(dummy.begin(), dummy.end(), mt_);
            db(dummy.data(), dummy.size());
        }
    }

    void generate_stdcall(int pre_junk_len, int post_junk_len, uint32_t ecx_value, uint32_t call_delta) {
        fillWithJunk(pre_junk_len, {eax, ecx, edx});
        auto slide_junk_len = rand_int(0, post_junk_len);
        post_junk_len -= slide_junk_len;

        mov(ecx, ecx_value);
        fillWithJunkSlideInst(slide_junk_len, {eax, edx});
        db(0xE9);
        dd(call_delta - slide_junk_len);

        if (post_junk_len != 0) {
            std::vector<uint8_t> dummy(post_junk_len);
            std::generate(dummy.begin(), dummy.end(), mt_);
            db(dummy.data(), dummy.size());
        }
    }
};

std::vector<uint8_t> GenerateCallProxyStubWithEcx(int pre_junk_len, int post_junk_len, uint32_t ecx_value, uint32_t call_delta) {
    return CallProxyStubWithEcxGen{false, 0, pre_junk_len, post_junk_len, ecx_value, call_delta}.vec();
}

std::vector<uint8_t> GenerateCallProxyStubWithEcxCdecl(int arg_count, int pre_junk_len, int post_junk_len, uint32_t ecx_value, uint32_t call_delta) {
    return CallProxyStubWithEcxGen{true, arg_count, pre_junk_len, post_junk_len, ecx_value, call_delta}.vec();
}
