#!/bin/bash

cmake -DCMAKE_TOOLCHAIN_FILE=mingw-w64-x86_64.cmake .
make
strip --strip-unneeded bin/xpchatter.exe
cp bin/xpchatter.exe /opt/winxp