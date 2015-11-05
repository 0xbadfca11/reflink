$src = "E:\test.vhdx";
$dest = "E:\test2.vhdx";

foreach($i in @(1GB, 2GB, 3GB ,4GB))
{
    \\win-56oprnnmqao\tmp\vhdxtool.exe create -s $i -f $src > $null
    if( (Start-Process -FilePath "x64/Debug/reflink.exe" -ArgumentList $src,$dest -NoNewWindow -PassThru -Wait).ExitCode -ne 0 )
    {
        throw "reflink fail";
    }
    (Measure-Command{
    if( [string]::Compare( (Get-FileHash $src -Verbose).Hash, (Get-FileHash $dest -Verbose).Hash ) -ne 0 )
    {
        throw "file hash mismatch";
    }
    }).TotalSeconds
}