#include "WindowsHModular/custom.h"
#include "debugTools.c"
// clang-format off
#include "WindowsHModular/windows_loader.h"
#include "Detours/include/detours.h"
// clang-format on
// detours is a bit crap and doesn't include windows.h itself but it needs it
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
} ModInfo;

typedef struct {
	int length;
	ModInfo **mods;
} ModList;

void appendModToModList(ModList *modlist, ModInfo *mod) {
	modlist->length++;
	modlist->mods =
	    realloc(modlist->mods, sizeof(ModInfo *) * modlist->length);
	modlist->mods[modlist->length - 1] = mod;
}

ModInfo *init_mod(char *modFolder) {
	char buffer[256];
	char modFolderName[256];

	sprintf(modFolderName, ".\\mods\\%s\\mod-info.txt", modFolder);

	OFSTRUCT of = {0};
	DWORD bytes_read = 0;
	HANDLE ModInfoFile = (HANDLE)OpenFile(modFolderName, &of, OF_READ);

	ReadFile(ModInfoFile, &buffer, sizeof(buffer), &bytes_read, NULL);

	ModInfo *mod = malloc(sizeof(*mod));

	mod->name = strtok(buffer, "\r\n");
	mod->version = strtok(NULL, "\r\n");
	mod->entrydll = strtok(NULL, "\r\n");

	char EntryDLL[256];
	sprintf(EntryDLL, ".\\mods\\%s\\%s", modFolder, mod->entrydll);
	printf("entry path: %s\n", EntryDLL);
	HINSTANCE hinstLib = LoadLibraryA(EntryDLL);
	if (hinstLib != NULL) {
		mod->init = (void (*)())GetProcAddress(hinstLib, "init");

		if (mod->init == NULL) {
			perror("FUCK"); // will change to something better later
			return NULL; // also checks like this should be added to the
					 // other functions
		}
		mod->init();

		printf("mod name: %s\n", mod->name);
		printf("mod version: %s\n", mod->version);
		printf("entry dll: %s\n", mod->entrydll);

		return mod;
	}
}
ModList *modlist;
void init_modloader() {
	modlist = malloc(sizeof(ModList));
	modlist->length = 0;
	modlist->mods = malloc(sizeof(ModInfo *));

	WIN32_FIND_DATA fileData;
	HANDLE findHandle = INVALID_HANDLE_VALUE;
	char *path = ".\\mods\\*";
	WIN32_FIND_DATA modFolder;
	findHandle = FindFirstFile(path, &modFolder);
	if (findHandle != INVALID_HANDLE_VALUE) {
		while (FindNextFile(findHandle, &fileData) != 0) {
			if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) !=
				  0 &&
			    (fileData.cFileName[0] != '.')) {

				ModInfo *mod = init_mod(fileData.cFileName);
				appendModToModList(modlist, mod);
			}
		}
	}
}

#pragma comment(lib, "user32.lib")

ModInfo *initMod(char *modFolderName) {
	char buffer[MAX_PATH];
	char modFolderPath[MAX_PATH];

	sprintf(modFolderPath, ".\\mods\\%s\\mod-info.txt", modFolderName);

	// opens and reads info file, will be changed to fopen if I can get
	// it working on windows
	OFSTRUCT of = {0};
	DWORD bytes_read = 0;
	HANDLE ModInfoFile =
	    (HANDLE)OpenFile(modFolderName, &of,
				   OF_READ); // mingw64 might complain about this
						 // if you are building on linux
	ReadFile(ModInfoFile, &buffer, sizeof(buffer), &bytes_read, NULL);

	// reads mod info from mod-info.txt file
	ModInfo *mod = malloc(sizeof(ModInfo));
	mod->name = strtok(buffer, "\r\n");
	mod->version = strtok(NULL, "\r\n");
	mod->entrydll = strtok(NULL, "\r\n");

	// loads entry dll
	char EntryDLL[MAX_PATH];
	sprintf(EntryDLL, ".\\mods\\%s\\%s", modFolderName, mod->entrydll);
	printf("entry path: %s\n", EntryDLL);
	HINSTANCE hinstLib = LoadLibraryA(EntryDLL);
	if (hinstLib != NULL) {
		mod->init = (void *)GetProcAddress(hinstLib, "init");

		// calls init function if it exists
		if (mod->init != NULL) {
			mod->init();
		}

		printf("mod name: %s\n", mod->name);
		printf("mod version: %s\n", mod->version);
		printf("entry dll: %s\n", mod->entrydll);

		return mod;
	}
}

ModList *modlist;
void initModLoader() {
	modlist = malloc(sizeof(ModList));
	modlist->length = 0;
	modlist->mods = malloc(sizeof(ModInfo *));

	WIN32_FIND_DATA fileData;
	HANDLE findHandle = INVALID_HANDLE_VALUE;

	// iterates subfolders in ./mods/ folder
	char *path = ".\\mods\\*";
	char modFolder[MAX_PATH];
	findHandle = FindFirstFile(path, (LPWIN32_FIND_DATAA)&modFolder);
	if (findHandle != INVALID_HANDLE_VALUE) {
		while (FindNextFile(findHandle, &fileData) != 0) {
			if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) !=
				  0 &&
			    (fileData.cFileName[0] != '.')) {
				ModInfo *mod = initMod(fileData.cFileName);
				appendModToModList(modlist, mod);
			}
		}
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD nReason, LPVOID lpReserved) {

	if (nReason == DLL_PROCESS_ATTACH) {
		AllocConsole();
		freopen("CONOUT$", "w", stdout);

		initModLoader();
	} else if (nReason == DLL_PROCESS_DETACH) {
		// needs something to deinit the modloader
	}

	return TRUE;
}
