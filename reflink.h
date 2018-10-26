#pragma once
#include <sal.h>
#include <stdbool.h>

#ifndef _INC_WINDOWS
#ifdef __cplusplus
extern "C"
#endif
_Success_(return == true)
bool reflink(_In_z_ const wchar_t oldpath[], _In_z_ const wchar_t newpath[]);
#else
EXTERN_C
_Success_(return == true)
bool reflink(_In_z_ PCWSTR oldpath, _In_z_ PCWSTR newpath);
#endif