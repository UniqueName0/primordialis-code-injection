cl ./inject.c /LD /Fe:"./out/inject.dll" /I "./Detours/include" /link "./Detours/lib.X64/detours.lib"
cl ./main.c /Fe:"./out/main.exe"
