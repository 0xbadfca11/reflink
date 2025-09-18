#define WIL_SUPPRESS_EXCEPTIONS
#include <windows.h>
#include <wil/common.h>
#include <wil/filesystem.h>
#include <wil/resource.h>
#include <algorithm>
#include "reflink.h"
#include <crtdbg.h>

template <typename Ty1, typename Ty2>
[[nodiscard]]
constexpr auto round_up(Ty1 value, Ty2 base_multiple)
{
	return (value + base_multiple - 1) / base_multiple * base_multiple;
}
static_assert(round_up(1024 * 8 + 1, 1024) == 1024 * 9);

bool reflink(PCWSTR oldpath, PCWSTR newpath)
{
	const wil::unique_hfile source(CreateFileW(oldpath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr));
	if (!source)
	{
		return false;
	}

	ULONG fs_flags;
	if (!GetVolumeInformationByHandleW(source.get(), nullptr, 0, nullptr, nullptr, &fs_flags, nullptr, 0))
	{
		return false;
	}
	if (WI_IsFlagClear(fs_flags, FILE_SUPPORTS_BLOCK_REFCOUNTING))
	{
		SetLastError(ERROR_NOT_CAPABLE);
		return false;
	}

	FILE_END_OF_FILE_INFO file_size;
	if (!GetFileSizeEx(source.get(), &file_size.EndOfFile))
	{
		return false;
	}
	FILE_BASIC_INFO file_basic;
	if (!GetFileInformationByHandleEx(source.get(), FileBasicInfo, &file_basic, sizeof file_basic))
	{
		return false;
	}
	ULONG _;
	FSCTL_GET_INTEGRITY_INFORMATION_BUFFER get_integrity;
	if (!DeviceIoControl(source.get(), FSCTL_GET_INTEGRITY_INFORMATION, nullptr, 0, &get_integrity, sizeof get_integrity, &_, nullptr))
	{
		return false;
	}

#if _DEBUG
	const wil::unique_hfile destination(CreateFileW(newpath, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_FLAG_DELETE_ON_CLOSE, source.get()));
#elif NTDDI_VERSION < NTDDI_WIN10_RS3
	const wil::unique_hfile destination(CreateFileW(newpath, GENERIC_READ | GENERIC_WRITE | DELETE, 0, nullptr, CREATE_NEW, 0, source.get()));
#else
	const wil::unique_hfile destination(CreateFileW(newpath, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_FLAG_DELETE_ON_CLOSE, source.get()));
#endif
	if (!destination)
	{
		return false;
	}
#if !_DEBUG && NTDDI_VERSION < NTDDI_WIN10_RS3
	FILE_DISPOSITION_INFO dispose = { TRUE };
	if (!SetFileInformationByHandle(destination.get(), FileDispositionInfo, &dispose, sizeof dispose))
	{
		return false;
	}
#endif

	if (!DeviceIoControl(destination.get(), FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &_, nullptr))
	{
		return false;
	}
	FSCTL_SET_INTEGRITY_INFORMATION_BUFFER set_integrity = { get_integrity.ChecksumAlgorithm, get_integrity.Reserved, get_integrity.Flags };
	if (!DeviceIoControl(destination.get(), FSCTL_SET_INTEGRITY_INFORMATION, &set_integrity, sizeof set_integrity, nullptr, 0, nullptr, nullptr))
	{
		return false;
	}
	if (!SetFileInformationByHandle(destination.get(), FileEndOfFileInfo, &file_size, sizeof file_size))
	{
		return false;
	}

	const LONG64 split_threshold = (1LL << 32) - get_integrity.ClusterSizeInBytes;

	DUPLICATE_EXTENTS_DATA dup_extent;
	dup_extent.FileHandle = source.get();
	for (LONG64 offset = 0, remain = round_up(file_size.EndOfFile.QuadPart, get_integrity.ClusterSizeInBytes); remain > 0; offset += split_threshold, remain -= split_threshold)
	{
		dup_extent.SourceFileOffset.QuadPart = dup_extent.TargetFileOffset.QuadPart = offset;
		dup_extent.ByteCount.QuadPart = (std::min)(split_threshold, remain);
		if (!DeviceIoControl(destination.get(), FSCTL_DUPLICATE_EXTENTS_TO_FILE, &dup_extent, sizeof dup_extent, nullptr, 0, &_, nullptr))
		{
			_CrtDbgBreak();
			return false;
		}
	}

	if (WI_IsFlagClear(file_basic.FileAttributes, FILE_ATTRIBUTE_SPARSE_FILE))
	{
		FILE_SET_SPARSE_BUFFER set_sparse = { FALSE };
		if (!DeviceIoControl(destination.get(), FSCTL_SET_SPARSE, &set_sparse, sizeof set_sparse, nullptr, 0, &_, nullptr))
		{
			return false;
		}
	}

	file_basic.CreationTime.QuadPart = 0;
	if (!SetFileInformationByHandle(destination.get(), FileBasicInfo, &file_basic, sizeof file_basic))
	{
		return false;
	}
	if (!FlushFileBuffers(destination.get()))
	{
		return false;
	}
#if !_DEBUG && NTDDI_VERSION < NTDDI_WIN10_RS3
	dispose = { FALSE };
	return SetFileInformationByHandle(destination.get(), FileDispositionInfo, &dispose, sizeof dispose);
#else
	FILE_DISPOSITION_INFO_EX fdie = { FILE_DISPOSITION_FLAG_DO_NOT_DELETE | FILE_DISPOSITION_FLAG_ON_CLOSE };
	return SetFileInformationByHandle(destination.get(), FileDispositionInfoEx, &fdie, sizeof fdie);
#endif
}