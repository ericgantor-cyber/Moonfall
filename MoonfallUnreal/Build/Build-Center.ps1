param([string]$UnrealRoot = $env:UE54_ROOT)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
if (-not $UnrealRoot) { $UnrealRoot = 'C:\Program Files\Epic Games\UE_5.4' }
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$out = Join-Path $repo 'MoonfallUnrealLocalBuild'
$form = New-Object Windows.Forms.Form
$form.Text = 'Moonfall Build Center'
$form.Size = New-Object Drawing.Size(840,560)
$form.MinimumSize = New-Object Drawing.Size(750,440)
$form.StartPosition = 'CenterScreen'
$form.BackColor = [Drawing.Color]::FromArgb(23,19,39)
$form.ForeColor = [Drawing.Color]::White
$form.Font = New-Object Drawing.Font('Segoe UI',10)
$title = New-Object Windows.Forms.Label
$title.Text = 'MOONFALL BUILD CENTER'
$title.Font = New-Object Drawing.Font('Segoe UI',18,[Drawing.FontStyle]::Bold)
$title.SetBounds(16,10,790,44)
$form.Controls.Add($title)
$engine = New-Object Windows.Forms.TextBox
$engine.Text = $UnrealRoot
$engine.SetBounds(16,61,790,27)
$engine.Anchor = 'Top,Left,Right'
$engine.AccessibleName = 'Unreal Engine 5.4 installation folder'
$form.Controls.Add($engine)
$logs = New-Object Windows.Forms.TextBox
$logs.Multiline = $true
$logs.ScrollBars = 'Vertical'
$logs.ReadOnly = $true
$logs.BackColor = [Drawing.Color]::FromArgb(12,12,26)
$logs.ForeColor = [Drawing.Color]::Gainsboro
$logs.Font = New-Object Drawing.Font('Consolas',9)
$logs.SetBounds(16,147,790,355)
$logs.Anchor = 'Top,Bottom,Left,Right'
$logs.AccessibleName = 'Build progress and diagnostic output'
$form.Controls.Add($logs)
function New-Button($label,$x,$width) {
    $button = New-Object Windows.Forms.Button
    $button.Text = $label
    $button.SetBounds($x,101,$width,33)
    $form.Controls.Add($button)
    return $button
}
$check = New-Button 'CHECK TOOLS' 16 135
$install = New-Button 'INSTALL TOOLS' 156 145
$build = New-Button 'BUILD MOONFALL' 306 165
$cancel = New-Button 'CANCEL' 476 120
$folder = New-Button 'OPEN OUTPUT' 601 205
$cancel.Enabled = $false
$check.AccessibleName = 'Check missing build tools'
$install.AccessibleName = 'Install missing tools with permission'
$build.AccessibleName = 'Compile Moonfall and create Windows installer'
$cancel.AccessibleName = 'Cancel build and subprocesses'
$folder.AccessibleName = 'Open Moonfall build output'
$timer = New-Object Windows.Forms.Timer
$timer.Interval = 350
$script:process = $null
$script:seenOut = 0
$script:seenErr = 0
$script:outLog = ''
$script:errLog = ''
function Begin-Job([string]$scriptName,[bool]$installMissing) {
    if ($script:process -and -not $script:process.HasExited) { return }
    $script:seenOut = 0
    $script:seenErr = 0
    $logs.Clear()
    $script:outLog = Join-Path $env:TEMP ('moonfall-out-' + [Guid]::NewGuid().ToString('N') + '.log')
    $script:errLog = "$script:outLog.err"
    $tool = Join-Path $PSScriptRoot $scriptName
    $arguments = '-NoProfile -ExecutionPolicy Bypass -File "' + $tool + '" -UnrealRoot "' + $engine.Text + '"'
    if ($installMissing) { $arguments += ' -InstallMissing' }
    $info = New-Object Diagnostics.ProcessStartInfo
    $info.FileName = 'powershell.exe'
    $info.Arguments = $arguments
    $info.WorkingDirectory = $repo
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    # Asynchronous output events avoid a blocked child process on full pipes.
    $script:process = New-Object Diagnostics.Process
    $script:process.StartInfo = $info
    $script:process.EnableRaisingEvents = $true
    $script:process.add_OutputDataReceived({ if ($null -ne $EventArgs.Data) { [IO.File]::AppendAllText($script:outLog,$EventArgs.Data + [Environment]::NewLine) } })
    $script:process.add_ErrorDataReceived({ if ($null -ne $EventArgs.Data) { [IO.File]::AppendAllText($script:errLog,$EventArgs.Data + [Environment]::NewLine) } })
    [void]$script:process.Start()
    $script:process.BeginOutputReadLine()
    $script:process.BeginErrorReadLine()
    $check.Enabled = $false
    $install.Enabled = $false
    $build.Enabled = $false
    $cancel.Enabled = $true
    $timer.Start()
}
$check.Add_Click({ Begin-Job 'Setup-WindowsPrerequisites.ps1' $false })
$install.Add_Click({
    if ([Windows.Forms.MessageBox]::Show('Install missing dependencies through winget? Administrator approval may be required.','Confirm tool installation','YesNo','Question') -eq 'Yes') { Begin-Job 'Setup-WindowsPrerequisites.ps1' $true }
})
$build.Add_Click({
    if ([Windows.Forms.MessageBox]::Show('Run Unreal compilation and build installer? Previous output will be overwritten.','Confirm build','YesNo','Question') -eq 'Yes') { Begin-Job 'Build-Windows.ps1' $false }
})
$cancel.Add_Click({
    if ($script:process -and -not $script:process.HasExited) {
        & taskkill.exe /PID $script:process.Id /T /F | Out-Null
        $logs.AppendText("Cancellation requested." + [Environment]::NewLine)
    }
})
$folder.Add_Click({ New-Item -ItemType Directory -Path $out -Force | Out-Null; Start-Process explorer.exe -ArgumentList ('"' + $out + '"') })
$timer.Add_Tick({
    foreach ($item in @(@($script:outLog,'seenOut'),@($script:errLog,'seenErr'))) {
        $path = $item[0]
        $key = $item[1]
        if (-not (Test-Path -LiteralPath $path)) { continue }
        try {
            $value = [IO.File]::ReadAllText($path)
            $offset = if ($key -eq 'seenOut') { $script:seenOut } else { $script:seenErr }
            if ($value.Length -gt $offset) {
                $logs.AppendText($value.Substring($offset))
                if ($key -eq 'seenOut') { $script:seenOut = $value.Length } else { $script:seenErr = $value.Length }
            }
        } catch [IO.IOException] { }
    }
    if ($script:process -and $script:process.HasExited) {
        $timer.Stop()
        $check.Enabled = $true
        $install.Enabled = $true
        $build.Enabled = $true
        $cancel.Enabled = $false
        $logs.AppendText("Process exit code: $($script:process.ExitCode)" + [Environment]::NewLine)
    }
})
$form.Add_FormClosing({
    if ($script:process -and -not $script:process.HasExited) {
        $_.Cancel = $true
        [Windows.Forms.MessageBox]::Show('Cancel the running job first.','Moonfall Build Center')
    }
})
[void]$form.ShowDialog()
