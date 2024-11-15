#include <stdio.h>
#include <stdint.h>
#include <windows.h>
#include "debugTools.c"

#pragma comment(lib, "user32.lib")


typedef struct {
    char* name;
    char* version;
    char* entrydll;

    void(*init)();
} ModInfo;

typedef struct {
    int length;
    ModInfo** mods;
} ModList;

void appendModToModList(ModList* modlist, ModInfo* mod){
    modlist->length++;
    modlist->mods = realloc(modlist->mods, sizeof(ModInfo*) * modlist->length);
    modlist->mods[modlist->length - 1] = mod;
}

ModInfo* initMod(char* modFolderName) {
    char buffer[MAX_PATH];
    char modFolderPath[MAX_PATH];

    sprintf(modFolderPath, ".\\mods\\%s\\mod-info.txt", modFolderName);

    // opens and reads info file, will be changed to fopen if I can get it working on windows
    OFSTRUCT of = {0};
    DWORD bytes_read = 0;
    HANDLE modInfoFile = (HANDLE)OpenFile(modFolderName, &of, OF_READ); // mingw64 might complain about this if you are building on linux
    ReadFile(modInfoFile, &buffer, sizeof(buffer), &bytes_read, NULL);

    // reads mod info from mod-info.txt file
    ModInfo* mod = malloc(sizeof(ModInfo));
    mod->name = strtok(buffer, "\r\n");
    mod->version = strtok(NULL, "\r\n");
    mod->entrydll = strtok(NULL, "\r\n");

    //loads entry dll
    char EntryDLL[MAX_PATH];
    sprintf(EntryDLL, ".\\mods\\%s\\%s", modFolderName, mod->entrydll);
    printf("entry path: %s\n", EntryDLL);
    HINSTANCE hinstLib = LoadLibraryA(EntryDLL);
    if (hinstLib != NULL)
    {
        mod->init = (void*)GetProcAddress(hinstLib, "init");

        // calls init function if it exists
        if (mod->init != NULL){
            mod->init();
        }

        printf("mod name: %s\n", mod->name);
        printf("mod version: %s\n", mod->version);
        printf("entry dll: %s\n", mod->entrydll);

        return mod;
    }
}

ModList* modlist;
void initModLoader(){
    modlist = malloc(sizeof(ModList));
    modlist->length = 0;
    modlist->mods = malloc(sizeof(ModInfo*));

    WIN32_FIND_DATA fileData;
    HANDLE findHandle = INVALID_HANDLE_VALUE;

    // iterates subfolders in ./mods/ folder
    char* path = ".\\mods\\*";
    char modFolder[MAX_PATH];
    findHandle = FindFirstFile(path, (LPWIN32_FIND_DATAA)&modFolder);
    if (findHandle != INVALID_HANDLE_VALUE)
    {
      while (FindNextFile(findHandle, &fileData) != 0)
        {
            if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0
                && (fileData.cFileName[0] != '.'))
            {
                ModInfo* mod = initMod(fileData.cFileName);
                appendModToModList(modlist, mod);
            }
        }
    }
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD nReason, LPVOID lpReserved)
{

    if (nReason == DLL_PROCESS_ATTACH)
    {
        AllocConsole();
        freopen("CONOUT$", "w", stdout);

        initModLoader();
    }
    else if (nReason == DLL_PROCESS_DETACH)
    {
        // needs something to deinit the modloader
    }

    return TRUE;
}
