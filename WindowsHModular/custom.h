#pragma once
#include "windows_loader.h"
#ifdef __unix__
#include "include/win32/windows.h"
typedef struct _OFSTRUCT {
	BYTE cBytes;
	BYTE fFixedDisk;
	WORD nErrCode;
	WORD Reserved1;
	WORD Reserved2;
	CHAR szPathName[128];
} OFSTRUCT, *LPOFSTRUCT, *POFSTRUCT;
typedef void *HFILE;
typedef struct _WIN32_FIND_DATA {
	DWORD dwFileAttributes;
	FILETIME ftCreationTime;
	FILETIME ftLastAccessTime;
	FILETIME ftLastWriteTime;
	DWORD nFileSizeHigh;
	DWORD nFileSizeLow;
	DWORD dwOID;
	TCHAR cFileName[MAX_PATH];
} WIN32_FIND_DATA, *LPWIN32_FIND_DATA;
#define OF_READ 0
BOOL FindNextFile(HANDLE hFindFile, LPWIN32_FIND_DATA lpFindFileData);
HANDLE FindFirstFile(LPCTSTR lpFileName, LPWIN32_FIND_DATA lpFindFileData);
HFILE OpenFile(LPCSTR lpFileName, LPOFSTRUCT lpReOpenBuff, UINT uStyle);
HANDLE CreateRemoteThread(HANDLE hProcess,
				  LPSECURITY_ATTRIBUTES lpThreadAttributes,
				  SIZE_T dwStackSize,
				  LPTHREAD_START_ROUTINE lpStartAddress,
				  LPVOID lpParameter, DWORD dwCreationFlags,
				  LPDWORD lpThreadId);
#endif
