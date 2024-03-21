#include <algorithm>
#include <array>

#include "CodeGenHelper.h"
#include "include/WndHandlerGen.h"

class WndHandlerGen : public CodeGenHelper {
public:
    explicit WndHandlerGen(uint32_t call_inst_address, uint32_t call_inst_delta) : call_inst_address_(call_inst_address), call_inst_delta_(call_inst_delta) {
        inLocalLabel();

        enterProc();

        // backup "this" ctx.
        mov(ptr_local_this_, ecx);
        resolveNextWndHandler();

        exitProc();

        outLocalLabel();
        ready();
    }

private:
    uint32_t call_inst_address_{};
    uint32_t call_inst_delta_{};
    std::vector<Xbyak::Reg32> backup_regs_{};
    Xbyak::Address ptr_local_this_ = dword[ebp - 4];
    Xbyak::Address ptr_arg_1_ = dword[ebp + 0x0C];
    Xbyak::Address ptr_arg_ret_addr_ = dword[ebp + 0x04];
    inline void initBackupRegs() {
        backup_regs_ = {edi, esi, ebx};

        if (next_bool()) backup_regs_.push_back(edx);
        if (next_bool()) backup_regs_.push_back(ecx);

        std::shuffle(backup_regs_.begin(), backup_regs_.end(), mt_);
    }
    inline void enterProc() {
        push(esp);
        mov(ebp, esp);

        auto locals_size = (mt_() & 0b1'1100) + 12;
        sub(esp, locals_size);
        auto local_select = std::uniform_int_distribution<>(1, (locals_size) >> 2)(mt_) << 2;
        ptr_local_this_ = dword[ebp - local_select];

        initBackupRegs();
        for (auto reg: backup_regs_) {
            push(reg);
        }
    }
    inline void exitProc() {
        L("prepare_exit");
        for (auto it = backup_regs_.crbegin(); it != backup_regs_.crend(); ++it) {
            maybeGenJunk({*it});
            pop(*it);
        }
        genJunk({ecx, edx});
        mov(esp, ebp);
        genJunk({ecx, edx});
        pop(ebp);
        genJunk({ecx, edx});
        add(esp, 4);
        genJunk({ecx, edx});
        ret(4);
    }
    inline void resolveNextWndHandler() {
        std::vector<Xbyak::Reg32> regs_for_access = {eax, ebx, edx, esi, edi};
        shuffle(regs_for_access);
        auto reg_temp = pop_last_item(regs_for_access);
        auto reg_temp_2 = pop_last_item(regs_for_access);

        // 00418810 | 55                       | push ebp
        // 0041883A | E8 B19AFFFF              | call 测试2_5.1_静态编译_q1.4122F0
        // (0041883A - 00418810) + 0xFFFF9AB1 + 5 === (getSize() + ?)
        mov(reg_temp_2, ptr_arg_ret_addr_);
        genJunk(regs_for_access);
        add(reg_temp_2, call_inst_delta_);

        mov(reg_temp, ptr_arg_1_);

        for (int i = 0x0C; i >= 0; i -= 4) {
            maybeGenJunk(regs_for_access);
            push(dword[reg_temp + i]);
        }
        maybeGenJunk(regs_for_access);
        call(reg_temp_2);

        pick_exec({
                [&]() { test(eax, eax); },
                [&]() { cmp(eax, 0); },
        });
        je("prepare_exit");

        prepareAndCallNextHandler();
        saveAndAssignHandlerResult();
    }

    inline void prepareAndCallNextHandler() {
        auto regs = shuffled<Xbyak::Reg32>({eax, ecx, edx, esi, ebx});

        auto reg_next_handler = regs[0];
        auto reg_arg_count = regs[1];
        auto reg_list_begin = regs[2];
        auto reg_list_end = regs[3];
        auto reg_arg_1 = regs[4];

        if (reg_next_handler != eax) {
            mov(reg_next_handler, eax);
        }
        mov(reg_arg_1, ptr_arg_1_);
        shuffle_exec({
                [&](void *) { lea(reg_list_begin, dword[reg_arg_1 + 0x10]); },
                [&](void *) { mov(reg_arg_count, dword[reg_arg_1 + 0x0C]); },
        });
        lea(reg_list_end, dword[reg_list_begin + reg_arg_count * 4]);
        cmp(reg_list_begin, reg_list_end);
        je("lb_end_push_args");
        /**/ maybeGenJunk({reg_arg_1, reg_arg_count});
        L("lb_push_next_arg");
        bool added{false};
        /**/ shuffle_exec({
                [&](void *) { push(dword[reg_list_begin - (added ? 4 : 0)]); },
                [&](void *) { maybeGenJunk({reg_arg_1, reg_arg_count}); },
                [&](void *) { add(reg_list_begin, 4); added = true; },
        });
        /**/ cmp(reg_list_begin, reg_list_end);
        /**/ jne("lb_push_next_arg", T_SHORT);
        /**/ maybeGenJunk({reg_arg_1, reg_arg_count});
        L("lb_end_push_args");
        call(reg_next_handler);
    }
    inline void saveAndAssignHandlerResult() {
        auto regs = shuffled<Xbyak::Reg32>({eax, ecx, edx, ebx, esi, edi});

        Xbyak::Reg32 reg_handler_result = *find_and_remove_item(regs, [&](auto &reg) {
            return reg != ebx;
        });
        Xbyak::Reg32 reg_flag = *find_and_remove_item(regs, [&](auto &reg) {
            return reg != esi && reg != edi;
        });

        auto reg_temp = regs[0];
        auto reg_temp_const = regs[1];

        if (reg_handler_result != eax) {
            mov(reg_handler_result, eax);
        }

        pick_exec({
                [&]() { test(ebx, ebx); },
                [&]() { cmp(ebx, 0); },
        });
        setne(reg_flag.cvt8());
        jz("skip_set_extra_flags");
        shuffle_exec({
                [&]() { mov(reg_temp, ptr_arg_1_); },
                [&]() { movzx(reg_flag, reg_flag.cvt8()); },
        });

        shuffle_exec({
                [&]() { mov(dword[reg_temp + reg_flag * 8 + 0x20], reg_handler_result); },
                [&]() { mov(dword[reg_temp + 0x24], reg_flag); },
        });

        L("skip_set_extra_flags");
        genJunk({reg_temp_const, reg_flag, esi, edi});
        shuffle_exec({
                [&]() { mov(reg_temp, ptr_local_this_); },
                [&]() { xor_(reg_temp_const, reg_temp_const); },
        });
        mov(dword[reg_temp + 0x1F0], reg_temp_const /* 0 */);
        pick_exec({
                [&]() { mov(eax, 1); },
                [&]() { lea(eax, ptr[reg_temp_const + 1]); },
                [&]() { mov(eax, reg_temp_const); inc(eax); },
        });
    }
};

std::vector<uint8_t> GenerateWndHandlerCode(uint32_t call_inst_address, uint32_t call_inst_delta) {
    return WndHandlerGen{call_inst_address, call_inst_delta}.vec();
}
