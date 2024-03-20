#pragma once
#include <memory>

#include "SearchMatcher.h"

class ELangPatcher {
public:
    virtual ~ELangPatcher() = default;

    virtual void PatchDllFunctionInvokeCall() = 0;
    virtual void PatchEWndV02() = 0;
    virtual void PatchEWndUltimate() = 0;
    virtual void PatchWndEventHandlerMain() = 0;
    virtual void PatchKernelInvokeCall() = 0;
    virtual void PatchProxyStub() = 0;
    virtual void PatchLoadWndCall() = 0;
    virtual void PatchELangLoaderInitStub() = 0;

    virtual void MiscAddFakeEWndStub() = 0;

    inline void PatchAll() {
        PatchDllFunctionInvokeCall();
        PatchEWndV02();
        PatchEWndUltimate();
        PatchWndEventHandlerMain();
        PatchKernelInvokeCall();
        PatchProxyStub();
        PatchLoadWndCall();
        PatchELangLoaderInitStub();
    }
};

std::unique_ptr<ELangPatcher> MakeELangPatcher(std::vector<uint8_t>& exe_data);
