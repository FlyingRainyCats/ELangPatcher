cmake_minimum_required(VERSION 3.27)
project(ELangPatcher)

set(CMAKE_CXX_STANDARD 20)
add_subdirectory(LibELangPatch)

add_executable(ELangPatcherTest src/test.cpp)
target_link_libraries(ELangPatcherTest LibELangPatch)

add_executable(ELangPatcher
        src/ELangPatcher.cpp
        src/ELangPatcher.h
        src/main.cpp
        src/ELangPatchFile.cpp
)
target_include_directories(ELangPatcher PRIVATE vendor/cxxopts/include)
target_compile_definitions(ELangPatcher PRIVATE WIN32_LEAN_AND_MEAN=1)
target_link_libraries(ELangPatcher LibELangPatch)
