$ErrorActionPreference = 'Stop'
$finder = Join-Path $PSScriptRoot 'Find-PackagedGame.ps1'
$temp = Join-Path ([IO.Path]::GetTempPath()) ('MoonfallPackageTest-' + [guid]::NewGuid().ToString('N'))
function New-Fixture([string]$root) {
    New-Item -ItemType Directory -Path (Join-Path $root 'Engine') -Force | Out-Null
    New-Item -ItemType Directory -Path (Join-Path $root 'MoonfallUnreal\Binaries\Win64') -Force | Out-Null
    [IO.File]::WriteAllBytes((Join-Path $root 'MoonfallUnreal.exe'), [byte[]]@(1))
    [IO.File]::WriteAllBytes((Join-Path $root 'MoonfallUnreal\Binaries\Win64\MoonfallUnreal.exe'), [byte[]]@(2))
}
function Assert-Rejected([string]$path) {
    $rejected = $false
    try { $null = & $finder -PackageDirectory $path } catch { $rejected = $true }
    if (-not $rejected) { throw "Expected package root rejection: $path" }
}
try {
    New-Item -ItemType Directory -Path $temp | Out-Null
    Assert-Rejected $temp
    $first = Join-Path $temp 'Windows'
    New-Fixture $first
    $found = & $finder -PackageDirectory $temp
    if ($found.FullName -ne (Join-Path $first 'MoonfallUnreal.exe')) {
        throw 'Selected nested binary instead of the package root.'
    }
    [IO.File]::WriteAllBytes($found.FullName, [byte[]]@())
    Assert-Rejected $temp
    [IO.File]::WriteAllBytes($found.FullName, [byte[]]@(1))
    $second = Join-Path $temp 'WindowsDuplicate'
    New-Fixture $second
    Assert-Rejected $temp
    Remove-Item -LiteralPath $second -Recurse -Force
    Remove-Item -LiteralPath (Join-Path $first 'Engine') -Recurse -Force
    Assert-Rejected $temp
    Assert-Rejected (Join-Path $temp 'Missing')
    Write-Host 'Package root discovery fixtures passed: nested binary, missing/empty root, ambiguous roots and missing runtime.'
} finally {
    if (Test-Path -LiteralPath $temp) { Remove-Item -LiteralPath $temp -Recurse -Force }
}
