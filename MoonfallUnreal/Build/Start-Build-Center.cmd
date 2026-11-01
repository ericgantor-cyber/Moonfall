@echo off
setlocal
set "MOONFALL_ROOT=%~dp0"
if not exist "%MOONFALL_ROOT%Build-Center.ps1" %* (
  echo Moonfall Build Center script is missing.
  pause
  exit /b 1
)
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%MOONFALL_ROOT%Build-Center.ps1"
if errorlevel 1 (
  echo.
  echo Moonfall Build Center failed to open. Check Windows PowerShell and installation files.
  if /I not "%~1"=="-SelfTest" pause
  exit /b 1
)
endlocal
