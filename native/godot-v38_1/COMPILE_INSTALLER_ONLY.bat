@echo off
setlocal EnableExtensions
cd /d "%~dp0"
if not exist build mkdir build
if not exist "build\MoonfallNative_v34_4.exe" (
 echo ERROR: Place MoonfallNative_v34_4.exe in the build folder first.
 echo This batch compiles the installer from an already verified executable.
 exit /b 1
)
copy /y "GODOT_LICENSE.txt" "build\GODOT_LICENSE.txt" >nul
set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if not exist "%ISCC%" set "ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe"
if not exist "%ISCC%" (
 echo ERROR: Install Inno Setup 6 to compile a Windows installer.
 exit /b 1
)
"%ISCC%" "%~dp0Moonfall_Native_v34_4_Setup.iss"
if errorlevel 1 exit /b 1
if not exist "Release_Windows\Moonfall_Native_v34_4_Windows_Setup.exe" exit /b 1
echo SUCCESS: Release_Windows\Moonfall_Native_v34_4_Windows_Setup.exe
exit /b 0
