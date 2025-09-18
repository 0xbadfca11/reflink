> [!IMPORTANT]
> # No longer needed
> [Since Windows 11 24H2, Windows copy engine uses block cloning by default.](https://learn.microsoft.com/windows-server/storage/refs/block-cloning#functionality-restrictions-and-remarks)

reflink for Windows
===
Copy file using [Block Cloning](https://learn.microsoft.com/windows-server/storage/refs/block-cloning), like `cp --reflink`.
```
Copy file without actual data write.

reflink <source> <destination>

source       Specifies a file to copy.
             source must have placed on the ReFS volume.
destination  Specifies new file name.
             destination must have placed on the same volume as source.
```

> [!NOTE]
> Block Cloning requires **ReFS v2**[^1].\
> You may check local volume by `fsutil fsinfo volumeinfo` or `fsutil fsinfo refsinfo`.
> - by volumeinfo\
>   When `Supports Block Cloning` is included in fsutil output, can use Block Cloning.
> - by refsinfo\
>   Version 1.2 or 1.1 is ReFS v1.\
>   Version 3.1 or upper is ReFS v2.

###### LICENSE
MIT License

[^1]: If including non-Microsoft products, then reflink capable with WinBtrfs, OpenZFS, and so on.
