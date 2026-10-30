@echo off
setlocal EnableExtensions
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0PUBLISH_NATIVE_TO_GITHUB.ps1"
if errorlevel 1 (
 echo.
 echo ERROR: GitHub publishing did not complete. Read the message above.
) else (
 echo.
 echo Done. Open the pull request URL shown above to start Windows CI.
)
pause
