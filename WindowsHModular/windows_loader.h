#pragma once
#define DoesNothing // good for make clangd shut up
#ifndef __unix__
#include <windows.h>
#else
#include "custom.h"
#include "include/win32/windows.h"
#endif
