#include "WindowsHLinux/windows_loader.h"
DoesNothing;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char injectDLL[] = ".\\inject.dll";
unsigned int injectLen = sizeof(injectDLL) + 1;

LPWSTR ConvertToLPWSTR(const char *charArray) {
	LPWSTR wideString = NULL;
	int bufferSize = MultiByteToWideChar(CP_UTF8, 0, charArray, -1, NULL, 0);
	if (bufferSize > 0) {
		wideString = (LPWSTR)malloc(bufferSize * sizeof(WCHAR));
		if (wideString != NULL) {
			MultiByteToWideChar(CP_UTF8, 0, charArray, -1, wideString,
						  bufferSize);
		}
	}
	return wideString;
}

int main() {
	HANDLE ph; // process handle
	HANDLE rt; // remote thread
	LPVOID rb; // remote buffer

	// handle to kernel32 and pass it to GetProcAddress
	HMODULE hKernel32 = GetModuleHandleA("Kernel32");
	VOID *lb = GetProcAddress(hKernel32, "LoadLibraryA");

	// paths
	char *exePath = ".\\primordialis.exe";
	char *parentPath = ".\\";
	system("Z:\\home\\nathan\\Documents\\CE\\Cheat_Engine.exe");

	STARTUPINFOA si = {};
	si.cb = sizeof(si);
	PROCESS_INFORMATION pi = {};
	printf("[*] Target Process: %s\n", exePath);

	// starts game in suspended state
	if (!CreateProcessA(exePath, GetCommandLineA(), NULL, NULL, FALSE,
				  CREATE_SUSPENDED, NULL, NULL, &si, &pi)) {
		printf("Failed to create process. Error code: %lu\n",
			 GetLastError());
		return 1;
	};
	printf("[*] pHandle: %lu\n", pi.dwProcessId);

	// gets the newly created game process
	ph = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pi.dwProcessId);

	// writes inject.dll into game memory
	rb = VirtualAllocEx(ph, NULL, injectLen, (MEM_RESERVE | MEM_COMMIT),
				  PAGE_EXECUTE_READWRITE);
	SIZE_T bytesWritten;
	BOOL success =
	    WriteProcessMemory(ph, rb, injectDLL, injectLen, &bytesWritten);

	printf("[*] WriteProcessMemory: %i\n", success);
	printf("    \\-- bytes written: %zu\n", bytesWritten);

	// start the modloader in new thread
	rt = CreateRemoteThread(ph, NULL, 0, (LPTHREAD_START_ROUTINE)lb, rb, 0,
					NULL);

	// waits untill remote thread is complete
	WaitForSingleObject(rt, INFINITE);

	// starts up game
	ResumeThread(pi.hThread);

	printf("created thread\n");
	CloseHandle(ph);
	return 0;
}
