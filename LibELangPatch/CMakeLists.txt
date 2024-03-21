cmake_minimum_required(VERSION 3.27)
project(LibELangPatcher)

add_library(LibELangPatch STATIC
        WndHandlerGen.cpp
        ELangInitFnGen.cpp
        CallProxyStubWithEcxGen.cpp
        VArgsProxyGen.cpp
        ELangLoaderInitGen.cpp
        include/ELangLoaderInitGen.h
        ResolveCallDllFunctionGen.cpp
        include/ResolveCallDllFunctionGen.h
        ELangBulkPushGen.cpp
        include/ELangBulkPushGen.h
        CdeclPushAndCallGen.cpp
        include/CdeclPushAndCallGen.h
        CdeclPushAndCallGen.cpp
)

target_compile_definitions(LibELangPatch PRIVATE XBYAK32=1)
target_include_directories(LibELangPatch
        PRIVATE ../vendor/xbyak/xbyak
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(LibELangPatch PRIVATE "/MP")
endif()
