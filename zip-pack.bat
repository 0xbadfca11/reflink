tar -C Release -caf reflink-x86.zip reflink.exe
tar -C Release -caf reflink-x86-debug-symbol.zip reflink.pdb
tar -C x64/Release -caf reflink-x64.zip reflink.exe
tar -C x64/Release -caf reflink-x64-debug-symbol.zip reflink.pdb
tar -C ARM64/Release -caf reflink-Arm64.zip reflink.exe
tar -C ARM64/Release -caf reflink-Arm64-debug-symbol.zip reflink.pdb
