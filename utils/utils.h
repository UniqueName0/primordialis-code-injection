// mods should also use this for applying patches to the game.
#pragma once
#include <stddef.h>
#include <stdio.h>
typedef unsigned char byte;

#define panic()                                                                \
	mod_logf("panic at %s:%d\n", __FILE__, __LINE__);                        \
	fflush(stdout);                                                          \
	flushLogs();                                                             \
	while (1) {                                                              \
	} // we loop to prevent closing the console so the user can read the
	  // error, having a logging system in future is a better idea
#define assert(cond)                                                           \
	if (!(cond)) {                                                           \
		mod_logf("assert(%s) failed\n", #cond);                            \
		panic();                                                           \
	}
#define len(arr)                                                               \
	(sizeof(arr) /                                                           \
	 sizeof(*arr)) // i've made this mistake twice now, not doing it again

typedef signed short
    PatternByte; // a negative PatternByte represents an ignored value

void *locatePatch(PatternByte *pattern, size_t patternLen, byte *start,
			byte *end);

void applyPatch(PatternByte *replace, size_t len, byte *start);

// effectively *(void **)to = value; but misallignment safe
void memWriteAddr(void *to, void *value, size_t stride);

void *allocExecutable(byte *data, size_t len);

extern FILE *util_log_file;

void setLogs(FILE *log_file);
void flushLogs();
#define mod_logf(format, ...)                                                  \
	printf(format __VA_OPT__(, ) __VA_ARGS__);                               \
	fprintf(util_log_file, format __VA_OPT__(, ) __VA_ARGS__);

typedef char xmm_word[0x10];

typedef struct {
	xmm_word xmm3;
	xmm_word xmm2;
	xmm_word xmm1;
	xmm_word xmm0;
	size_t r11;
	size_t r10;
	size_t r9;
	size_t r8;
	size_t rsi;
	size_t rdi;
	size_t rcx;
	size_t rdx;
	size_t rax;
} State;

void hookAt(void *addr, size_t move_len14, void (*hook)(State *state));
