#pragma once
#include <Windows.h>
#include <cstdint>
#include <utility>

#if !NDEBUG
#include <cassert>
#include <cstdio>
#endif

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace FlyingRainyCats {
    namespace helper {
        uint32_t round_up_to_section_size(uint32_t size) {
            if (size % 0x1000 != 0) {
                size += 0x1000 - (size % 0x1000);
            }
            return size;
        }
    }

    namespace PEParser {
        template<const bool IS_64_BIT = false>
        class PEParser {
        public:
            using INNER_IMAGE_NT_HEADERS = typename std::conditional_t<IS_64_BIT, IMAGE_NT_HEADERS64, IMAGE_NT_HEADERS32>;
            using INNER_PIMAGE_NT_HEADERS = typename std::conditional_t<IS_64_BIT, PIMAGE_NT_HEADERS64, PIMAGE_NT_HEADERS32>;
            using INNER_PIMAGE_DOS_HEADER = PIMAGE_DOS_HEADER;
            using INNER_PIMAGE_OPTIONAL_HEADER = typename std::conditional_t<IS_64_BIT, PIMAGE_OPTIONAL_HEADER64, PIMAGE_OPTIONAL_HEADER32>;

            std::vector<uint8_t>& exe_data_;
            inline PEParser(std::vector<uint8_t>& exe_data): exe_data_(exe_data) {
            }

            inline INNER_PIMAGE_NT_HEADERS GetNtHeader() const {
                return INNER_PIMAGE_NT_HEADERS(exe_data_.data() + INNER_PIMAGE_DOS_HEADER(exe_data_.data())->e_lfanew);
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
                auto p_nt_header = INNER_PIMAGE_NT_HEADERS((uint8_t *) (exe_data_.data()) + PIMAGE_DOS_HEADER(exe_data_.data())->e_lfanew);
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
                auto p_nt_header = INNER_PIMAGE_NT_HEADERS((uint8_t *) (exe_data_.data()) + PIMAGE_DOS_HEADER(exe_data_.data())->e_lfanew);
                auto p_file_header = &p_nt_header->FileHeader;
                auto section_count = std::size_t(p_file_header->NumberOfSections);

                auto section = (PIMAGE_SECTION_HEADER) ((uint8_t *) (p_nt_header) + offsetof(INNER_IMAGE_NT_HEADERS, OptionalHeader) + p_nt_header->FileHeader.SizeOfOptionalHeader);

                auto& image_size = GetNtOptionalHeader()->SizeOfImage;

                for (std::size_t i = 0; i < section_count; i++) {
                    if (strcmp((char *) section->Name, ".text") == 0) {
                        if (section->Misc.VirtualSize + size < section->SizeOfRawData) {
                            auto offset = section->Misc.VirtualSize;
                            section->Misc.VirtualSize += size;
                            return &exe_data_.at(section->PointerToRawData + offset);
                        }
                    } else if (strcmp((char *) section->Name, ".txt2") == 0) {
                        exe_data_.resize(exe_data_.size() + size);
                        auto offset = section->SizeOfRawData;
                        section->SizeOfRawData += size;
                        image_size -= section->Misc.VirtualSize;
                        section->Misc.VirtualSize = helper::round_up_to_section_size(section->SizeOfRawData);
                        image_size += section->Misc.VirtualSize;
                        return &exe_data_.at(section->PointerToRawData + offset);
                    }
                    section++;
                }

                auto last_section = &section[-1];
                p_file_header->NumberOfSections++;

                memset(section, 0, sizeof(*section));
                memcpy(section->Name, ".txt2", 6);
                section->PointerToRawData = exe_data_.size();
                section->SizeOfRawData = size;
                section->Misc.VirtualSize = helper::round_up_to_section_size(size);
                image_size = helper::round_up_to_section_size(image_size) + section->Misc.VirtualSize;
                section->VirtualAddress = helper::round_up_to_section_size(last_section->VirtualAddress + last_section->Misc.VirtualSize);
                section->Characteristics = IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE;
                exe_data_.resize(exe_data_.size() + size);

                return &exe_data_.at(exe_data_.size() - size);
            }
        };
    }// namespace PEParser
}// namespace FlyingRainyCats
