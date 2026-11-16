param(
    [Parameter(Mandatory = $true)][string]$ArchivePath,
    [Parameter(Mandatory = $true)][string]$ExpectedSha256,
    [string]$DestinationRoot = '',
    [long]$ReserveBytes = 15GB
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$archive = (Resolve-Path -LiteralPath $ArchivePath -ErrorAction Stop).Path
if ($ExpectedSha256 -notmatch '^[a-fA-F0-9]{64}$') {
    throw 'ExpectedSha256 must contain exactly 64 hexadecimal characters.'
}
$actualHash = (Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash
if ($actualHash -ine $ExpectedSha256) {
    throw 'Engine archive SHA-256 mismatch; refusing to extract.'
}
if ($ReserveBytes -lt 0) { throw 'ReserveBytes must be nonnegative.' }
Add-Type -AssemblyName System.IO.Compression
$zip = [System.IO.Compression.ZipFile]::OpenRead($archive)
try {
    if ($zip.Entries.Count -gt 500000) { throw 'Unreal archive contains too many entries.' }
    $seen = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $files = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $directories = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $size = [long]0
    $foundBuildTool = $false
    foreach ($entry in $zip.Entries) {
        $raw = $entry.FullName
        $normalized = $raw.Replace('\', '/')
        if ([string]::IsNullOrWhiteSpace($normalized) -or
            $normalized.StartsWith('/') -or
            $normalized -match '^[a-zA-Z]:' -or
            $normalized -match '(^|/)\.\.(/|$)' -or
            $normalized -match '(^|/)\.(/|$)' -or
            $normalized -match '[<>:"|?*]' -or
            $normalized.Contains([char]0)) {
            throw "Engine ZIP has unsafe entry path: $raw"
        }
        # Windows normalizes trailing spaces/dots and reserves DOS device names.
        # Reject names that could alias another archive entry on NTFS.
        $parts = @($normalized.TrimEnd('/').Split('/'))
        if ($parts.Count -eq 0 -or @($parts | Where-Object { $_ -eq '' }).Count -gt 0) {
            throw 'Engine ZIP contains empty path components.'
        }
        foreach ($part in $parts) {
            if ($part.EndsWith(' ') -or $part.EndsWith('.') -or
                $part -match '^(?i)(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])($|\.)') {
                throw "Engine ZIP has Windows-unsafe path segment: $part"
            }
        }
        if (-not $seen.Add($normalized.TrimEnd('/'))) {
            throw 'Engine ZIP contains duplicate paths differing only by case or slash.'
        }
        # A file cannot also act as a parent directory of another ZIP entry.
        # ZIP implicit directories are allowed, but explicit file/dir conflicts aren't.
        $isDirectory = $normalized.EndsWith('/')
        $fileKey = $normalized.TrimEnd('/')
        if ($isDirectory) {
            if ($files.Contains($fileKey)) { throw 'Engine ZIP file/directory collision.' }
            [void]$directories.Add($fileKey)
        } else {
            if ($directories.Contains($fileKey)) { throw 'Engine ZIP file/directory collision.' }
            [void]$files.Add($fileKey)
        }
        $unixFileType = ($entry.ExternalAttributes -shr 16) -band 61440
        if ($unixFileType -eq 40960) {
            throw 'Engine ZIP contains a symbolic link; refusing extraction.'
        }
        if ($entry.Length -gt ([long]::MaxValue - $size)) {
            throw 'Engine archive declared size overflows 64-bit limit.'
        }
        $size += [long]$entry.Length
        if ($normalized -ieq 'Engine/Build/BatchFiles/Build.bat') {
            $foundBuildTool = $true
        }
    }
    foreach ($fileKey in $files) {
        $parent = $fileKey
        while ($parent.Contains('/')) {
            $parent = $parent.Substring(0, $parent.LastIndexOf('/'))
            if ($files.Contains($parent)) {
                throw 'Engine ZIP has file entries used as parent directories.'
            }
        }
    }
    if (-not $foundBuildTool) {
        throw 'Archive is missing Engine/Build/BatchFiles/Build.bat at root.'
    }
    if ($DestinationRoot) {
        $drive = [IO.Path]::GetPathRoot([IO.Path]::GetFullPath($DestinationRoot)).TrimEnd('\')
        $disk = Get-CimInstance Win32_LogicalDisk -Filter "DeviceID='$drive'"
        if (-not $disk -or $disk.FreeSpace -lt $size + $ReserveBytes) {
            throw 'Insufficient disk for expanded engine size plus requested reserve.'
        }
    }
    Write-Host "MOONFALL_ARCHIVE_VALIDATED entries=$($zip.Entries.Count) uncompressed_bytes=$size"
} finally {
    $zip.Dispose()
}
