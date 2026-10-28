@echo off
setlocal EnableExtensions
cd /d "%~dp0"
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\updates\Update-DevTools.ps1" -Mode EnableWeekly -ProjectRoot "%~dp0"
set "RESULT=%ERRORLEVEL%"
echo.
if not "%RESULT%"=="0" echo Update task returned error %RESULT%.
pause
exit /b %RESULT%
