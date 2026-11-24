# UE5 talent foundation

This PR introduces the original 141-node Might/Agility/Arcana constellation as portable, header-only C++17 rules under `Source/MoonfallUnreal/MoonfallTalentRules.h`. It validates rank limits, prerequisite chains, level gates, two earned points per level, and safe respec calculations. `tools/test_unreal_talent_rules.cpp` is compiled and executed in the Unreal source-check workflow.

**Scope:** Talent rules and catalog only. This PR does **not** activate talents in live combat, change saved game formats, show the talent web in gameplay, or alter Z/Nyra art. Those changes remain in stacked draft PRs #1121–#1124 and need reconciliation with current `main` controls and native UE5 Windows testing.
