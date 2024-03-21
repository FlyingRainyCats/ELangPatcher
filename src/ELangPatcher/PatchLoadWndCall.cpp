#include "ELangBulkPushGen.h"
#include "ELangPatcherImpl.h"

void ELangPatcherImpl::PatchLoadWndCall() {
    // @formatter:off
    std::vector<ELang::PatternSearch::SearchMatcher> patterns{
            ELang::PatternSearch::SearchMatcher{{
                    {0x68}, PatternSegment::Skip(4),
                    {0x6A}, PatternSegment::Skip(1),
                    {0x68}, PatternSegment::Skip(4),
                    {0x68}, PatternSegment::Skip(4),
                    {0x68}, PatternSegment::Skip(4),
                    {0x68}, PatternSegment::Skip(4),
                    {0x68}, PatternSegment::Skip(4),
                    {0x68}, PatternSegment::Skip(4),
                    {0x68}, PatternSegment::Skip(4),
                    {0x68, 0x03, 0x00, 0x00, 0x00},// 3 args
                    {0xBB}, PatternSegment::Skip(4),
                    {0xE8}, PatternSegment::Skip(4),
            }},
            ELang::PatternSearch::SearchMatcher{{
                    {0x68}, PatternSegment::Skip(4),
                    {0x6A}, PatternSegment::Skip(1),
                    {0x68}, PatternSegment::Skip(4),
                    {0x6A}, PatternSegment::Skip(1),
                    {0x6A}, PatternSegment::Skip(1),
                    {0x6A}, PatternSegment::Skip(1),
                    {0x68}, PatternSegment::Skip(4),
                    {0x68}, PatternSegment::Skip(4),
                    {0x68}, PatternSegment::Skip(4),
                    {0x68, 0x03, 0x00, 0x00, 0x00},// 3 args
                    {0xBB}, PatternSegment::Skip(4), // mov ebx, ???
                    {0xE8}, PatternSegment::Skip(4), // call ???
            }},
    };

    std::vector<uint32_t> values{};
    values.reserve(10);

    for (auto &pattern: patterns) {
        for (auto it = data_.begin(); (it = pattern.search(it, data_.end())) != data_.end(); it += pattern.size()) {
            auto offset = std::distance(data_.begin(), it);
            auto ebx = read_u32(offset + pattern.offset_at_item(20));
            auto call_delta = read_u32(offset + pattern.offset_at_item(22));

            values.resize(0);
            for (int i = 1; i < 18; i += 2) {
                auto len = pattern.size_at_item(i);
                if (len == 4) {
                    values.push_back(read_u32(offset + pattern.offset_at_item(i)));
                } else {
                    values.push_back(data_[offset + pattern.offset_at_item(i)]);
                }
            }
            values.push_back(3);

            // Might be data pointer, skip...
            if (values[0] != 0x80000002 || (values[3] != 0x10001 && values[3] != 0) || values[6] != 0x10001) {
                continue;
            }

            fprintf(stderr, "  INFO: [PatchLoadWndCall] found (offset=0x%08x, ebx=0x%08x, call_delta=0x%08x)\n", static_cast<int>(offset), ebx, call_delta);

            auto padding_beg = rand_int(2, 7);
            auto padding_end = rand_int(2, 7);
            auto ret_addr_foa = offset + pattern.size();
            auto ret_addr_rva = pe_.FOAtoRVA(ret_addr_foa);
            auto snippet = GenerateBulkPushInstructionWithEBXCall(pattern.size() - 5, values, ebx - ret_addr_rva, call_delta);

            auto ptr_output = pe_.ExpandTextSection(padding_beg + snippet.size() + padding_end);
            it = data_.begin() + offset;

            // write new snippet
            std::generate_n(ptr_output, padding_beg, mt_);
            std::copy(snippet.cbegin(), snippet.cend(), ptr_output + padding_beg);
            std::generate_n(ptr_output + padding_beg + snippet.size(), padding_end, mt_);

            // write our call to new fn
            std::generate_n(it, pattern.size(), mt_);
            write_call(offset, ptr_output + padding_beg - data_.data());
        }
    }
}
