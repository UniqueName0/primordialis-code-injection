#include <stdio.h>
#include <windows.h>
#include "hook.c"

HookData testhook;

void testInjection()
{
  printf("runs before original function\n");
  MessageBoxA(NULL, "test injection", "Mod Loader", MB_OK);


  DetachHook(testhook);
  ReturnToOrigFunction(testhook);
  ReAttachHook(testhook);


  printf("runs after original function\n");
}

__declspec(dllexport) void init() {
    freopen("CONOUT$", "w", stdout);
    printf("\n\nexample mod init() ran\n\n");

    uintptr_t baseAddress = (uintptr_t)GetModuleHandleA(NULL);
    void* TargetFunctionAddress = (void*)Address(0x14002db80);

    testhook = AttachHook(TargetFunctionAddress, &testInjection);
}
