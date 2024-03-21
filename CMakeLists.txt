cmake_minimum_required(VERSION 3.27)
project(ELangPatcher)

set(CMAKE_CXX_STANDARD 20)
add_subdirectory(LibELangPatch)

add_executable(ELangPatcherTest src/test.cpp)
target_link_libraries(ELangPatcherTest LibELangPatch)

add_executable(ELangPatcher
        src/ELangPatcher.cpp
        src/main.cpp
        src/ELangPatchFile.cpp
        src/ELangPatcher/PatchEWndV02.cpp
        src/ELangPatcher/PatchEWndV02.cpp
        src/ELangPatcher/PatchEWndUltimate.cpp
        src/ELangPatcher/PatchEWndUltimate.cpp
        src/ELangPatcher/PatchWndEventHandlerMain.cpp
        src/ELangPatcher/PatchKernelInvokeCall.cpp
        src/ELangPatcher/PatchKernelInvokeCall.cpp
        src/ELangPatcher/PatchProxyStub.cpp
        src/ELangPatcher/PatchAddFakeEWndStub.cpp
        src/ELangPatcher/PatchELangLoaderInitStub.cpp
        src/ELangPatcher/PatchDllResolveAndCall.cpp
        src/ELangPatcher/PatchLoadWndCall.cpp
        src/ELangPatcher/PatchSuspiciousCallWithParam.cpp
)
target_include_directories(ELangPatcher PRIVATE vendor/cxxopts/include)
target_compile_definitions(ELangPatcher PRIVATE WIN32_LEAN_AND_MEAN=1)
target_link_libraries(ELangPatcher LibELangPatch)

if(MSVC)
    target_compile_options(ELangPatcher PRIVATE "/MP")
endif()
