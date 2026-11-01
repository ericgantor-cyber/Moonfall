param([string]$UnrealRoot = $env:UE54_ROOT, [switch]$SelfTest)
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
$logs.SetBounds(16,179,790,323)
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
$compile = New-Button 'TEST COMPILE' 306 165
$preflight = New-Button 'BUILD PREFLIGHT' 306 165

$cancel = New-Button 'CANCEL' 476 120
$folder = New-Button 'OPEN OUTPUT' 601 205
$preflight.SetBounds(16,140,140,27)
$compile.SetBounds(306,101,120,33)
$build.SetBounds(431,101,155,33)
$cancel.SetBounds(591,101,100,33)
$folder.SetBounds(696,101,125,33)
$folder.Width = 130
$diagnostics = New-Button 'OPEN LOGS' 16 130
$diagnostics.SetBounds(161,140,130,27)
$diagnostics.Anchor = 'Top,Right'
$diagnostics.AccessibleName = 'Open saved Moonfall build diagnostic logs'
$browseEngine = New-Button 'CHOOSE ENGINE' 301 155
$browseEngine.SetBounds(301,140,155,27)
$browseEngine.AccessibleName = 'Choose installed Unreal Engine 5.4 folder'
$browseEngine.Add_Click({
    $dialog = New-Object Windows.Forms.FolderBrowserDialog
    $dialog.Description = 'Select the Unreal Engine 5.4 root folder containing Engine'
    if (Test-Path -LiteralPath $engine.Text -PathType Container) {
        $dialog.SelectedPath = $engine.Text
    }
    if ($dialog.ShowDialog($form) -eq [Windows.Forms.DialogResult]::OK) {
        $candidate = $dialog.SelectedPath
        $requiredBuild = Join-Path $candidate 'Engine\Build\BatchFiles\Build.bat'
        if (Test-Path -LiteralPath $requiredBuild -PathType Leaf) {
            $engine.Text = $candidate
            $logs.AppendText("Selected Unreal Engine root: $candidate" + [Environment]::NewLine)
        } else {
            [Windows.Forms.MessageBox]::Show('This folder does not contain Engine\Build\BatchFiles\Build.bat. Select the UE_5.4 installation folder.','Invalid engine folder')
        }
    }
    $dialog.Dispose()
})
$cancel.Enabled = $false
$check.AccessibleName = 'Check missing build tools'
$install.AccessibleName = 'Install missing tools with permission'
$build.AccessibleName = 'Compile Moonfall and create Windows installer'
$compile.AccessibleName = 'Test Unreal Editor compilation without packaging'
$preflight.AccessibleName = 'Check Unreal compiler prerequisites without compiling'
$cancel.AccessibleName = 'Cancel build and subprocesses'
$folder.AccessibleName = 'Open Moonfall build output'
$timer = New-Object Windows.Forms.Timer
$timer.Interval = 350
$script:process = $null
$script:seenOut = 0
$script:seenErr = 0
$script:outLog = ''
$script:errLog = ''
$script:logDir = Join-Path $env:LOCALAPPDATA 'MoonfallBuildCenter\Logs'
New-Item -ItemType Directory -Force -Path $script:logDir | Out-Null
function Begin-Job([string]$scriptName,[bool]$installMissing,[bool]$compileOnly = $false,[bool]$preflightOnly = $false) {
    if ($script:process -and -not $script:process.HasExited) { return }
    $script:seenOut = 0
    $script:seenErr = 0
    $logs.Clear()
    $script:outLog = Join-Path $script:logDir ('build-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N').Substring(0,8) + '.log')
    $script:errLog = "$script:outLog.err"
    $tool = Join-Path $PSScriptRoot $scriptName
    $arguments = '-NoProfile -ExecutionPolicy Bypass -File "' + $tool + '" -UnrealRoot "' + $engine.Text + '"'
    if ($installMissing) { $arguments += ' -InstallMissing' }
    if ($compileOnly) { $arguments += ' -CompileOnly' }
    if ($preflightOnly) { $arguments += ' -PreflightOnly' }
    # PowerShell handles argument quoting and redirects output without fragile cmd /c nesting.
    try {
        $script:process = Start-Process -FilePath 'powershell.exe' -ArgumentList $arguments `
            -WorkingDirectory $repo -RedirectStandardOutput $script:outLog `
            -RedirectStandardError $script:errLog -WindowStyle Hidden -PassThru
    } catch {
        $logs.AppendText("Unable to start job: $($_.Exception.Message)" + [Environment]::NewLine)
        return
    }
    $check.Enabled = $false
    $install.Enabled = $false
    $build.Enabled = $false
    $compile.Enabled = $false
    $preflight.Enabled = $false
    $cancel.Enabled = $true
    $timer.Start()
}
$check.Add_Click({ Begin-Job 'Setup-WindowsPrerequisites.ps1' $false })
$install.Add_Click({
    if ([Windows.Forms.MessageBox]::Show('Install missing dependencies through winget? Administrator approval may be required.','Confirm tool installation','YesNo','Question') -eq 'Yes') { Begin-Job 'Setup-WindowsPrerequisites.ps1' $true }
})
$preflight.Add_Click({ Begin-Job 'Build-Windows.ps1' $false $false $true })
$compile.Add_Click({ Begin-Job 'Build-Windows.ps1' $false $true })
$build.Add_Click({
    if ([Windows.Forms.MessageBox]::Show('Run Unreal compilation and build installer? Previous output will be overwritten.','Confirm build','YesNo','Question') -eq 'Yes') { Begin-Job 'Build-Windows.ps1' $false }
})
$cancel.Add_Click({
    if ($script:process -and -not $script:process.HasExited) {
        & taskkill.exe /PID $script:process.Id /T /F | Out-Null
        $logs.AppendText("Cancellation requested. Child processes will be terminated." + [Environment]::NewLine)
    }
})
$diagnostics.Add_Click({ Start-Process explorer.exe -ArgumentList ('"' + $script:logDir + '"') })
$exportLog = New-Button 'EXPORT LOG' 461 130
$exportLog.SetBounds(461,140,125,27)
$exportLog.AccessibleName = 'Save latest Moonfall diagnostic log to a selected location'
$exportLog.Add_Click({
    if (-not $script:outLog -or -not (Test-Path -LiteralPath $script:outLog)) {
        [Windows.Forms.MessageBox]::Show('Run a build or prerequisite check first.','No log available')
        return
    }
    $dialog = New-Object Windows.Forms.SaveFileDialog
    $dialog.Title = 'Export Moonfall build diagnostics'
    $dialog.Filter = 'Text files (*.txt)|*.txt'
    $dialog.FileName = 'Moonfall-Build-Diagnostics.txt'
    if ($dialog.ShowDialog($form) -eq [Windows.Forms.DialogResult]::OK) {
        try {
            $body = @('Moonfall Build Center diagnostics',
                      'Generated: ' + (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'),
                      'Engine: ' + $engine.Text,
                      '----- Standard output -----',
                      [IO.File]::ReadAllText($script:outLog),
                      '----- Standard error -----')
            if (Test-Path -LiteralPath $script:errLog) {
                $body += [IO.File]::ReadAllText($script:errLog)
            }
            [IO.File]::WriteAllText($dialog.FileName, ($body -join [Environment]::NewLine))
            $logs.AppendText('Exported diagnostics: ' + $dialog.FileName + [Environment]::NewLine)
        } catch {
            [Windows.Forms.MessageBox]::Show($_.Exception.Message,'Export failed')
        }
    }
    $dialog.Dispose()
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
        $compile.Enabled = $true
        $preflight.Enabled = $true
        $cancel.Enabled = $false
        $exitCode = $script:process.ExitCode
        $logs.AppendText("Process exit code: $exitCode" + [Environment]::NewLine)
        $logs.AppendText("Saved build log: $script:outLog" + [Environment]::NewLine)
        if ($exitCode -ne 0) {
            $logs.AppendText("BUILD FAILED: Review the saved log and error log before retrying." + [Environment]::NewLine)
            $diagnosticFiles = @($script:outLog, $script:errLog)
            $matches = @()
            foreach ($file in $diagnosticFiles) {
                if (-not (Test-Path -LiteralPath $file)) { continue }
                $matches += @(Select-String -LiteralPath $file -Pattern 'error C[0-9]+:','fatal error','error CS[0-9]+:','ERROR:','Exception:','BUILD FAILED' -ErrorAction SilentlyContinue | Select-Object -Last 12)
            }
            if ($matches.Count -gt 0) {
                $logs.AppendText("---- Recent error details ----" + [Environment]::NewLine)
                foreach ($match in ($matches | Select-Object -Last 12)) {
                    $logs.AppendText(($match.Line.Trim()) + [Environment]::NewLine)
                }
            } else {
                $logs.AppendText("No recognized compiler error line found. Open full logs for details." + [Environment]::NewLine)
            }
        }
    }
})
$form.Add_FormClosing({
    param($sender, $eventArgs)
    if ($script:process -and -not $script:process.HasExited) {
        $eventArgs.Cancel = $true
        [Windows.Forms.MessageBox]::Show('Cancel the running job first.','Moonfall Build Center')
    }
})
if ($SelfTest) {
    # Exercise real WinForms control construction on a hosted Windows runner.
    $required = @($check, $install, $exportLog, $browseEngine, $preflight, $compile, $build, $cancel, $folder, $diagnostics, $logs, $engine)
    foreach ($control in $required) {
        if ($null -eq $control -or -not $control.IsHandleCreated) {
            if ($null -eq $control) { throw 'Build Center control was not created.' }
            [void]$control.Handle
        }
        if ([string]::IsNullOrWhiteSpace($control.AccessibleName)) {
            throw "Missing accessible name for control: $($control.Name)"
        }
    }
    if ($cancel.Enabled) { throw 'Cancel must start disabled.' }
    if (-not $build.Enabled -or -not $compile.Enabled) { throw 'Build buttons must start enabled.' }
    Write-Host 'Build Center WinForms controls instantiated successfully. No Unreal compilation was performed.'
    $form.Dispose()
    exit 0
}
[void]$form.ShowDialog()
