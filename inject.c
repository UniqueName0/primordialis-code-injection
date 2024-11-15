#include "Detours/include/detours.h"
#include "WindowsHModular/windows_loader.h"
#include "debugTools.c"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "user32.lib")

#define Address(value)                                                         \
	((value < 0x140000000) ? (baseAddress + value)                           \
				     : (baseAddress + value - 0x140000000))

uintptr_t baseAddress;

typedef struct {
	char *name;
	char *version;
	char *entrydll;

	void (*init)();
	void (*detourInit)(long long (*DetourAttach)(void **, void *));
} modInfo;

typedef struct {
	int length;
	modInfo **mods;
} modList;

void appendModToModList(modList *modlist, modInfo *mod) {
	modlist->length++;
	modlist->mods =
	    realloc(modlist->mods, sizeof(modInfo *) * modlist->length);
	modlist->mods[modlist->length - 1] = mod;
}

modInfo *init_mod(char *modFolder) {
	char buffer[256];
	char modFolderName[256];

	sprintf(modFolderName, ".\\mods\\%s\\mod-info.txt", modFolder);

	OFSTRUCT of = {0};
	DWORD bytes_read = 0;
	HANDLE modInfoFile = OpenFile(modFolderName, &of, OF_READ);

	ReadFile(modInfoFile, &buffer, sizeof(buffer), &bytes_read, NULL);

	modInfo *mod = malloc(sizeof(mod));

	mod->name = strtok(buffer, "\r\n");
	mod->version = strtok(NULL, "\r\n");
	mod->entrydll = strtok(NULL, "\r\n");

	char EntryDLL[256];
	sprintf(EntryDLL, ".\\mods\\%s\\%s", modFolder, mod->entrydll);
	printf("entry path: %s\n", EntryDLL);
	HINSTANCE hinstLib = LoadLibraryA(EntryDLL);
	if (hinstLib != NULL) {
		mod->init = GetProcAddress(hinstLib, "init");

		if (mod->init == NULL) {
			perror("FUCK"); // will change to something better later
			return NULL; // also checks like this should be added to the
					 // other functions
		}
		mod->init();
		mod->detourInit = GetProcAddress(hinstLib, "detourInit");

		printf("mod name: %s\n", mod->name);
		printf("mod version: %s\n", mod->version);
		printf("entry dll: %s\n", mod->entrydll);

		return mod;
	}
}
modList *modlist;
void init_modloader() {
	modlist = malloc(sizeof(modList));
	modlist->length = 0;
	modlist->mods = malloc(sizeof(modInfo *));

	WIN32_FIND_DATA fileData;
	HANDLE findHandle = INVALID_HANDLE_VALUE;
	char *path = ".\\mods\\*";
	char modFolder[MAX_PATH];
	findHandle = FindFirstFile(path, &modFolder);
	if (findHandle != INVALID_HANDLE_VALUE) {
		while (FindNextFile(findHandle, &fileData) != 0) {
			if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) !=
				  0 &&
			    (fileData.cFileName[0] != '.')) {

				modInfo *mod = init_mod(fileData.cFileName);
				appendModToModList(modlist, mod);
			}
		}
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD nReason, LPVOID lpReserved) {
	baseAddress = (uintptr_t)GetModuleHandleA(NULL);
	PVOID *func_ptr = (PVOID *)malloc(sizeof(PVOID));
	void *TargetFunctionAddress = (void *)Address(0x14002db80);
	*func_ptr = TargetFunctionAddress;
	freopen(".\\logs.txt", "w", stdout);
	printf("nReason = %lu\n", nReason);

	if (nReason == DLL_PROCESS_ATTACH) {

		AllocConsole();
		freopen("CONOUT$", "w", stdout);

		init_modloader();

		DetourRestoreAfterWith();

		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());

		for (size_t i = 0; i < modlist->length; i++) {
			modlist->mods[i]->detourInit(DetourAttach);
		}

		DetourTransactionCommit();
	} else if (nReason == DLL_PROCESS_DETACH) {
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourTransactionCommit();
	}

	return TRUE;
}
