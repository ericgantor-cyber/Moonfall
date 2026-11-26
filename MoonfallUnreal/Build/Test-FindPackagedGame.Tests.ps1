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

# Header-only fixtures: never launch these synthetic files.
$validator = Join-Path $PSScriptRoot 'Assert-Win64Executable.ps1'
$headerRoot = Join-Path ([IO.Path]::GetTempPath()) ('MoonfallPEFixtures-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $headerRoot | Out-Null
function New-Header([int]$length = 512, [int]$offset = 128, [int]$machine = 0x8664, [int]$magic = 0x20B) {
    $bytes = [byte[]]::new($length)
    if ($length -ge 64) {
        [BitConverter]::GetBytes([uint16]0x5A4D).CopyTo($bytes,0)
        [BitConverter]::GetBytes([int32]$offset).CopyTo($bytes,60)
    }
    if ($offset -ge 64 -and $offset -le $length - 26) {
        [BitConverter]::GetBytes([uint32]0x00004550).CopyTo($bytes,$offset)
        [BitConverter]::GetBytes([uint16]$machine).CopyTo($bytes,$offset+4)
        [BitConverter]::GetBytes([uint16]$magic).CopyTo($bytes,$offset+24)
    }
    return ,$bytes
}
function Assert-Header([string]$name,[byte[]]$bytes,[bool]$expected) {
    $path = Join-Path $headerRoot ($name + '.exe')
    [IO.File]::WriteAllBytes($path,$bytes)
    $accepted = $true
    try { & $validator -Executable $path } catch { $accepted = $false }
    if ($accepted -ne $expected) { throw "Incorrect PE validation: $name" }
    Remove-Item -LiteralPath $path -Force
}
try {
    Assert-Header 'valid-amd64-header' (New-Header) $true
    Assert-Header 'truncated' (New-Header -length 63) $false
    Assert-Header 'negative-offset' (New-Header -offset -1) $false
    Assert-Header 'offset-inside-dos' (New-Header -offset 32) $false
    Assert-Header 'offset-outside-file' (New-Header -offset 500) $false
    Assert-Header 'x86' (New-Header -machine 0x14C) $false
    Assert-Header 'arm64' (New-Header -machine 0xAA64) $false
    Assert-Header 'pe32' (New-Header -magic 0x10B) $false
    $bad = New-Header
    $bad[0] = 0
    Assert-Header 'missing-mz' $bad $false
    $bad = New-Header
    $bad[128] = 0
    Assert-Header 'missing-pe-signature' $bad $false
    $rejected = $false
    try { & $validator -Executable (Join-Path $headerRoot 'missing.exe') } catch { $rejected = $true }
    if (-not $rejected) { throw 'Missing executable was accepted.' }
    Write-Host 'Win64 executable header fixtures: eleven scenarios passed.'
} finally {
    Remove-Item -LiteralPath $headerRoot -Recurse -Force
}

# Cooked archive layout fixtures: these are not real cooked Unreal assets.
$cookedValidator = Join-Path $PSScriptRoot 'Assert-CookedContent.ps1'
$cookedRoot = Join-Path ([IO.Path]::GetTempPath()) ('MoonfallCookedFixtures-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $cookedRoot | Out-Null
function Assert-CookedCase([string]$name,[string[]]$files,[bool]$expected,[bool]$makePaks = $true) {
    $root = Join-Path $cookedRoot $name
    New-Item -ItemType Directory -Path $root | Out-Null
    $paks = Join-Path $root 'MoonfallUnreal/Content/Paks'
    if ($makePaks) { New-Item -ItemType Directory -Path $paks -Force | Out-Null }
    foreach ($fixture in $files) {
        $parts = $fixture.Split('|')
        $path = Join-Path $root $parts[0]
        New-Item -ItemType Directory -Path ([IO.Path]::GetDirectoryName($path)) -Force | Out-Null
        [IO.File]::WriteAllBytes($path,[byte[]]::new([int]$parts[1]))
    }
    $accepted = $true
    try { & $cookedValidator -GameRoot $root } catch { $accepted = $false }
    if ($accepted -ne $expected) { throw "Incorrect cooked content verdict: $name" }
}
try {
    $p = 'MoonfallUnreal/Content/Paks'
    Assert-CookedCase 'missing-directory' @() $false $false
    Assert-CookedCase 'empty-directory' @() $false
    Assert-CookedCase 'empty-pak' @("$p/a.pak|0") $false
    Assert-CookedCase 'valid-pak' @("$p/a.pak|1") $true
    Assert-CookedCase 'misplaced-pak' @('MoonfallUnreal/Other/a.pak|1') $false
    Assert-CookedCase 'index-only' @("$p/a.utoc|1") $false
    Assert-CookedCase 'data-only' @("$p/a.ucas|1") $false
    Assert-CookedCase 'mismatched-pair' @("$p/a.utoc|1","$p/b.ucas|1") $false
    Assert-CookedCase 'empty-data' @("$p/a.utoc|1","$p/a.ucas|0") $false
    Assert-CookedCase 'empty-index' @("$p/a.utoc|0","$p/a.ucas|1") $false
    Assert-CookedCase 'different-directory' @("$p/a.utoc|1","$p/sub/a.ucas|1") $false
    Assert-CookedCase 'valid-pair' @("$p/a.utoc|1","$p/a.ucas|1") $true
    Assert-CookedCase 'nested-pair' @("$p/sub/a.utoc|1","$p/sub/a.ucas|1") $true
    Write-Host 'Cooked content layout fixtures: thirteen scenarios passed.'
} finally {
    Remove-Item -LiteralPath $cookedRoot -Recurse -Force
}
