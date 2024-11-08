#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include "debugTools.c"
#include <detours.h>

#pragma comment(lib, "user32.lib")

#define Address(value) ((baseAddress < 0x140000000) ? (baseAddress + value) : (baseAddress + value - 0x140000000))

uintptr_t baseAddress;



void testInjection()
{

  printf("test injection\n");

  MessageBoxA(NULL, "test injection", "Mod Loader", MB_OK);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD nReason, LPVOID lpReserved)
{
  baseAddress = (uintptr_t)GetModuleHandleA(NULL);
  PVOID* func_ptr = (PVOID*)malloc(sizeof(PVOID));
  void* TargetFunctionAddress = (void*)Address(0x14002db80);
  *func_ptr = TargetFunctionAddress;

  if (DetourIsHelperProcess())
  {
    return TRUE;
  }

  if (nReason == DLL_PROCESS_ATTACH)
  {
    AllocConsole();
    freopen("CONOUT$", "w", stdout);

    DetourRestoreAfterWith();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());

    DetourAttach(func_ptr, &testInjection);

    DetourTransactionCommit();
  }
  else if (nReason == DLL_PROCESS_DETACH)
  {
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourDetach(func_ptr, &testInjection);
    DetourTransactionCommit();
  }

  return TRUE;
}
