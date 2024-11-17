// mods should also use this for applying patches to the game.

#include <stddef.h>
#include <stdio.h>
typedef unsigned char byte;

typedef signed short
    PatternByte; // a negative PatternByte represents an ignored value

void *locatePatch(PatternByte *pattern, size_t patternLen, byte *start,
			byte *end);

void applyPatch(PatternByte *replace, size_t len, byte *start);

// effectively *(void **)to = value; but misallignment safe
void memWrite(void *to, void *value);

void *allocExecutable(byte *data, size_t len);

extern FILE *util_log_file;

void setLogs(FILE *log_file);
void flushLogs();
#define mod_logf(format, ...)                                                  \
	printf(format __VA_OPT__(, ) __VA_ARGS__);                               \
	fprintf(util_log_file, format __VA_OPT__(, ) __VA_ARGS__);
