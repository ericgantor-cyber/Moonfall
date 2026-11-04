@echo off
setlocal
cd /d "%~dp0.."
title Moonfall - Approve Reviewed AI Artwork
set "MANIFEST=%~1"
if "%MANIFEST%"=="" set "MANIFEST=ai_art_staging\review\moonfall_art_approval.json"
if not exist "%MANIFEST%" (
 echo Approval list not found: %MANIFEST%
 echo In the local art gallery, check reviewed assets and choose Export checked artwork.
 echo Move the downloaded moonfall_art_approval.json file to ai_art_staging\review,
 echo or drag it onto this script.
 pause
 exit /b 1
)
where py >nul 2>nul
if not errorlevel 1 (
 set "PYTHON=py -3"
) else (
 set "PYTHON=python"
)
echo Reviewing selected artwork checksums, provenance and safe paths...
%PYTHON% tools\ai_art_approve_selected.py --manifest "%MANIFEST%"
if errorlevel 1 goto failed
echo.
echo Only approve artwork you have personally inspected in the gallery.
choice /C YN /N /M "Approve these selected PNG and GLB files into Moonfall? [Y/N] "
if errorlevel 2 exit /b 0
%PYTHON% tools\ai_art_approve_selected.py --manifest "%MANIFEST%" --apply
if errorlevel 1 goto failed
echo.
echo Approved! Re-import your Godot project and rebuild the Windows game.
pause
exit /b 0
:failed
echo Approval rejected. No unverified files should be imported.
pause
exit /b 1
