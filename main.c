#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <tlhelp32.h>

char injectDLL[] = ".\\inject.dll";
unsigned int injectLen = sizeof(injectDLL) + 1;


LPWSTR ConvertToLPWSTR(const char* charArray) {
    LPWSTR wideString = NULL;
    int bufferSize = MultiByteToWideChar(CP_UTF8, 0, charArray, -1, NULL, 0);
    if (bufferSize > 0) {
        wideString = (LPWSTR)malloc(bufferSize * sizeof(WCHAR));
        if (wideString != NULL) {
            MultiByteToWideChar(CP_UTF8, 0, charArray, -1, wideString, bufferSize);
        }
    }
    return wideString;
}





int main() {
  printf("Primordialis Modloader started\n");
  HANDLE ph; // process handle
  HANDLE rt; // remote thread
  LPVOID rb; // remote buffer

  // handle to kernel32 and pass it to GetProcAddress
  HMODULE hKernel32 = GetModuleHandle("Kernel32");
  VOID *lb = GetProcAddress(hKernel32, "LoadLibraryA");

  // paths
  char* exePath = ".\\primordialis.exe";
  char* parentPath = ".\\";

  STARTUPINFOA si;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi;
  ZeroMemory(&pi, sizeof(pi));
  printf("[*] Target Process: %s\n", exePath);

  if (!CreateProcessA(exePath, GetCommandLineA(), NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi)){
    printf("Failed to create process. Error code: %d\n", GetLastError());
    return 1;
  };
  //printf("Error code: %d\n", GetLastError());
  printf("[*] pHandle: %i\n", pi.dwProcessId);

  ph = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pi.dwProcessId);

  rb = VirtualAllocEx(ph, NULL, injectLen, (MEM_RESERVE | MEM_COMMIT), PAGE_EXECUTE_READWRITE);

  SIZE_T bytesWritten;
  BOOL success = WriteProcessMemory(ph, rb, injectDLL, injectLen, &bytesWritten);
  printf("[*] WriteProcessMemory: %i\n", success);
  printf("    \\-- bytes written: %zu\n", bytesWritten);
  // our process start new thread
  rt = CreateRemoteThread(ph, NULL, 0, (LPTHREAD_START_ROUTINE)lb, rb, 0, NULL);

  //continues game start after running mod loading dll
  ResumeThread(pi.hThread);

  printf("created thread\n");
  CloseHandle(ph);
  while (1) {
    printf("Primordialis Modloader going to sleep");
    Sleep(-1); // infinite sleep
    printf("Primordialis Modloader awoke for some reason");
  } // this makes the console not close
  return 0;
}
