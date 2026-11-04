@echo off
setlocal
cd /d "%~dp0.."
title Moonfall AI Art - Automatic Audit
where py >nul 2>nul
if not errorlevel 1 (
 set "PYTHON=py -3"
) else (
 set "PYTHON=python"
)
echo.
echo MOONFALL AUTOMATIC AI ART AUDIT
echo Checking approved sprites, rigged GLBs, 4K maps, model geometry,
echo animation clips, staged art, approval signatures and import inbox.
echo This runs offline: NO paid AI generation or user data uploads.
echo.
%PYTHON% tools\ai_art_auto_audit.py --strict --inbox
set "AUDIT_RESULT=%ERRORLEVEL%"
if exist "ai_art_staging\review\auto_audit.html" start "" "ai_art_staging\review\auto_audit.html"
echo.
if "%AUDIT_RESULT%"=="0" (
 echo Audit completed. Review warnings in the local HTML dashboard.
) else (
 echo Audit detected invalid files. Review the errors in the local HTML dashboard.
)
pause
exit /b %AUDIT_RESULT%
