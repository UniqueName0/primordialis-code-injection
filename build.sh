#!/bin/bash

# you probably want to use https://github.com/NathanSnail/Detours/tree/linux
# which is just a mingw gcc makefile for detours so that you can compile everything on linux
x86_64-w64-mingw32-gcc --shared inject.c -o inject.dll -I Detours/src/ -L Detours/lib.X64 -l detours
