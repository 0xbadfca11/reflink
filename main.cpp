#define WIL_SUPPRESS_EXCEPTIONS
#include <windows.h>
#include <wil/resource.h>
#include <wil/result.h>
#include <clocale>
#include <cstdio>
#include "reflink.h"
#include <crtdbg.h>

int wmain(int argc, PWSTR argv[])
{
	FAIL_FAST_IF_WIN32_BOOL_FALSE(SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32));
	_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
	setlocale(LC_CTYPE, "");

	if (argc != 3)
	{
		fputs(
			"Copy file without actual data write.\n"
			"\n"
			"reflink <source> <destination>\n"
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
		wil::unique_hlocal_string error_msg;
		FAIL_FAST_IF(!FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), 0, reinterpret_cast<PWSTR>(&error_msg), 0, nullptr));
		fputws(error_msg.get(), stderr);
		return EXIT_FAILURE;
	}
}