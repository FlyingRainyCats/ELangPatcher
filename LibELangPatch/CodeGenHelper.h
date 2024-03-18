#pragma once

#include <functional>
#include <optional>
#include <random>

#include "../vendor/xbyak/xbyak/xbyak.h"

#ifdef min
#undef min
#endif

class CodeGenHelper : public Xbyak::CodeGenerator {
public:
    [[nodiscard]] inline std::vector<uint8_t> vec() {
        ready();
        return {getCode(), getCurr()};
    }

protected:
    typedef std::vector<std::function<void()>> ExecItem;
    typedef Xbyak::Reg32 Reg32;
    typedef std::vector<Reg32> Reg32List;

    std::mt19937 mt_{std::random_device{}()};
    template<typename T>
    inline std::vector<T> shuffled(std::vector<T> items) {
        std::shuffle(items.begin(), items.end(), mt_);
        return items;
    }
    template<typename T>
    inline void shuffle(T &items) {
        std::shuffle(items.begin(), items.end(), mt_);
    }

    inline void shuffle_exec_2(ExecItem &items) {
        while (!items.empty()) {
            std::shuffle(items.begin(), items.end(), mt_);
            auto next_fn = items.back();
            items.pop_back();
            next_fn();
        }
    }
    inline void set_zero(Xbyak::Operand &reg) {
        if (reg.getKind() == Xbyak::Operand::REG) {
            pick_exec({
                    [&]() { xor_(reg, reg); },
                    [&]() { mov(reg, 0); },
                    [&]() { and_(reg, 0); },
            });
        } else {
            pick_exec({
                    [&]() { mov(reg, 0); },
                    [&]() { and_(reg, 0); },
            });
        }
    }

    inline void shuffle_exec(std::vector<std::function<void(void *)>> items) {
        while (!items.empty()) {
            std::shuffle(items.begin(), items.end(), mt_);
            auto next_fn = items.back();
            items.pop_back();
            next_fn(&items);
        }
    }
    inline void shuffle_exec(std::vector<std::function<void()>> items) {
        std::shuffle(items.begin(), items.end(), mt_);
        for (auto &fn: items) {
            fn();
        }
    }
    inline void pick_exec(const std::initializer_list<std::function<void()>> &items) {
        if (items.size() != 0) {
            std::uniform_int_distribution<> distr(0, int(items.size() - 1));
            auto &it = *(items.begin() + distr(mt_));
            it();
        }
    }
    template<typename T>
    inline T pick_random_item(std::vector<T> items) {
        std::uniform_int_distribution<> distr(0, int(items.size() - 1));
        return items.at(distr(mt_));
    }
    std::uniform_int_distribution<> dist_bool_{0, 1};
    inline bool next_bool() {
        return dist_bool_(mt_) == 1;
    }
    template<typename T>
    inline T rand_int() {
        return static_cast<T>(mt_());
    }
    template<typename T>
    inline T rand_int(T min_value, T max_value) {
        std::uniform_int_distribution<T> dist(min_value, max_value);
        return static_cast<T>(dist(mt_));
    }
    inline void getJunkInstByLen(size_t len, const std::vector<Xbyak::Reg32> &regs) {
        std::uniform_int_distribution<> dist_register(0, static_cast<int>(regs.size()) - 1);
        auto rand_reg = [&]() { return regs[dist_register(mt_)]; };
        std::uniform_int_distribution<int> dis_signed_byte(0, 0xFF);
        auto rand_signed_byte = [&]() { return static_cast<uint32_t>(static_cast<int32_t>(static_cast<int8_t>(dis_signed_byte(mt_)))); };
        std::uniform_int_distribution<int> dist_lea_shift(0, 3);// 1/2/4/8
        auto rand_lea_multiplier = [&]() { return 1 << dist_lea_shift(mt_); };

        auto ro_regs = shuffled<Xbyak::Reg32>({eax, ebx, ecx, edx, ebp, esp, esi, edi});
        std::uniform_int_distribution<> dist_ro_register(0, static_cast<int>(ro_regs.size()) - 1);
        auto rand_ro_reg = [&]() { return ro_regs[dist_ro_register(mt_)]; };

        switch (len) {
            case 1:
                pick_exec({
                        [&]() { inc(rand_reg()); },
                        [&]() { dec(rand_reg()); },
                        [&]() { nop(); },
                });
                break;

            case 2:
                pick_exec({
                        [&]() { or_(rand_reg(), rand_ro_reg()); },
                        [&]() { xor_(rand_reg(), rand_ro_reg()); },
                        [&]() { and_(rand_reg(), rand_ro_reg()); },
                        [&]() { add(rand_reg(), rand_ro_reg()); },
                        [&]() { sub(rand_reg(), rand_ro_reg()); },
                        [&]() { adc(rand_reg(), rand_ro_reg()); },
                        [&]() { test(rand_reg(), rand_ro_reg()); },
                        [&]() { cmp(rand_reg(), rand_ro_reg()); },
                });
                break;

            case 3:
                pick_exec({
                        [&]() { or_(rand_reg(), rand_signed_byte()); },
                        [&]() { xor_(rand_reg(), rand_signed_byte()); },
                        [&]() { and_(rand_reg(), rand_signed_byte()); },
                        [&]() { add(rand_reg(), rand_signed_byte()); },
                        [&]() { sub(rand_reg(), rand_signed_byte()); },
                        [&]() { adc(rand_reg(), rand_signed_byte()); },
                });
                break;

            case 4:
                pick_exec({
                        [&]() {
                            Xbyak::Reg32 index_reg;
                            do {
                                index_reg = rand_ro_reg();
                            } while (index_reg == esp);
                            lea(rand_reg(), ptr[rand_ro_reg() + index_reg * rand_lea_multiplier() + rand_signed_byte()]);
                        },
                        [&]() { lea(rand_reg(), ptr[rand_ro_reg() + rand_signed_byte()]); },
                });
                break;

            case 0:
            default:
                break;
        };
    }
    inline void genJunk(const std::vector<Xbyak::Reg32> &regs) {
        std::uniform_int_distribution<> dist_len(1, 4);
        getJunkInstByLen(dist_len(mt_), regs);
    }
    inline void fillWithJunkSlideInst(size_t size, const std::initializer_list<Xbyak::Reg32> &regs) {
        int junk_code_left = static_cast<int>(size);
        while (junk_code_left > 0) {
            auto this_round = rand_int(1, std::min(4, junk_code_left));
            getJunkInstByLen(this_round, regs);
            junk_code_left -= this_round;
        }
    }
    inline void fillWithJunk(size_t size, const std::initializer_list<Xbyak::Reg32> &regs) {
        // too short to have anything meaningful
        if (size <= 20) {
            fillWithJunkSlideInst(size, regs);
            return;
        }

        auto end_label = Xbyak::Label{};
        std::uniform_int_distribution<int> dist_u8(0, 0xff);
        auto gen_byte = [&]() { return static_cast<uint8_t>(dist_u8(mt_)); };

        std::vector<uint8_t> buffer(size);
        std::generate(buffer.begin(), buffer.end(), gen_byte);

        int junk_inst_len = rand_int(6, 8);
        bool use_slide = next_bool() && false;

        int junk_padding = static_cast<int>(size) - 5 - 4 - 1 - junk_inst_len;
        int padding_start = rand_int(2, junk_padding - 4);
        int padding_end = junk_padding - padding_start;

        call(end_label);
        db(buffer.data(), padding_start);
        L(end_label);

        add(dword[esp], static_cast<int>(size) - 5);
        fillWithJunkSlideInst(junk_inst_len, regs);
        ret();

        fillWithJunkSlideInst(padding_end, regs);
    }

    inline void maybeGenJunk(const std::vector<Xbyak::Reg32> &regs) {
        if (next_bool()) {
            genJunk(regs);
        }
    }
};


template<typename T, typename F>
std::optional<T> find_and_remove_item(std::vector<T> &list, F fn) {
    std::optional<T> result{};
    auto it = std::find_if(list.begin(), list.end(), fn);
    if (it != list.end()) {
        result = std::make_optional<T>(std::move(*it));
        list.erase(it);
    }
    return result;
}

template<typename T>
inline T pop_last_item(std::vector<T> &list) {
    T result{};
    result = std::move(list.back());
    list.pop_back();
    return result;
}
