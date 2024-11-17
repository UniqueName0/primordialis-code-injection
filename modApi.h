// messy circular type dependency
typedef struct {
	int length;
	struct ModInfo **mods;
} ModList;

typedef struct {
	unsigned char _data[0x530]; // TODO: actually fill in some of the type
					    // here
} GameState;

typedef struct ModApi {
	void *(*acquireSharedResource)(
	    char *key, void (*constructor)(void **resource_pointer));
	ModList *(*getEnabledMods)();
	GameState *(
	    *getGameState)(); // this acquires the "game state" which is the state
				    // that's passed around to most functions, according
				    // to kali providing a concrete is ok
} ModApi, *ModApiHandle;

typedef struct ModInfo {
	char *name;
	char *id;
	char *version;
	char *entrydll;

	void (*init)(ModApiHandle handle);
} ModInfo;
