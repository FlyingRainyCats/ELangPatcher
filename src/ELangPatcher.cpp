#include "ELangPatcher.h"
#include "ELangPatcher/ELangPatcherImpl.h"

std::unique_ptr<ELangPatcher> MakeELangPatcher(std::vector<uint8_t>& exe_data) {
    return std::make_unique<ELangPatcherImpl>(exe_data);
}
