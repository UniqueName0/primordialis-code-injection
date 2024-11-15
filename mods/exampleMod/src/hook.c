
#include <windows.h>
#include <stdint.h>

#define Address(value) ((value < 0x140000000) ? (baseAddress + value) : (baseAddress + value - 0x140000000))
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
    uint8_t newBytes[] = {0x50, 0x48, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x87,
    0x04, 0x24, 0xC3 };
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
    VirtualProtect(hook.targetFunction, sizeof(hook.newBytes), PAGE_EXECUTE_READWRITE,&oldProtect);
    memcpy(hook.targetFunction, hook.newBytes, sizeof(hook.newBytes));
    VirtualProtect(hook.targetFunction, sizeof(hook.newBytes), oldProtect, NULL);
}

void ReturnToOrigFunction(HookData hook) {
    __asm__("pushq %rax\n");
    register void* rax_register asm("%rax");
    rax_register = hook.targetFunction;


    __asm__("xchgq (%rsp), %rax\n"
        "popq %rax\n");
}

void DetachHook(HookData hook) {
    DWORD oldProtect;
    VirtualProtect(hook.targetFunction, sizeof(hook.originalBytes), PAGE_EXECUTE_READWRITE,
    &oldProtect);
    memcpy(hook.targetFunction, hook.originalBytes, sizeof(hook.originalBytes));
    VirtualProtect(hook.targetFunction, sizeof(hook.originalBytes), oldProtect, NULL);
}
