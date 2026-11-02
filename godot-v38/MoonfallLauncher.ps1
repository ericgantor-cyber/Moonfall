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
$resolution = '1280x720'
if (Test-Path -LiteralPath $settingsFile) {
  try {
    $saved = Get-Content -LiteralPath $settingsFile -Raw | ConvertFrom-Json
    $fullscreen = [bool]$saved.fullscreen
    if ($saved.resolution -in @('1280x720','1600x900','1920x1080')) { $resolution = [string]$saved.resolution }
  } catch {}
}
$form = New-Object System.Windows.Forms.Form
$form.Text = 'Moonfall Launcher'
$form.ClientSize = New-Object System.Drawing.Size(410,348)
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
$display.SetBounds(32,165,240,30)
$form.Controls.Add($display)
$resLabel = New-Object System.Windows.Forms.Label
$resLabel.Text = 'Window resolution'
$resLabel.ForeColor = [System.Drawing.Color]::White
$resLabel.SetBounds(32,200,145,24)
$form.Controls.Add($resLabel)
$res = New-Object System.Windows.Forms.ComboBox
$res.DropDownStyle = [System.Windows.Forms.ComboBoxStyle]::DropDownList
[void]$res.Items.AddRange([object[]]@('1280x720','1600x900','1920x1080'))
$res.SelectedItem = $resolution
$res.SetBounds(186,196,190,28)
$res.AccessibleName = 'Game window resolution'
$form.Controls.Add($res)
$updates = New-Object System.Windows.Forms.Button
$updates.Text = 'Check for &updates'
$updates.SetBounds(30,239,168,31)
$form.Controls.Add($updates)
$folder = New-Object System.Windows.Forms.Button
$folder.Text = 'Open game &folder'
$folder.SetBounds(208,239,170,31)
$form.Controls.Add($folder)
$status = New-Object System.Windows.Forms.Label
$status.ForeColor = [System.Drawing.Color]::LightGray
$status.SetBounds(30,289,290,26)
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
$cancel.SetBounds(330,289,60,26)
$form.CancelButton = $cancel
$form.Controls.Add($cancel)
$persistSettings = {
  try {
    New-Item -ItemType Directory -Force -Path $settingsDir | Out-Null
    @{fullscreen=[bool]$display.Checked; resolution=[string]$res.SelectedItem} | ConvertTo-Json | Set-Content -LiteralPath $settingsFile -Encoding UTF8
  } catch {
    $status.Text = 'Preferences could not be saved; playing is still possible.'
  }
}
$display.Add_CheckedChanged($persistSettings)
$res.Add_SelectedIndexChanged($persistSettings)
<#
$display.Add_CheckedChanged({
  try {
    New-Item -ItemType Directory -Force -Path $settingsDir | Out-Null
    @{fullscreen=[bool]$display.Checked} | ConvertTo-Json | Set-Content -LiteralPath $settingsFile -Encoding UTF8
  } catch {
    [System.Windows.Forms.MessageBox]::Show('Could not save fullscreen preference. You can still play Moonfall.','Launcher settings') | Out-Null
  }
})
#>
$play.Add_Click({
  if (!(Test-Path -LiteralPath $game -PathType Leaf)) {
    [System.Windows.Forms.MessageBox]::Show('The Moonfall game executable is missing. Reinstall Moonfall.','Moonfall launcher') | Out-Null
    return
  }
  # Validate PE magic before launching; a missing or corrupt download must fail safely.
  try {
    $fs = [System.IO.File]::OpenRead($game)
    try {
      if ($fs.Length -lt 256 -or $fs.ReadByte() -ne 77 -or $fs.ReadByte() -ne 90) { throw 'Invalid Windows executable header.' }
    } finally { $fs.Dispose() }
  } catch {
    [System.Windows.Forms.MessageBox]::Show('Moonfall game file is damaged or unreadable. Reinstall from a verified release. Saves will not be deleted.','Game repair') | Out-Null
    return
  }
  $arguments = if ($display.Checked) { '--fullscreen' } else { '--windowed' }
  if (!$display.Checked) {
    $parts = ([string]$res.SelectedItem).Split('x')
    $arguments += ' --resolution ' + $parts[0] + 'x' + $parts[1]
  }
  try {
    Start-Process -FilePath $game -ArgumentList $arguments -WorkingDirectory $root
    $form.Close()
  } catch {
    [System.Windows.Forms.MessageBox]::Show($_.Exception.Message,'Cannot start Moonfall') | Out-Null
  }
})
$updates.Add_Click({
  try {
    $status.Text = 'Checking GitHub releases...'
    $request = [System.Net.HttpWebRequest]::Create('https://api.github.com/repos/ericgantor-cyber/Moonfall/releases/latest')
    $request.Timeout = 4500
    $request.UserAgent = 'MoonfallLauncher/1.1'
    $response = $request.GetResponse()
    try {
      $reader = New-Object System.IO.StreamReader($response.GetResponseStream())
      try { $latest = $reader.ReadToEnd() | ConvertFrom-Json } finally { $reader.Dispose() }
    } finally { $response.Dispose() }
    $status.Text = ('Latest published release: ' + $latest.tag_name)
    if ([System.Windows.Forms.MessageBox]::Show('Open release page for verified manual update? Saves remain untouched.','Moonfall updates','YesNo') -eq 'Yes') { Start-Process $releasePage }
  } catch {
    $status.Text = 'Update lookup unavailable; opening releases page.'
    try { Start-Process $releasePage } catch {}
    [System.Windows.Forms.MessageBox]::Show('Cannot open releases page. Visit '+$releasePage,'Update link') | Out-Null
  }
})
$folder.Add_Click({
  try { Start-Process -FilePath explorer.exe -ArgumentList ('"' + $root + '"') } catch {
    [System.Windows.Forms.MessageBox]::Show('Could not open the game folder.','Moonfall launcher') | Out-Null
  }
})
[void]$form.ShowDialog()
