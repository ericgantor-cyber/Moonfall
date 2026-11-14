#include "MoonfallRaceLibrary.h"

namespace
{
 struct FRaceRow
 {
  const TCHAR* Id;
  const TCHAR* Name;
  const TCHAR* Family;
  bool bPlayable;
  float Vitality;
  float Power;
  float Speed;
 };

 // All profiles are original Moonfall data. No Tolkien art or lore assets.
 constexpr FRaceRow RaceRows[] =
 {
  {TEXT("human"), TEXT("Human"), TEXT("FreePeoples"), true, 1.f, 1.f, 1.f},
  {TEXT("high_elf"), TEXT("High Elf"), TEXT("Elven"), true, 0.95f, 1.1f, 1.05f},
  {TEXT("wood_elf"), TEXT("Wood Elf"), TEXT("Elven"), true, 0.9f, 1.05f, 1.12f},
  {TEXT("dark_elf"), TEXT("Twilight Elf"), TEXT("Elven"), true, 0.95f, 1.12f, 1.02f},
  {TEXT("dwarf"), TEXT("Dwarf"), TEXT("Dwarven"), true, 1.18f, 1.06f, 0.88f},
  {TEXT("smallfolk"), TEXT("Smallfolk"), TEXT("FreePeoples"), true, 0.86f, 0.9f, 1.12f},
  {TEXT("orc"), TEXT("Orc"), TEXT("Shadowkin"), false, 1.02f, 1.09f, 1.f},
  {TEXT("war_orc"), TEXT("War Orc"), TEXT("Shadowkin"), false, 1.16f, 1.17f, 0.93f},
  {TEXT("goblin"), TEXT("Goblin"), TEXT("Shadowkin"), false, 0.78f, 0.86f, 1.18f},
  {TEXT("troll"), TEXT("Troll"), TEXT("Giants"), false, 1.55f, 1.25f, 0.72f},
  {TEXT("ogre"), TEXT("Ogre"), TEXT("Giants"), false, 1.42f, 1.19f, 0.81f},
  {TEXT("giant"), TEXT("Giant"), TEXT("Giants"), false, 1.65f, 1.3f, 0.67f},
  {TEXT("treekin"), TEXT("Treekin"), TEXT("Nature"), false, 1.38f, 1.15f, 0.64f},
  {TEXT("eaglekin"), TEXT("Eaglekin"), TEXT("Sky"), false, 0.9f, 1.05f, 1.17f},
  {TEXT("dragon"), TEXT("Dragon"), TEXT("Draconic"), false, 1.8f, 1.45f, 0.94f},
  {TEXT("drake"), TEXT("Drake"), TEXT("Draconic"), false, 1.35f, 1.22f, 1.1f},
  {TEXT("warg"), TEXT("Warg"), TEXT("Beasts"), false, 1.f, 1.04f, 1.2f},
  {TEXT("spiderkin"), TEXT("Spiderkin"), TEXT("Beasts"), false, 0.9f, 1.f, 1.14f},
  {TEXT("wraith"), TEXT("Wraith"), TEXT("Spirits"), false, 0.82f, 1.2f, 1.14f},
  {TEXT("ancient_spirit"), TEXT("Ancient Spirit"), TEXT("Spirits"), false, 1.3f, 1.38f, 1.04f},
  {TEXT("fire_demon"), TEXT("Fire Demon"), TEXT("Infernal"), false, 1.55f, 1.4f, 0.94f},
  {TEXT("stonekin"), TEXT("Stonekin"), TEXT("Elemental"), false, 1.42f, 1.22f, 0.73f},
  {TEXT("riverfolk"), TEXT("Riverfolk"), TEXT("FreePeoples"), true, 0.95f, 0.97f, 1.13f},
  {TEXT("beastkin"), TEXT("Beastkin"), TEXT("Beasts"), true, 1.07f, 1.04f, 1.09f},
  {TEXT("half_elf"), TEXT("Half Elf"), TEXT("Elven"), true, 1.f, 1.06f, 1.04f},
  {TEXT("hill_dwarf"), TEXT("Hill Dwarf"), TEXT("Dwarven"), true, 1.12f, 1.02f, 0.93f},
  {TEXT("mountain_dwarf"), TEXT("Mountain Dwarf"), TEXT("Dwarven"), true, 1.24f, 1.12f, 0.84f},
  {TEXT("cave_goblin"), TEXT("Cave Goblin"), TEXT("Shadowkin"), false, 0.82f, 0.92f, 1.12f},
  {TEXT("swamp_troll"), TEXT("Swamp Troll"), TEXT("Giants"), false, 1.48f, 1.16f, 0.76f},
  {TEXT("forest_guardian"), TEXT("Forest Guardian"), TEXT("Nature"), false, 1.45f, 1.2f, 0.8f},
 };
}

TArray<FName> UMoonfallRaceLibrary::GetRaceIds()
{
 TArray<FName> Result;
 Result.Reserve(UE_ARRAY_COUNT(RaceRows));
 for (const FRaceRow& Row : RaceRows)
  Result.Add(FName(Row.Id));
 return Result;
}

TArray<FName> UMoonfallRaceLibrary::GetPlayableRaceIds()
{
 TArray<FName> Result;
 for (const FRaceRow& Row : RaceRows)
  if (Row.bPlayable) Result.Add(FName(Row.Id));
 return Result;
}

TArray<FName> UMoonfallRaceLibrary::GetRaceIdsByFamily(FName Family)
{
 TArray<FName> Result;
 if (Family.IsNone()) return Result;
 for (const FRaceRow& Row : RaceRows)
  if (Family == FName(Row.Family)) Result.Add(FName(Row.Id));
 return Result;
}

bool UMoonfallRaceLibrary::FindRaceProfile(FName RaceId, FMoonfallRaceProfile& Profile)
{
 Profile = FMoonfallRaceProfile();
 if (RaceId.IsNone()) return false;
 for (const FRaceRow& Row : RaceRows)
 {
  if (RaceId != FName(Row.Id)) continue;
  Profile.RaceId = RaceId;
  Profile.DisplayName = FText::FromString(Row.Name);
  Profile.Family = FName(Row.Family);
  Profile.bPlayableCandidate = Row.bPlayable;
  Profile.VitalityMultiplier = Row.Vitality;
  Profile.PowerMultiplier = Row.Power;
  Profile.SpeedMultiplier = Row.Speed;
  return true;
 }
 return false;
}
