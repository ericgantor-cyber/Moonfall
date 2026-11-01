param(
    [Parameter(Mandatory=$true)][string]$GameExecutable,
    [int]$ObservationSeconds = 8
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$game = [IO.Path]::GetFullPath($GameExecutable)
if (-not (Test-Path -LiteralPath $game -PathType Leaf)) { throw "Game executable not found: $game" }
if ([IO.Path]::GetFileName($game) -ne 'MoonfallUnreal.exe') {
    throw 'This check only accepts a packaged MoonfallUnreal.exe.'
}
if ($ObservationSeconds -lt 3 -or $ObservationSeconds -gt 60) {
    throw 'ObservationSeconds must be between 3 and 60.'
}
Write-Host "Launching packaged game for $ObservationSeconds seconds: $game"
$process = $null
try {
    $process = Start-Process -FilePath $game -WorkingDirectory ([IO.Path]::GetDirectoryName($game)) -PassThru
    Start-Sleep -Seconds $ObservationSeconds
    $process.Refresh()
    if ($process.HasExited) {
        throw "Moonfall exited during the launch smoke test (exit code $($process.ExitCode)). Check Saved/Logs and Windows Event Viewer."
    }
    Write-Host 'PASS: packaged Moonfall process started and remained running through the observation window.'
    Write-Warning 'This is only a process-launch check, not verification of graphics, controls, gameplay, saves, or performance.'
}
finally {
    if ($null -ne $process) {
        $process.Refresh()
        if (-not $process.HasExited) {
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
            Write-Host 'Stopped only the game process started by this test.'
        }
    }
}
