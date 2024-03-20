#include "CallProxyStubWithEcxGen.h"
#include "ELangPatcherImpl.h"

void ELangPatcherImpl::PatchEWndV02() {
    ELang::PatternSearch::SearchMatcher pattern{{
            {0x8B, 0x44, 0x24, 0x0C, 0x8B, 0x4C, 0x24, 0x08, 0x8B, 0x54, 0x24, 0x04, 0x50, 0x51, 0x52, 0xB9},
            PatternSegment::Skip(4),
            {0xE8},
            PatternSegment::Skip(4),
            {0xC2, 0x0C, 0x00},
    }};

    auto it = data_.begin();
    while ((it = pattern.search(it, data_.end())) != data_.end()) {
        auto offset = std::distance(data_.begin(), it);
        auto ecx_value = read_u32(offset + pattern.offset_at_item(1));
        auto call_delta = read_u32(offset + pattern.offset_at_item(3));
        fprintf(stderr, "  INFO: [PatchEWndV02] found (offset=0x%08x, ecx=0x%08x, call_delta=0x%08x)\n", static_cast<int>(offset), ecx_value, call_delta);

        auto snippet = GenerateCallProxyStubWithEcx(15, 3, ecx_value, call_delta);
        std::copy(snippet.cbegin(), snippet.cend(), it);
        it += pattern.size();
    }
}
