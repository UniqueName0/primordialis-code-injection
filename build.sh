#!/bin/bash

x86_64-w64-mingw32-gcc --shared inject.c -o inject.dll -I Detours/src/ -L Detours/lib.X64 -l detours
