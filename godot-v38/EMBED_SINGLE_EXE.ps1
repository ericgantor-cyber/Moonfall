param(
  [Parameter(Mandatory=$true)][string]$Engine,
  [Parameter(Mandatory=$true)][string]$Pck,
  [Parameter(Mandatory=$true)][string]$Output
)
$ErrorActionPreference = 'Stop'
$engineStream = [System.IO.File]::OpenRead($Engine)
$packStream = [System.IO.File]::OpenRead($Pck)
try {
  $header = New-Object byte[] 4
  if ($engineStream.Read($header,0,4) -ne 4 -or [Text.Encoding]::ASCII.GetString($header,0,2) -ne 'MZ') {
    throw 'Input engine is not a Windows PE executable.'
  }
  if ($packStream.Read($header,0,4) -ne 4 -or [Text.Encoding]::ASCII.GetString($header) -ne 'GDPC') {
    throw 'Input pack is not a Godot PCK.'
  }
  $engineStream.Position=0
  $packStream.Position=0
  $outStream=[System.IO.File]::Create($Output)
  try {
    $engineStream.CopyTo($outStream)
    $packStream.CopyTo($outStream)
    $outStream.Write([System.BitConverter]::GetBytes([int64]$packStream.Length),0,8)
    $magic=[System.Text.Encoding]::ASCII.GetBytes('GDPC')
    $outStream.Write($magic,0,4)
  } finally { $outStream.Dispose() }
  $result=[System.IO.File]::OpenRead($Output)
  try {
    if ($result.Length -ne ($engineStream.Length+$packStream.Length+12)) { throw 'Size verification failed' }
    $result.Seek(-12,[System.IO.SeekOrigin]::End) | Out-Null
    $buffer=New-Object byte[] 12
    if ($result.Read($buffer,0,12) -ne 12) { throw 'Trailer truncated' }
    if ([BitConverter]::ToInt64($buffer,0) -ne $packStream.Length -or [Text.Encoding]::ASCII.GetString($buffer,8,4) -ne 'GDPC') {
      throw 'Embedded PCK footer verification failed'
    }
  } finally { $result.Dispose() }
  Write-Output "Single-file Moonfall game created: $Output"
} finally {
  $engineStream.Dispose()
  $packStream.Dispose()
}
