#define WIN32_LEAN_AND_MEAN
#define STRICT
#define STRICT_GS_ENABLED
#define _ATL_NO_AUTOMATIC_NAMESPACE
#define _ATL_NO_COM_SUPPORT
#define _CRTDBG_MAP_ALLOC
#include <windows.h>
#include <atlbase.h>
#include <pathcch.h>
#include <shlwapi.h>
#include <winioctl.h>
#include <algorithm>
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
void PrintWindowsError( ULONG error_code = GetLastError() )
{
	if( auto error_msg = GetWindowsError( error_code ) )
	{
		fprintf( stderr, "%ls\n", error_msg.get() );
	}
}
_Success_( return != 0 )
ULONG GetCluserSizeByPath( _In_z_ PCWSTR path )
{
	auto mount_point = std::make_unique<WCHAR[]>( PATHCCH_MAX_CCH );
	if( !GetVolumePathNameW( path, mount_point.get(), PATHCCH_MAX_CCH ) )
	{
		return 0;
	}
	ULONG sectors_per_cluster, sector_size, junk;
	if( !GetDiskFreeSpaceW( mount_point.get(), &sectors_per_cluster, &sector_size, &junk, &junk ) )
	{
		return 0;
	}
	return sectors_per_cluster * sector_size;
}
_Success_( return == true )
bool reflink( _In_z_ PCWSTR oldpath, _In_z_ PCWSTR newpath )
{
	_ASSERTE( oldpath != nullptr && newpath != nullptr );
	HANDLE source = CreateFileW( oldpath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr );
	if( source == INVALID_HANDLE_VALUE )
	{
		return false;
	}
	ATL::CHandle c_source( source );
	ULONG fs_flags;
	if( !GetVolumeInformationByHandleW( source, nullptr, 0, nullptr, nullptr, &fs_flags, nullptr, 0 ) )
	{
		return false;
	}
	if( !( fs_flags & FILE_SUPPORTS_BLOCK_REFCOUNTING ) )
	{
		SetLastError( ERROR_NOT_CAPABLE );
		return false;
	}
	FILE_STANDARD_INFO file_standard;
	if( !GetFileInformationByHandleEx( source, FileStandardInfo, &file_standard, sizeof file_standard ) )
	{
		return false;
	}
	FILE_BASIC_INFO file_basic;
	if( !GetFileInformationByHandleEx( source, FileBasicInfo, &file_basic, sizeof file_basic ) )
	{
		return false;
	}

#ifdef _DEBUG
	HANDLE destination = CreateFileW( newpath, GENERIC_WRITE | DELETE, 0, nullptr, CREATE_ALWAYS, 0, source );
#else
	HANDLE destination = CreateFileW( newpath, GENERIC_WRITE | DELETE, 0, nullptr, CREATE_NEW, 0, source );
#endif
	if( destination == INVALID_HANDLE_VALUE )
	{
		return false;
	}
	ATL::CHandle c_destination( destination );
	FILE_DISPOSITION_INFO dispose = { TRUE };
	if( !SetFileInformationByHandle( destination, FileDispositionInfo, &dispose, sizeof dispose ) )
	{
		return false;
	}

	if( file_basic.FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE )
	{
		ULONG dummy;
		if( !DeviceIoControl( destination, FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &dummy, nullptr ) )
		{
			return false;
		}
	}
	FILE_END_OF_FILE_INFO end_of_file = { file_standard.EndOfFile };
	if( !SetFileInformationByHandle( destination, FileEndOfFileInfo, &end_of_file, sizeof end_of_file ) )
	{
		return false;
	}

	const unsigned split_threshold = 2U * 1024 * 1024 * 1024;
	ULONG cluster_size = GetCluserSizeByPath( oldpath );
	if( !cluster_size )
	{
		return false;
	}
	if( split_threshold % cluster_size != 0 )
	{
		SetLastError( ERROR_NOT_SUPPORTED );
		return false;
	}

	DUPLICATE_EXTENTS_DATA dup_extent = { source };
	for( LONG64 offset = 0, remain = file_standard.AllocationSize.QuadPart; remain > 0; offset += split_threshold, remain -= split_threshold )
	{
		dup_extent.SourceFileOffset.QuadPart = dup_extent.TargetFileOffset.QuadPart = offset;
		dup_extent.ByteCount.QuadPart = std::min<LONG64>( split_threshold, remain );
		_ASSERTE( dup_extent.ByteCount.HighPart == 0 );
		_RPT3( _CRT_WARN, "r=%llx\no=%llx\nb=%llx\n\n", remain, dup_extent.SourceFileOffset.QuadPart, dup_extent.ByteCount.QuadPart );
		ULONG dummy;
		if( !DeviceIoControl( destination, FSCTL_DUPLICATE_EXTENTS_TO_FILE, &dup_extent, sizeof dup_extent, nullptr, 0, &dummy, nullptr ) )
		{
			_CrtDbgBreak();
			return false;
		}
	}

#pragma warning( suppress: 4838 )
	FILETIME atime = { file_basic.LastAccessTime.LowPart, file_basic.LastAccessTime.HighPart };
#pragma warning( suppress: 4838 )
	FILETIME mtime = { file_basic.LastWriteTime.LowPart, file_basic.LastWriteTime.HighPart };
	SetFileTime( destination, nullptr, &atime, &mtime );
	dispose.DeleteFile = FALSE;
	return SetFileInformationByHandle( destination, FileDispositionInfo, &dispose, sizeof dispose ) != 0;
}
int __cdecl wmain( int argc, PWSTR argv[] )
{
	_CrtSetDbgFlag( _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) | _CRTDBG_LEAK_CHECK_DF );
	_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDERR );
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE );
	setlocale( LC_ALL, "" );

	if( argc != 3 )
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
			PathFindFileNameW( argv[0] )
			);
		return EXIT_FAILURE;
	}
	auto source = std::make_unique<WCHAR[]>( PATHCCH_MAX_CCH );
	auto destination = std::make_unique<WCHAR[]>( PATHCCH_MAX_CCH );
	if( FAILED( PathCchCanonicalizeEx( source.get(), PATHCCH_MAX_CCH, argv[1], PATHCCH_ALLOW_LONG_PATHS ) )
		|| FAILED( PathCchCanonicalizeEx( destination.get(), PATHCCH_MAX_CCH, argv[2], PATHCCH_ALLOW_LONG_PATHS ) ) )
	{
		PrintWindowsError();
		return EXIT_FAILURE;
	}
	if( !reflink( source.get(), destination.get() ) )
	{
		PrintWindowsError();
		return EXIT_FAILURE;
	}
}