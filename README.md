reflink for Windows
===
Windows Server 2016 introduce [Block Cloning feature](https://msdn.microsoft.com/en-us/library/windows/desktop/mt590820.aspx).  
I wanted `cp --reflink`. But, only exposed to API.

### Important note
* Block Cloning requires **ReFS v2**.  
  ReFS v2 is only available in Windows Server 2016 and Windows 10 version 1703 (build 15063).  
  Windows 10 version 1607 (build 14393) and earlier Windows only can use ReFS v1.  
  Even moving ReFS v1 volume from earlier Windows to WS2016 or W10 v1703 cannot use Block Cloning. Require format to ReFS v2 in WS2016 or W10 v1703.  
  You may check by `fsutil fsinfo refsinfo` or `fsutil fsinfo volumeinfo`.  
  - by refsinfo
    Version 1.2 or 1.1 is ReFS v1.  
    Version 3.2 or 3.1 is ReFS v2.  
  - by volumeinfo
    When `Supports Block Cloning` is included in fsutil output, can use Block Cloning.  

###### LICENSE
MIT License
