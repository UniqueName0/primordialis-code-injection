gcc -m64 -I include/ -shared -o ./out/inject.dll inject.c
gcc -O2 main.c -o ./out/main.exe
