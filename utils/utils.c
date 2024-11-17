#include "../WindowsHLinux/windows_loader.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
DoesNothing;

FILE *util_log_file;
void setLogs(FILE *log_file) { util_log_file = log_file; }
void flushLogs() { fflush(util_log_file); }

void *locatePatch(PatternByte *pattern, size_t patternLen, byte *start,
			byte *end) {
	for (; start < end; ++start) {
		size_t i = 0;
		for (; i < patternLen; ++i) {
			if (pattern[i] > 0 && pattern[i] != start[i])
				break;
		}
		if (i == patternLen)
			return start;
	}
	return NULL;
}

void applyPatch(PatternByte *replace, size_t replaceLen, byte *start) {
	for (size_t i = 0; i < replaceLen; ++i) {
		if (replace[i] > 0)
			start[i] = replace[i];
	}
}

void memWrite(void *to, void *value) { memcpy(to, &value, sizeof(value)); }

void *allocExecutable(byte *data, size_t len) {
	void *addr = VirtualAlloc(NULL, len, 0x3000, 0x40);
	memcpy(addr, data, len);
	return addr;
}
