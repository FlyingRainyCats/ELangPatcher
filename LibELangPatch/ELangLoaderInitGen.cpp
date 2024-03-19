#include "ELangLoaderInitGen.h"
#include "CodeGenHelper.h"

class ELangLoaderInitGen : public CodeGenHelper {
public:
    explicit ELangLoaderInitGen(std::optional<uint32_t> call_delta) {
        auto regs = shuffled<Reg32>({eax, edx, ecx});
        fillWithJunkSlideInst(rand_int(1, 5), regs);
        shuffle_exec({
                [&]() { cld(); genJunk(regs); },
                [&]() { fninit(); genJunk(regs); },
        });
        fillWithJunkSlideInst(rand_int(1, 5), regs);

        bool use_ret_trick{false};
        if (call_delta) {
            use_ret_trick = next_bool();
            auto reg_ret_addr = pop_last_item(regs);
            mov(reg_ret_addr, dword[esp]);
            genJunk(regs);
            if (use_ret_trick) {
                add(dword[esp], 3);
                genJunk(regs);
            }
            auto delta_signed = static_cast<int32_t>(*call_delta);
            if (delta_signed > 0) {
                add(reg_ret_addr, delta_signed);
            } else {
                sub(reg_ret_addr, -delta_signed);
            }
            genJunk(regs);

            if (use_ret_trick) {
                jmp(reg_ret_addr);
            } else {
                call(reg_ret_addr);
            }
        }

        regs = shuffled<Reg32>({eax, edx, ecx});
        if (!use_ret_trick) {
            genJunk(regs);
            add(dword[esp], 3);
        }
        genJunk(regs);
        ret();

        std::vector<uint8_t> junk(rand_int(4, 10));
        std::generate(junk.begin(), junk.end(), mt_);
        db(junk.data(), junk.size());
    }
};

std::vector<uint8_t> GenerateELangLoaderInit(std::optional<uint32_t> call_delta) {
    return ELangLoaderInitGen{call_delta}.vec();
}