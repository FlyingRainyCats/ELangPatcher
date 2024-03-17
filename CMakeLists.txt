cmake_minimum_required(VERSION 3.27)
project(ELangPatcher)

set(CMAKE_CXX_STANDARD 20)
add_subdirectory(LibELangPatch)

add_executable(ELangPatcherTest src/test.cpp)
target_link_libraries(ELangPatcherTest LibELangPatch)

add_executable(ELangPatcher src/main.cpp
        src/ELangPatcher.cpp
        src/ELangPatcher.h)
target_link_libraries(ELangPatcher LibELangPatch)
