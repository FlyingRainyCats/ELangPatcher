#include "ELangPatcher.h"

#include "PEParser.h"

#include <CallProxyStubWithEcxGen.h>
#include <ELangInitFnGen.h>
#include <ELangLoaderInitGen.h>
#include <VArgsProxyGen.h>
#include <WndHandlerGen.h>

#include <algorithm>
#include <array>
#include <random>

#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

void ELangPatcher::PatchEWndV02() {
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
void ELangPatcher::PatchEWndUltimate() {
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

    std::mt19937 mt(std::random_device{}());
    std::array<uint8_t, 12> data_elang_header{};
    std::array<uint8_t, 0x2F> data_stub_junk{};

    auto it = data_.begin();
    while ((it = pattern.search(it, data_.end())) != data_.end()) {
        auto offset = std::distance(data_.begin(), it);
        auto heap_offset = read_u32(offset + pattern.offset_at_item(1));
        auto has_ole_offset = read_u32(offset + pattern.offset_at_item(7));
        auto wnd_data_address = read_u32(offset + pattern.offset_at_item(3));
        auto wnd_data_offset = static_cast<int32_t>(parser.RVAtoFOA(wnd_data_address));
        fprintf(stderr, "  INFO: [PatchEWndUltimate] found (offset=0x%08tx, data=0x%08x, wnd_data_offset=0x%08x)\n", offset, wnd_data_address, wnd_data_offset);

        std::copy_n(data_.begin() + wnd_data_offset, data_elang_header.size(), data_elang_header.begin());

        size_t initial_junk_len = std::uniform_int_distribution<>(0x04, 0x20)(mt);
        std::vector<uint8_t> initial_junk(initial_junk_len);
        std::generate(initial_junk.begin(), initial_junk.end(), mt);

        // Inject our new header
        auto snippet = GenerateELangInitSnippet(heap_offset, has_ole_offset, reinterpret_cast<uint32_t *>(data_elang_header.data()));
        snippet.insert(snippet.begin(), initial_junk.cbegin(), initial_junk.cend());
        auto injected_code_ptr = parser.ExpandTextSection(snippet.size());
        it = data_.begin() + offset;
        std::copy_n(snippet.begin(), snippet.size(), injected_code_ptr);

        auto *p_stub_addr = injected_code_ptr + initial_junk_len;
        fprintf(stderr, "    - stub added: 0x%08x (file offset: %08x)\n", static_cast<uint32_t>(parser.FOAtoRVA(p_stub_addr - data_.data())), static_cast<uint32_t>(p_stub_addr - data_.data()));
        auto ptr_code = data_.data() + offset;
        ptr_code[0] = 0xE8;
        auto injected_code_rva = parser.FOAtoRVA(p_stub_addr - data_.data());
        auto ptr_code_rva = parser.FOAtoRVA(offset);
        *reinterpret_cast<uint32_t *>(&ptr_code[1]) = injected_code_rva - ptr_code_rva - 5;

        {
            uint32_t len = std::uniform_int_distribution<uint32_t>(0x04, 0xFF)(mt) & 0xFC;
            auto p_stack_offset = reinterpret_cast<uint32_t *>(data_.data() + offset - (0x00436A9F - 0x00436A88) + 2);
            *p_stack_offset += len;
        }

        // Write junk to where the header was
        std::generate(data_elang_header.begin(), data_elang_header.end(), mt);
        std::copy(data_elang_header.cbegin(), data_elang_header.cend(), data_.begin() + wnd_data_offset);

        std::generate(data_stub_junk.begin(), data_stub_junk.end(), mt);
        std::copy(data_stub_junk.cbegin(), data_stub_junk.cend(), &ptr_code[5]);

        it += pattern.size();
    }
}

void ELangPatcher::PatchWndEventHandlerMain() {
    ELang::PatternSearch::SearchMatcher pattern{{
            {0x55, 0x8B, 0xEC, 0x83, 0xEC, 0x18, 0x53, 0x56, 0x57, 0x89, 0x4D, 0xE8, 0x8B, 0x45, 0x08,
             0x8B, 0x48, 0x0C, 0x51, 0x8B, 0x55, 0x08, 0x8B, 0x42, 0x08, 0x50, 0x8B, 0x4D, 0x08, 0x8B,
             0x51, 0x04, 0x52, 0x8B, 0x45, 0x08, 0x8B, 0x08, 0x51, 0x8B, 0x4D, 0xE8},
            {0xE8},
            PatternSegment::Skip(4),
    }};
    ELang::PatternSearch::SearchMatcher pattern_end_of_function{
            {0x8B, 0xE5, 0x5D, 0xC2, 0x04, 0x00},
    };

    FlyingRainyCats::PEParser::PEParser<false> parser(data_);

    auto it = data_.begin();
    while ((it = pattern.search(it, data_.end())) != data_.end()) {
        auto offset = std::distance(data_.begin(), it);
        auto call_inst_address_foa = static_cast<uint32_t>(offset + pattern.offset_at_item(1));
        auto call_inst_address = static_cast<uint32_t>(parser.FOAtoRVA(call_inst_address_foa));
        auto call_inst_delta = read_u32(offset + pattern.offset_at_item(2));
        auto it_end_of_function_end = std::min(it + 0x200, data_.end());
        auto it_end_of_function = pattern_end_of_function.search(it, it_end_of_function_end);
        auto eof_found = it_end_of_function != it_end_of_function_end;
        auto eof_offset = eof_found ? std::distance(data_.begin(), it_end_of_function) : 0;
        fprintf(stderr, "  INFO: [PatchWndEventHandlerMain] found (eof=%08tx, offset=0x%08tx, inst=0x%08x(p: 0x%08x), delta=0x%08x)\n", eof_offset, offset, call_inst_address_foa, call_inst_address, call_inst_delta);
        if (!eof_found) {
            continue;
        }

        auto function_size = eof_offset + pattern_end_of_function.size() - offset;
        auto snippet = GenerateWndHandlerCode(static_cast<uint32_t>(call_inst_address), call_inst_delta);
        if (snippet.size() > function_size) {
            fprintf(stderr, "  WARN: snippet too big, ignored (expected %zu, got %d bytes)\n", snippet.size(), static_cast<uint32_t>(function_size));
            continue;
        }
        if (auto padding_size = function_size - snippet.size()) {
            std::vector<uint8_t> padding(padding_size);
            std::mt19937 mt{std::random_device{}()};
            std::generate(padding.begin(), padding.end(), mt);
            snippet.insert(snippet.end(), padding.cbegin(), padding.cend());
        }
        std::copy(snippet.cbegin(), snippet.cend(), it);
        it += pattern.size();
    }
}

void ELangPatcher::PatchWndEventHandlerSecondary() {
}
void ELangPatcher::AddFakeEWndStub() {
    std::vector<uint8_t> junk_data_inner = {
            // start of function
            0x55, 0x8B, 0xEC,
            // junk 1
            0xe8,
            0x50, 0x64, 0x89, 0x25, 0x00, 0x00, 0x00, 0x00, 0x81, 0xec, 0xac, 0x01, 0x00, 0x00, 0x53, 0x56, 0x57,
            // junk 2
            0xea,
            0x8b, 0x44, 0x24, 0x0c, 0x8b, 0x4c, 0x24, 0x08, 0x8b, 0x54, 0x24, 0x04, 0x50, 0x51, 0x52, 0xb9,
            // junk 3
            0xe9,
            0x83, 0xec, 0x0c, 0x33, 0xc0, 0x56, 0x8b, 0x74, 0x24, 0x1c, 0x57, 0x8b, 0x7c, 0x24, 0x18, 0xc7, 0x07, 0x00, 0x00, 0x00, 0x00, 0x8b, 0x4e, 0x14, 0x85, 0xc9, 0x74, 0x13, 0x50, 0x8b, 0x46, 0x0c, 0x50, 0x68, 0xd6, 0x07, 0x00, 0x00,
            // end of function
            0x8B, 0xE5, 0x5D, 0xC2, 0x0C, 0x00};

    FlyingRainyCats::PEParser::PEParser<false> parser(data_);
    std::mt19937 mt{std::random_device{}()};
    std::uniform_int_distribution<> dist_padding(0x10, 0x20);

    auto prefix_len = dist_padding(mt);
    auto suffix_len = dist_padding(mt);
    auto payload_len = prefix_len + junk_data_inner.size() + suffix_len;
    fprintf(stderr, "  INFO: [AddFakeEWndStub] add stub (len=%d bytes)\n", static_cast<int>(payload_len));

    std::vector<uint8_t> junk_data(payload_len);
    std::generate(junk_data.begin(), junk_data.end(), mt);
    std::copy(junk_data.cbegin(), junk_data.cend(), parser.ExpandTextSection(payload_len));
}

void ELangPatcher::PatchKernelInvokeCall() {
    std::vector<ELang::PatternSearch::SearchMatcher> patterns{
            ELang::PatternSearch::SearchMatcher{{
                    {0x8D, 0x54, 0x24, 0x08, 0x83, 0xEC, 0x0C, 0x52, 0xFF, 0x74, 0x24, 0x14, 0xC7, 0x44, 0x24, 0x08, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x44, 0x24, 0x0C, 0x00, 0x00, 0x00, 0x00, 0xC7, 0x44, 0x24, 0x10, 0x00, 0x00, 0x00, 0x00, 0x8D, 0x54, 0x24, 0x08, 0x52, 0xFF, 0xD3, 0x8B, 0x44, 0x24, 0x0C, 0x8B, 0x54, 0x24, 0x10, 0x8B, 0x4C, 0x24, 0x14, 0x83, 0xC4, 0x18, 0xC3},
            }},
            ELang::PatternSearch::SearchMatcher{{
                    {0x8D, 0x44, 0x24, 0x08, 0x83, 0xEC, 0x0C, 0x50, 0xFF, 0x74, 0x24, 0x14, 0x33, 0xC0, 0x89, 0x44, 0x24, 0x08, 0x89, 0x44, 0x24, 0x0C, 0x89, 0x44, 0x24, 0x10, 0x8D, 0x54, 0x24, 0x08, 0x52, 0xFF, 0xD3, 0x8B, 0x44, 0x24, 0x0C, 0x8B, 0x54, 0x24, 0x10, 0x8B, 0x4C, 0x24, 0x14, 0x83, 0xC4, 0x18, 0xC3},
            }},
    };

    FlyingRainyCats::PEParser::PEParser<false> parser(data_);
    std::mt19937 mt{std::random_device{}()};

    int pattern_id{0};
    for (auto &pattern: patterns) {
        auto pattern_size = pattern.size();

        auto it = data_.begin();
        while ((it = pattern.search(it, data_.end())) != data_.end()) {
            auto offset = std::distance(data_.begin(), it);
            auto snippet = GenerateVArgsProxyCode();
            for (int retries{3}; snippet.size() >= 0x40 && retries >= 0; retries--) {
                snippet = GenerateVArgsProxyCode();
            }

            fprintf(stderr, "  INFO: [PatchKernelInvokeCall#%d] found (offset=0x%08tx, len=%04x, replace_len=%04x)\n", pattern_id, offset, static_cast<int>(pattern_size), static_cast<int>(snippet.size()));
            std::copy(snippet.cbegin(), snippet.cend(), it);

            if (pattern_size > snippet.size()) {
                std::vector<uint8_t> junk(pattern_size - snippet.size());
                std::generate(junk.begin(), junk.end(), mt);
                std::copy(junk.begin(), junk.end(), it + static_cast<int>(snippet.size()));
            }

            it += pattern_size;
        }
        pattern_id++;
    }
}

class ProxyStubContainer {
public:
    ProxyStubContainer(const char *name, int arg_count, std::initializer_list<ELang::PatternSearch::PatternSegment> pattern_segments) : name_(name) {
        pattern_ = {pattern_segments};
        arg_count_ = {arg_count};
        beg_padding_ = pattern_.offset_at_item(1) - 1;
        end_padding_ = pattern_.size() - (pattern_.offset_at_item(pattern_.pattern_count() - 1));
    }
    const char *name_{};
    ELang::PatternSearch::SearchMatcher pattern_{};
    int arg_count_{};
    int beg_padding_{};
    int end_padding_{};
};

void ELangPatcher::PatchProxyStub() {
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
        auto it = data_.begin();
        while ((it = pattern.search(it, data_.end())) != data_.end()) {
            auto offset = std::distance(data_.begin(), it);
            auto ecx_value = read_u32(offset + pattern.offset_at_item(1));
            auto call_delta = read_u32(offset + pattern.offset_at_item(3));
            fprintf(stderr, "  INFO: [%s#%d] found (offset=0x%08x, args=[%d], ecx=0x%08x, call_delta=0x%08x)\n", name, count, static_cast<int>(offset), arg_count, ecx_value, call_delta);

            auto snippet = GenerateCallProxyStubWithEcxCdecl(arg_count, beg_padding, end_padding, ecx_value, call_delta);
            std::copy(snippet.cbegin(), snippet.cend(), it);
            it += pattern.size();
            count++;
        }
    }
}
void ELangPatcher::PatchELangLoaderInitStub() {
    ELang::PatternSearch::SearchMatcher pattern{{
            {0xFC, 0xDB, 0xE3, 0xE8, 0xEC},
    }};

    FlyingRainyCats::PEParser::PEParser<false> parser(data_);
    std::mt19937 mt{std::random_device{}()};
    std::array<uint8_t, 3> junk_padding{};

    auto it = data_.begin();
    while ((it = pattern.search(it, data_.end())) != data_.end()) {
        auto offset = std::distance(data_.begin(), it);
        fprintf(stderr, "  INFO: [PatchELangLoaderInitStub] found (offset=0x%08tx)\n", offset);

        it[0] = 0xE8;
        const auto call_delta = *reinterpret_cast<uint32_t *>(data_.data() + offset + 4);

        auto snippet = GenerateELangLoaderInit({call_delta + 3});
        auto p_snippet_out = parser.ExpandTextSection(snippet.size());
        it = data_.begin() + offset;
        std::copy(snippet.cbegin(), snippet.cend(), p_snippet_out);

        uint32_t snippet_out_rva = parser.FOAtoRVA(p_snippet_out - data_.data());
        uint32_t target_rva = parser.FOAtoRVA(offset);

        *reinterpret_cast<uint32_t *>(data_.data() + offset + 1) = snippet_out_rva - target_rva - 5;

        std::generate(junk_padding.begin(), junk_padding.end(), mt);
        std::copy(junk_padding.cbegin(), junk_padding.cend(), it + 5);

        it += pattern.size();
    }
}
