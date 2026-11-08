# Moonfall Native Windows launchpad. Uses built-in Windows PowerShell / WinForms.
# No updater, account login, elevated permissions, or gameplay files modified.
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[System.Windows.Forms.Application]::EnableVisualStyles()

$install = Split-Path -Parent $PSScriptRoot
$game = Join-Path $install 'MoonfallNative_v34_4.exe'
$logs = Join-Path $env:APPDATA 'Godot\app_userdata'
$window = New-Object System.Windows.Forms.Form
$window.Text = 'Moonfall | Launcher'
$window.ClientSize = New-Object System.Drawing.Size(690,450)
$window.MinimumSize = New-Object System.Drawing.Size(630,420)
$window.StartPosition = 'CenterScreen'
$window.BackColor = [System.Drawing.Color]::FromArgb(20,17,34)
$window.ForeColor = [System.Drawing.Color]::WhiteSmoke
$window.Font = New-Object System.Drawing.Font('Segoe UI',11)
$window.KeyPreview = $true
$window.FormBorderStyle = 'FixedDialog'
$window.MaximizeBox = $false

$title = New-Object System.Windows.Forms.Label
$title.Text = 'MOONFALL'
$title.Font = New-Object System.Drawing.Font('Segoe UI',30,[System.Drawing.FontStyle]::Bold)
$title.ForeColor = [System.Drawing.Color]::FromArgb(236,148,211)
$title.TextAlign = 'MiddleCenter'
$title.SetBounds(20,24,650,72)
$window.Controls.Add($title)

$subtitle = New-Object System.Windows.Forms.Label
$subtitle.Text = 'NATIVE EDITION  |  WINDOWS 10 / 11'
$subtitle.ForeColor = [System.Drawing.Color]::FromArgb(186,184,218)
$subtitle.TextAlign = 'MiddleCenter'
$subtitle.SetBounds(30,100,630,30)
$window.Controls.Add($subtitle)

$status = New-Object System.Windows.Forms.Label
$status.TextAlign = 'MiddleCenter'
$status.SetBounds(30,149,630,54)
$status.AccessibleName = 'Moonfall game installation status'
$window.Controls.Add($status)

function New-MoonfallButton([string]$text,[string]$accessible,[int]$x,[int]$y,[int]$width,[int]$height) {
 $button = New-Object System.Windows.Forms.Button
 $button.Text = $text
 $button.AccessibleName = $accessible
 $button.SetBounds($x,$y,$width,$height)
 $button.FlatStyle = 'Flat'
 $button.FlatAppearance.BorderSize = 1
 $button.BackColor = [System.Drawing.Color]::FromArgb(56,39,78)
 $button.ForeColor = [System.Drawing.Color]::WhiteSmoke
 $window.Controls.Add($button)
 return $button
}

$play = New-MoonfallButton 'PLAY MOONFALL' 'Launch Moonfall game' 200 213 290 54
$play.Font = New-Object System.Drawing.Font('Segoe UI',14,[System.Drawing.FontStyle]::Bold)
$check = New-MoonfallButton 'CHECK GAME FILES' 'Check game executable is present' 45 293 193 43
$folder = New-MoonfallButton 'GAME FOLDER' 'Open game installation folder' 248 293 193 43
$logButton = New-MoonfallButton 'GAME LOGS' 'Open Godot user log directory' 451 293 193 43
$help = New-MoonfallButton 'HELP / CONTROLS' 'Show Moonfall launcher help' 146 355 193 41
$exit = New-MoonfallButton 'EXIT' 'Close Moonfall launcher' 350 355 193 41

function Update-MoonfallStatus {
 if (Test-Path -LiteralPath $game -PathType Leaf) {
  $status.Text = 'READY: Moonfall executable found. Press Play to begin.'
  $status.ForeColor = [System.Drawing.Color]::FromArgb(156,235,188)
  $play.Enabled = $true
 } else {
  $status.Text = 'GAME NOT FOUND: Reinstall Moonfall using the complete Windows installer.'
  $status.ForeColor = [System.Drawing.Color]::FromArgb(255,172,168)
  $play.Enabled = $false
 }
}
$check.Add_Click({ Update-MoonfallStatus })
$play.Add_Click({
 try {
  if (-not (Test-Path -LiteralPath $game -PathType Leaf)) {
   Update-MoonfallStatus
   return
  }
  Start-Process -FilePath $game -WorkingDirectory $install -ErrorAction Stop
  $status.Text = 'Moonfall launched. Settings and camera FOV are inside the game (Esc).'
 } catch {
  [System.Windows.Forms.MessageBox]::Show(
   "Moonfall could not start: $($_.Exception.Message)`nCheck the installation and game logs.",
   'Launch error','OK','Error') | Out-Null
  Update-MoonfallStatus
 }
})
$folder.Add_Click({ Start-Process -FilePath 'explorer.exe' -ArgumentList @($install) })
$logButton.Add_Click({
 if (Test-Path -LiteralPath $logs -PathType Container) {
  Start-Process -FilePath 'explorer.exe' -ArgumentList @($logs)
 } else {
  [System.Windows.Forms.MessageBox]::Show(
   'No Godot user log directory was found yet. Launch the game first, then retry.',
   'Game logs','OK','Information') | Out-Null
 }
})
$help.Add_Click({
 [System.Windows.Forms.MessageBox]::Show(
  "PLAY starts the installed Moonfall game.`n" +
  "Esc opens Moonfall's Atlas and Settings. Camera FOV is adjusted inside System > Camera Settings.`n" +
  "Use the in-game Help / Controls screen for the currently configured key bindings.`n" +
  "CHECK GAME FILES only checks that the game executable exists; it does not verify integrity.`n" +
  "F5 checks files again. Enter starts the game. Escape closes this launcher.",
  'Moonfall help','OK','Information') | Out-Null
})
$exit.Add_Click({ $window.Close() })
$window.Add_KeyDown({
 param($sender,$event)
 if ($event.KeyCode -eq [System.Windows.Forms.Keys]::F5) {
  Update-MoonfallStatus
  $event.Handled = $true
 }
 if ($event.KeyCode -eq [System.Windows.Forms.Keys]::Escape) {
  $window.Close()
  $event.Handled = $true
 }
})
$window.AcceptButton = $play
Update-MoonfallStatus
[void]$window.ShowDialog()
$window.Dispose()
