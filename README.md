reflink for Windows
===
Windows Server 2016 introduce [Block Cloning feature](https://msdn.microsoft.com/en-us/library/windows/desktop/mt590820.aspx).  
I wanted `cp --reflink`. But, only exposed to API.

### Important note
* Block Cloning requires **ReFS v2**.  
  ReFS v2 is only available in Windows Server 2016.  
  Windows 10 and earlier Windows only can use ReFS v1.  
  Moving ReFS v1 volume to WS2016 from earlier Windows cannot use Block Cloning. Require format to ReFS v2 in WS2016.  
  You may check by `fsutil fsinfo refsinfo`.  
  Version 1.2 or 1.1 is ReFS v1.  
  Version 3.1 is ReFS v2.

###### LICENSE
MIT License
