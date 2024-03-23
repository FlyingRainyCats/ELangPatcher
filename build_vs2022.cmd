@echo off
:: git submodule update --init --recursive

cmake -Bcmake-build-vs2022 -G "Visual Studio 17 2022" -A Win32
cmake --build cmake-build-vs2022 --config Release
pause
