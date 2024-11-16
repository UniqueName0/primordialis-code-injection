#include "WindowsHLinux/windows_loader.h"
DoesNothing;
#include "debugTools.c"
#include "modApi.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "user32.lib")

#define Address(value)                                                         \
	((value < 0x140000000) ? (baseAddress + value)                           \
				     : (baseAddress + value - 0x140000000))

size_t baseAddress;

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

void appendModToModList(ModList *modlist, ModInfo *mod) {
	modlist->length++;
	modlist->mods =
	    realloc(modlist->mods, sizeof(ModInfo *) * modlist->length);
	modlist->mods[modlist->length - 1] = mod;
}

ModList *mod_list;
#pragma comment(lib, "user32.lib")

void ParseConfigFile(char *content, char *modID, ModInfo *mod) {
	size_t id_len = strlen(modID);
	mod->id = malloc(id_len);
	memcpy(mod->id, modID, id_len);
	char **fields[3] = {&mod->name, &mod->version, &mod->entrydll};
	printf("Parsing config for modid '%s'\n", modID);
	printf("%s\n", content);
	for (int i = 0; i < 3; ++i) {
		if (!*content) {
			printf(
			    "Config file for modid '%s' malformed, missing values\n",
			    modID);
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

ModInfo *initMod(char *modID, ModApiHandle api) {
	char modInfoBuffer[MAX_PATH] = {'\0'};
	char modInfoPath[MAX_PATH] = {'\0'};

	sprintf(modInfoPath, ".\\mods\\%s\\mod-info.txt", modID);

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
	ParseConfigFile(modInfoBuffer, modID, mod);

	// loads entry dll
	char EntryDLL[MAX_PATH];
	sprintf(EntryDLL, ".\\mods\\%s\\%s", modID, mod->entrydll);
	printf("entry path: %s\n", EntryDLL);
	HINSTANCE hinstLib = LoadLibraryA(EntryDLL);
	if (hinstLib != NULL) {
		mod->init = (void *)GetProcAddress(hinstLib, "init");

		// calls init function if it exists
		if (mod->init != NULL) {
			mod->init(api);
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
			 mod->name, modID, mod->entrydll);
		panic();
	}
}

ModList *mod_list;
ModApi api;

ModList *getEnabledMods() { return mod_list; }

typedef struct SharedResource {
	char *key;
	void *value;
	struct SharedResource
	    *next; // we want a linked list here as we aren't allowed to ever move
		     // the data, and it's also a one time lookup so not so bad
} SharedResource;

SharedResource sharedResources = {};

void *acquireSharedResource(char *key,
				    void (*constructor)(void **resource_pointer)) {
	SharedResource *searching = &sharedResources;
}

void initModLoader() {
	api.getGameState = NULL;
	api.getEnabledMods = getEnabledMods;
	api.acquireSharedResource = acquireSharedResource;
	mod_list->length = 0;
	mod_list->mods = malloc(sizeof(*mod_list->mods));

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
				ModInfo *mod = initMod(fileData.cFileName, &api);
				appendModToModList(mod_list, mod);
			}
		}
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD nReason, LPVOID lpReserved) {

	if (nReason == DLL_PROCESS_ATTACH) {
		baseAddress = (size_t)GetModuleHandleA(NULL);
		AllocConsole();
		freopen("CONOUT$", "w", stdout);

		initModLoader();
	} else if (nReason == DLL_PROCESS_DETACH) {
		// needs something to deinit the modloader
	}

	return TRUE;
}
