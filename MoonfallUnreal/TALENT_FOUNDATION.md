# UE5 talent foundation

This PR introduces the original 141-node Might/Agility/Arcana constellation as portable, header-only C++17 rules under `Source/MoonfallUnreal/MoonfallTalentRules.h`. It validates rank limits, prerequisite chains, level gates, two earned points per level, and safe respec calculations. `tools/test_unreal_talent_rules.cpp` is compiled and executed in the Unreal source-check workflow.

**Scope:** Talent rules and catalog only. This PR does **not** activate talents in live combat, change saved game formats, show the talent web in gameplay, or alter Z/Nyra art. Those changes remain in stacked draft PRs #1121–#1124 and need reconciliation with current `main` controls and native UE5 Windows testing.

## V13 live-character rank persistence (draft integration)

The UE5 hero now holds a 141-rank in-memory talent ledger. Blueprint-accessible APIs query available points, inspect ranks, spend a talent point, and respec all points. Spending/respec use the same validated C++17 rules and immediately save to the existing world slot; failure rolls the rank ledger back without silently consuming points. Invalid V13 talent arrays (wrong length, negative/oversized ranks, unmet prerequisites, overspent budgets, invalid level gates) prevent restoration **before** checkpoint teleport and block autosave from overwriting the original file. V1-V12 saves load with all ranks at zero and continue to use their established world progress.

**Important gameplay boundary:** This stage persists purchased ranks only; it deliberately does **not** apply talent bonuses to the live hero’s max health, mana, stamina, attack, or spell damage. The HUD talent constellation and its mouse/controller actions still require a separate integration pass. Do not claim this PR produces a playable, fully effective talent tree or a verified native UE5 Windows build. Full UE5 5.4 compile, hands-on spend/respec, loading old saves, invalid-save rejection, and quit/relaunch tests are required before merging this persistence slice.
