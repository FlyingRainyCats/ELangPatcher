#pragma once
#include <span>

#include "../SearchMatcher.h"

class ELangPatcher {
    typedef ELang::PatternSearch::PatternSegment PatternSegment;

public:
    ELangPatcher(std::span<uint8_t> data): data_(data) {}
    void PatchEWndV02();
    void PatchEWndUltimate();
    void PatchWndEventHandlerMain();
    void PatchWndEventHandlerSecondary();
    void AddEWndStub();

private:
    std::span<uint8_t> data_;
    inline uint32_t read_u32 (size_t offset) {
        return *reinterpret_cast<uint32_t *>(&data_[offset]);
    };
};
