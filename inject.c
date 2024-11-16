#include "WindowsHLinux/windows_loader.h"
DoesNothing;
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

#define panic()                                                                \
	printf("panic at %s:%d\n", __FILE__, __LINE__);                          \
	fflush(stdout);                                                          \
	while (1) {                                                              \
	} // we loop to prevent closing the console so the user can read the
	  // error, having a logging system in future is a better idea
#define assert(cond)                                                           \
	if (!(cond)) {                                                           \
		printf("assert(%s) failed\n", #cond);                              \
		panic();                                                           \
	}

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

ModList *modlist;
#pragma comment(lib, "user32.lib")

void ParseConfigFile(char *content, char *modname, ModInfo *info) {
	char **fields[3] = {&info->name, &info->version, &info->entrydll};
	printf("Parsing config for modid '%s'\n", modname);
	printf("%s\n", content);
	for (int i = 0; i < 3; ++i) {
		if (!*content) {
			printf(
			    "Config file for modid '%s' malformed, missing values\n",
			    modname);
			panic();
		}
		char *last = content;
		for (; *content != '\r' && *content != '\n' && *content;
		     ++content) {
		}
		size_t len = content - last + 1;
		char *field = malloc(len);
		memcpy(field, last, len);
		field[len - 1] = '\0';
		*fields[i] = field;
		if (*content == '\r')
			++content;
		++content;
	}
}

ModInfo *initMod(char *modFolderName) {
	char modInfoBuffer[MAX_PATH] = {'\0'};
	char modInfoPath[MAX_PATH] = {'\0'};

	sprintf(modInfoPath, ".\\mods\\%s\\mod-info.txt", modFolderName);

	// opens and reads info file, will be changed to fopen if I can get
	// it working on windows
	OFSTRUCT of = {0};
	DWORD bytes_read = 0;
	HANDLE ModInfoFile =
	    (HANDLE)OpenFile(modInfoPath, &of,
				   OF_READ); // mingw64 might complain about this
						 // if you are building on linux
	ReadFile(ModInfoFile, &modInfoBuffer, sizeof(modInfoBuffer), &bytes_read,
		   NULL);

	// reads mod info from mod-info.txt file
	ModInfo *mod = malloc(sizeof(ModInfo));
	ParseConfigFile(modInfoBuffer, modFolderName, mod);

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
		} else {
			printf("Mod '%s' is missing init function\n", mod->name);
			panic()
		}

		printf("Loaded mod '%s' version %s with dll %s\n", mod->name,
			 mod->version, mod->entrydll);
		return mod;
	} else {
		printf("Error while loading mod '%s', no dll found - should be at "
			 "mods\\%s\\%s\n",
			 mod->name, modFolderName, mod->entrydll);
		panic();
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
