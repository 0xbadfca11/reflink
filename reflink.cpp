#define WIN32_LEAN_AND_MEAN
#define STRICT
#define STRICT_GS_ENABLED
#define _CRTDBG_MAP_ALLOC
#include <windows.h>
#include <atlbase.h>
#include <pathcch.h>
#include <shlwapi.h>
#include <winioctl.h>
#include <clocale>
#include <cstdio>
#include <memory>
#include <crtdbg.h>
#include "banned.h"
#pragma comment(lib, "pathcch")
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "user32")

std::unique_ptr<WCHAR[]> GetWindowsError( ULONG error_code = GetLastError() )
{
	auto msg = std::make_unique<WCHAR[]>( USHRT_MAX );
	if( FormatMessageW( FORMAT_MESSAGE_FROM_SYSTEM, nullptr, error_code, 0, msg.get(), USHRT_MAX, nullptr ) )
	{
		return msg;
	}
	return nullptr;
}
void PrintfWindowsError( ULONG error_code = GetLastError() )
{
	if( auto error_msg = GetWindowsError( error_code ) )
	{
		fprintf( stderr, "%ls\n", error_msg.get() );
	}
}
void PrintUsage()
{
	fprintf(
		stderr,
		"Copy file without actual data write.\n"
		"\n"
		"%ls source destination\n"
		"\n"
		"source       Specifies a file to copy.\n"
		"             source must have placed on the ReFS volume.\n"
		"destination  Specifies new file name.\n"
		"             destination must have placed on the same volume as source.\n",
		PathFindFileNameW( __wargv[0] )
		);
}
std::unique_ptr<bool> CheckReFSVersion( PCWSTR on_volume_path )
{
	auto mount_point = std::make_unique<WCHAR[]>( PATHCCH_MAX_CCH );
	if( !GetVolumePathNameW( on_volume_path, mount_point.get(), PATHCCH_MAX_CCH ) )
	{
		PrintfWindowsError();
		return nullptr;
	}
	WCHAR guid_path[50];
	if( !GetVolumeNameForVolumeMountPointW( mount_point.get(), guid_path, ARRAYSIZE( guid_path ) ) )
	{
		PrintfWindowsError();
		return nullptr;
	}
	PWCHAR end_bslash;
	if( FAILED( PathCchRemoveBackslashEx( guid_path, ARRAYSIZE( guid_path ), &end_bslash, nullptr ) ) )
	{
		PrintfWindowsError();
		return nullptr;
	}
	*end_bslash = L'\0';
	HANDLE volume = CreateFileW( guid_path, FILE_EXECUTE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr );
	if( volume == INVALID_HANDLE_VALUE )
	{
		PrintfWindowsError();
		return nullptr;
	}
	ATL::CHandle c_volume( volume );
	ULONG dummy;
	REFS_VOLUME_DATA_BUFFER refs_info;
	if( DeviceIoControl( volume, FSCTL_GET_REFS_VOLUME_DATA, nullptr, 0, &refs_info, sizeof refs_info, &dummy, nullptr ) )
	{
		fprintf( stderr, "%ls is ReFS %lu.%lu\n", mount_point.get(), refs_info.MajorVersion, refs_info.MinorVersion );
		if( refs_info.MajorVersion < 1 || ( refs_info.MajorVersion == 1 && refs_info.MinorVersion <= 2 ) )
		{
			return std::make_unique<bool>( false );
		}
		else
		{
			return std::make_unique<bool>( true );
		}
	}
	WCHAR fs_name_buffer[MAX_PATH + 1];
	if( GetVolumeInformationByHandleW( volume, nullptr, 0, nullptr, nullptr, nullptr, fs_name_buffer, ARRAYSIZE( fs_name_buffer ) ) )
	{
		fprintf( stderr, "%ls is %ls\n", mount_point.get(), fs_name_buffer );
	}
	return nullptr;
}
BOOL reflink( _In_z_ PCWSTR oldpath, _In_z_ PCWSTR newpath )
{
	HANDLE source = CreateFileW( oldpath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr );
	if( source == INVALID_HANDLE_VALUE )
	{
		return FALSE;
	}
	ATL::CHandle c_source( source );
	FILE_STANDARD_INFO file_standard;
	if( !GetFileInformationByHandleEx( source, FileStandardInfo, &file_standard, sizeof file_standard ) )
	{
		return FALSE;
	}
	FILE_BASIC_INFO file_basic;
	if( !GetFileInformationByHandleEx( source, FileBasicInfo, &file_basic, sizeof file_basic ) )
	{
		return FALSE;
	}

	HANDLE destination = CreateFileW( newpath, GENERIC_WRITE | DELETE, 0, nullptr, CREATE_NEW, 0, source );
	if( destination == INVALID_HANDLE_VALUE )
	{
		return FALSE;
	}
	ATL::CHandle c_destination( destination );

	BOOL success = true;
	ULONG dummy;

	if( file_basic.FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE )
	{
		success = DeviceIoControl( destination, FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &dummy, nullptr );
	}
	if( success )
	{
		FILE_END_OF_FILE_INFO end_of_file = { file_standard.EndOfFile };
		success = SetFileInformationByHandle( destination, FileEndOfFileInfo, &end_of_file, sizeof end_of_file );
	}
	if( success )
	{
		DUPLICATE_EXTENTS_DATA dup_extent = { source, { 0 }, { 0 }, file_standard.AllocationSize };
		success = DeviceIoControl( destination, FSCTL_DUPLICATE_EXTENTS_TO_FILE, &dup_extent, sizeof dup_extent, nullptr, 0, &dummy, nullptr );
	}
	if( success )
	{
#pragma warning( suppress: 4838 )
		FILETIME atime = { file_basic.LastAccessTime.LowPart, file_basic.LastAccessTime.HighPart };
#pragma warning( suppress: 4838 )
		FILETIME mtime = { file_basic.LastWriteTime.LowPart, file_basic.LastWriteTime.HighPart };
		SetFileTime( destination, nullptr, &atime, &mtime );
	}
	if( !success )
	{
		FILE_DISPOSITION_INFO dispose = { TRUE };
		SetFileInformationByHandle( destination, FileDispositionInfo, &dispose, sizeof dispose );
	}
	return success;
}
int __cdecl wmain( int argc, PWSTR argv[] )
{
	_CrtSetDbgFlag( _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) | _CRTDBG_LEAK_CHECK_DF );
	_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDERR );
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE );
	setlocale( LC_ALL, "" );

	if( argc != 3 )
	{
		PrintUsage();
		return EXIT_FAILURE;
	}
	auto source = std::make_unique<WCHAR[]>( PATHCCH_MAX_CCH );
	auto destination = std::make_unique<WCHAR[]>( PATHCCH_MAX_CCH );
	if( FAILED( PathCchCanonicalizeEx( source.get(), PATHCCH_MAX_CCH, argv[1], PATHCCH_ALLOW_LONG_PATHS ) )
		|| FAILED( PathCchCanonicalizeEx( destination.get(), PATHCCH_MAX_CCH, argv[2], PATHCCH_ALLOW_LONG_PATHS ) ) )
	{
		PrintfWindowsError();
		return EXIT_FAILURE;
	}
	auto refs_capability = CheckReFSVersion( source.get() );
	if( !refs_capability || !*refs_capability )
	{
		fputs( "reflink may fail.\n", stderr );
	}
	if( !reflink( source.get(), destination.get() ) )
	{
		PrintfWindowsError();
		return EXIT_FAILURE;
	}
}