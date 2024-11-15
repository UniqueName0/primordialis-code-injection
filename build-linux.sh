#!/bin/bash

/usr/lib/mingw64-toolchain/bin/x86_64-w64-mingw32-gcc -O2 ./main.c -o ./out/main.exe
/usr/lib/mingw64-toolchain/bin/x86_64-w64-mingw32-gcc -m64 -shared -o ./out/inject.dll ./inject.c
