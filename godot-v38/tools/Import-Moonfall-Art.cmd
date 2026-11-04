@echo off
setlocal
cd /d "%~dp0.."
title Moonfall AI Art Importer - Offline
if not exist "ai_art_inbox" mkdir "ai_art_inbox"
for %%G in (guardians animals regions chambers glyphs spells gear scenery rigged) do (
 if not exist "ai_art_inbox\%%G" mkdir "ai_art_inbox\%%G"
)
where py >nul 2>nul
if not errorlevel 1 (
 set "PYTHON=py -3"
) else (
 set "PYTHON=python"
)
echo Moonfall AI Art Importer
echo Copy named PNG or GLB files into ai_art_inbox\category folders.
echo This command will stage files for review only. No AI credits or API keys.
%PYTHON% tools\ai_art_intake.py --inbox ai_art_inbox --stage
if errorlevel 1 goto failed
%PYTHON% tools\ai_art_review.py --category all --gallery
if errorlevel 1 goto failed
echo.
echo Review the gallery, check specific PNG/GLB art, and export your selection.
echo Then drag the downloaded moonfall_art_approval.json file onto
echo tools\Approve-Reviewed-Moonfall-Art.cmd to approve reviewed art.
start "" "ai_art_staging\review\index.html"
pause
exit /b 0
:failed
echo.
echo Import could not complete. Check the error above.
pause
exit /b 1
