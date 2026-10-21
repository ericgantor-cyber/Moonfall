@echo off
cd /d "%~dp0"
where node >nul 2>nul
if errorlevel 1 (echo Install Node.js LTS first.&pause&exit /b 1)
node verify-project.js
if errorlevel 1 (echo Project checks FAILED.&pause&exit /b 1)
echo Project checks passed. This does not replace Windows gameplay testing.
pause
