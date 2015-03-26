reflink for Windows
===
Windows 10 introduce new FSCTL, `FSCTL_DUPLICATE_EXTENTS_TO_FILE` to **ReFS**.  
This FSCTL is like `BTRFS_IOC_CLONE_RANGE`.  
This tool is using FSCTL simply like `cp --reflink`.

### Important notice
Default ReFS disk format does not support FSCTL\_DUPLICATE\_EXTENTS\_TO\_FILE.  
You must set *HKLM\\SYSTEM\\CurrentControlSet\\Control\\FileSystemUtilities!RefsFormatVersion=dword:2*, before format volume.

You may check by `fsutil fsinfo refsinfo`.  
ReFS version 1.2(10 and 8.1/2012 R2's default) or 1.1(2012's default) is bad.  
version 22.2 is needed.  