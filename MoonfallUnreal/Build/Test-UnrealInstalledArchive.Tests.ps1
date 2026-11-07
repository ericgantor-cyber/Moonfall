$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Add-Type -AssemblyName System.IO.Compression
$validator = Join-Path $PSScriptRoot 'Test-UnrealInstalledArchive.ps1'
$folder = Join-Path $env:RUNNER_TEMP 'MoonfallUnrealArchiveTests'
New-Item -ItemType Directory -Force -Path $folder | Out-Null

function New-TestZip {
    param([string]$FileName, [string[]]$Entries)
    $target = Join-Path $folder $FileName
    if (Test-Path $target) { Remove-Item -LiteralPath $target -Force }
    $zip = [System.IO.Compression.ZipFile]::Open($target, [System.IO.Compression.ZipArchiveMode]::Create)
    try {
        foreach ($entryName in $Entries) {
            $entry = $zip.CreateEntry($entryName)
            $writer = New-Object IO.StreamWriter($entry.Open())
            try { $writer.Write('moonfall test') } finally { $writer.Dispose() }
        }
    } finally { $zip.Dispose() }
    return $target
}

function Assert-Archive {
    param([string]$Name, [string[]]$Entries, [bool]$ShouldPass, [bool]$WrongHash = $false)
    $zip = New-TestZip -FileName "$Name.zip" -Entries $Entries
    $hash = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash
    if ($WrongHash) { $hash = '0' * 64 }
    $succeeded = $false
    try {
        & $validator -ArchivePath $zip -ExpectedSha256 $hash
        $succeeded = $true
    } catch {
        Write-Host "Expected rejection for $Name : $($_.Exception.Message)"
    }
    if ($succeeded -ne $ShouldPass) {
        throw "Archive test $Name mismatch: expected=$ShouldPass actual=$succeeded"
    }
    Write-Host "MOONFALL_ARCHIVE_TEST_PASS $Name"
}

Assert-Archive -Name 'valid' -Entries @('Engine/Build/BatchFiles/Build.bat', 'Engine/Binaries/Win64/test.dll') -ShouldPass $true
Assert-Archive -Name 'traversal' -Entries @('Engine/Build/BatchFiles/Build.bat', '../outside.txt') -ShouldPass $false
Assert-Archive -Name 'nested-traversal' -Entries @('Engine/Build/BatchFiles/Build.bat', 'Engine/../outside.txt') -ShouldPass $false
Assert-Archive -Name 'missing-build' -Entries @('Engine/Binaries/Win64/test.dll') -ShouldPass $false
Assert-Archive -Name 'duplicate-case' -Entries @('Engine/Build/BatchFiles/Build.bat', 'engine/build/batchfiles/build.BAT') -ShouldPass $false
Assert-Archive -Name 'hash-mismatch' -Entries @('Engine/Build/BatchFiles/Build.bat') -ShouldPass $false -WrongHash $true
Assert-Archive -Name 'reserved-device' -Entries @('Engine/Build/BatchFiles/Build.bat', 'Engine/AUX.txt') -ShouldPass $false
Assert-Archive -Name 'trailing-period' -Entries @('Engine/Build/BatchFiles/Build.bat', 'Engine/Bad./file.dll') -ShouldPass $false
Assert-Archive -Name 'trailing-space' -Entries @('Engine/Build/BatchFiles/Build.bat', 'Engine/Bad /file.dll') -ShouldPass $false
Assert-Archive -Name 'empty-path' -Entries @('Engine/Build/BatchFiles/Build.bat', 'Engine//doubled.dll') -ShouldPass $false
Assert-Archive -Name 'file-parent-collision' -Entries @('Engine/Build/BatchFiles/Build.bat', 'Engine/SomeFile', 'Engine/SomeFile/nested.dll') -ShouldPass $false
Write-Host 'All Moonfall Unreal ZIP safety tests passed.'
