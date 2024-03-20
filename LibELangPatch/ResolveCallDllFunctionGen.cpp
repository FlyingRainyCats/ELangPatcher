#include "CodeGenHelper.h"
#include "VArgsProxyGen.h"

class ResolveCallDllFunctionGen : public CodeGenHelper {
public:
    explicit ResolveCallDllFunctionGen(uint32_t call_delta) {
        Reg32List regs({eax, ecx, edx});
        shuffle(regs);
        auto reg_to_save = pop_last_item(regs);
        if (reg_to_save != eax) {
            fillWithJunkSlideInst(rand_int(2, 5), {edx, ecx});
            mov(reg_to_save, eax);
        }

        int fn_offset = 0;
        fillWithJunkSlideInst(rand_int(2, 5), regs);

        Reg32 reg_fn_call{};
        shuffle_exec({
                [&]() {
                    push(reg_to_save);
                    fn_offset += 4;
                    regs.push_back(reg_to_save);
                    shuffle(regs);
                    fillWithJunkSlideInst(rand_int(2, 7), regs);
                },
                [&]() {
                    reg_fn_call = pop_last_item(regs);
                    mov(reg_fn_call, dword[esp + fn_offset]);
                    pick_exec({
                            [&]() { add(reg_fn_call, call_delta); },
                            [&]() {
                                auto reg_fn_call_new = pop_last_item(regs);
                                lea(reg_fn_call_new, dword[reg_fn_call + call_delta]);
                                regs.push_back(reg_fn_call);
                                shuffle(regs);
                                reg_fn_call = reg_fn_call_new;
                            },
                    });
                },
        });
        call(reg_fn_call);
        regs = {eax, ecx, edx};
        shuffle(regs);

        auto reg_next_fn_addr = pop_last_item(regs);
        if (reg_next_fn_addr != eax) {
            genJunk({ecx, edx});
            mov(reg_next_fn_addr, eax);
        }
        genJunk(regs);

        int stack_delta = 8;
        if (next_bool()) {
            stack_delta -= 4;
            pop(pick_random_item(regs));
        }
        Xbyak::Label lb_junk;
        pick_exec({
                [&]() {
                    add(esp, stack_delta);
                    jz(lb_junk);
                    genJunk(regs);
                    jmp(reg_next_fn_addr);
                },
                [&]() {
                    if (stack_delta - 4 != 0) {
                        add(esp, stack_delta - 4);
                        jz(lb_junk);
                        genJunk(regs);
                    }
                    mov(dword[esp], reg_next_fn_addr);
                    genJunk({eax, ecx, edx});
                    ret();
                },
        });
        L(lb_junk);
        fillWithJunkSlideInst(rand_int(10, 20), {eax, ecx, edx});
        jnz(lb_junk);
        ret(static_cast<uint8_t>(mt_() & 0x3C));
    }
};


std::vector<uint8_t> GenerateResolveCallDllFunction(uint32_t call_delta) {
    return ResolveCallDllFunctionGen{call_delta}.vec();
}
