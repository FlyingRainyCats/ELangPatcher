#include "VArgsProxyGen.h"
#include "CodeGenHelper.h"

class VArgsProxyGen : public CodeGenHelper {
public:
    explicit VArgsProxyGen() {
        Reg32List regs({eax, ebx, ecx, edx});
        shuffle(regs);

        auto reg_stack_tracker = pop_last_item(regs);
        auto reg_zero = *find_and_remove_item(regs, [&](auto &r) { return r != ebx; });
        auto reg_next_func = pop_last_item(regs);

        fillWithJunkSlideInst(rand_int(2, 4), {ecx, edx});

        shuffle_exec({
                [&]() {
                    if (reg_next_func != ebx) {
                        mov(reg_next_func, ebx);
                    }
                },
                [&]() { set_zero(reg_zero); },
        });

        int stack_offset = 0;
        bool stack_reg_set{false};
        for (int i = 0; i < 3; i++) {
            pick_exec({
                    [&]() { push(reg_zero); },
                    [&]() { push(0); },
            });
            if (stack_reg_set) continue;

            stack_offset += 4;
            if (next_bool()) {
                stack_reg_set = true;
                lea(reg_stack_tracker, dword[esp + (stack_offset + 8)]);
            }
        }
        regs.push_back(reg_zero);

        if (!stack_reg_set) {
            lea(reg_stack_tracker, dword[esp + (stack_offset + 8)]);
        }
        maybeGenJunk(regs);
        push(reg_stack_tracker);
        maybeGenJunk(regs);
        push(dword[reg_stack_tracker - 4]);
        maybeGenJunk(regs);
        pick_exec({
                [&]() { sub(reg_stack_tracker, 0x14); },
                [&]() { lea(reg_stack_tracker, ptr[reg_stack_tracker - 0x14]); },
        });
        maybeGenJunk(regs);
        push(reg_stack_tracker);
        regs.push_back(reg_stack_tracker);
        if (reg_next_func != ebx) {
            mov(ebx, reg_next_func);
        }
        maybeGenJunk(regs);
        call(reg_next_func);

        add(esp, 0x0C);
        regs = {ecx, edx, eax};
        while (!regs.empty()) {
            maybeGenJunk(regs);
            pop(pop_last_item(regs));
        }
        ret();
    }
};


std::vector<uint8_t> GenerateVArgsProxyCode() {
    return VArgsProxyGen{}.vec();
}
