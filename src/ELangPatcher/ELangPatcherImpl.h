#pragma once

#include "../ELangPatcher.h"
#include "../PEParser.h"
#include "../SearchMatcher.h"

#include <cstdint>
#include <random>
#include <vector>

class ELangPatcherImpl : public ELangPatcher {
    typedef ELang::PatternSearch::PatternSegment PatternSegment;
    typedef ELang::PatternSearch::SearchMatcher SearchMatcher;
    typedef FlyingRainyCats::PEParser::PEParser<false> PEParser;

public:
    explicit ELangPatcherImpl(std::vector<uint8_t>& data): data_(data), pe_(data_), mt_(std::random_device{}()) {}

    void PatchEWndV02() override;
    void PatchEWndUltimate() override;
    void PatchWndEventHandlerMain() override;
    void PatchKernelInvokeCall() override;
    void PatchProxyStub() override;
    void PatchELangLoaderInitStub() override;
    void PatchAddFakeEWndStub() override;

private:
    std::vector<uint8_t>& data_;
    PEParser pe_;
    std::mt19937 mt_;

    inline int rand_int(int min, int max) {
        return std::uniform_int_distribution<>(min, max)(mt_);
    }

    inline uint32_t read_u32 (size_t offset) {
        return *reinterpret_cast<uint32_t *>(&data_[offset]);
    };

    inline void write_jmp(uint32_t foa_inst_offset, uint32_t foa_target_pos, uint8_t opcode = 0xE9) {
        auto rva_inst = pe_.FOAtoRVA(foa_inst_offset);
        auto rva_target = pe_.FOAtoRVA(foa_target_pos);
        data_[foa_inst_offset] = opcode;
        *reinterpret_cast<uint32_t*>(&data_.at(foa_inst_offset + 1)) = rva_target - (rva_inst + 5);
    }

    inline void write_call(uint32_t foa_inst_offset, uint32_t foa_target_pos) {
        write_jmp(foa_inst_offset, foa_target_pos, 0xE8);
    }
};
