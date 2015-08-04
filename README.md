reflink for Windows
===
Windows 10 introduce new FSCTL, `FSCTL_DUPLICATE_EXTENTS_TO_FILE` to **ReFS 2.0**.  
This FSCTL is like `BTRFS_IOC_CLONE_RANGE`.  
This tool is using FSCTL simply like `cp --reflink`.

### Important notice
1. ~~Windows 10 Technical/Insider Preview still using ReFS 1.2 by default.  
You must do  
`reg add HKLM\SYSTEM\CurrentControlSet\Control\FileSystemUtilities /v RefsFormatVersion /t REG_DWORD /d 2`  
before format volume.~~  
Windows Insider Preview build 10130 or later stopped supporting ReFS 2.0.  
  * Windows Server Technical Preview 2 uses ReFS 2.0 by default, this tweak is not require.
2. ReFS 2.0 is still in active development.

  >They are likely to become inaccessible on later Technical Preview releases.  
  >https://technet.microsoft.com/en-us/library/mt126109.aspx

3. Microsoft issued errata for FSCTL_DUPLICATE_EXTENTS_TO_FILE.  
Current source code does not work future Preview.

  >https://msdn.microsoft.com/en-us/library/dn785066.aspx#BKMK_FSCC

You may check by `fsutil fsinfo refsinfo`.
###### LICENSE
MIT License