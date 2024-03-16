#include <algorithm>
#include <array>

#include "xbyak.h"

#include "WndHandlerGen.h"

class WndHandlerGen : public Xbyak::CodeGenerator {
public:
    explicit WndHandlerGen(std::random_device &rd) : rd_(rd) {
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
    std::random_device &rd_;
    std::uniform_int_distribution<> dist_bool_{0, 1};
    inline bool next_bool() {
        return dist_bool_(rd_) == 1;
    }

    std::vector<Xbyak::Reg32> backup_regs_{};
    Xbyak::Address ptr_local_this_ = dword[ebp - 4];
    Xbyak::Address ptr_arg_1_ = dword[ebp + 8];
    inline void initBackupRegs() {
        backup_regs_ = {edi, esi, ebx};
        auto rnd_value = rd_();

        if (rnd_value & (1 << 0)) backup_regs_.push_back(edx);
        if (rnd_value & (1 << 1)) backup_regs_.push_back(ecx);

        std::shuffle(backup_regs_.begin(), backup_regs_.end(), rd_);
    }
    inline void enterProc() {
        push(esp);
        mov(ebp, esp);

        auto locals_size = (rd_() & 0b1'1100) + 12;
        sub(esp, locals_size);
        auto local_select = std::uniform_int_distribution<>(1, (locals_size) >> 2)(rd_) << 2;
        ptr_local_this_ = dword[ebp - local_select];

        initBackupRegs();
        for (auto reg: backup_regs_) {
            push(reg);
        }
    }
    inline void exitProc() {
        L("prepare_exit");
        for (auto it = backup_regs_.crbegin(); it != backup_regs_.crend(); ++it) {
            pop(*it);
        }
        mov(esp, ebp);
        pop(ebp);
        ret(4);
        L("ext_get_handler");
    }
    inline void genJunk(const std::vector<Xbyak::Reg32> &regs) {
        std::uniform_int_distribution<> dis(0, static_cast<int>(regs.size()) - 1);
        auto rand_reg = [&]() { return regs[dis(rd_)]; };

        auto rnd = rd_();
        bool use_imm = (rnd & (1 << 3)) != 0;
        if (use_imm) {
            switch (rnd & 3) {
                case 0:
                    or_(rand_reg(), rd_() & 0x7F);
                    break;
                case 1:
                    xor_(rand_reg(), rd_() & 0x7F);
                    break;
                case 2:
                    sub(rand_reg(), rd_() & 0x7F);
                    break;
                case 3:
                    and_(rand_reg(), rd_() & 0x7F);
                    break;
            }
        } else {
            switch (rnd & 3) {
                case 0:
                    or_(rand_reg(), rand_reg());
                    break;
                case 1:
                    xor_(rand_reg(), rand_reg());
                    break;
                case 2:
                    sub(rand_reg(), rand_reg());
                    break;
                case 3:
                    and_(rand_reg(), rand_reg());
                    break;
            }
        }
    }
    inline void resolveNextWndHandler() {
        std::vector<Xbyak::Reg32> regs_for_access = {eax, ebx, edx};
        std::shuffle(regs_for_access.begin(), regs_for_access.end(), rd_);
        auto reg_temp = regs_for_access.back();
        regs_for_access.pop_back();

        mov(reg_temp, ptr_arg_1_);

        for (int i = 0x0C; i >= 0; i -= 4) {
            if (next_bool()) genJunk(regs_for_access);
            push(dword[reg_temp + i]);
            if (i == 8) genJunk(regs_for_access);
        }
        if (next_bool()) genJunk(regs_for_access);
        // call("ext_get_handler");
        db(0xE8);
        dd(1);
        if (next_bool()) {
            test(eax, eax);
            jz("prepare_exit");
        } else {
            cmp(eax, 0);
            je("prepare_exit");
        }

        prepareAndCallNextHandler();
        saveAndAssignHandlerResult();
    }

    inline void prepareAndCallNextHandler() {
        std::vector<Xbyak::Reg32> regs = {eax, ecx, edx, esi, ebx};
        std::shuffle(regs.begin(), regs.end(), rd_);

        auto reg_next_handler = regs[0];
        if (reg_next_handler != eax) {
            mov(reg_next_handler, eax);
        }
        auto reg_arg_count = regs[1];
        auto reg_list_begin = regs[2];
        auto reg_arg_1 = next_bool() ? reg_arg_count : reg_list_begin;
        auto reg_list_end = regs[3];
        auto reg_temp_unused = regs[4];

        mov(reg_arg_1, ptr_arg_1_);
        if (reg_arg_1 == reg_arg_count) {
            lea(reg_list_begin, dword[reg_arg_1 + 0x10]);
            mov(reg_arg_count, dword[reg_arg_1 + 0x0C]);
        } else {
            mov(reg_arg_count, dword[reg_arg_1 + 0x0C]);
            add(reg_list_begin, 0x10);
        }

        lea(reg_list_end, dword[reg_list_begin + reg_arg_count * 4]);
        cmp(reg_list_begin, reg_list_end);
        je("lb_end_push_args");
        /**/ if (next_bool()) genJunk({reg_temp_unused, reg_arg_count});
        L("lb_push_next_arg");
        /**/ push(dword[reg_list_begin]);
        /**/ if (next_bool()) genJunk({reg_temp_unused, reg_arg_count});
        /**/ add(reg_list_begin, 4);
        /**/ cmp(reg_list_begin, reg_list_end);
        /**/ jne("lb_push_next_arg", T_SHORT);
        /**/ if (next_bool()) genJunk({reg_temp_unused, reg_arg_count});
        L("lb_end_push_args");
        call(reg_next_handler);
    }
    inline void saveAndAssignHandlerResult() {
        std::vector<Xbyak::Reg32> regs = {eax, ecx, edx, ebx, esi, edi};
        std::shuffle(regs.begin(), regs.end(), rd_);

        Xbyak::Reg32 reg_handler_result;
        {
            auto it = std::find_if(regs.begin(), regs.end(), [&](auto& reg) {
                return reg != ebx;
            });
            reg_handler_result = *it;
            regs.erase(it);
        }
        Xbyak::Reg32 reg_flag;
        {
            auto it = std::find_if(regs.begin(), regs.end(), [&](auto& reg) {
                return reg != esi && reg != edi;
            });
            reg_flag = *it;
            regs.erase(it);
        }

        auto reg_temp = regs[0];
        auto reg_temp_const = regs[1];

        if (reg_handler_result != eax) {
            mov(reg_handler_result, eax);
        }

        if (next_bool()) {
            test(ebx, ebx);
        } else {
            cmp(ebx, 0);
        }
        setne(reg_flag.cvt8());
        jz("skip_set_extra_flags");
        mov(reg_temp, ptr_arg_1_);
        movzx(reg_flag, reg_flag.cvt8());
        mov(dword[reg_temp + reg_flag * 8 + 0x20], reg_handler_result);
        mov(dword[reg_temp + 0x24], reg_flag);
        L("skip_set_extra_flags");
        genJunk({reg_temp_const, reg_flag, esi, edi});
        mov(reg_temp, ptr_local_this_);
        xor_(reg_temp_const, reg_temp_const);
        mov(dword[reg_temp + 0x1F0], reg_temp_const /* 0 */);
        lea(eax, ptr[reg_temp_const + 1]);
    }
};

std::vector<uint8_t> GenerateWndHandlerCode(std::random_device &rd) {
    WndHandlerGen c(rd);
    return {c.getCode(), c.getCurr()};
}
