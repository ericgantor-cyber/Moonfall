# Check the Windows executable header without executing untrusted files.
param([Parameter(Mandatory=$true)][string]$Executable)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$stream = [IO.File]::OpenRead([IO.Path]::GetFullPath($Executable))
$reader = [IO.BinaryReader]::new($stream)
try {
    if ($stream.Length -lt 128 -or $reader.ReadUInt16() -ne 0x5A4D) { throw 'Invalid Windows executable header.' }
    $stream.Position = 0x3C
    $peOffset = $reader.ReadInt32()
    if ($peOffset -lt 0x40 -or $peOffset -gt $stream.Length - 26) { throw 'Invalid Windows PE offset.' }
    $stream.Position = $peOffset
    if ($reader.ReadUInt32() -ne 0x00004550 -or $reader.ReadUInt16() -ne 0x8664) { throw 'Packaged game is not an AMD64 PE executable.' }
    $stream.Position = $peOffset + 24
    if ($reader.ReadUInt16() -ne 0x20B) { throw 'Packaged game is not a PE32+ executable.' }
} finally {
    $reader.Dispose()
    $stream.Dispose()
}
