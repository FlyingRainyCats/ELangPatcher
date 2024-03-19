#include "ELangLoaderInitGen.h"
#include "ELangPatcherImpl.h"

void ELangPatcherImpl::PatchELangLoaderInitStub() {
    ELang::PatternSearch::SearchMatcher pattern{{
            {0xFC, 0xDB, 0xE3, 0xE8, 0xEC},
    }};

    FlyingRainyCats::PEParser::PEParser<false> parser(data_);
    std::mt19937 mt{std::random_device{}()};

    auto it = data_.begin();
    while ((it = pattern.search(it, data_.end())) != data_.end()) {
        auto offset = std::distance(data_.begin(), it);
        fprintf(stderr, "  INFO: [PatchELangLoaderInitStub] found (offset=0x%08tx)\n", offset);

        const auto call_delta = *reinterpret_cast<uint32_t *>(data_.data() + offset + 4);

        // Generate call inst
        auto snippet = GenerateELangLoaderInit({call_delta + 3});
        auto p_snippet_out = parser.ExpandTextSection(snippet.size());
        it = data_.begin() + offset;
        std::copy(snippet.cbegin(), snippet.cend(), p_snippet_out);

        // Write call inst + 3 byte junk
        auto offset_snippet = p_snippet_out - data_.data();
        write_call(offset, offset_snippet);
        std::generate_n(it + 5, 3, mt_);

        it += pattern.size();
    }
}
