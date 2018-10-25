#pragma once
#include <winnt.h>
#include <sal.h>

_Success_(return == true)
EXTERN_C
bool reflink(_In_z_ PCWSTR oldpath, _In_z_ PCWSTR newpath);