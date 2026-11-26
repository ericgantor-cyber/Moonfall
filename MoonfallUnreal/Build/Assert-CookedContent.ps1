# Check package layout, not the semantic validity of the cooked game.
param([Parameter(Mandatory=$true)][string]$GameRoot)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$pakDirectory = Join-Path $GameRoot 'MoonfallUnreal/Content/Paks'
if (-not (Test-Path -LiteralPath $pakDirectory -PathType Container)) {
    throw "Cooked Moonfall Paks directory missing: $pakDirectory"
}
$pakFiles = @(Get-ChildItem -LiteralPath $pakDirectory -Recurse -File -Filter '*.pak' |
    Where-Object { $_.Length -gt 0 })
$ioStoreIndexes = @(Get-ChildItem -LiteralPath $pakDirectory -Recurse -File -Filter '*.utoc' |
    Where-Object { $_.Length -gt 0 })
$hasIoStorePair = $false
foreach ($indexFile in $ioStoreIndexes) {
    $dataFile = [IO.Path]::ChangeExtension($indexFile.FullName, '.ucas')
    if ((Test-Path -LiteralPath $dataFile -PathType Leaf) -and
        (Get-Item -LiteralPath $dataFile).Length -gt 0) {
        $hasIoStorePair = $true
        break
    }
}
if ($pakFiles.Count -eq 0 -and -not $hasIoStorePair) {
    throw 'No non-empty cooked .pak or paired .utoc/.ucas archive was found. Refusing incomplete package.'
}
Write-Host "Cooked archive layout verified: $($pakFiles.Count) nonempty pak(s); matching IoStore pair: $hasIoStorePair"
