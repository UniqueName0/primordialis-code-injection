#include "../WindowsHLinux/windows_loader.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
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
			if (pattern[i] >= 0 && pattern[i] != start[i])
				break;
		}
		if (i == patternLen)
			return start;
	}
	return NULL;
}

void applyPatch(PatternByte *replace, size_t replaceLen, byte *start) {
	for (size_t i = 0; i < replaceLen; ++i) {
		if (replace[i] >= 0)
			start[i] = replace[i];
	}
}

void memWriteAddr(void *to, void *value, size_t stride) {
	for (int i = 0; i < 8; i++) {
		((byte *)to)[stride * i] = ((byte *)&value)[i];
	}
}

void *allocExecutable(byte *data, size_t len) {
	void *addr = VirtualAlloc(NULL, len, 0x3000, 0x40);
	memcpy(addr, data, len);
	return addr;
}

void hookAt(void *addr, size_t move_len14, void (*hook)(State *state)) {
	assert(move_len14 >= 14);
	// clang-format off
	byte replace[] = {
		0x50, // push rax
		0x48, 0xB8, // set rax
		0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01, // trampoline addr
		0xFF, 0xE0, // jmp rax
		0x58, // pop
	};
	// clang-format on
	// clang-format off
	byte trampoline_instructions[] = {
				// push all the state to the stack
				0x50, 0x52, 0x51, 0x57, 0x56, 0x41, 0x50, 0x41, 0x51, 0x41, 0x52, 0x41, 0x53, 0x48, 0x83, 0xEC, 0x10, 0xF3, 0x0F, 0x7F, 0x04, 0x24, 0x48, 0x83, 0xEC, 0x10, 0xF3, 0x0F, 0x7F, 0x0C, 0x24, 0x48, 0x83, 0xEC, 0x10, 0xF3, 0x0F, 0x7F, 0x14, 0x24, 0x48, 0x83, 0xEC, 0x10, 0xF3, 0x0F, 0x7F, 0x1C, 0x24,

				0x48, 0xB8, // set rax
				0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01, // hook addr
				0x48, 0x89, 0xE1, // store rsp in rcx
				0xFF, 0xD0, // call rax
				
				// pop all state from stack
				0xF3, 0x0F, 0x6F, 0x1C, 0x24, 0x48, 0x83, 0xC4, 0x10, 0xF3, 0x0F, 0x6F, 0x14, 0x24, 0x48, 0x83, 0xC4, 0x10, 0xF3, 0x0F, 0x6F, 0x0C, 0x24, 0x48, 0x83, 0xC4, 0x10, 0xF3, 0x0F, 0x6F, 0x04, 0x24, 0x48, 0x83, 0xC4, 0x10, 0x41, 0x5B, 0x41, 0x5A, 0x41, 0x59, 0x41, 0x58, 0x5E, 0x5F, 0x59, 0x5A,
				
				0x48, 0xB8, // set rax
				0xEF, 0xCD, 0xAB, 0x89, 0x67, 0x45, 0x23, 0x01, // ret addr
				0xFF, 0xE0, // jump to rax
	};
	// clang-format on
	memWriteAddr(trampoline_instructions + 51, hook, 1);
	memWriteAddr(trampoline_instructions + 114, addr + 13,
			 1); // need to call the pop rax
	void *cpy_src = malloc(len(trampoline_instructions) +
				     move_len14); // slow but idc, too lazy to be proper
	memcpy(cpy_src, addr, move_len14);
	memcpy(cpy_src + move_len14, trampoline_instructions,
		 len(trampoline_instructions));
	void *trampoline =
	    allocExecutable(cpy_src, move_len14 + len(trampoline_instructions));
	free(cpy_src);
	memWriteAddr(replace + 3, trampoline, 1);
	memset(addr, 0x90, move_len14);
	memcpy(addr, replace, 14);
}
