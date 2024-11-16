#!/bin/bash

# you probably want to use https://github.com/NathanSnail/Detours/tree/linux
# which is just a mingw gcc makefile for detours so that you can compile everything on linux
rm ./out/inject.dll
rm ./out/PrimordialisModloader.exe
x86_64-w64-mingw32-gcc --shared inject.c -o ./out/inject.dll
x86_64-w64-mingw32-gcc -o ./out/PrimordialisModloader.exe main.c
