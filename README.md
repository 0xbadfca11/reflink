# No longer needed
[Since Windows 11 24H2, Windows copy engine uses block cloning by default.](https://learn.microsoft.com/windows-server/storage/refs/block-cloning#functionality-restrictions-and-remarks)

reflink for Windows
===
```
Copy file without actual data write.

reflink <source> <destination>

source       Specifies a file to copy.
             source must have placed on the ReFS volume.
destination  Specifies new file name.
             destination must have placed on the same volume as source.
```
Windows Server 2016 introduce [Block Cloning feature](https://learn.microsoft.com/windows-server/storage/refs/block-cloning).  
I wanted `cp --reflink`. But, only exposed to API.

### Important note
* Block Cloning requires **ReFS v2**.  
  ReFS v2 is only available in Windows Server 2016 and Windows 10 version 1703 (build 15063) or later.  
  Windows 10 version 1607 (build 14393) and earlier Windows only can use ReFS v1.  
  Even moving ReFS v1 volume from earlier Windows to WS2016 or W10 v1703 cannot use Block Cloning. Require format to ReFS v2 in WS2016 or W10 v1703.  
  You may check by `fsutil fsinfo volumeinfo` or `fsutil fsinfo refsinfo`.  
  - by volumeinfo  
    When `Supports Block Cloning` is included in fsutil output, can use Block Cloning.  
  - by refsinfo  
    Version 1.2 or 1.1 is ReFS v1.  
    Version 3.1 or upper is ReFS v2.  

###### LICENSE
MIT License
