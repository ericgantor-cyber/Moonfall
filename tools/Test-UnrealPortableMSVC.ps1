# Test all remaining standalone C++ policies with the real Windows compiler.
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$PSNativeCommandUseErrorActionPreference = $false
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$compiler = (Get-Command cl.exe -ErrorAction Stop).Source
$tests = @(Get-ChildItem -LiteralPath $PSScriptRoot -File |
    Where-Object { $_.Name -match '^test_(unreal|v27)_.+\.cpp$' -and $_.Name -ne 'test_unreal_gear_advisor_policy.cpp' } |
    Sort-Object Name)
if ($tests.Count -eq 0) { throw 'No additional portable C++ tests discovered.' }
$temp = Join-Path ([IO.Path]::GetTempPath()) ('MoonfallMSVC-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $temp | Out-Null
$results = [ordered]@{}
try {
    Push-Location $temp
    try {
        foreach ($test in $tests) {
            $exe = Join-Path $temp ($test.BaseName + '.exe')
            $obj = Join-Path $temp ($test.BaseName + '.obj')
            Write-Host "=== $($test.Name) ==="
            & $compiler /nologo /std:c++17 /W4 /WX /EHsc /O2 "/I$root/MoonfallUnreal/Source/MoonfallUnreal" "/Fo$obj" "/Fe$exe" $test.FullName
            $compileExit = $LASTEXITCODE
            if ($compileExit -ne 0 -or -not (Test-Path -LiteralPath $exe -PathType Leaf)) {
                $results[$test.Name] = "Compile failed ($compileExit)"
                continue
            }
            & $exe
            $testExit = $LASTEXITCODE
            $results[$test.Name] = if ($testExit -eq 0) { 'Passed' } else { "Test failed ($testExit)" }
        }
    } finally { Pop-Location }
    $passed = @($results.Values | Where-Object { $_ -ceq 'Passed' }).Count
    $summary = @('### Additional Windows MSVC portable policy tests', '',
        "Passed: $passed/$($tests.Count)", '',
        '| Suite | Result |', '| --- | --- |')
    foreach ($entry in $results.GetEnumerator()) {
        $summary += "| $($entry.Key) | $($entry.Value) |"
    }
    $summary | ForEach-Object { Write-Host $_ }
    if ($env:GITHUB_STEP_SUMMARY) {
        $summary | Add-Content -LiteralPath $env:GITHUB_STEP_SUMMARY -Encoding UTF8
    }
    if ($env:MOONFALL_POLICY_RESULTS) {
        $results.GetEnumerator() | ForEach-Object { "$($_.Key)`t$($_.Value)" } |
            Set-Content -LiteralPath $env:MOONFALL_POLICY_RESULTS -Encoding utf8
    }
    if ($passed -ne $tests.Count) { throw 'One or more portable Windows C++ suites failed.' }
} finally {
    Remove-Item -LiteralPath $temp -Recurse -Force
}
