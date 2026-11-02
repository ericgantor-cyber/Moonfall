# Moonfall optional Windows launcher. Requires Windows PowerShell 5.1 and WinForms.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
$root = $PSScriptRoot
$game = Join-Path $root 'MoonfallNative_v34_4.exe'
$settingsDir = Join-Path $env:LOCALAPPDATA 'Moonfall'
$settingsFile = Join-Path $settingsDir 'launcher-settings.json'
$releasePage = 'https://github.com/ericgantor-cyber/Moonfall/releases'
$fullscreen = $false
if (Test-Path -LiteralPath $settingsFile) {
  try { $fullscreen = [bool]((Get-Content -LiteralPath $settingsFile -Raw | ConvertFrom-Json).fullscreen) } catch {}
}
$form = New-Object System.Windows.Forms.Form
$form.Text = 'Moonfall Launcher'
$form.ClientSize = New-Object System.Drawing.Size(410,280)
$form.AutoScaleMode = [System.Windows.Forms.AutoScaleMode]::Dpi
$form.StartPosition = 'CenterScreen'
$form.FormBorderStyle = 'FixedDialog'
$form.MaximizeBox = $false
$form.KeyPreview = $true
$form.BackColor = [System.Drawing.Color]::FromArgb(22,18,34)
$title = New-Object System.Windows.Forms.Label
$title.Text = 'MOONFALL'
$title.Font = New-Object System.Drawing.Font('Segoe UI',24,[System.Drawing.FontStyle]::Bold)
$title.ForeColor = [System.Drawing.Color]::White
$title.SetBounds(28,20,350,48)
$form.Controls.Add($title)
$version = New-Object System.Windows.Forms.Label
$version.Text = 'Moonfall Windows | Launcher'
$version.ForeColor = [System.Drawing.Color]::LightGray
$version.SetBounds(30,73,350,23)
$form.Controls.Add($version)
$play = New-Object System.Windows.Forms.Button
$play.Text = '&Play'
$play.SetBounds(30,113,348,44)
$play.BackColor = [System.Drawing.Color]::FromArgb(91,43,130)
$play.ForeColor = [System.Drawing.Color]::White
$play.FlatStyle = 'Flat'
$form.Controls.Add($play)
$form.AcceptButton = $play
$display = New-Object System.Windows.Forms.CheckBox
$display.Text = 'Start in &fullscreen'
$display.ForeColor = [System.Drawing.Color]::White
$display.Checked = $fullscreen
$display.SetBounds(32,170,240,30)
$form.Controls.Add($display)
$updates = New-Object System.Windows.Forms.Button
$updates.Text = 'Check for &updates'
$updates.SetBounds(30,211,168,31)
$form.Controls.Add($updates)
$folder = New-Object System.Windows.Forms.Button
$folder.Text = 'Open game &folder'
$folder.SetBounds(208,211,170,31)
$form.Controls.Add($folder)
$status = New-Object System.Windows.Forms.Label
$status.ForeColor = [System.Drawing.Color]::LightGray
$status.SetBounds(30,246,285,22)
$form.Controls.Add($status)
$play.AccessibleName = 'Play Moonfall'
$display.AccessibleName = 'Start Moonfall in fullscreen'
$updates.AccessibleName = 'Open Moonfall releases page'
$folder.AccessibleName = 'Open Moonfall installation folder'
$status.AccessibleName = 'Moonfall launcher readiness status'
if (Test-Path -LiteralPath $game -PathType Leaf) {
  $status.Text = 'Ready to play'
} else {
  $status.Text = 'Game executable missing. Please reinstall Moonfall.'
  $play.Enabled = $false
}
$form.Add_Shown({ if ($play.Enabled) { $play.Focus() } })
# Escape closes the launcher without affecting the game or saves.
$cancel = New-Object System.Windows.Forms.Button
$cancel.Text = 'Close'
$cancel.DialogResult = [System.Windows.Forms.DialogResult]::Cancel
$cancel.SetBounds(330,246,60,26)
$form.CancelButton = $cancel
$form.Controls.Add($cancel)
$display.Add_CheckedChanged({
  try {
    New-Item -ItemType Directory -Force -Path $settingsDir | Out-Null
    @{fullscreen=[bool]$display.Checked} | ConvertTo-Json | Set-Content -LiteralPath $settingsFile -Encoding UTF8
  } catch {
    [System.Windows.Forms.MessageBox]::Show('Could not save fullscreen preference. You can still play Moonfall.','Launcher settings') | Out-Null
  }
})
$play.Add_Click({
  if (!(Test-Path -LiteralPath $game -PathType Leaf)) {
    [System.Windows.Forms.MessageBox]::Show('The Moonfall game executable is missing. Reinstall Moonfall.','Moonfall launcher') | Out-Null
    return
  }
  $arguments = if ($display.Checked) { '--fullscreen' } else { '--windowed' }
  try {
    Start-Process -FilePath $game -ArgumentList $arguments -WorkingDirectory $root
    $form.Close()
  } catch {
    [System.Windows.Forms.MessageBox]::Show($_.Exception.Message,'Cannot start Moonfall') | Out-Null
  }
})
$updates.Add_Click({
  try { Start-Process $releasePage } catch {
    [System.Windows.Forms.MessageBox]::Show('Cannot open releases page. Visit '+$releasePage,'Update link') | Out-Null
  }
})
$folder.Add_Click({
  try { Start-Process -FilePath explorer.exe -ArgumentList ('"' + $root + '"') } catch {
    [System.Windows.Forms.MessageBox]::Show('Could not open the game folder.','Moonfall launcher') | Out-Null
  }
})
[void]$form.ShowDialog()
