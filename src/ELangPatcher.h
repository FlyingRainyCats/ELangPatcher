#pragma once
#include <memory>

#include "SearchMatcher.h"

class ELangPatcher {
public:
    virtual void PatchEWndV02() = 0;
    virtual void PatchEWndUltimate() = 0;
    virtual void PatchWndEventHandlerMain() = 0;
    virtual void PatchKernelInvokeCall() = 0;
    virtual void PatchProxyStub() = 0;
    virtual void PatchELangLoaderInitStub() = 0;
    virtual void PatchAddFakeEWndStub() = 0;
};

std::unique_ptr<ELangPatcher> MakeELangPatcher(std::vector<uint8_t>& exe_data);
