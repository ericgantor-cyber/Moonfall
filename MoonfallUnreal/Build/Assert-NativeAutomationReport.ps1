# Validate exact report fields once, including conflicting duplicate results.
param(
    [Parameter(Mandatory=$true)][AllowEmptyCollection()][string[]]$Lines,
    [Parameter(Mandatory=$true)][string[]]$Required
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$expected = [System.Collections.Generic.Dictionary[string,string]]::new([StringComparer]::Ordinal)
$expected.Add('EditorExitCode','0')
$expected.Add('AnyNativeTestFailed','False')
foreach ($name in $Required) {
    if ([string]::IsNullOrWhiteSpace($name)) { throw 'Empty required native test name.' }
    $expected.Add($name,'True')
}
$seen = [System.Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
foreach ($line in $Lines) {
    $separator = $line.IndexOf('=')
    if ($separator -lt 1) { continue }
    $key = $line.Substring(0,$separator)
    if (-not $expected.ContainsKey($key)) { continue }
    if (-not $seen.Add($key)) { throw "Duplicate native report field: $key" }
    $value = $line.Substring($separator + 1)
    if ($value -cne $expected[$key]) { throw "Unexpected native report result: $key=$value" }
}
foreach ($key in $expected.Keys) {
    if (-not $seen.Contains($key)) { throw "Missing required native report field: $key" }
}
