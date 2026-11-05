#define GameName "Moonfall Native"
#define GameVersion "38.0"
#define ExeName "MoonfallNative_v34_4.exe"
#define BuildDir "build"
[Setup]
AppId={{DAB293A9-1CC1-4270-B286-B3569670A24B}
AppName={#GameName}
AppVersion={#GameVersion}
AppPublisher=Moonfall
DefaultDirName={localappdata}\Programs\Moonfall Native
DefaultGroupName=Moonfall Native
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
OutputDir=Release_Windows
OutputBaseFilename=Moonfall_Native_v34_4_Windows_Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayIcon={app}\{#ExeName}
[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
[Tasks]
Name: "desktopicon"; Description: "Create desktop shortcut"; GroupDescription: "Shortcuts:"; Flags: checkedonce
[Files]
Source: "{#BuildDir}\{#ExeName}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#BuildDir}\GODOT_LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "launcher\MoonfallLauncher.ps1"; DestDir: "{app}\launcher"; Flags: ignoreversion
[Icons]
Name: "{autoprograms}\Moonfall Launcher"; Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\launcher\MoonfallLauncher.ps1"""; WorkingDir: "{app}"; IconFilename: "{app}\{#ExeName}"
Name: "{autoprograms}\Moonfall Native (Direct Play)"; Filename: "{app}\{#ExeName}"
Name: "{autodesktop}\Moonfall Launcher"; Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\launcher\MoonfallLauncher.ps1"""; WorkingDir: "{app}"; IconFilename: "{app}\{#ExeName}"; Tasks: desktopicon
[Run]
Filename: "{sys}\WindowsPowerShell\v1.0\powershell.exe"; Parameters: "-NoProfile -ExecutionPolicy Bypass -File ""{app}\launcher\MoonfallLauncher.ps1"""; WorkingDir: "{app}"; Description: "Open Moonfall Launcher"; Flags: nowait postinstall skipifsilent
