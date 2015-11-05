reflink for Windows
===
Windows 10 and Windows Server 2016 Technical Preview introduce new FSCTL, `FSCTL_DUPLICATE_EXTENTS_TO_FILE`.  
This FSCTL is like `BTRFS_IOC_CLONE_RANGE`.  
This tool is using FSCTL simply like `cp --reflink`.

### Important note
* FSCTL\_DUPLICATE\_EXTENTS\_TO\_FILE requires **ReFS v2**.  
  ReFS v2 is only available in Windows Server 2016 Technical Preview.  
  Windows 10 and earlier Windows only can use ReFS v1.  
  Moving ReFS v1 volume to WS2016 TP from earlier Windows cannot use DUPLICATE\_EXTENTS\_TO\_FILE. Require format to ReFS v2 in WS2016 TP.  
  You may check by `fsutil fsinfo refsinfo`.  
  Version 1.2 or 1.1 is ReFS v1.  
  Version 2.0 is ReFS v2.

* ReFS 2.0 is still in active development.

  >They are likely to become inaccessible on later Technical Preview releases.  
  >https://technet.microsoft.com/en-us/library/mt126109.aspx

###### LICENSE
MIT License
