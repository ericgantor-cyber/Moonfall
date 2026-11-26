# Read-only Windows hardware inventory for Moonfall. No network, elevation,
# benchmark, settings writes, serial numbers, host names or account data.
[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
function Read-Cim([string]$Class, [string[]]$Properties) {
    try { Get-CimInstance -ClassName $Class -Property $Properties -ErrorAction Stop }
    catch { Write-Warning "Unable to read $Class; leaving it unknown." }
}
$cpu = @(Read-Cim 'Win32_Processor' @('Name','NumberOfCores','NumberOfLogicalProcessors') | Where-Object { $_ })
$os = @(Read-Cim 'Win32_OperatingSystem' @('Caption','Version','BuildNumber','OSArchitecture','TotalVisibleMemorySize') | Where-Object { $_ } | Select-Object -First 1)
$gpu = @(Read-Cim 'Win32_VideoController' @('Name','DriverVersion','CurrentHorizontalResolution','CurrentVerticalResolution') | Where-Object { $_ })
$ram = if ($os.Count -and $os[0].TotalVisibleMemorySize -gt 0) { [math]::Round([double]$os[0].TotalVisibleMemorySize / 1MB, 1) } else { $null }
$threads = if ($cpu.Count) { ($cpu | Measure-Object NumberOfLogicalProcessors -Sum).Sum } else { $null }
# Never trust Win32_VideoController.AdapterRAM (32-bit values may truncate).
# GPU name alone cannot establish DX12 feature level, Lumen, VRAM or FPS.
$recommendation = 'Start Low or Medium at 1080p; verify in the actual game before increasing.'
if ($null -eq $ram -or $null -eq $threads) {
    $recommendation = 'Hardware information incomplete. Start Low and verify gameplay.'
} elseif ($ram -lt 8 -or $threads -lt 4) {
    $recommendation = 'Memory or CPU resources are limited. Try Low at 720p; performance is not guaranteed.'
} elseif ($ram -lt 16 -or $threads -lt 8) {
    $recommendation = 'Try Low at 1080p with reduced render scale; measure performance.'
}
[pscustomobject]@{
    Windows = @($os | Select-Object Caption,Version,BuildNumber,OSArchitecture)
    CPU = @($cpu | Select-Object Name,NumberOfCores,NumberOfLogicalProcessors)
    VisibleRAMGiB = $ram
    GraphicsAdapters = @($gpu | Select-Object Name,DriverVersion,CurrentHorizontalResolution,CurrentVerticalResolution)
    Recommendation = $recommendation
    DirectX12FeatureSupport = 'Not tested'
    DedicatedVRAM = 'Not determined'
    LumenNaniteAndFrameGeneration = 'Not tested'
    MeasuredFPS = 'Not tested'
} | ConvertTo-Json -Depth 5
