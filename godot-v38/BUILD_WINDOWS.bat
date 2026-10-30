@echo off
setlocal EnableExtensions
cd /d "%~dp0"
set "ENGINE=%~dp0Godot_4.7.2\Godot_v4.7.2-stable_win64.exe"
rem Developer tool updater may validate and activate a newer 4.7.x patch side-by-side.
rem Explicit MOONFALL_GODOT_EXE environment variable takes precedence.
if defined MOONFALL_GODOT_EXE set "ENGINE=%MOONFALL_GODOT_EXE%"
if not defined MOONFALL_GODOT_EXE if exist "%LOCALAPPDATA%\MoonfallDev\active_godot.txt" (
 for /f "usebackq delims=" %%G in ("%LOCALAPPDATA%\MoonfallDev\active_godot.txt") do set "ENGINE=%%G"
)
if not exist "%ENGINE%" (
 echo ERROR: Copy official Godot_v4.7.2-stable_win64.exe into Godot_4.7.2 next to this script.
 exit /b 1
)
python tests\verify_official_windows_export.py
if errorlevel 1 goto failed
python tests\verify_build_test_manifest.py
if errorlevel 1 goto failed
python tests\verify_godot_log_gate.py
if errorlevel 1 goto failed
python tests\verify_engine_test_parity.py
if errorlevel 1 goto failed
python tests\verify_v35_release_gate.py
if errorlevel 1 goto failed
python tests\verify_v37_5_lean.py
if errorlevel 1 goto failed
python tests\verify_v37_6_all_tips.py
if errorlevel 1 goto failed
python tests\verify_v37_7_playability.py
if errorlevel 1 goto failed
python tests\verify_legacy_release_guards.py
if errorlevel 1 goto failed
python tests\verify_v34_4_release.py
if errorlevel 1 goto failed
python tests\verify_v34_1_invasions.py
if errorlevel 1 goto failed
python tests\verify_v34_2_elemental.py
if errorlevel 1 goto failed
python tests\verify_v34_3_lunar_war.py
if errorlevel 1 goto failed
python tests\verify_v34_overhaul.py
if errorlevel 1 goto failed
python tests\verify_world_depth.py
if errorlevel 1 goto failed
python tests\verify_living_rifts.py
if errorlevel 1 goto failed
python tests\verify_publish_handoff.py
if errorlevel 1 goto failed
if not exist build mkdir build
rem Checkpoint-writing regression tests fail closed unless user data points to a sandbox.
set "MOONFALL_TEST_PROFILE=%CD%\build\test-profile"
set "APPDATA=%CD%\build\test-profile\Roaming"
set "LOCALAPPDATA=%CD%\build\test-profile\Local"
if not exist "%APPDATA%" mkdir "%APPDATA%"
if not exist "%LOCALAPPDATA%" mkdir "%LOCALAPPDATA%"
"%ENGINE%" --headless --editor --path "%CD%" --import --quit > GODOT_IMPORT_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /i /c:"SCRIPT ERROR" /c:"Parse Error" GODOT_IMPORT_LOG.txt >nul
if not errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/smoke_v11.gd > GODOT_SMOKE_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL NATIVE v11 SMOKE PASS" GODOT_SMOKE_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v13_features.gd > GODOT_V13_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL NATIVE v13 FEATURES PASS" GODOT_V13_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v14_visuals.gd > GODOT_VISUAL_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL NATIVE v14 VISUALS PASS" GODOT_VISUAL_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v15_living_art.gd > GODOT_V15_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL NATIVE v15 LIVING ART PASS" GODOT_V15_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v16_visuals.gd > GODOT_V16_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL NATIVE v16 VISUALS PASS" GODOT_V16_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v17_release.gd > GODOT_V17_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL NATIVE v17 RELEASE PASS" GODOT_V17_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v18_grid.gd > GODOT_V18_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL NATIVE v18 GRID PASS" GODOT_V18_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v18_stability.gd > GODOT_V18STABILITY_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v18 STABILITY PASS" GODOT_V18STABILITY_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v19_world_art.gd > GODOT_V19WORLDART_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v19 WORLD ART PASS" GODOT_V19WORLDART_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v20_guardians.gd > GODOT_V20GUARDIANS_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v20 GUARDIANS PASS" GODOT_V20GUARDIANS_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v21_grand_art.gd > GODOT_V21ART_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v21 GRAND ART PASS" GODOT_V21ART_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v22_art.gd > GODOT_V24ART_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v22 ART PASS" GODOT_V24ART_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v23_animation.gd > GODOT_V23_ANIMATION_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v23 ANIMATION PASS" GODOT_V23_ANIMATION_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v24_overhaul.gd > GODOT_V24_OVERHAUL_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v24 OVERHAUL PASS" GODOT_V24_OVERHAUL_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v24_1_accessibility.gd > GODOT_V24_1_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v24.1 ACCESSIBILITY PASS" GODOT_V24_1_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v25_legacy_modernization.gd > GODOT_V25_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v25 LEGACY MODERNIZATION PASS" GODOT_V25_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v26_modular_core.gd > GODOT_V26_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v26 MODULAR CORE PASS" GODOT_V26_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/live_v11.gd > GODOT_LIVE_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL NATIVE v11 LIVE PASS" GODOT_LIVE_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v27_system_modules.gd > GODOT_V30_MODULES_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v27 SYSTEM MODULES PASS" GODOT_V30_MODULES_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v28_ai_scheduler.gd > GODOT_V30_AI_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v28 AI SCHEDULER PASS" GODOT_V30_AI_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v29_performance.gd > GODOT_V30_PERF_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v29 PERFORMANCE PASS" GODOT_V30_PERF_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v30_release.gd > GODOT_V30_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v30 RELEASE PASS" GODOT_V30_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v30_1_profiler.gd > GODOT_V30_1_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v30.1 PROFILER PASS" GODOT_V30_1_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v30_3_world_cycle.gd > GODOT_V30_3_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v30.3 WORLD CYCLE PASS" GODOT_V30_3_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v30_4_inventory_rules.gd > GODOT_V30_4_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v30.4 INVENTORY RULES PASS" GODOT_V30_4_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v30_5_inventory_expansion.gd > GODOT_V30_5_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v30.5 INVENTORY EXPANSION PASS" GODOT_V30_5_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v30_6_money_economy.gd > GODOT_V30_6_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v30.6 CURRENCY & MERCHANT PASS" GODOT_V30_6_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v30_7_forge_treasure.gd > GODOT_V30_7_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v30.7 FORGE & TREASURE PASS" GODOT_V30_7_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v30_8_shop_ui.gd > GODOT_V30_8_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v30.8 SHOP UI PASS" GODOT_V30_8_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v30_9_graphics.gd > GODOT_V30_9_GRAPHICS_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v30.9 GRAPHICS PASS" GODOT_V30_9_GRAPHICS_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v31_0_graphics.gd > GODOT_V31_0_GRAPHICS_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v31.0 GRAPHICS PASS" GODOT_V31_0_GRAPHICS_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v34_4_4k_display.gd > GODOT_4K_DISPLAY_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL 4K DISPLAY PASS" GODOT_4K_DISPLAY_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v31_1_realism.gd > GODOT_V31_1_REALISM_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v31.1 REALISM PASS" GODOT_V31_1_REALISM_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v31_2_visual_performance.gd > GODOT_V31_4_VISUAL_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v31.2 VISUAL PERFORMANCE PASS" GODOT_V31_4_VISUAL_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v31_2_presets.gd > GODOT_V31_4_PRESETS_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v31.2 PRESET SWEEP PASS" GODOT_V31_4_PRESETS_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v31_3_gameplay.gd > GODOT_V31_3_GAMEPLAY_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v31.3 GAMEPLAY PASS" GODOT_V31_3_GAMEPLAY_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v31_3_art.gd > GODOT_V31_4_ART_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v31.3 ART PASS" GODOT_V31_4_ART_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v31_4_bestiary.gd > GODOT_V31_4_BESTIARY_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v31.4 BESTIARY PASS" GODOT_V31_4_BESTIARY_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v31_5_living_meshes.gd > GODOT_V31_5_CREATURE_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v31.5 LIVING MESH PASS" GODOT_V31_5_CREATURE_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v31_6_anatomy_ai.gd > GODOT_V31_6_ANATOMY_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v31.6 ANATOMY AND AI PASS" GODOT_V31_6_ANATOMY_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v31_6_second_pass.gd > GODOT_V31_6_SECOND_PASS_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v31.6 SECOND PASS PASS" GODOT_V31_6_SECOND_PASS_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v32_0_world_reborn.gd > GODOT_V32_WORLD_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v32 WORLD REBORN PASS" GODOT_V32_WORLD_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v32_0_stream_budget.gd > GODOT_V32_STREAM_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v32 STREAM BUDGET PASS" GODOT_V32_STREAM_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v32_1_world_reborn_stage2.gd > GODOT_V32_1_STAGE2_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v32.1 STAGE 2 PASS" GODOT_V32_1_STAGE2_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v32_2_lean_chambers.gd > GODOT_V32_2_CHAMBERS_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"v32.2 PASS" GODOT_V32_2_CHAMBERS_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v33_8_death_resurrection.gd > GODOT_DEATH_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v33.8 DEATH PASS" GODOT_DEATH_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v33_8_moon_sigil_forge.gd > GODOT_SIGIL_FORGE_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOON SIGIL FORGE PASS" GODOT_SIGIL_FORGE_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v33_8_world_depth.gd > GODOT_WORLD_DEPTH_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL WORLD DEPTH PASS" GODOT_WORLD_DEPTH_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v33_8_living_rifts.gd > GODOT_LIVING_RIFTS_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL LIVING RIFTS PASS" GODOT_LIVING_RIFTS_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v34_0_grand_overhaul.gd > GODOT_V34_OVERHAUL_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v34.0 GRAND OVERHAUL PASS" GODOT_V34_OVERHAUL_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v34_1_dynamic_invasions.gd > GODOT_V34_1_INVASIONS_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v34.1 DYNAMIC INVASIONS PASS" GODOT_V34_1_INVASIONS_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v34_2_elemental_invasions.gd > GODOT_V34_2_ELEMENTAL_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v34.2 ELEMENTAL INVASIONS PASS" GODOT_V34_2_ELEMENTAL_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v34_3_lunar_war.gd > GODOT_V34_3_WAR_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v34.3 LUNAR WAR PASS" GODOT_V34_3_WAR_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v34_4_fair_fight.gd > GODOT_V34_4_FAIR_FIGHT_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v34.4 FAIR FIGHT PASS" GODOT_V34_4_FAIR_FIGHT_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v34_4_invasion_guard.gd > GODOT_INVASION_GUARD_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL INVASION GUARD PASS" GODOT_INVASION_GUARD_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v36_0_third_person_3d.gd > GODOT_3D_CAMERA_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL 3D CAMERA PASS" GODOT_3D_CAMERA_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v36_1_world_art.gd > GODOT_WORLD_ART_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v36.1 WORLD ART PASS" GODOT_WORLD_ART_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v36_2_water_professions.gd > GODOT_WATER_PROFESSIONS_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v36.2 WATER PROFESSIONS PASS" GODOT_WATER_PROFESSIONS_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v36_3_living_world.gd > GODOT_LIVING_WORLD_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v36.3 LIVING WORLD PASS" GODOT_LIVING_WORLD_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v37_0_living_realm.gd > GODOT_LIVING_REALM_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v37 LIVING REALM PASS" GODOT_LIVING_REALM_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v37_2_oxygen.gd > GODOT_V37_2_OXYGEN_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v37.2 OXYGEN PASS" GODOT_V37_2_OXYGEN_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v37_3_deep_water.gd > GODOT_V37_3_DEEP_WATER_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v37.3 DEEP WATER PASS" GODOT_V37_3_DEEP_WATER_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v37_4_solid_grottos.gd > GODOT_V37_4_SOLID_GROTTOS_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v37.4 SOLID GROTTOS PASS" GODOT_V37_4_SOLID_GROTTOS_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v37_6_all_tips.gd > GODOT_V37_6_TIPS_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v37.6 ALL TIPS PASS" GODOT_V37_6_TIPS_LOG.txt >nul
if errorlevel 1 goto failed
rem A real exported Windows game requires Godot's official Windows x64 export template.
rem The Windows CI installs it with INSTALL_GODOT_WINDOWS_TEMPLATES.ps1.
rem Gameplay tests deliberately redirected APPDATA above. Reinstall from SHA-verified
rem local cache into that isolated profile so Godot can locate templates at export.
set "GODOT_TEMPLATE_SOURCE=%~dp0Godot_4.7.2\export_templates\4.7.2.stable\windows_release_x86_64.exe"
if not exist "%GODOT_TEMPLATE_SOURCE%" goto failed
set "GODOT_TEMPLATE_DIR=%APPDATA%\Godot\export_templates\4.7.2.stable"
if not exist "%GODOT_TEMPLATE_DIR%" mkdir "%GODOT_TEMPLATE_DIR%"
if errorlevel 1 goto failed
copy /y "%GODOT_TEMPLATE_SOURCE%" "%GODOT_TEMPLATE_DIR%\windows_release_x86_64.exe" >nul
if errorlevel 1 goto failed
if not exist "%~dp0Godot_4.7.2\export_templates\4.7.2.stable\windows_debug_x86_64.exe" goto failed
copy /y "%~dp0Godot_4.7.2\export_templates\4.7.2.stable\windows_debug_x86_64.exe" "%GODOT_TEMPLATE_DIR%\windows_debug_x86_64.exe" >nul
if errorlevel 1 goto failed
set "GODOT_TEMPLATE=%GODOT_TEMPLATE_DIR%\windows_release_x86_64.exe"
if not exist "%GODOT_TEMPLATE%" (
 echo ERROR: Missing Godot Windows x64 release export template.
 echo Install it with INSTALL_GODOT_WINDOWS_TEMPLATES.ps1 before building.
 goto failed
)
"%ENGINE%" --headless --path "%CD%" --export-release "Windows Desktop" "%CD%\build\MoonfallNative_v34_4.exe" > GODOT_EXPORT_LOG.txt 2>&1
if errorlevel 1 goto failed
if not exist build\MoonfallNative_v34_4.exe goto failed
findstr /i /c:"SCRIPT ERROR" /c:"Parse Error" /c:"ERROR:" GODOT_EXPORT_LOG.txt >nul
if not errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v37_7_playability.gd > GODOT_V37_7_PLAYABILITY_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v37.7 PLAYABILITY PASS" GODOT_V37_7_PLAYABILITY_LOG.txt >nul
if errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v38_0_overhaul.gd > GODOT_V38_0_OVERHAUL_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v38.0 OVERHAUL PASS" GODOT_V38_0_OVERHAUL_LOG.txt >nul
if errorlevel 1 goto failed
rem The export preset embeds all game data directly into the official release executable.
if exist build\MoonfallNative_v34_4.pck goto failed
copy /y GODOT_LICENSE.txt build\GODOT_LICENSE.txt >nul
"%CD%\build\MoonfallNative_v34_4.exe" --headless --quit-after 300 > GODOT_PACKED_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /i /c:"SCRIPT ERROR" /c:"Parse Error" GODOT_PACKED_LOG.txt >nul
if not errorlevel 1 goto failed
"%ENGINE%" --headless --path "%CD%" --script res://tests/v38_2_procedural.gd > GODOT_V38_2_PROCEDURAL_LOG.txt 2>&1
if errorlevel 1 goto failed
findstr /c:"MOONFALL v38.2 PROCEDURAL PASS" GODOT_V38_2_PROCEDURAL_LOG.txt >nul
if errorlevel 1 goto failed
rem A script may print its PASS banner while Godot also emits ERROR diagnostics.
rem Inspect all import, gameplay, export and packed startup logs before success.
python tests\check_godot_logs.py
if errorlevel 1 goto failed
 echo SUCCESS: build\MoonfallNative_v34_4.exe - one file contains game and data.
 echo Windows gameplay and signature still require user testing.
exit /b 0
:failed
 echo ERROR: Build failed. Open the newest GODOT_*_LOG.txt or run tests\verify_build_test_manifest.py to inspect test dependencies.
exit /b 1
