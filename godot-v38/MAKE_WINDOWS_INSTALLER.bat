@echo off
setlocal EnableExtensions
cd /d "%~dp0"
call "%~dp0BUILD_WINDOWS.bat"
if errorlevel 1 exit /b 1
set "ISCC=%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe"
if not exist "%ISCC%" set "ISCC=%ProgramFiles%\Inno Setup 6\ISCC.exe"
if not exist "%ISCC%" (
 echo ERROR: Windows installer was NOT created. Install Inno Setup 6 and retry.
 exit /b 1
)
"%ISCC%" "%~dp0Moonfall_Native_v34_4_Setup.iss"
exit /b %errorlevel%
