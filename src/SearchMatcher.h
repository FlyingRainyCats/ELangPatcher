#pragma once

#include <algorithm>
#include <cstdint>
#include <vector>

namespace ELang::PatternSearch {
    enum class Type {
        Match = 0,
        Skip,
    };

    struct PatternSegment {
        Type type_;
        size_t len_;
        std::vector<uint8_t> data_{};

        explicit PatternSegment(size_t seek_len) : type_(Type::Skip), len_(seek_len) {}
        PatternSegment(std::initializer_list<uint8_t> data) : type_(Type::Match) {
            len_ = data.size();
            data_ = {data.begin(), data.end()};
        }
        PatternSegment(size_t data_len, const void *data) : type_(Type::Match), len_(data_len) {
            auto p_data = reinterpret_cast<const uint8_t *>(data);
            data_ = {p_data, p_data + data_len};
        }

        static PatternSegment Skip(size_t n) {
            return PatternSegment(n);
        }
    };

    class SearchMatcher {
    public:
        SearchMatcher(std::initializer_list<PatternSegment> pattern) {
            pattern_.insert(pattern_.end(), pattern.begin(), pattern.end());
            for (auto &segment: pattern) {
                len_ += segment.len_;
            }
        }

        [[nodiscard]] size_t pattern_count() const {
            return pattern_.size();
        }

        [[nodiscard]] size_t offset_at_item(size_t idx) const {
            size_t offset = 0;
            for (size_t i = 0; i < idx; ++i) {
                offset += pattern_[i].len_;
            }
            return offset;
        }

        template<typename It>
        bool matches(It first) const {
            for (const auto &segment: pattern_) {
                switch (segment.type_) {
                    case Type::Match:
                        if (!std::equal(segment.data_.cbegin(), segment.data_.cend(), first)) {
                            return false;
                        }
                        break;
                    case Type::Skip:
                        break;
                }
                first += segment.len_;
            }

            return true;
        }

        template<typename It>
        It search(It first, It last) const {
            It search_end = last - len_;
            while (first < search_end) {
                if (matches(first)) {
                    return first;
                }
                first++;
            }

            return last;
        }

        [[nodiscard]] ptrdiff_t size() const {
            return static_cast<ptrdiff_t>(len_);
        }

    private:
        std::vector<PatternSegment> pattern_{};
        size_t len_{0};
    };

}// namespace ELang::PatternSearch
