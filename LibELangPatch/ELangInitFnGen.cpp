#include "include/ELangInitFnGen.h"
#include "CodeGenHelper.h"

class InitHandlerGen : public CodeGenHelper {
public:
    explicit InitHandlerGen(uint32_t offset_process_heap, uint32_t offset_has_ole, uint32_t *header_data) {
        auto ebx_offset = static_cast<int>(mt_());
        if (next_bool()) {
            ebx_offset &= 0x7F;
        }
//        ebx_offset = 0;
        // ebx_offset &= 0b1111'1100;
        fillWithJunk((mt_() & 0b1111) | 1, {ecx, edx, esi, edi});

        auto regs = shuffled<Xbyak::Reg32>({eax, ebx, ecx, edx, esi, edi});

        auto reg_temp_heap = *find_and_remove_item(regs, [&](auto& r) { return r != ebx; });
        auto new_ebx = *find_and_remove_item(regs, [&](auto& r) { return true; });

        if (reg_temp_heap != eax) {
            mov(reg_temp_heap, eax);
        }

        auto mov_to_value = [&](const Xbyak::Operand& op, uint32_t imm) {
            if (imm == 0) {
                pick_exec({
                        [&]() { mov(op, imm); },
                        [&]() { and_(op, imm); },
                });
            } else {
                mov(op, imm);
            }
        };

        shuffle_exec({
                [&](void *) { genJunk(regs); },
                [&](void *p_vec) {
                    lea(new_ebx, ptr[ebx + ebx_offset]);

                    auto &vec = *reinterpret_cast<std::vector<std::function<void(void *)>> *>(p_vec);
                    vec.emplace_back([&](void *) { genJunk(regs); });
                    vec.emplace_back([&](void *) { genJunk(regs); });
                    vec.emplace_back([&](void *) { genJunk(regs); });
                    vec.emplace_back([&](void *) { mov_to_value(dword[new_ebx + (0xC4 - ebx_offset)], header_data[0]); });
                    vec.emplace_back([&](void *) { mov_to_value(dword[new_ebx + (0xC8 - ebx_offset)], header_data[1]); });
                    vec.emplace_back([&](void *) { mov_to_value(dword[new_ebx + (0xCC - ebx_offset)], header_data[2] + 1); });
                    vec.emplace_back([&](void *) {
                        mov(dword[new_ebx + (offset_process_heap - ebx_offset)], reg_temp_heap);
                        regs.push_back(reg_temp_heap);
                    });
                },
        });

        regs = shuffled<Xbyak::Reg32>({eax, ebx, edx, ecx, esi, edi});
        std::erase_if(regs, [&](auto& r){ return r == new_ebx; });

        Xbyak::Reg32 reg_jump_offset = *find_and_remove_item(regs, [&](auto& r) { return r != esi && r != edi && r != ebx; });
        std::shuffle(regs.begin(), regs.end(), mt_);

        maybeGenJunk(regs);
        cmp(dword[new_ebx + (offset_has_ole - ebx_offset)], 0);
        setne(reg_jump_offset.cvt8());
        maybeGenJunk(regs);

        shuffle_exec({
                [&](void *) { genJunk(regs); },
                [&](void *) { genJunk(regs); },
                [&](void *p_vec) {
                    pick_exec({
                            [&]() { neg(reg_jump_offset.cvt8()); },
                            [&]() { neg(reg_jump_offset); },
                    });
                    auto &vec = *reinterpret_cast<std::vector<std::function<void(void *)>> *>(p_vec);
                    vec.emplace_back([&](void *p_vec) {
                        and_(reg_jump_offset, 0x12);

                        auto &vec = *reinterpret_cast<std::vector<std::function<void(void *)>> *>(p_vec);
                        vec.emplace_back([&](void *) { add(reg_jump_offset, 0x2F); });
                    });
                },
                [&](void *) {
                    if (new_ebx == ebx) {
                        sub(ebx, ebx_offset);
                    } else {
                        lea(ebx, dword[new_ebx - ebx_offset]);
                        regs.push_back(new_ebx);
                    }
                    std::erase_if(regs, [&](auto& r) { return r == ebx; });
                },
        });
        maybeGenJunk(regs);
        add(dword[esp], reg_jump_offset);
        regs.push_back(reg_jump_offset);
        maybeGenJunk(regs);
        ret();

        std::vector<uint8_t> junk_padding((mt_() & 0b1111) | 1);
        std::generate(junk_padding.begin(), junk_padding.end(), mt_);
        db(junk_padding.data(), junk_padding.size());
    }
};

std::vector<uint8_t> GenerateELangInitSnippet(uint32_t offset_process_heap, uint32_t offset_has_ole, uint32_t *header_data) {
    InitHandlerGen code(offset_process_heap, offset_has_ole, header_data);
    code.ready();
    return {code.getCode(), code.getCurr()};
}
