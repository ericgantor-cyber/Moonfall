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

# The first C press opens Attributes; C cycles Ancestry, Equipment and Reputation.
# Tab always belongs to Atlas Chronicle. Only one page can be selected.
assert 'bool bCharacterRacePageOpen = false;' in header
assert 'bool bCharacterGearPageOpen = false;' in header
assert 'bool bCharacterRepPageOpen = false;' in header
assert 'void SelectCharacterPage(int32 Page);' in header
assert 'void ToggleCharacterRacePage();' in header
select_page = section(hero,"void AMoonfallHeroPawn::SelectCharacterPage(int32 Page)",
                      "void AMoonfallHeroPawn::ToggleCharacterRacePage()")
assert 'Page < 0 || Page > 3' in select_page
assert 'bCharacterStatsOpen' in select_page and 'bInventoryOpen' in select_page
assert 'bCharacterRacePageOpen = Page == 1;' in select_page
assert 'bCharacterGearPageOpen = Page == 2;' in select_page
assert 'bCharacterRepPageOpen = Page == 3;' in select_page
assert 'ChooseRace(' not in select_page and 'SaveWorldProgress()' not in select_page
assert 'SelectCharacterPage(bCharacterRacePageOpen ? 2 : (bCharacterGearPageOpen ? 3 : (bCharacterRepPageOpen ? 0 : 1)));' in hero
stats_action = section(hero,"void AMoonfallHeroPawn::ToggleCharacterStats()",
                       "void AMoonfallHeroPawn::ToggleAtlasChronicleFocus()")
assert 'if (bCharacterStatsOpen) { ToggleCharacterRacePage(); return; }' in stats_action
assert 'bCharacterStatsOpen = true;' in stats_action
atlas_action = section(hero,"void AMoonfallHeroPawn::ToggleAtlasChronicleFocus()",
                       "void AMoonfallHeroPawn::StepAtlasChronicle(")
assert 'ToggleCharacterRacePage();' not in atlas_action
assert 'SelectCharacterPage(bCharacterRacePageOpen ? 0 : (bCharacterGearPageOpen ? 1 : (bCharacterRepPageOpen ? 2 : 3)));' in hero
assert 'SelectCharacterPage(bCharacterRacePageOpen ? 2 : (bCharacterGearPageOpen ? 3 : (bCharacterRepPageOpen ? 0 : 1)));' in hero
assert 'ActionName="CharacterStats",Key=C' in inputs
assert 'ActionName="ChronicleFocus",Key=Tab' in inputs
assert 'bCharacterGearPageOpen = false;' in hero
assert 'bCharacterRepPageOpen = false;' in hero

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
assert 'TEXT("ANCESTRY & PASSIVES")' in hud
# The WoW Forever-inspired UI groups live values; it must not replace them
# with fake/static paper-doll combat values or alter character bindings.
assert 'const float StatPanelWidth = PanelWidth - 30.f;' in hud
assert 'if (RowY+RowHeight > Top+PanelHeight-65.f) break;' in hud
assert 'if (RowY+RaceRowHeight > Top+PanelHeight-65.f) break;' in hud
assert 'const bool bSection = Row == 1 || Row == 5 || Row == 8 || Row == 12 || Row == 16;' in hud
assert 'const bool bSection = Row==0 || Row==2 || Row==5 || Row==7 || Row==10 || Row==12;' in hud
assert 'Hero->GetCriticalChance() * 100.f' in hud
assert 'Hero->GetRacePowerCooldownRemaining()' in hud

assert 'TEXT("ATTRIBUTES")' in hud
assert 'Select ATTRIBUTES to view your character stats' in hud
assert 'Tab or D-pad Left: return' not in hud
assert 'C / D-pad Left/Right: stats / ancestry' in hud
assert 'Tab / D-pad Left/Right: stats / ancestry' not in hud
assert 'DrawLiveMenuHitCue();' in hud
# Equipment tab reads the same owned, validated equipment as combat.
gear_ui = section(hud,"} else if (Hero->bCharacterGearPageOpen) {",
                  "const FString CharacterLines[] = {")
for token in ("Hero->EquippedWeaponCode", "Hero->EquippedArmorCode",
              "Hero->EquippedAccessoryCode", "Hero->GetValidatedEquippedTier(Slot)",
              "Hero->GetItemPower(Codes[Slot])", "Hero->GetEquipmentPower()",
              "Hero->GetSetPieceCount(Affix)", "Hero->HasLegendaryPerk(Slot)",
              "Hero->Health", "Hero->Mana", "Hero->Stamina",
              "RarityNames", "RarityColors", "SafeMax>0.f"):
    assert token in gear_ui, token
assert "SaveWorldProgress()" not in gear_ui
assert "EquipInventoryItem(" not in gear_ui
assert "3 SLOTS" in gear_ui
assert 'TEXT("EQUIPMENT")' in hud
assert 'TEXT("GEAR")' in hud
# The Reputation page reads the five real V4+ save-backed factions only.
assert 'TEXT("REPUTATION")' in hud and 'TEXT("REP")' in hud
assert 'bCharacterRepPageOpen ? 3' in hud
rep_ui = section(hud, "} else if (Hero->bCharacterRepPageOpen) {",
                 "const FString CharacterLines[] = {")
for token in ('FName(TEXT("Wanderers"))', 'FName(TEXT("VerdantCircle"))',
              'FName(TEXT("StoneHearth"))', 'FName(TEXT("MoonCoven"))',
              'FName(TEXT("AshenClans"))', 'Hero->GetFactionReputation(FactionIds[FactionIndex])',
              'Score<=-50', 'Score<=-15', 'Score>=15', 'Score>=50',
              'for (int32 FactionIndex=0;FactionIndex<5;++FactionIndex)',
              'const float Fraction=FMath::Clamp((Score+100.f)/200.f,0.f,1.f);',
              'if (Y+ReputationRowHeight>Top+PanelHeight-66.f) break;'):
    assert token in rep_ui, token
for forbidden in ("ChangeFactionReputation(", "SaveWorldProgress()",
                  "FactionReputation.Add(", "EquipInventoryItem(", "ChooseRace("):
    assert forbidden not in rep_ui, forbidden
assert 'ActionName="ChronicleFocus",Key=Tab' in inputs



# Both visible tabs must have independent hit targets and no gameplay effect.
assert 'const float PageTabWidth = (PanelWidth - 68.f) / 4.f;' in hud
assert 'const float PageTabTop = Top + 77.f;' in hud
assert 'const float TabW=(PanelW-68.f)/4.f;' in hero
assert 'const float TabY=PanelT+77.f;' in hero
mouse = section(hero, "void AMoonfallHeroPawn::ResidentContextMouseClick()",
                "if (!bResidentContextOpen) { RightHandAction(); return; }")
assert "if (bCharacterStatsOpen) {" in mouse
assert 'SelectCharacterPage(Tab);' in mouse
assert 'for (int32 Tab=0;Tab<4;++Tab)' in mouse
assert 'if (HitTestInventorySortButton())' in mouse
assert mouse.index("SelectCharacterPage(Tab);") < mouse.index("if (HitTestInventorySortButton())")
assert "ToggleCharacterRacePage();" not in mouse

# Creator gives comparative, accurately labelled previews only; game applies
# the 12 persistent gameplay ancestries by their existing stable int32 IDs.
assert "RACE TRAITS (PREVIEW)" in creator
assert "Next.Profile.VitalityMultiplier-1.f" in creator
assert "Next.Profile.PowerMultiplier-1.f" in creator
assert "Next.Profile.SpeedMultiplier-1.f" in creator
assert "Press C again to display your saved passives." in creator
keyboard_handler = creator.split(
    "FReply UMoonfallCharacterCreationWidget::NativeOnKeyDown(", 1)[1]
for control in ("EKeys::Left ||", "EKeys::Right ||", "EKeys::Up ||",
                "EKeys::Down ||", "Key == EKeys::Left)",
                "Key == EKeys::Right)", "Key == EKeys::Up)",
                "Key == EKeys::Down)"):
    assert control not in keyboard_handler, control
print("UE5 character creation, C ancestry and 12 saved racial passive/power contracts passed.")
