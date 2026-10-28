@echo off
cd /d "%~dp0..\.."
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "tools\updates\Update-DevTools.ps1" -Mode EnableWeekly -ProjectRoot "%CD%"
pause
