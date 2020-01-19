#define WIN32_LEAN_AND_MEAN
#define STRICT_GS_ENABLED
#define _ATL_NO_AUTOMATIC_NAMESPACE
#define _ATL_NO_DEFAULT_LIBS
#define _ATL_NO_WIN_SUPPORT
#define _CRTDBG_MAP_ALLOC
#include <atlbase.h>
#include <windows.h>
#include <clocale>
#include <cstdio>
#include <memory>
#include "reflink.h"
#include <crtdbg.h>

std::unique_ptr<WCHAR[]> GetWindowsError(ULONG error_code = GetLastError())
{
	auto msg = std::make_unique<WCHAR[]>(USHRT_MAX);
	if (FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error_code, 0, msg.get(), USHRT_MAX, nullptr))
	{
		return msg;
	}
	return nullptr;
}
void PrintWindowsError(ULONG error_code = GetLastError())
{
	if (auto error_msg = GetWindowsError(error_code))
	{
		fprintf(stderr, "%ls\n", error_msg.get());
	}
}
int __cdecl wmain(int argc, PWSTR argv[])
{
	ATLENSURE(SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32));
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	setlocale(LC_ALL, "");

	if (argc != 3)
	{
		fputs(
			"Copy file without actual data write.\n"
			"\n"
			"reflink source destination\n"
			"\n"
			"source       Specifies a file to copy.\n"
			"             source must have placed on the ReFS volume.\n"
			"destination  Specifies new file name.\n"
			"             destination must have placed on the same volume as source.\n",
			stderr
		);
		return EXIT_FAILURE;
	}
	if (!reflink(argv[1], argv[2]))
	{
		PrintWindowsError();
		return EXIT_FAILURE;
	}
}