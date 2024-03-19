#include "ELangPatcherImpl.h"
#include "WndHandlerGen.h"

void ELangPatcherImpl::PatchWndEventHandlerMain() {
    ELang::PatternSearch::SearchMatcher pattern{{
            {0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x18, 0x53, 0x56, 0x57, 0x89, 0x4D, 0xE8, 0x8B, 0x45, 0x08,
             0x8B, 0x48, 0x0C, 0x51, 0x8B, 0x55, 0x08, 0x8B, 0x42, 0x08, 0x50, 0x8B, 0x4D, 0x08, 0x8B,
             0x51, 0x04, 0x52, 0x8B, 0x45, 0x08, 0x8B, 0x08, 0x51, 0x8B, 0x4D, 0xE8},
            {0xE8},
            PatternSegment::Skip(4),
    }};
    ELang::PatternSearch::SearchMatcher pattern_fn_end{
            {0x8B, 0xE5, 0x5D, 0xC2, 0x04, 0x00},
    };

    constexpr size_t kMaxSearchFunctionSize = 0x200;

    for (auto it = data_.begin(); (it = pattern.search(it, data_.end())) != data_.end(); it += pattern.size()) {
        auto offset = std::distance(data_.begin(), it);
        auto call_foa = static_cast<uint32_t>(offset + pattern.offset_at_item(1));
        auto call_rva = static_cast<uint32_t>(pe_.FOAtoRVA(call_foa));
        auto call_inst_delta = read_u32(offset + pattern.offset_at_item(2));

        auto it_fn_end_search_last = std::min(it + kMaxSearchFunctionSize, data_.end());
        auto it_fn_end = pattern_fn_end.search(it, it_fn_end_search_last);
        if (it_fn_end == it_fn_end_search_last) {
            continue;
        }
        auto offset_fn_end = std::distance(data_.begin(), it_fn_end);
        fprintf(stderr, "  INFO: [PatchWndEventHandlerMain] found (fn_end=%08tx, offset=0x%08tx, inst=0x%08x(p: 0x%08x), delta=0x%08x)\n", offset_fn_end, offset, call_foa, call_rva, call_inst_delta);

        auto function_size = static_cast<int>(offset_fn_end + pattern_fn_end.size() - offset);
        auto snippet = GenerateWndHandlerCode(static_cast<uint32_t>(call_rva), call_inst_delta);
        if (snippet.size() > function_size) {
            fprintf(stderr, "  WARN: snippet too big, ignored (expected %zu, got %d bytes)\n", snippet.size(), function_size);
            continue;
        }
        std::copy(snippet.cbegin(), snippet.cend(), it);
        if (auto padding_size = function_size - static_cast<int>(snippet.size()); padding_size > 0) {
            std::generate_n(it + static_cast<int>(snippet.size()), padding_size, mt_);
        }
    }
}
