#include <stdio.h>
#include <windows.h>

#define Address(value) ((baseAddress < 0x140000000) ? (baseAddress + value) : (baseAddress + value - 0x140000000))



__declspec(dllexport) void init() {
    freopen("CONOUT$", "w", stdout);
    printf("\n\nexample mod init() ran\n\n");
}

void testInjection()
{
  printf("test injection\n");

  MessageBoxA(NULL, "test injection", "Mod Loader", MB_OK);
}

__declspec(dllexport) void detourInit(void (*DetourAttach)(void*, void*)) {
    freopen("CONOUT$", "w", stdout);
    printf("example mod detourInit() ran\n");
    DetourAttach(NULL,NULL);

    uintptr_t baseAddress = (uintptr_t)GetModuleHandleA(NULL);
    PVOID* func_ptr = (PVOID*)malloc(sizeof(PVOID));
    void* TargetFunctionAddress = (void*)Address(0x14002db80);
    *func_ptr = TargetFunctionAddress;

    
    DetourAttach(func_ptr, &testInjection);
}

