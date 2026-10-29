@echo off
setlocal EnableExtensions
cd /d "%~dp0"
echo ===================================================
echo  MOONFALL v38.1 - PLAYABLE RELEASE GATE
echo ===================================================
echo.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0BUILD_MOONFALL_ONE_CLICK.ps1"
set "STATUS=%ERRORLEVEL%"
echo.
if not "%STATUS%"=="0" (
 echo FAILED: No verified release was produced.
 echo Run COLLECT_BUILD_LOGS.bat if you want a diagnostic ZIP.
) else (
 echo The build command completed. See the status file and verification logs.
)
echo.
pause
exit /b %STATUS%
