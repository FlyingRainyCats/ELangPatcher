#include "CdeclPushAndCallGen.h"
#include "ELangPatcherImpl.h"
#include "ResolveCallDllFunctionGen.h"

#include <array>

void ELangPatcherImpl::PatchSuspiciousCallWithParam() {
    ELang::PatternSearch::SearchMatcher pattern{{
            {0x68},// push imm32
            PatternSegment::Skip(4),// push_value
            {0xE8},// call $+????
            PatternSegment::Skip(4),
    }};

    constexpr std::array<uint8_t, 2> kSigJumpIndirect{0xFF, 0x25};
    constexpr std::array<uint8_t, 2> kSigBalanceStack{0x83, 0xC4}; // add esp, ??

    for (auto it = data_.begin(); (it = pattern.search(it, data_.end())) != data_.end(); it += pattern.size()) {
        auto offset = std::distance(data_.begin(), it);
        auto push_value = read_u32(offset + pattern.offset_at_item(1));
        auto call_delta = read_u32(offset + pattern.offset_at_item(3));

        auto offset_to_call_target = offset + 10 + call_delta;
        if (offset_to_call_target >= data_.size() + 1) continue;
        if (!std::equal(kSigJumpIndirect.cbegin(), kSigJumpIndirect.cend(), &data_[offset_to_call_target])) continue;
        if ((push_value & 0xFFFF0000) != 0 && !std::equal(kSigBalanceStack.cbegin(), kSigBalanceStack.cend(), &data_[offset + pattern.size()])) {
            continue;
        }

        fprintf(stderr, "  INFO: [PatchSuspiciousCallWithParam] found (offset=0x%08x, push_value=0x%08x, call_delta=0x%08x)\n", static_cast<int>(offset), push_value, call_delta);

        auto padding_beg = rand_int(2, 7);
        auto padding_end = rand_int(2, 7);
        auto snippet = GenerateCdeclPushAndCall(push_value, call_delta + 5, 5);

        auto ptr_output = pe_.ExpandTextSection(padding_beg + snippet.size() + padding_end);
        it = data_.begin() + offset;

        std::generate_n(ptr_output, padding_beg, mt_);
        std::copy(snippet.cbegin(), snippet.cend(), ptr_output + padding_beg);
        std::generate_n(ptr_output + padding_beg + snippet.size(), padding_end, mt_);

        std::generate_n(it, pattern.size(), mt_);
        write_call(offset, ptr_output + padding_beg - data_.data());
    }
}
