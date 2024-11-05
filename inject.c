#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include "debugTools.c"

#pragma comment (lib, "user32.lib")

#define Address(value) ((baseAddress < 0x140000000) ? (baseAddress + value) : (baseAddress + value - 0x140000000))


uintptr_t baseAddress;


typedef struct {
  BYTE originalBytes[16];
  BYTE newBytes[16];
  void* targetFunction;
  void* hookedFunction;
} HookData;

HookData AttachHook(void* targetFunction, void* hookAddress) {
    HookData data;

    // generate new bytes for hook
    uint8_t newBytes[] = {0x50, 0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x87, 0x04, 0x24, 0xC3 };
    memcpy(newBytes+3, &hookAddress, 8);

    //save newBytes to HookData
    memcpy(data.newBytes, newBytes, sizeof(newBytes));

    // Save the original bytes of the target function.
    memcpy(data.originalBytes, targetFunction, sizeof(newBytes));
    data.targetFunction = targetFunction;
    data.hookedFunction = hookAddress;

    // Change memory protection to read/write.
    DWORD oldProtect;
    VirtualProtect(targetFunction, sizeof(newBytes), PAGE_EXECUTE_READWRITE, &oldProtect);

    //apply hook
    memcpy(targetFunction, newBytes, sizeof(newBytes));

    // Restore the original memory protection.
    VirtualProtect(targetFunction, sizeof(newBytes), oldProtect, NULL);

    return data;
}

void ReAttachHook(HookData hook) {
    DWORD oldProtect;
    VirtualProtect(hook.targetFunction, sizeof(hook.newBytes), PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(hook.targetFunction, hook.newBytes, sizeof(hook.newBytes));
    VirtualProtect(hook.targetFunction, sizeof(hook.newBytes), oldProtect, NULL);
}

void ReturnToOrigFunction(HookData hook) {
  register void* rax_register asm("%r12");
  rax_register = hook.targetFunction;

  //printf("rax: %p\n", rax_register);

  __asm__("pushq %r12\n"
          "xchgq (%rsp), %r12\n"
          "popq %r12\n");
}

void detachHook(HookData hook) {
    DWORD oldProtect;
    VirtualProtect(hook.targetFunction, sizeof(hook.originalBytes), PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(hook.targetFunction, hook.originalBytes, sizeof(hook.originalBytes));
    VirtualProtect(hook.targetFunction, sizeof(hook.originalBytes), oldProtect, NULL);

}

void hookHelper(HookData hook) {
  detachHook(hook);
  ReturnToOrigFunction(hook);
  ReAttachHook(hook);
}


HookData testhook;
void  testInjection() {
  printf("test injection\n");
  //MessageBoxA(NULL, "test injection", "Mod Loader", MB_OK);

  hookHelper(testhook);

}

BOOL APIENTRY DllMain(HMODULE hModule,  DWORD  nReason, LPVOID lpReserved) {
  baseAddress = (uintptr_t)GetModuleHandleA(NULL);
  switch (nReason) {
  case DLL_PROCESS_ATTACH:

    AllocConsole();
    freopen("CONOUT$", "w", stdout);

    //MessageBoxA(NULL, "Mod loader started", "Mod Loader", MB_OK);

    uintptr_t TargetFunctionAddress = Address(0x14004af60); // something to do with the main menu
    testhook = AttachHook((void*)TargetFunctionAddress, &testInjection);

    break;
  case DLL_PROCESS_DETACH:
    break;
  case DLL_THREAD_ATTACH:
    break;
  case DLL_THREAD_DETACH:
    break;
  }
  return TRUE;
}
