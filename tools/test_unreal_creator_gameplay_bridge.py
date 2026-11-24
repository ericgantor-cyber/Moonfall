"""Validate safe UI-to-save gameplay ancestry bridging in the UE5 character creator.

Source contract test, not a native UE5 packaged-game test.
"""
from pathlib import Path
import re

ROOT=Path(__file__).resolve().parents[1]
SRC=ROOT/"MoonfallUnreal/Source/MoonfallUnreal"
catalog=(SRC/"MoonfallRaceLibrary.cpp").read_text(encoding="utf-8")
library_h=(SRC/"MoonfallRaceLibrary.h").read_text(encoding="utf-8")
creator=(SRC/"MoonfallCharacterCreationWidget.cpp").read_text(encoding="utf-8")
creator_h=(SRC/"MoonfallCharacterCreationWidget.h").read_text(encoding="utf-8")
hero=(SRC/"MoonfallHeroPawn.cpp").read_text(encoding="utf-8")
hero_h=(SRC/"MoonfallHeroPawn.h").read_text(encoding="utf-8")

# Keep all 30 existing world-race IDs stable for NPC/world consumers.
rows_section=catalog.split("constexpr FRaceRow RaceRows[] =",1)[1].split("\n };\n}",1)[0]
rows=re.findall(r'\{TEXT\("([^"]+)"\), TEXT\("([^"]+)"\), TEXT\("([^"]+)"\), (true|false),',rows_section)
assert len(rows)==30, len(rows)
world_ids=[r[0] for r in rows]
assert len(set(world_ids))==30
playable={r[0] for r in rows if r[3]=="true"}
assert len(playable)==15, playable
for name in ("orc","goblin","troll","treekin"):
    assert name in playable, name
for name in ("war_orc","ogre","giant","dragon","drake","wraith",
             "fire_demon","cave_goblin","forest_guardian"):
    assert name not in playable, name

# All playable creator profiles resolve explicitly and deterministically
# to a canonical 0..11 gameplay ancestry. No unknown/NPC ids can resolve.
bridge=catalog.split("bool UMoonfallRaceLibrary::ResolveHeroAncestryId(",1)[1].split(
    "\nTArray<FName> UMoonfallRaceLibrary::GetRaceIds()",1)[0]
mapping=dict((name,int(idx)) for name,idx in re.findall(
    r'\{TEXT\("([^"]+)"\),(\d+)\}',bridge))
assert len(mapping)==15, mapping
assert set(mapping)==playable, (playable-set(mapping),set(mapping)-playable)
assert set(mapping.values())==set(range(12)), mapping
assert mapping["smallfolk"]==4 and mapping["treekin"]==8
assert mapping["dark_elf"]==9 and mapping["riverfolk"]==10
assert mapping["beastkin"]==11 and mapping["half_elf"]==1
assert mapping["hill_dwarf"]==mapping["mountain_dwarf"]==3
assert "HeroAncestryId = -1;" in bridge
assert "FindPlayableRaceProfile(CreatorRaceId, Candidate)" in bridge
assert "HeroAncestryId = Row.SavedId;" in bridge
assert "static bool ResolveHeroAncestryId(FName CreatorRaceId, int32& HeroAncestryId);" in library_h

# The real hero provides the preview using the exact same canonical runtime
# profile and race-power names as saved gameplay. No second set of bonuses.
preview=hero.split("FString AMoonfallHeroPawn::PreviewGameplayAncestry(",1)[1].split(
    "FString AMoonfallHeroPawn::GetRacePowerName()",1)[0]
assert "CandidateRaceId < 0 || CandidateRaceId >= 12" in preview
assert "const auto& R=RaceProfile(CandidateRaceId);" in preview
for prop in ("R.Name","R.Might","R.Insight","R.Resolve",
             "R.StaminaRegen","R.ManaRegen",
             "MoonfallRacePowerNames[CandidateRaceId]"):
    assert prop in preview, prop
assert 'static FString PreviewGameplayAncestry(int32 CandidateRaceId);' in hero_h
assert 'AMoonfallHeroPawn::PreviewGameplayAncestry(SavedAncestryId)' in creator
assert 'UMoonfallRaceLibrary::ResolveHeroAncestryId(' in creator

# The widget binds only a controlled hero. Its confirm button cannot report
# success unless the actual save-backed ChooseRace method succeeds.
construct=creator.split("void UMoonfallCharacterCreationWidget::NativeConstruct()",1)[1].split(
    "void UMoonfallCharacterCreationWidget::BuildLayout()",1)[0]
assert "UGameplayStatics::GetPlayerPawn(this,0)" in construct
assert "Hero->IsPlayerControlled()" in construct
confirmation=creator.split("void UMoonfallCharacterCreationWidget::HandleConfirmRace()",1)[1].split(
    "FReply UMoonfallCharacterCreationWidget::NativeOnKeyDown(",1)[0]
assert "Selection.Profile.bPlayableCandidate" in confirmation
assert "ResolveHeroAncestryId(" in confirmation
assert "if (!IsValid(Hero) || !Hero->IsPlayerControlled())" in confirmation
assert "if (Hero->bRaceChosen)" in confirmation
assert "if (!Hero->ChooseRace(SavedAncestryId))" in confirmation
assert "Race not saved." in confirmation
assert "OnGameplayAncestryConfirmed.Broadcast(SavedAncestryId);" in confirmation
assert "OnRaceConfirmed.Broadcast(Selection.RaceId);" in confirmation
assert confirmation.index("if (!Hero->ChooseRace(") < confirmation.index(
    "OnGameplayAncestryConfirmed.Broadcast")
assert confirmation.index("OnGameplayAncestryConfirmed.Broadcast") < confirmation.index(
    "OnRaceConfirmed.Broadcast")
assert confirmation.count("OnRaceConfirmed.Broadcast(")==1
assert "void BindHeroForConfirmation(AMoonfallHeroPawn* Hero);" in creator_h
assert "FMoonfallGameplayAncestryConfirmed OnGameplayAncestryConfirmed;" in creator_h
assert "TWeakObjectPtr<AMoonfallHeroPawn> ConfirmationHero;" in creator_h
assert 'TEXT("GAMEPLAY ANCESTRY SAVED.' in creator
assert "CONFIRM will save the displayed lineage." in creator

# The original save path has already-chosen protection and rollback on error.
choose=hero.split("bool AMoonfallHeroPawn::ChooseRace(int32 NewRaceId)",1)[1].split(
    "FString AMoonfallHeroPawn::GetRaceName()",1)[0]
for guard in ("bRaceChosen || NewRaceId < 0 || NewRaceId > 11",
              "if (!SaveWorldProgress())", "RaceId = OldId;",
              "bRaceChosen = false;"):
    assert guard in choose
assert "OnRaceConfirmed.Broadcast(" not in choose
assert "SaveWorldProgress()" not in preview

print("UE5 character creator: 30 stable world races, 15 explicit choices, all 12 save-backed ancestry lineages and rollback guards validated.")
