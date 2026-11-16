#define MyAppName "Moonfall 3D"
#define MyAppVersion "0.1.0-experimental"
#define MyAppExeName "Moonfall3D.exe"

[Setup]
AppId={{F48CA341-A3F0-4BD4-89B9-CE3E13929434}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
DefaultDirName={autopf}\Moonfall3D
DefaultGroupName={#MyAppName}
OutputDir=..\installer-output
OutputBaseFilename=Moonfall3D-Setup
Compression=lzma
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
UninstallDisplayIcon={app}\{#MyAppExeName}

[Files]
Source: "..\dist\Moonfall3D.exe"; DestDir: "{app}"; Flags: ignoreversion

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Icons]
Name: "{group}\Moonfall 3D"; Filename: "{app}\{#MyAppExeName}"
Name: "{autodesktop}\Moonfall 3D"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch Moonfall 3D"; Flags: nowait postinstall skipifsilent
