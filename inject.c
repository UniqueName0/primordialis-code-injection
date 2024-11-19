#include "WindowsHLinux/windows_loader.h"
DoesNothing;
#include "debugTools.c"
#include "modApi.h"
#include "utils/utils.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *const func_start = (void *)0x140001005;

#pragma comment(lib, "user32.lib")

size_t baseAddress;
static inline void *Address(void *value) {
	return baseAddress + value - 0x140000000;
}

void appendModToModList(ModList *modlist, ModInfo *mod) {
	modlist->length++;
	modlist->mods =
	    realloc(modlist->mods, sizeof(ModInfo *) * modlist->length);
	modlist->mods[modlist->length - 1] = mod;
}

ModList mod_list;
#pragma comment(lib, "user32.lib")

void parseConfigFile(char *content, char *modID, ModInfo *mod) {
	size_t id_len = strlen(modID);
	mod->id = malloc(id_len);
	memcpy(mod->id, modID, id_len);
	char **fields[3] = {&mod->name, &mod->version, &mod->entrydll};
	mod_logf("Parsing config for modid '%s'\n", modID);
	mod_logf("Config file content:");
	mod_logf("%s", content);
	for (int i = 0; i < 3; ++i) {
		if (!*content) {
			mod_logf(
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
	parseConfigFile(modInfoBuffer, modID, mod);

	// loads entry dll
	char EntryDLL[MAX_PATH];
	sprintf(EntryDLL, ".\\mods\\%s\\%s", modID, mod->entrydll);
	mod_logf("Loading mod '%s' through dll %s\n", mod->name, EntryDLL);
	HINSTANCE hinstLib = LoadLibraryA(EntryDLL);
	if (hinstLib != NULL) {
		mod->init = (void *)GetProcAddress(hinstLib, "init");

		// calls init function if it exists
		if (mod->init != NULL) {
			mod->init(api);
		} else {
			mod_logf("Mod '%s' is missing init function\n", mod->name);
			panic()
		}

		mod_logf("Loaded mod '%s' version %s with dll %s\n", mod->name,
			   mod->version, mod->entrydll);
		return mod;
	} else {
		mod_logf(
		    "Error while loading mod '%s', no dll found - should be at "
		    "mods\\%s\\%s\n",
		    mod->name, modID, mod->entrydll);
		panic();
	}
}

ModApi api;

ModList *getEnabledMods() { return &mod_list; }

typedef struct SharedResource {
	char *key;
	void *value;
	struct SharedResource
	    *next; // we want a linked list here as we aren't allowed to ever move
		     // the data, and it's also a one time lookup so not so bad
} SharedResource;

SharedResource shared_resources = {};

void *acquireSharedResource(char *key,
				    void (*constructor)(void **resource_pointer)) {
	SharedResource *searching = &shared_resources;
	while (searching != NULL) {
		if (strcmp(key, searching->key) == 0) {
			return searching->value;
		}
		searching = searching->next;
	}
	SharedResource *new = calloc(1, sizeof(*searching->next));
	searching->next = new;
	size_t key_len = strlen(key);
	new->key = malloc(key_len);
	memcpy(new->key, key, key_len);
	constructor(&new->value);
	return new;
}

// i'm way too lazy to write a hashmap, and a linked list is asymptotically
// equivalent to a vector
typedef struct IdRegistry {
	char *key;
	struct IdRegistry *next;
} IdRegistry;

IdRegistry registry = {};

unsigned registerID(char *key) {
	unsigned id = 0;
	IdRegistry *search = &registry;
#define RET_ID                                                                 \
	assert((id & (0xFF << 0x18)) < 0x20);                                    \
	return id
	while (search->next) {
		if (strcmp(search->key, key) == 0)
			RET_ID;
		++id;
		search = search->next;
	}
	if (strcmp(search->key, key) == 0)
		RET_ID;
	search->next = malloc(sizeof(*search->next));
	search->next->next = NULL;
	size_t key_len = strlen(key);
	search->next->key = malloc(key_len);
	memcpy(search->next->key, key, key_len);
	RET_ID;
#undef RET_ID
}

GameState *game_state;

void halt() {
	while (1) {
	}
}

void applyGameStatePatch() {
	PatternByte patch[] = {0x48, 0x89, 0xbe, 0x70, 0x03, 0x00, 0x00, 0x48,
				     0xb9, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
				     0x00, 0x48, 0x89, 0x8e, 0x78, 0x03, 0x00, 0x00,
				     0x48, 0x89, 0x86, 0x80, 0x03, 0x00, 0x00, 0xc7,
				     0x86, 0x88, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
				     0x00, 0xb9, 0x00, 0x00, 0x20, 0x00};
	/*
	```asm
	mov rcx, &new_addr
	jmp rcx
	```
	*/
	PatternByte replace[sizeof(patch)] = {
	    0x48, 0xB9, 0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01, 0xFF,
	    0xE1, 0x90, 0x90, 0x90, 0x90, 0x90, -1,   -1,   -1,   -1,   -1,
	    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	    -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
	    -1,   -1,   -1,   -1};
	void *patch_addr = locatePatch(
	    patch, len(patch), Address(func_start),
	    Address(func_start + 0x1000000)); // TODO: parse the PE headers to get
							  // the size of the executable region
	assert(patch_addr != NULL);
	mod_logf("Located GameState core patch address at 0x%p\n", patch_addr);

	// we want to capture the GameState for sharing with the api
	// rdi is safe to clobber, it gets overwritten later
	/*
	```asm
	mov rdi, &game_state
	mov [rdi], rsi
	mov rdi, &ret_addr
	jmp rdi
	```
	*/
	// clang-format off
	byte new_executable_instructions[] = {
	 	0x48, 0x89, 0xbe, 0x70, 0x03, 0x00, 0x00, 0x48, 0xb9, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x48, 0x89, 0x8e, 0x78, 0x03, 0x00, 0x00, 0x48, 0x89, 0x86, 0x80, 0x03, 0x00, 0x00, // previous stuff
		0x48, 0xBF,
		0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01, // &game_state
		0x48, 0x89, 0x37, 0x48, 0xBF,
		0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01, // &ret_addr = start_addr + 0x1F
		0xFF, 0xE7};
	// clang-format on
	memWriteAddr(new_executable_instructions + 33, &game_state,
			 1); // the memory isn't alligned so technically i think
			     // that byte casting is UB, but realistically a
			     // modern CPU should be able to perform an unalligned
			     // write so maybe do that later to make it nicer
	memWriteAddr(new_executable_instructions + 46, patch_addr + 0x1F, 1);
	void *new_executable = allocExecutable(
	    new_executable_instructions, sizeof(new_executable_instructions));
	mod_logf("GameState will be captured in 0x%p\n", &game_state);
	mod_logf("Alllocated new code for core GameState patch at 0x%p\n",
		   new_executable);
	hexDump("With new code content", new_executable,
		  len(new_executable_instructions));
	memWriteAddr(replace + 2, new_executable,
			 2); // patterns are not byte, they are word so we need stride
			     // 1 to not corrupt our data.
	applyPatch(replace, len(replace), patch_addr);
	hexDump("Changed core GameState patch memory to", patch_addr,
		  len(replace));
}

GameState *getGameState() { return game_state; }

void initModLoader() {
	setLogs(fopen(".\\logs\\PrimordialisModloader.log", "w"));
	mod_logf("Primordialis Modloader injected dll initialisation starting\n");

	DWORD oldProtect;
	unsigned text_size = *(unsigned *)Address(
	    (void *)0x140000218); // this is the size of the .text section,
					  // hopefully the address doesn't wander around

	mod_logf("Parsing PE header resolved to %x executable bytes\n",
		   text_size);
	VirtualProtect((void *)baseAddress, text_size, 0x40, &oldProtect);
	mod_logf("Successfully disabled memory protection\n");

	applyGameStatePatch();
	mod_logf("Successfully applied GameState core patch\n");

	api.getGameState = getGameState;
	api.getEnabledMods = getEnabledMods;
	api.acquireSharedResource = acquireSharedResource;
	mod_list.length = 0;
	mod_list.mods = malloc(sizeof(*mod_list.mods));

	WIN32_FIND_DATA fileData;
	HANDLE findHandle = INVALID_HANDLE_VALUE;

	// iterates subfolders in ./mods/ folder
	char *path = ".\\mods\\*";
	WIN32_FIND_DATA modFolder;
	mod_logf("Started loading mods\n");
	findHandle = FindFirstFile(path, &modFolder);
	if (findHandle != INVALID_HANDLE_VALUE) {
		while (FindNextFile(findHandle, &fileData) != 0) {
			mod_logf("Found file in mods folder '%s'\n",
				   fileData.cFileName);
			if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) !=
				  0 &&
			    (fileData.cFileName[0] != '.')) {
				ModInfo *mod = initMod(fileData.cFileName, &api);
				appendModToModList(&mod_list, mod);
			}
		}
	}
	mod_logf(
	    "Primordialis Modloader injected dll initialisation completed\n");
	flushLogs();
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
