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

# The first C opens combined Stats & Gear; C cycles Ancestry and Reputation.
# Tab remains Atlas Chronicle. Page navigation never saves or equips items.
assert 'bool bCharacterRacePageOpen = false;' in header
assert 'bool bCharacterRepPageOpen = false;' in header
assert 'bCharacterGearPageOpen' not in header and 'bCharacterGearPageOpen' not in hero
assert 'void SelectCharacterPage(int32 Page);' in header
assert 'void ToggleCharacterRacePage();' in header
select_page = section(hero,"void AMoonfallHeroPawn::SelectCharacterPage(int32 Page)",
                      "void AMoonfallHeroPawn::ToggleCharacterRacePage()")
assert 'Page < 0 || Page > 2' in select_page
assert 'bCharacterStatsOpen' in select_page and 'bInventoryOpen' in select_page
assert 'bCharacterRacePageOpen = Page == 1;' in select_page
assert 'bCharacterRepPageOpen = Page == 2;' in select_page
assert 'ChooseRace(' not in select_page and 'SaveWorldProgress()' not in select_page
forward = 'SelectCharacterPage(bCharacterRacePageOpen ? 2 : (bCharacterRepPageOpen ? 0 : 1));'
reverse = 'SelectCharacterPage(bCharacterRacePageOpen ? 0 : (bCharacterRepPageOpen ? 1 : 2));'
assert forward in hero and reverse in hero
stats_action = section(hero,"void AMoonfallHeroPawn::ToggleCharacterStats()",
                       "void AMoonfallHeroPawn::ToggleAtlasChronicleFocus()")
assert 'if (bCharacterStatsOpen) { ToggleCharacterRacePage(); return; }' in stats_action
assert 'bCharacterStatsOpen = true;' in stats_action
atlas_action = section(hero,"void AMoonfallHeroPawn::ToggleAtlasChronicleFocus()",
                       "void AMoonfallHeroPawn::StepAtlasChronicle(")
assert 'ToggleCharacterRacePage();' not in atlas_action
assert 'ActionName="CharacterStats",Key=C' in inputs
assert 'ActionName="ChronicleFocus",Key=Tab' in inputs
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
# Equipment and attributes live in one read-only page, with responsive
# rows and owned-only equipment. No fake equipment slots or page switching.
assert 'const float StatPanelWidth = PanelWidth - 30.f;' in hud
assert 'if (RowY+StatsRowHeight > StatsBottom) break;' in hud
assert 'if (RowY+RaceRowHeight > Top+PanelHeight-65.f) break;' in hud
assert 'const bool bSection = Row == 1 || Row == 5 || Row == 8 || Row == 12 || Row == 16;' in hud
assert 'const bool bSection = Row==0 || Row==2 || Row==5 || Row==7 || Row==10 || Row==12;' in hud
assert 'TEXT("STATS & GEAR")' in hud and 'TEXT("STATS+GEAR")' in hud
assert 'Select STATS & GEAR to view your equipment and attributes' in hud
assert 'C / D-pad Left/Right: stats & gear / ancestry / rep' in hud
assert 'TEXT("EQUIPPED GEAR")' in hud
assert 'bCharacterGearPageOpen' not in hud
assert 'DrawLiveMenuHitCue();' in hud
stats_gear_ui = section(hud,"const FString CharacterLines[] = {",
                        'DrawText(TEXT("C / D-pad Left/Right: stats & gear / ancestry / rep")')
for token in ("Hero->GetMight()", "Hero->GetInsight()", "Hero->GetResolve()",
              "Hero->GetCriticalChance() * 100.f", "Hero->GetHasteBonus() * 100.f",
              "Hero->GetSetPieceCount(1)", "Hero->GetSetPieceCount(2)",
              "Hero->GetSetPieceCount(3)", "Hero->HasLegendaryPerk(0)",
              "Hero->GetEquipmentSlotCode(0)", "Hero->GetEquipmentSlotCode(1)",
              "Hero->GetEquipmentSlotCode(2)", "Hero->GetValidatedEquippedTier(Slot)",
              "Hero->GetItemPower(Codes[Slot])", "Hero->GetEquipmentPower()",
              "Hero->HasLegendaryPerk(Slot)", "RarityNames", "RarityColors",
              "Hero->Health", "Hero->Mana", "Hero->Stamina",
              "const bool bCompactStats = PanelHeight < 570.f;",
              "if (Y+CardHeight>GearBottom-ResourceReserve+1.f) break;"):
    assert token in stats_gear_ui, token
for forbidden in ("SaveWorldProgress()", "EquipInventoryItem(",
                  "ChangeFactionReputation(", "ChooseRace("):
    assert forbidden not in stats_gear_ui, forbidden
assert 'Tab or D-pad Left: return' not in hud

# The Reputation page reads the five real V4+ save-backed factions only.
assert 'TEXT("REPUTATION")' in hud and 'TEXT("REP")' in hud
assert 'bCharacterRepPageOpen ? 2' in hud
rep_ui = section(hud, "} else if (Hero->bCharacterRepPageOpen) {",
                 "const FString CharacterLines[] = {")
for token in ('FName(TEXT("Wanderers"))', 'FName(TEXT("VerdantCircle"))',
              'FName(TEXT("StoneHearth"))', 'FName(TEXT("MoonCoven"))',
              'FName(TEXT("AshenClans"))', 'Hero->GetFactionReputation(FactionIds[FactionIndex])',
              'Score<=-50', 'Score<=-15', 'Score>=15', 'Score>=50',
              'for (int32 FactionIndex=0;FactionIndex<5;++FactionIndex)',
              'const float Fraction=FMath::Clamp((Score+100.f)/200.f,0.f,1.f);',
              'if (Y+ReputationRowHeight>RowsBottom) break;'):
    assert token in rep_ui, token
for forbidden in ("ChangeFactionReputation(", "SaveWorldProgress()",
                  "FactionReputation.Add(", "EquipInventoryItem(", "ChooseRace("):
    assert forbidden not in rep_ui, forbidden
assert 'ActionName="ChronicleFocus",Key=Tab' in inputs



# All three visible tabs use independent hit targets and no gameplay effect.
assert 'const float PageTabWidth = (PanelWidth - 60.f) / 3.f;' in hud
assert 'const float PageTabTop = Top + 77.f;' in hud
assert 'const float TabW=(PanelW-60.f)/3.f;' in hero
assert 'const float TabY=PanelT+77.f;' in hero
mouse = section(hero, "void AMoonfallHeroPawn::ResidentContextMouseClick()",
                "if (!bResidentContextOpen) { RightHandAction(); return; }")
assert "if (bCharacterStatsOpen) {" in mouse
assert 'SelectCharacterPage(Tab);' in mouse
assert 'for (int32 Tab=0;Tab<3;++Tab)' in mouse
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
