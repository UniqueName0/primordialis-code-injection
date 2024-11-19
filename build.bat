cl ./inject.c ./utils/utils.c /LD /Fe:"./out/inject.dll" /I "./Detours/include" /link "./Detours/lib.X64/detours.lib"
cl ./main.c /Fe:"./out/PrimordialisModloader.exe"
