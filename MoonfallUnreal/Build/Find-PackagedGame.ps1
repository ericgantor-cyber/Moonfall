param(
    [Parameter(Mandatory=$true)][string]$PackageDirectory
)
$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $PackageDirectory -PathType Container)) {
    throw "Packaged output directory does not exist: $PackageDirectory"
}
# Unreal stages a root bootstrap executable and may also stage an identically
# named game binary under MoonfallUnreal\Binaries\Win64. Only the root entry
# point has both runtime and project directories beside it.
$roots = @(Get-ChildItem -LiteralPath $PackageDirectory -Recurse -File -Filter 'MoonfallUnreal.exe' |
    Where-Object {
        (Test-Path -LiteralPath (Join-Path $_.Directory.FullName 'Engine') -PathType Container) -and
        (Test-Path -LiteralPath (Join-Path $_.Directory.FullName 'MoonfallUnreal') -PathType Container)
    })
if ($roots.Count -ne 1) {
    throw "Expected exactly one packaged Moonfall root entry point, found $($roots.Count): $PackageDirectory"
}
if ($roots[0].Length -le 0) { throw 'Packaged Moonfall root entry point is empty.' }
return $roots[0]
