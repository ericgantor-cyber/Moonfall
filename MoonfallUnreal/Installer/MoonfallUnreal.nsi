Unicode True
!ifndef SOURCE_DIR
  !error "Define SOURCE_DIR as the directory containing MoonfallUnreal.exe"
!endif
!ifndef OUTPUT_EXE
  !error "Define OUTPUT_EXE for the generated installer"
!endif
; Fail during installer compilation instead of installing a partial payload.
; The shipping workflow supplies fully cooked Unreal output, not the synthetic CI fixture.
!ifexist "${SOURCE_DIR}\\MoonfallUnreal.exe"
!else
  !error "MoonfallUnreal.exe missing from installer source."
!endif
!ifexist "${SOURCE_DIR}\\MoonfallLauncher.exe"
!else
  !error "MoonfallLauncher.exe missing from installer source."
!endif
!ifexist "${SOURCE_DIR}\\Engine"
!else
  !error "Unreal Engine runtime folder missing from installer source."
!endif
!ifexist "${SOURCE_DIR}\\MoonfallUnreal"
!else
  !error "Moonfall cooked content folder missing from installer source."
!endif
Name "Moonfall Unreal"
OutFile "${OUTPUT_EXE}"
InstallDir "$LOCALAPPDATA\\Programs\\MoonfallUnreal"
RequestExecutionLevel user
ShowInstDetails show
ShowUninstDetails show
Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles
Section "Moonfall Unreal" Main
  SetOutPath "$INSTDIR"
  File /r "${SOURCE_DIR}\\*.*"
  IfFileExists "$INSTDIR\\MoonfallUnreal.exe" +2 0
    Abort "Missing game executable"
  IfFileExists "$INSTDIR\\Engine\\*.*" +2 0
    Abort "Missing packaged Unreal runtime files"
  IfFileExists "$INSTDIR\\MoonfallUnreal\\*.*" +2 0
    Abort "Missing cooked game content"
  IfFileExists "$INSTDIR\\MoonfallLauncher.exe" +2 0
    Abort "Missing Moonfall launcher"
  CreateDirectory "$SMPROGRAMS\\Moonfall Unreal"
  CreateShortcut "$SMPROGRAMS\\Moonfall Unreal\\Moonfall Unreal.lnk" "$INSTDIR\\MoonfallLauncher.exe"
  CreateShortcut "$DESKTOP\\Moonfall Unreal.lnk" "$INSTDIR\\MoonfallLauncher.exe"
  WriteUninstaller "$INSTDIR\\Uninstall.exe"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MoonfallUnreal" "DisplayName" "Moonfall Unreal"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MoonfallUnreal" "UninstallString" '$\"$INSTDIR\\Uninstall.exe$\"'
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MoonfallUnreal" "InstallLocation" "$INSTDIR"
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MoonfallUnreal" "NoModify" 1
  WriteRegDWORD HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MoonfallUnreal" "NoRepair" 1
SectionEnd
Section "Uninstall"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\MoonfallUnreal"
  Delete "$DESKTOP\\Moonfall Unreal.lnk"
  Delete "$SMPROGRAMS\\Moonfall Unreal\\Moonfall Unreal.lnk"
  RMDir "$SMPROGRAMS\\Moonfall Unreal"
  RMDir /r "$INSTDIR"
SectionEnd
