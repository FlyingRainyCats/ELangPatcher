#pragma once
#include <Windows.h>
#include <cstdint>
#include <utility>

#if !NDEBUG
#include <cassert>
#include <cstdio>
#endif

namespace FlyingRainyCats {
    namespace PEParser {
        template<const bool IS_64_BIT = false>
        class PEParser {
        public:
            using INNER_IMAGE_NT_HEADERS = typename std::conditional_t<IS_64_BIT, IMAGE_NT_HEADERS64, IMAGE_NT_HEADERS32>;
            using INNER_PIMAGE_NT_HEADERS = typename std::conditional_t<IS_64_BIT, PIMAGE_NT_HEADERS64, PIMAGE_NT_HEADERS32>;
            using INNER_PIMAGE_DOS_HEADER = PIMAGE_DOS_HEADER;
            using INNER_PIMAGE_OPTIONAL_HEADER = typename std::conditional_t<IS_64_BIT, PIMAGE_OPTIONAL_HEADER64, PIMAGE_OPTIONAL_HEADER32>;

            inline PEParser(void *p_base) {
                base_ = (uint8_t *) p_base;
            }

            inline INNER_PIMAGE_NT_HEADERS GetNtHeader() const {
                return INNER_PIMAGE_NT_HEADERS(base_ + INNER_PIMAGE_DOS_HEADER(base_)->e_lfanew);
            }

            inline INNER_PIMAGE_OPTIONAL_HEADER GetNtOptionalHeader() const {
                return &GetNtHeader()->OptionalHeader;
            }

            [[nodiscard]] inline uint32_t RVAtoFOA(DWORD address) const {
                auto p_nt_header = GetNtHeader();
                auto p_file_header = &p_nt_header->FileHeader;
                auto section_count = std::size_t(p_file_header->NumberOfSections);

                address -= p_nt_header->OptionalHeader.ImageBase;

                auto section = (PIMAGE_SECTION_HEADER) ((uint8_t *) (p_nt_header) + offsetof(IMAGE_NT_HEADERS, OptionalHeader) + p_nt_header->FileHeader.SizeOfOptionalHeader);

                for (std::size_t i = 0; i < section_count; i++) {
                    if ((section->VirtualAddress <= address) && (address < (section->VirtualAddress + section->Misc.VirtualSize))) {
                        return section->PointerToRawData + (address - section->VirtualAddress);
                    }
                    section++;
                }

                return 0;
            }

            [[nodiscard]] inline uintptr_t FOAtoRVA(const DWORD address) const {
                auto p_nt_header = INNER_PIMAGE_NT_HEADERS((uint8_t *) (base_) + PIMAGE_DOS_HEADER(base_)->e_lfanew);
                auto p_file_header = &p_nt_header->FileHeader;
                auto section_count = std::size_t(p_file_header->NumberOfSections);

                auto section = (PIMAGE_SECTION_HEADER) ((uint8_t *) (p_nt_header) + offsetof(INNER_IMAGE_NT_HEADERS, OptionalHeader) + p_nt_header->FileHeader.SizeOfOptionalHeader);

                for (std::size_t i = 0; i < section_count; i++) {
                    if ((section->PointerToRawData <= address) && (address < (section->PointerToRawData + section->SizeOfRawData))) {
                        return p_nt_header->OptionalHeader.ImageBase + address - section->PointerToRawData + section->VirtualAddress;
                    }
                    section++;
                }

                return 0;
            }

            inline uint8_t* ExpandTextSection(uint32_t size) {
                auto p_nt_header = INNER_PIMAGE_NT_HEADERS((uint8_t *) (base_) + PIMAGE_DOS_HEADER(base_)->e_lfanew);
                auto p_file_header = &p_nt_header->FileHeader;
                auto section_count = std::size_t(p_file_header->NumberOfSections);

                auto section = (PIMAGE_SECTION_HEADER) ((uint8_t *) (p_nt_header) + offsetof(INNER_IMAGE_NT_HEADERS, OptionalHeader) + p_nt_header->FileHeader.SizeOfOptionalHeader);

                for (std::size_t i = 0; i < section_count; i++) {
                    if (strcmp((char *) section->Name, ".text") == 0) {
                        if (section->Misc.VirtualSize + size < section->SizeOfRawData) {
                            auto offset = section->Misc.VirtualSize;
                            section->Misc.VirtualSize += size;
                            return base_ + section->PointerToRawData + offset;
                        }
                    }
                    section++;
                }
                // TODO: add new section at the end of exe
                return nullptr;
            }

        private:
            uint8_t *base_;
        };
    }// namespace PEParser
}// namespace FlyingRainyCats
