#include "ELangBulkPushGen.h"
#include "CodeGenHelper.h"

class BulkPushInstruction : public CodeGenHelper {
public:
    explicit BulkPushInstruction(uint32_t ret_delta, std::vector<uint32_t> values_to_push, uint32_t ebx_delta_to_ret_addr, uint32_t call_delta_to_ret_addr) {
        auto regs = Reg32List{eax, ecx, edx};
        shuffle(regs);

        fillWithJunkSlideInst(rand_int(2, 4), regs);
        {
            auto reg_stack_offset = pop_last_item(regs);
            auto reg_arg_count_mul_4 = pop_last_item(regs);

            IntGenerator int_gen_stack_delta{rand_int(2, 5), ret_delta};
            IntGenerator int_gen_arg_count{rand_int(2, 5), static_cast<uint32_t>(values_to_push.size() - 1) * 4};

            while (!int_gen_stack_delta.done() || !int_gen_arg_count.done()) {
                pick_exec({
                        [&]() { int_gen_stack_delta.generate_step(*this, reg_stack_offset); },
                        [&]() { int_gen_arg_count.generate_step(*this, reg_arg_count_mul_4); },
                });
            }

            add(dword[esp], reg_stack_offset);
            regs.push_back(reg_stack_offset);
            genJunk(regs);

            sub(esp, reg_arg_count_mul_4);
            regs.push_back(reg_arg_count_mul_4);
            genJunk(regs);
        }

        regs = shuffled<Reg32>({eax, ecx, edx});
        std::vector<std::tuple<int, std::shared_ptr<IntGenerator>>> int_gens_idle{};
        std::vector<std::tuple<int, Xbyak::Reg32, std::shared_ptr<IntGenerator>>> int_gens_working{};

        auto arg_count = values_to_push.size();
        for (auto i = 1; i < arg_count; i++) {
            int offset = (static_cast<int>(arg_count) - i - 1) * 4;
            int_gens_idle.emplace_back(offset, std::make_shared<IntGenerator>(rand_int(2, 5), values_to_push[i]));
        }
        shuffle(int_gens_idle);
        int_gens_idle.insert(int_gens_idle.begin(), {-4, std::make_shared<IntGenerator>(rand_int(2, 5), values_to_push[0])});

        while (!regs.empty()) {
            auto temp_reg = pop_last_item(regs);
            auto [op, int_gen] = pop_last_item(int_gens_idle);
            int_gens_working.emplace_back(op, temp_reg, int_gen);
        }

        Reg32 reg_last_value{};
        while (!int_gens_working.empty()) {
            auto idx = rand_int(0, int(int_gens_working.size() - 1));
            auto [esp_offset, reg_temp, int_gen] = int_gens_working[idx];
            int_gen->generate_step(*this, reg_temp);

            if (int_gen->done()) {
                int_gens_working.erase(int_gens_working.begin() + idx);

                // Don't bother
                if (esp_offset == -4) {
                    reg_last_value = reg_temp;
                    continue;
                }

                mov(dword[esp + esp_offset], reg_temp);

                regs.push_back(reg_temp);
                shuffle(regs);
                genJunk(regs);
                reg_temp = pop_last_item(regs);

                if (!int_gens_idle.empty()) {
                    auto [op_next, int_gen_next] = pop_last_item(int_gens_idle);
                    int_gens_working.emplace_back(op_next, reg_temp, int_gen_next);
                }
            }
        }

        regs = shuffled<Reg32>({eax, ecx, edx});
        find_and_remove_item(regs, [&](auto &r) { return r == reg_last_value; });
        genJunk(regs);
        xchg(reg_last_value, dword[esp + (arg_count * 4 - 4)]);
        genJunk(regs);

        if (ebx_delta_to_ret_addr && call_delta_to_ret_addr) {
            auto reg_temp_delta_to_ret = pop_last_item(regs);
            IntGenerator ebx_gen{rand_int(2, 5), ebx_delta_to_ret_addr};
            while(!ebx_gen.done()){
                ebx_gen.generate_step(*this, reg_temp_delta_to_ret);
                genJunk(regs);
            }
            lea(ebx, dword[reg_last_value + reg_temp_delta_to_ret]);
            regs.push_back(reg_temp_delta_to_ret);
            shuffle(regs);
            genJunk(regs);

            push(reg_last_value);
            regs.push_back(reg_last_value);
            shuffle(regs);
            genJunk(regs);

            reg_last_value = pop_last_item(regs);
            lea(reg_last_value, dword[ebx + (call_delta_to_ret_addr - ebx_delta_to_ret_addr)]);
        }

        genJunk(regs);
        if (next_bool()) {
            test(reg_last_value, reg_last_value);
            jz("out");
            fillWithJunkSlideInst(rand_int(2, 5), regs);
        }
        pick_exec({
                [&]() { jmp(reg_last_value); },
                [&]() {
                    push(reg_last_value);
                    genJunk({eax, ecx, edx});
                    ret();
                },
        });
        fillWithJunkSlideInst(rand_int(5, 10), {eax, ecx, edx, esi, edi, ebx});
        L("out");
    }
};

std::vector<uint8_t> GenerateBulkPushInstruction(uint32_t ret_delta, std::vector<uint32_t> values_to_push) {
    return BulkPushInstruction{ret_delta, std::move(values_to_push), 0, 0}.vec();
}
std::vector<uint8_t> GenerateBulkPushInstructionWithEBXCall(uint32_t ret_delta, std::vector<uint32_t> values_to_push, uint32_t ebx_delta_to_ret_addr, uint32_t call_delta_to_ret_addr) {
     return BulkPushInstruction{ret_delta, std::move(values_to_push), ebx_delta_to_ret_addr, call_delta_to_ret_addr}.vec();
}
