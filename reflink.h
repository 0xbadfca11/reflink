#pragma once
#include <windows.h>
#include <stdbool.h>

EXTERN_C
bool reflink(PCWSTR oldpath, PCWSTR newpath);