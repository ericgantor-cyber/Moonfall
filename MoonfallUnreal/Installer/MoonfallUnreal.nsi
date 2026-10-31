Unicode True
!ifndef SOURCE_DIR
  !error "Define SOURCE_DIR as the directory containing MoonfallUnreal.exe"
!endif
!ifndef OUTPUT_EXE
  !error "Define OUTPUT_EXE for the generated installer"
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
  CreateShortcut "$SMPROGRAMS\\Moonfall Unreal\\Moonfall Unreal.lnk" "$INSTDIR\\MoonfallUnreal.exe"
  CreateShortcut "$DESKTOP\\Moonfall Unreal.lnk" "$INSTDIR\\MoonfallUnreal.exe"
  WriteUninstaller "$INSTDIR\\Uninstall.exe"
SectionEnd
Section "Uninstall"
  Delete "$DESKTOP\\Moonfall Unreal.lnk"
  Delete "$SMPROGRAMS\\Moonfall Unreal\\Moonfall Unreal.lnk"
  RMDir "$SMPROGRAMS\\Moonfall Unreal"
  RMDir /r "$INSTDIR"
SectionEnd
