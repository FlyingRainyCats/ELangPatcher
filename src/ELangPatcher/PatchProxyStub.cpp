#include "CallProxyStubWithEcxGen.h"
#include "ELangPatcherImpl.h"

class ProxyStubContainer {
public:
    ProxyStubContainer(const char *name, int arg_count, std::initializer_list<ELang::PatternSearch::PatternSegment> pattern_segments) : name_(name) {
        pattern_ = {pattern_segments};
        arg_count_ = {arg_count};
        beg_padding_ = static_cast<int>(pattern_.offset_at_item(1)) - 1;
        end_padding_ = static_cast<int>(pattern_.size()) - static_cast<int>(pattern_.offset_at_item(pattern_.pattern_count() - 1));
    }

    const char *name_{};
    ELang::PatternSearch::SearchMatcher pattern_{};
    int arg_count_{};
    int beg_padding_{};
    int end_padding_{};
};

void ELangPatcherImpl::PatchProxyStub() {
    std::vector<ProxyStubContainer> patterns{
            {"DLLInvokeCall", 1,
             {
                     {0x55, 0x8B, 0xEC, 0x8B, 0x45, 0x08, 0x50, 0xB9},
                     PatternSegment::Skip(4),
                     {0xE8},
                     PatternSegment::Skip(4),
                     {0x5D, 0xC3},
             }},

            {"LoadInitWindow", 4,
             {
                     {0x55, 0x8B, 0xEC, 0x8B, 0x45, 0x14, 0x50, 0x8B, 0x4D, 0x10, 0x51, 0x8B, 0x55, 0x0C, 0x52, 0x8B, 0x45, 0x08, 0x50, 0xB9},
                     PatternSegment::Skip(4),
                     {0xE8},
                     PatternSegment::Skip(4),
                     {0x5D, 0xC3},
             }},

            {"UnknownCtrlRelated", 6,
             {
                     {0x55, 0x8B, 0xEC, 0x8B, 0x45, 0x1C, 0x50, 0x8B, 0x4D, 0x18, 0x51, 0x8B, 0x55, 0x14, 0x52, 0x8B, 0x45, 0x10, 0x50, 0x8B, 0x4D, 0x0C, 0x51, 0x8B, 0x55, 0x08, 0x52, 0xB9},
                     PatternSegment::Skip(4),
                     {0xE8},
                     PatternSegment::Skip(4),
                     {0x5D, 0xC3},
             }},
    };

    for (const auto& item: patterns) {
        auto &pattern = item.pattern_;
        auto beg_padding = item.beg_padding_;
        auto end_padding = item.end_padding_;
        auto arg_count = item.arg_count_;
        const auto *name = item.name_;

        int count{0};
        for (auto it = data_.begin(); (it = pattern.search(it, data_.end())) != data_.end(); it += pattern.size()) {
            auto offset = std::distance(data_.begin(), it);
            auto ecx_value = read_u32(offset + pattern.offset_at_item(1));
            auto call_delta = read_u32(offset + pattern.offset_at_item(3));
            fprintf(stderr, "  INFO: [%s#%d] found (offset=0x%08x, args=[%d], ecx=0x%08x, call_delta=0x%08x)\n", name, count, static_cast<int>(offset), arg_count, ecx_value, call_delta);

            auto snippet = GenerateCallProxyStubWithEcxCdecl(arg_count, beg_padding, end_padding, ecx_value, call_delta);
            std::copy(snippet.cbegin(), snippet.cend(), it);
            count++;
        }
    }
}
