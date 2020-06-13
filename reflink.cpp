#define WIN32_LEAN_AND_MEAN
#define STRICT_GS_ENABLED
#define _ATL_NO_AUTOMATIC_NAMESPACE
#define _ATL_NO_DEFAULT_LIBS
#define _ATL_NO_WIN_SUPPORT
#define _CRTDBG_MAP_ALLOC
#include <atlbase.h>
#include <windows.h>
#include <winioctl.h>
#include <algorithm>
#include "reflink.h"
#include <crtdbg.h>

constexpr LONG64 inline ROUNDUP(LONG64 file_size, ULONG cluster_size) noexcept
{
	return (file_size + cluster_size - 1) / cluster_size * cluster_size;
}
static_assert(ROUNDUP(5678, 4 * 1024) == 8 * 1024);
_Success_(return == true)
bool reflink(_In_z_ PCWSTR oldpath, _In_z_ PCWSTR newpath)
{
	ATL::CHandle source(CreateFileW(oldpath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr));
	if (source == INVALID_HANDLE_VALUE)
	{
		source.Detach();
		return false;
	}

	ULONG fs_flags;
	if (!GetVolumeInformationByHandleW(source, nullptr, 0, nullptr, nullptr, &fs_flags, nullptr, 0))
	{
		return false;
	}
	if (!(fs_flags & FILE_SUPPORTS_BLOCK_REFCOUNTING))
	{
		SetLastError(ERROR_NOT_CAPABLE);
		return false;
	}

	FILE_END_OF_FILE_INFO file_size;
	if (!GetFileSizeEx(source, &file_size.EndOfFile))
	{
		return false;
	}
	FILE_BASIC_INFO file_basic;
	if (!GetFileInformationByHandleEx(source, FileBasicInfo, &file_basic, sizeof file_basic))
	{
		return false;
	}
	ULONG junk;
	FSCTL_GET_INTEGRITY_INFORMATION_BUFFER get_integrity;
	if (!DeviceIoControl(source, FSCTL_GET_INTEGRITY_INFORMATION, nullptr, 0, &get_integrity, sizeof get_integrity, &junk, nullptr))
	{
		return false;
	}

#ifdef _DEBUG
	SetFileAttributesW(newpath, FILE_ATTRIBUTE_NORMAL);
	ATL::CHandle destination(CreateFileW(newpath, GENERIC_READ | GENERIC_WRITE | DELETE, 0, nullptr, CREATE_ALWAYS, 0, source));
#else
	ATL::CHandle destination(CreateFileW(newpath, GENERIC_READ | GENERIC_WRITE | DELETE, 0, nullptr, CREATE_NEW, 0, source));
#endif
	if (destination == INVALID_HANDLE_VALUE)
	{
		destination.Detach();
		return false;
	}
	FILE_DISPOSITION_INFO dispose = { TRUE };
	if (!SetFileInformationByHandle(destination, FileDispositionInfo, &dispose, sizeof dispose))
	{
		return false;
	}

	if (!DeviceIoControl(destination, FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &junk, nullptr))
	{
		return false;
	}
	FSCTL_SET_INTEGRITY_INFORMATION_BUFFER set_integrity = { get_integrity.ChecksumAlgorithm, get_integrity.Reserved, get_integrity.Flags };
	if (!DeviceIoControl(destination, FSCTL_SET_INTEGRITY_INFORMATION, &set_integrity, sizeof set_integrity, nullptr, 0, nullptr, nullptr))
	{
		return false;
	}
	if (!SetFileInformationByHandle(destination, FileEndOfFileInfo, &file_size, sizeof file_size))
	{
		return false;
	}

	const LONG64 split_threshold = (1LL << 32) - get_integrity.ClusterSizeInBytes;

	DUPLICATE_EXTENTS_DATA dup_extent;
	dup_extent.FileHandle = source;
	for (LONG64 offset = 0, remain = ROUNDUP(file_size.EndOfFile.QuadPart, get_integrity.ClusterSizeInBytes); remain > 0; offset += split_threshold, remain -= split_threshold)
	{
		dup_extent.SourceFileOffset.QuadPart = dup_extent.TargetFileOffset.QuadPart = offset;
		dup_extent.ByteCount.QuadPart = (std::min)(split_threshold, remain);
		_ASSERTE(dup_extent.SourceFileOffset.QuadPart % get_integrity.ClusterSizeInBytes == 0);
		_ASSERTE(dup_extent.ByteCount.QuadPart % get_integrity.ClusterSizeInBytes == 0);
		_ASSERTE(dup_extent.ByteCount.QuadPart <= UINT32_MAX);
		_RPT3(_CRT_WARN, "Remain=%llx\nOffset=%llx\nLength=%llx\n\n", remain, dup_extent.SourceFileOffset.QuadPart, dup_extent.ByteCount.QuadPart);
		if (!DeviceIoControl(destination, FSCTL_DUPLICATE_EXTENTS_TO_FILE, &dup_extent, sizeof dup_extent, nullptr, 0, &junk, nullptr))
		{
			_CrtDbgBreak();
			return false;
		}
	}

	if (!(file_basic.FileAttributes & FILE_ATTRIBUTE_SPARSE_FILE))
	{
		FILE_SET_SPARSE_BUFFER set_sparse = { FALSE };
		if (!DeviceIoControl(destination, FSCTL_SET_SPARSE, &set_sparse, sizeof set_sparse, nullptr, 0, &junk, nullptr))
		{
			return false;
		}
	}

	file_basic.CreationTime.QuadPart = 0;
	if (!SetFileInformationByHandle(destination, FileBasicInfo, &file_basic, sizeof file_basic))
	{
		return false;
	}
	if (!FlushFileBuffers(destination))
	{
		return false;
	}
	dispose = { FALSE };
	return !!SetFileInformationByHandle(destination, FileDispositionInfo, &dispose, sizeof dispose);
}