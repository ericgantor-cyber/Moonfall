@echo off
setlocal
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0COLLECT_BUILD_LOGS.ps1"
set "STATUS=%ERRORLEVEL%"
echo.
if not "%STATUS%"=="0" echo Diagnostic packaging failed.
pause
exit /b %STATUS%
