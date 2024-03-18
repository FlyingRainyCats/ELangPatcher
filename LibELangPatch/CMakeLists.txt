cmake_minimum_required(VERSION 3.27)
project(LibELangPatcher)

add_library(LibELangPatch STATIC
        WndHandlerGen.cpp
        ELangInitFnGen.cpp
        WndEventProxyGen.cpp
        VArgsProxyGen.cpp
)

target_compile_definitions(LibELangPatch PRIVATE XBYAK32=1)
target_include_directories(LibELangPatch
        PRIVATE ../vendor/xbyak/xbyak
        PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
