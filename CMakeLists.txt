cmake_minimum_required(VERSION 3.27)
project(ELangPatcher)

set(CMAKE_CXX_STANDARD 17)

add_executable(ELangPatcher
        src/main.cpp
        src/WndHandlerGen.cpp
)

target_compile_definitions(ELangPatcher PRIVATE XBYAK32=1)
target_include_directories(ELangPatcher PRIVATE vendor/xbyak/xbyak)
