#include "ResolveCallDllFunctionGen.h"
#include "ELangPatcherImpl.h"

void ELangPatcherImpl::PatchDllFunctionInvokeCall() {
    ELang::PatternSearch::SearchMatcher pattern{{
            {0x50, 0xE8},
            PatternSegment::Skip(4), // call_delta
            {0x83, 0xC4, 0x04, 0xFF, 0xE0},
    }};

    for (auto it = data_.begin(); (it = pattern.search(it, data_.end())) != data_.end(); it += pattern.size()) {
        auto offset = std::distance(data_.begin(), it);
        auto call_delta = read_u32(offset + pattern.offset_at_item(1));
        fprintf(stderr, "  INFO: [PatchDllFunctionInvokeCall] found (offset=0x%08x, call_delta=0x%08x)\n", static_cast<int>(offset), call_delta);

        auto padding_beg = rand_int(2, 7);
        auto padding_end = rand_int(2, 7);
        auto snippet = GenerateResolveCallDllFunction(call_delta + 1);

        auto ptr_output = pe_.ExpandTextSection(padding_beg + snippet.size() + padding_end);
        it = data_.begin() + offset;

        std::generate_n(ptr_output, padding_beg, mt_);
        std::copy(snippet.cbegin(), snippet.cend(), ptr_output + padding_beg);
        std::generate_n(ptr_output + padding_beg + snippet.size(), padding_end, mt_);

        std::generate_n(it, pattern.size(), mt_);
        write_call(offset, ptr_output + padding_beg - data_.data());
    }
}
