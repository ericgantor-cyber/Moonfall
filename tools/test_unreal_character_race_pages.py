"""UE5 C character ancestry / racial passive regression contracts.

This validates source integration without pretending to run the UE5 editor.
"""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "MoonfallUnreal/Source/MoonfallUnreal"
hero = (SRC / "MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
header = (SRC / "MoonfallHeroPawn.h").read_text(encoding="utf-8")
hud = (SRC / "MoonfallCombatHUD.cpp").read_text(encoding="utf-8")
creator = (SRC / "MoonfallCharacterCreationWidget.cpp").read_text(encoding="utf-8")
inputs = (ROOT / "MoonfallUnreal/Config/DefaultInput.ini").read_text(encoding="utf-8")

def section(code, start, end):
    return code.split(start, 1)[1].split(end, 1)[0]

# Preserve twelve stable saved hero race IDs. Creator's 30 world-race rows
# are a separate preview-only roster and must never be mistaken for these IDs.
profiles = section(hero, "constexpr FMoonfallRaceProfile MoonfallRaces[] = {",
                   "const FMoonfallRaceProfile& RaceProfile")
names = re.findall(r'\{TEXT\("([^"]+)"\),\s*\d+,\s*\d+,\s*\d+,\s*[0-9.]+f,\s*[0-9.]+f\}', profiles)
assert names == ["Human", "High Elf", "Wood Elf", "Dwarf", "Halfling",
                 "Orc", "Goblin", "Troll", "Entkin", "Shadowborn",
                 "Moonborn", "Draconid"], names
power_names = hero.split("const TCHAR* MoonfallRacePowerNames[] = {", 1)[1].split(
    "};", 1
)[0]
assert power_names.count('TEXT("') == 12
descriptions = section(hero, "static const TCHAR* Descriptions[] = {",
                       "return FString(Descriptions[FMath::Clamp(RaceId,0,11)]);")
assert descriptions.count('TEXT("') == 12
assert "45-second cooldown" in hud and "RacePowerReadyAt = Now + 45.f;" in hero

# All character info comes from current saved gameplay rank/bonuses, never a
# new copy of race stats that could diverge from live damage and regeneration.
assert 'FString AMoonfallHeroPawn::GetRacePassiveSummary() const' in hero
assert 'FString AMoonfallHeroPawn::GetRacePowerDescription() const' in hero
passives = section(hero, "GetRacePassiveSummary() const {",
                   "FString AMoonfallHeroPawn::GetRacePowerDescription() const")
assert "if (!bRaceChosen)" in passives
assert "const auto& R=RaceProfile(RaceId);" in passives
for prop in ("R.Might", "R.Insight", "R.Resolve",
             "R.StaminaRegen", "R.ManaRegen"):
    assert prop in passives, prop
assert 'bRaceChosen ? RaceProfile(RaceId).Might : 0' in hero
assert 'bRaceChosen ? RaceProfile(RaceId).Insight : 0' in hero
assert 'bRaceChosen ? RaceProfile(RaceId).Resolve : 0' in hero

# Do not introduce a second race-persistence format, reroll the ancestry or
# mutate real hero meshes while browsing the character screen.
race_choose = section(hero, "bool AMoonfallHeroPawn::ChooseRace(int32 NewRaceId) {",
                      "FString AMoonfallHeroPawn::GetRaceName()")
assert "if (bRaceChosen || NewRaceId < 0 || NewRaceId > 11" in race_choose
assert "if (!SaveWorldProgress())" in race_choose
assert "RaceId = OldId;" in race_choose
assert "bRaceChosen = false;" in race_choose
assert "MoonfallTalents::Ranks TalentRanks{};" in header

# The first C press still opens familiar stats. Tab changes ONLY the C page.
assert 'bool bCharacterRacePageOpen = false;' in header
assert 'void ToggleCharacterRacePage();' in header
toggle = section(hero, "void AMoonfallHeroPawn::ToggleCharacterRacePage()",
                 "void AMoonfallHeroPawn::ToggleCharacterStats()")
assert 'bCharacterStatsOpen && bInventoryOpen && !bOptionsOpen' in toggle
assert 'bCharacterRacePageOpen = !bCharacterRacePageOpen;' in toggle
assert 'SaveWorldProgress()' not in toggle and 'ChooseRace(' not in toggle
assert 'bCharacterStatsOpen = true;\n  bCharacterRacePageOpen = false;' in hero
assert 'if (bInventoryOpen && bCharacterStatsOpen) { ToggleCharacterRacePage(); return; }' in hero
assert 'if (bCharacterStatsOpen && bInventoryOpen) { bCharacterRacePageOpen = false; return; }' in hero
assert 'if (bCharacterStatsOpen && bInventoryOpen) { bCharacterRacePageOpen = true; return; }' in hero
assert 'ActionName="CharacterStats",Key=C' in inputs
assert 'ActionName="ChronicleFocus",Key=Tab' in inputs

# Separate page displays live power readiness, duration, passive regeneration
# and 12-ancestry identity; not a static fantasy race profile.
race_ui = section(hud, "if (Hero->bCharacterRacePageOpen) {",
                  "const FString CharacterLines[] = {")
for token in ("Hero->bRaceChosen", "Hero->GetRaceName()",
              "Hero->GetRaceMightBonus()", "Hero->GetRaceInsightBonus()",
              "Hero->GetRaceResolveBonus()", "Hero->GetRaceStaminaRegenBonus()",
              "Hero->GetRaceManaRegenBonus()", "Hero->GetRacePassiveSummary()",
              "Hero->GetRacePowerName()", "Hero->GetRacePowerDescription()",
              "Hero->GetRacePowerCooldownRemaining()",
              "Hero->GetRacePowerDurationRemaining()", "READY", "COOLDOWN",
              "CHOSEN - PERMANENT", "NOT YET CHOSEN"):
    assert token in race_ui, token
assert 'TEXT("MOONFALL  |  ANCESTRY")' in hud
assert 'TEXT("RACE & PASSIVES")' in hud
assert 'TEXT("ATTRIBUTES")' in hud
assert 'Tab / D-pad Left/Right: stats / ancestry' in hud
assert 'DrawLiveMenuHitCue();' in hud

# Mouse tab matches the HUD's right-header button and consumes the click.
mouse = section(hero, "void AMoonfallHeroPawn::ResidentContextMouseClick()",
                "if (!bResidentContextOpen) { RightHandAction(); return; }")
assert "if (bCharacterStatsOpen) {" in mouse
assert "ToggleCharacterRacePage();" in mouse
assert "PanelW<550.f?105.f:172.f" in mouse
assert 'if (HitTestInventorySortButton())' in mouse
assert mouse.index("ToggleCharacterRacePage();") < mouse.index("if (HitTestInventorySortButton())")
# Creator gives comparative, accurately labelled previews only; game applies
# the 12 persistent gameplay ancestries by their existing stable int32 IDs.
assert "RACE TRAITS (PREVIEW)" in creator
assert "Next.Profile.VitalityMultiplier-1.f" in creator
assert "Next.Profile.PowerMultiplier-1.f" in creator
assert "Next.Profile.SpeedMultiplier-1.f" in creator
assert "C > Tab displays your saved passives." in creator
keyboard_handler = creator.split(
    "FReply UMoonfallCharacterCreationWidget::NativeOnKeyDown(", 1)[1]
for control in ("EKeys::Left ||", "EKeys::Right ||", "EKeys::Up ||",
                "EKeys::Down ||", "Key == EKeys::Left)",
                "Key == EKeys::Right)", "Key == EKeys::Up)",
                "Key == EKeys::Down)"):
    assert control not in keyboard_handler, control
print("UE5 character creation, C ancestry and 12 saved racial passive/power contracts passed.")
