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
$logs.SetBounds(16,213,790,289)
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
$script:seenOut = [long]0
$script:seenErr = [long]0
$script:outDecoder = [System.Text.Encoding]::UTF8.GetDecoder()
$script:errDecoder = [System.Text.Encoding]::UTF8.GetDecoder()
$script:outLog = ''
$script:errLog = ''
$script:logDir = Join-Path $env:LOCALAPPDATA 'MoonfallBuildCenter\Logs'
New-Item -ItemType Directory -Force -Path $script:logDir | Out-Null
function Begin-Job([string]$scriptName,[bool]$installMissing,[bool]$compileOnly = $false,[bool]$preflightOnly = $false) {
    if ($script:process -and -not $script:process.HasExited) { return }
    $script:seenOut = [long]0
    $script:seenErr = [long]0
    $script:outDecoder.Reset()
    $script:errDecoder.Reset()
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
$bugReports = New-Button 'BUG REPORTS' 16 180
$bugReports.SetBounds(16,174,180,29)
$bugReports.AccessibleName = 'Review local Moonfall bug reports and recovery snapshots'
$bugReports.Add_Click({
    if ($script:process -and -not $script:process.HasExited) {
        [void][Windows.Forms.MessageBox]::Show('Finish or cancel the current build before reviewing reports.','Moonfall bug reports')
        return
    }
    $indexScript = Join-Path $PSScriptRoot 'Get-MoonfallBugReports.ps1'
    if (-not (Test-Path -LiteralPath $indexScript -PathType Leaf)) {
        [void][Windows.Forms.MessageBox]::Show('The report viewer script is missing from this checkout.','Moonfall bug reports')
        return
    }
    try {
        # Use only the local, allowlisted report index. No uploads or raw logs.
        $index = & $indexScript -IncludeInstalled | ConvertFrom-Json -ErrorAction Stop
        $viewer = New-Object Windows.Forms.Form
        try {
            $viewer.Text = 'Moonfall local bug reports'
            $viewer.Size = New-Object Drawing.Size(760,520)
            $viewer.MinimumSize = New-Object Drawing.Size(460,320)
            $viewer.StartPosition = 'CenterParent'
            $viewer.Font = New-Object Drawing.Font('Segoe UI',11)
            $note = New-Object Windows.Forms.Label
            $note.Dock = 'Top'
            $note.Height = 76
            $note.Text = 'Local checkout and standard installed-game report folders. Recovery snapshots may belong to a running session. No report is sent automatically.'
            $note.AccessibleName = 'Local report privacy and recovery limitations'
            $body = New-Object Windows.Forms.TextBox
            $body.Multiline = $true
            $body.ReadOnly = $true
            $body.ScrollBars = 'Both'
            $body.WordWrap = $false
            $body.Dock = 'Fill'
            $body.AccessibleName = 'Read-only bug report summaries'
            $body.Text = if ($index.Count -gt 0) {
                $index.Reports | Format-List Source,File,LastWriteUtc,Kind,Classification,AverageFPS,P95Milliseconds,Hitches | Out-String -Width 160
            } else {
                'No valid reports found in the checked folders. Custom installation or Saved directory overrides may use another location.'
            }
            $close = New-Object Windows.Forms.Button
            $close.Dock = 'Bottom'
            $close.Height = 34
            $close.Text = 'CLOSE'
            $close.AccessibleName = 'Close bug report viewer'
            $close.DialogResult = [Windows.Forms.DialogResult]::Cancel
            $viewer.CancelButton = $close
            $viewer.Controls.Add($body)
            $viewer.Controls.Add($close)
            $viewer.Controls.Add($note)
            [void]$viewer.ShowDialog($form)
        } finally {
            $viewer.Dispose()
        }
    } catch {
        [void][Windows.Forms.MessageBox]::Show('Unable to read the local report index. Existing reports have not been modified.','Moonfall bug reports')
    }
})
# Explicit user-controlled opt-in. Auto-upload is never enabled by a build.
$testUpload = New-Button 'AUTO TEST UPLOAD' 591 230
$testUpload.SetBounds(591,140,230,27)
$testUpload.Anchor = 'Top,Right'
$testUpload.AccessibleName = 'Enable or disable automatic GitHub test summary upload'
$testUpload.Add_Click({
    if ($script:process -and -not $script:process.HasExited) {
        [Windows.Forms.MessageBox]::Show('Finish or cancel the current build first.','Moonfall Build Center')
        return
    }
    $setup = Join-Path $PSScriptRoot 'Install-LocalTestUpload.ps1'
    if (-not (Test-Path -LiteralPath $setup -PathType Leaf)) {
        [Windows.Forms.MessageBox]::Show('Test uploader setup is missing from this Moonfall checkout.','Moonfall Build Center')
        return
    }
    $currentStatus = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $setup -Status 2>&1 | Out-String
    $choice = [Windows.Forms.MessageBox]::Show(
        ($currentStatus.Trim() + [Environment]::NewLine + [Environment]::NewLine) +
        'YES: Enable automatic GitHub test summaries every five minutes.' + [Environment]::NewLine +
        'NO: Disable automatic uploads and remove the scheduled task.' + [Environment]::NewLine +
        'CANCEL: Make no changes.' + [Environment]::NewLine + [Environment]::NewLine +
        'Only small status summaries are uploaded. GitHub CLI must already be installed and authorized.',
        'Moonfall automatic test reports', 'YesNoCancel', 'Question')
    if ($choice -eq [Windows.Forms.DialogResult]::Cancel) { return }
    $args = @('-NoProfile','-ExecutionPolicy','Bypass','-File',$setup)
    if ($choice -eq [Windows.Forms.DialogResult]::No) { $args += '-Uninstall' }
    try {
        # Run setup as the current user, without asking for elevated privileges.
        $result = & powershell.exe @args 2>&1 | Out-String
        if ($LASTEXITCODE -ne 0) {
            throw ('Setup failed. ' + $result.Trim())
        }
        $logs.AppendText(($result.Trim() + [Environment]::NewLine))
        [Windows.Forms.MessageBox]::Show($result,'Moonfall automatic test reports')
    } catch {
        $logs.AppendText(('Automatic test upload setup failed: ' + $_.Exception.Message + [Environment]::NewLine))
        [Windows.Forms.MessageBox]::Show($_.Exception.Message,'Moonfall upload setup failed')
    }
})
$folder.Add_Click({ New-Item -ItemType Directory -Path $out -Force | Out-Null; Start-Process explorer.exe -ArgumentList ('"' + $out + '"') })
$timer.Add_Tick({
    # Read only newly appended bytes, not an ever-growing compiler log
    # every 350 ms. Keep UTF-8 decoder state across reads for split characters.
    foreach ($item in @(@($script:outLog,'seenOut'),@($script:errLog,'seenErr'))) {
        $path = $item[0]
        $key = $item[1]
        if (-not (Test-Path -LiteralPath $path)) { continue }
        $stream = $null
        try {
            $stream = [IO.File]::Open($path, [IO.FileMode]::Open,
                [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)
            $offset = if ($key -eq 'seenOut') { $script:seenOut } else { $script:seenErr }
            $decoder = if ($key -eq 'seenOut') { $script:outDecoder } else { $script:errDecoder }
            if ($stream.Length -lt $offset) {
                # The log was truncated or rotated; do not retain a stale offset.
                $offset = [long]0
                $decoder.Reset()
                $logs.AppendText('[Build log restarted]' + [Environment]::NewLine)
            }
            $stream.Position = $offset
            $buffer = [byte[]]::new(65536)
            $bytesRead = $stream.Read($buffer, 0, $buffer.Length)
            if ($bytesRead -gt 0) {
                $characters = [char[]]::new($bytesRead)
                $characterCount = $decoder.GetChars($buffer, 0, $bytesRead,
                    $characters, 0, $false)
                if ($characterCount -gt 0) {
                    $logs.AppendText([string]::new($characters, 0, $characterCount))
                }
            }
            $offset += $bytesRead
            if ($key -eq 'seenOut') { $script:seenOut = $offset } else { $script:seenErr = $offset }
            # Full details stay in the saved logs; bound the visible textbox
            # so hours of compiler output cannot degrade WinForms responsiveness.
            if ($logs.TextLength -gt 240000) {
                $logs.Text = $logs.Text.Substring($logs.TextLength - 160000)
                $logs.SelectionStart = $logs.TextLength
                $logs.ScrollToCaret()
            }
        } catch [IO.IOException] {
            # The writer may temporarily lock or rotate a log between polls.
        } finally {
            if ($null -ne $stream) { $stream.Dispose() }
        }
    }
    if ($script:process -and $script:process.HasExited) {
        # A fast build can write more than one polling chunk. Drain the saved
        # logs before stopping the timer or reporting the final exit status.
        $outPending = (Test-Path -LiteralPath $script:outLog -PathType Leaf) -and
            ((Get-Item -LiteralPath $script:outLog).Length -gt $script:seenOut)
        $errPending = (Test-Path -LiteralPath $script:errLog -PathType Leaf) -and
            ((Get-Item -LiteralPath $script:errLog).Length -gt $script:seenErr)
        if ($outPending -or $errPending) { return }
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
# First-launch opt-in: never enable uploads without an explicit Yes click.
# Remember declining or postponing, so the dialog cannot nag on every launch.
$form.Add_Shown({
    $setup = Join-Path $PSScriptRoot 'Install-LocalTestUpload.ps1'
    if (-not (Test-Path -LiteralPath $setup -PathType Leaf)) { return }
    $preferenceDirectory = Join-Path $env:LOCALAPPDATA 'MoonfallBuildCenter'
    $preferenceFile = Join-Path $preferenceDirectory 'upload-first-launch-choice.txt'
    if (Test-Path -LiteralPath $preferenceFile -PathType Leaf) { return }
    # Already-enabled users should not see the invitation or have task settings altered.
    $status = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $setup -Status 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0) {
        $logs.AppendText('Unable to check automatic test upload status.' + [Environment]::NewLine)
        return
    }
    if ($status -match 'automatic test uploads: ENABLED') { return }
    $answer = [Windows.Forms.MessageBox]::Show(
        'Would you like Moonfall to automatically report small Windows test and build summaries to GitHub every five minutes?' +
        [Environment]::NewLine + [Environment]::NewLine +
        'Raw logs, screenshots, saves, and personal file paths are not uploaded. GitHub CLI authentication is required.' +
        [Environment]::NewLine + [Environment]::NewLine +
        'YES: Set up reports now.' + [Environment]::NewLine +
        'NO: Not now. You can enable them later with AUTO TEST UPLOAD.',
        'Moonfall: optional test reporting', 'YesNo', 'Question')
    # Save this first-run decision; this file contains no credentials or personal data.
    try {
        New-Item -ItemType Directory -Force -Path $preferenceDirectory | Out-Null
        [IO.File]::WriteAllText($preferenceFile, 'prompted')
    } catch {
        $logs.AppendText('Could not save first-launch preference.' + [Environment]::NewLine)
    }
    if ($answer -ne [Windows.Forms.DialogResult]::Yes) { return }
    try {
        $result = & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $setup 2>&1 | Out-String
        if ($LASTEXITCODE -ne 0) { throw ('Upload setup failed. ' + $result.Trim()) }
        $logs.AppendText(($result.Trim() + [Environment]::NewLine))
        [Windows.Forms.MessageBox]::Show('Automatic test summaries enabled. You can disable them with AUTO TEST UPLOAD.', 'Moonfall')
    } catch {
        $logs.AppendText(('Automatic test upload setup failed: ' + $_.Exception.Message + [Environment]::NewLine))
        [Windows.Forms.MessageBox]::Show(
            'Automatic upload was not enabled. Install and authenticate GitHub CLI, then use AUTO TEST UPLOAD to retry.',
            'Moonfall upload setup')
    }
})
if ($SelfTest) {
    # Exercise real WinForms control construction on a hosted Windows runner.
    $required = @($check, $install, $exportLog, $bugReports, $testUpload, $browseEngine, $preflight, $compile, $build, $cancel, $folder, $diagnostics, $logs, $engine)
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
