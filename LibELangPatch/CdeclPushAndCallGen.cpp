#include "CdeclPushAndCallGen.h"
#include "CodeGenHelper.h"

class CdeclPushAndCallGen : public CodeGenHelper {
public:
    explicit CdeclPushAndCallGen(uint32_t push_value, uint32_t call_delta, uint32_t ret_delta) {
        auto regs = shuffled<Reg32>({eax, ecx, edx});
        fillWithJunkSlideInst(rand_int(2, 5), regs);

        auto reg_ret_delta = pop_last_item(regs);
        IntGenerator int_gen_ret_delta{rand_int(2, 5), ret_delta};
        while(!int_gen_ret_delta.generate_step(*this, reg_ret_delta)) {
            maybeGenJunk(regs);
        }
        add(dword[esp], reg_ret_delta);
        regs = shuffled<Reg32>({eax, ecx, edx});

        std::optional<Reg32> reg_push_const = std::nullopt;
        std::optional<Reg32> reg_call_addr = std::nullopt;

        IntGenerator int_gen_push_value{rand_int(2, 5), push_value};
        IntGenerator int_gen_call_delta{rand_int(2, 5), call_delta - ret_delta};

        bool xchg_done{false};
        bool target_addr_fixed{false};
        ExecItem fns_decode{};
        ExecItem::value_type fn_exchange_stack_value = [&]() {
            if (!int_gen_push_value.done()) {
                fns_decode.emplace_back(fn_exchange_stack_value);
                return;
            }
            genJunk(regs);
            xchg(*reg_push_const, dword[esp]);
            xchg_done = true;
            fns_decode.emplace_back([&]() {
                genJunk(regs);
                push(*reg_push_const);
            });
        };
        ExecItem::value_type fn_increment_ret_addr = [&]() {
            if (!int_gen_call_delta.done()) {
                fns_decode.emplace_back(fn_increment_ret_addr);
                return;
            }

            genJunk(regs);
            if (xchg_done) {
                add(*reg_call_addr, *reg_push_const);
            } else {
                add(*reg_call_addr, dword[esp]);
            }
            target_addr_fixed = true;
        };
        ExecItem::value_type fn_decode_call_delta = [&]() {
            maybeGenJunk(regs);
            if (!reg_push_const) {
                reg_push_const = std::make_optional(pop_last_item(regs));
            }
            if (!int_gen_push_value.generate_step(*this, *reg_push_const)) {
                fns_decode.emplace_back(fn_decode_call_delta);
            }
        };
        ExecItem::value_type fn_decode_push_value = [&]() {
            maybeGenJunk(regs);
            if (!reg_call_addr) {
                reg_call_addr = std::make_optional(pop_last_item(regs));
            }
            if (!int_gen_call_delta.generate_step(*this, *reg_call_addr)) {
                fns_decode.emplace_back(fn_decode_push_value);
            }
        };
        fns_decode.emplace_back(fn_decode_call_delta);
        fns_decode.emplace_back(fn_decode_push_value);
        fns_decode.emplace_back(fn_exchange_stack_value);
        fns_decode.emplace_back(fn_increment_ret_addr);
        shuffle_exec_2(fns_decode);
        regs.push_back(*reg_push_const);

        genJunk(regs);
        test(*reg_call_addr, *reg_call_addr);
        jz("junk");
        genJunk(regs);
        jmp(*reg_call_addr);
        regs = shuffled<Reg32>({eax, ecx, edx, esi, edi, ebx});
        fillWithJunkSlideInst(rand_int(5, 10), regs);
        L("junk");
        fillWithJunkSlideInst(rand_int(10, 15), regs);
        jnz("junk");
        fillWithJunkSlideInst(rand_int(2, 5), regs);
    }
};

std::vector<uint8_t> GenerateCdeclPushAndCall(uint32_t push_value, uint32_t call_delta, uint32_t ret_delta) {
    return CdeclPushAndCallGen{push_value, call_delta, ret_delta}.vec();
}
