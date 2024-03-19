#include "ELangInitFnGen.h"
#include "ELangPatcherImpl.h"

#include <array>

void ELangPatcherImpl::PatchEWndUltimate() {
    ELang::PatternSearch::SearchMatcher pattern{{
            {0x89, 0x83},
            PatternSegment::Skip(4),// offset: heap
            {0xA1},
            PatternSegment::Skip(4),// wnd_data_offset
            {0x89, 0x83, 0xC4, 0x00, 0x00, 0x00, 0x8B, 0x0D},
            PatternSegment::Skip(4),
            {0x8B, 0x83},
            PatternSegment::Skip(4),// offset: ole_enabled
            {0x89, 0x8B, 0xC8, 0x00, 0x00, 0x00, 0x8B, 0x15},
            PatternSegment::Skip(4),
            {0x42, 0x85, 0xC0, 0x89, 0x93, 0xCC, 0x00, 0x00, 0x00},
    }};

    FlyingRainyCats::PEParser::PEParser<false> parser(data_);

    std::array<uint8_t, 12> data_elang_header{};
    constexpr size_t kPostCallJunkLen = 0x2F;

    auto it = data_.begin();
    while ((it = pattern.search(it, data_.end())) != data_.end()) {
        auto offset = std::distance(data_.begin(), it);
        auto heap_offset = read_u32(offset + pattern.offset_at_item(1));
        auto has_ole_offset = read_u32(offset + pattern.offset_at_item(7));
        auto wnd_data_address = read_u32(offset + pattern.offset_at_item(3));
        auto wnd_data_offset = static_cast<int32_t>(parser.RVAtoFOA(wnd_data_address));
        fprintf(stderr, "  INFO: [PatchEWndUltimate] found (offset=0x%08tx, data=0x%08x, wnd_data_offset=0x%08x)\n", offset, wnd_data_address, wnd_data_offset);

        std::copy_n(data_.begin() + wnd_data_offset, data_elang_header.size(), data_elang_header.begin());

        // Inject our new header (junk + code)
        size_t pre_stub_junk_len = rand_int(0x04, 0x20);
        size_t post_stub_junk_len = rand_int(0x04, 0x20);
        auto snippet = GenerateELangInitSnippet(heap_offset, has_ole_offset, reinterpret_cast<uint32_t *>(data_elang_header.data()));
        auto injected_code_ptr = parser.ExpandTextSection(pre_stub_junk_len + snippet.size() + post_stub_junk_len);
        it = data_.begin() + offset;

        // Write junk + stub + junk
        std::generate_n(injected_code_ptr, pre_stub_junk_len, mt_);
        std::copy_n(snippet.begin(), snippet.size(), injected_code_ptr + pre_stub_junk_len);
        std::generate_n(injected_code_ptr + pre_stub_junk_len + snippet.size(), post_stub_junk_len, mt_);

        auto offset_stub = injected_code_ptr + pre_stub_junk_len - data_.data();
        fprintf(stderr, "    - stub added: 0x%08x (file offset: %08x)\n", static_cast<uint32_t>(parser.FOAtoRVA(offset_stub)), static_cast<uint32_t>(offset_stub));
        write_call(offset, offset_stub);

        // Stack frame adjustment to bypass some sig matching
        {
            uint32_t len = std::uniform_int_distribution<uint32_t>(0x04, 0xFF)(mt_) & 0xFC;
            auto p_stack_offset = reinterpret_cast<uint32_t *>(data_.data() + offset - (0x00436A9F - 0x00436A88) + 2);
            *p_stack_offset += len;
        }

        // Write junk to where the header data was and post call to our stub
        std::generate_n(data_.begin() + wnd_data_offset, data_elang_header.size(), mt_);
        std::generate_n(it + 5, kPostCallJunkLen, mt_);

        it += pattern.size();
    }
}
