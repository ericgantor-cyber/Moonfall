#pragma once
// Original Moonfall storyline. Six acts x seven narrative beats = 42.
// Z and Nyra remain gods with their original artwork, never player avatars.
// This is data and read-only progression, not new boss spawns or rewards.
// No Unreal SDK, actor scans, heap allocations, or additional save fields.
#include <cstdint>

namespace MoonfallChronicle {
inline constexpr int ActCount = 6;
inline constexpr int BeatsPerAct = 7;
inline constexpr int TotalBeats = ActCount * BeatsPerAct;
inline constexpr int DungeonsPerAct = 5;
inline constexpr std::uint32_t ValidDungeonBits = (1u << 30) - 1u;
enum class EBeatState : int { Hidden = 0, Uncovered = 1, Cleared = 2 };
struct FBeat { const char* Id; const char* Title; const char* Lore; };
struct FProgress {
 bool Valid;
 int Revealed;
 int BossesDefeated;
 int FeaturedBeat;
 bool Complete;
};
inline constexpr const char* ActNames[ActCount] = {
 "The Broken Hearth",
 "The Covenant of Roots",
 "The Choir Below",
 "The Clockwork Schism",
 "The Silent Winter",
 "The Unwritten Dawn"
};
inline constexpr const char* RegionNames[ActCount] = {
 "Ashen Wilds",
 "Moonveil Forest",
 "Hollow Depths",
 "Iron Foundry",
 "Frostbound Reach",
 "Eclipse Wastes"
};
inline constexpr FBeat Acts[ActCount][BeatsPerAct] = {
  { // Act 1: Ashen Wilds
   {"act1_ch1", "Ash Oath", "The Shattered Sanctuary shelters travelers who remember the moon before it broke. Their maps mark six roads, but no one agrees what struck the sky. A damaged vow is all that remains."},
   {"act1_ch2", "Emberfall Crypt", "Below the embers, burial tablets name the first guardians who refused to abandon their people. The oldest tablet is cracked exactly where a witness should have signed."},
   {"act1_ch3", "Cinderwatch Bastion", "Cinderwatch once kept warning fires across the wilds. Its abandoned signal mirrors still point toward a night when every watchtower saw a different moon."},
   {"act1_ch4", "Ashen Reliquary", "The reliquary's empty cradle suggests someone removed the flame-seal before the catastrophe. Ash-stained notes ask whether protecting a secret can become a betrayal."},
   {"act1_ch5", "Pyreborn Hollow", "Survivors of Pyreborn recorded an impossible heat beneath cold rock. Their testimony speaks of two distant divine presences, never of mortal rulers."},
   {"act1_ch6", "Sable Furnace", "The furnace forged chains meant to bind something larger than a kingdom. Its final ledger shows that the chains were divided between six regions."},
   {"act1_ch7", "Ember Testament", "With the five wardens silenced, the Ashen Wilds offers its fragment of the broken oath. It is evidence, not absolution: the other roads must be heard."}
  },
  { // Act 2: Moonveil Forest
   {"act2_ch1", "The Whispering Bough", "Moonveil elders say the forest kept its own record when royal archives burned. Every root remembers a name, but the living must learn to listen without forcing an answer."},
   {"act2_ch2", "Briarshade Grotto", "Briarshade's underground pools reflect branches that are no longer there. One reflection follows the same crack that crossed the moon on the final peaceful night."},
   {"act2_ch3", "Moonpetal Ruins", "A garden of stone petals marks where envoys once exchanged safe passage. The missing sixth inscription suggests the pact was already fraying before the sky fell."},
   {"act2_ch4", "Weepingroot Sanctum", "Weepingroot preserved witnesses in woven bark rather than paper. Their accounts disagree about the culprit but agree that the warning was ignored."},
   {"act2_ch5", "Veilwood Depths", "In the old veilwood, moonlight bends around an empty clearing. A trail of faded offerings points toward an oath given to gods, not to kings."},
   {"act2_ch6", "Thornbound Chapel", "The chapel's thorn gates protected a seed chest through generations of darkness. What lies inside could heal the forest, or recall the debt that wounded it."},
   {"act2_ch7", "The Living Covenant", "When Moonveil's guardians fall, the forest releases a living memory of the covenant. Its truth can travel with the wanderer, but cannot yet settle the dispute."}
  },
  { // Act 3: Hollow Depths
   {"act3_ch1", "The Buried Chorus", "The Hollows were not always a prison. Miners built a city beneath the roots, then sealed its doors when strange echoes began answering questions no one had asked."},
   {"act3_ch2", "Blackwater Ossuary", "Names scratched onto blackwater stones belong to people who vanished from every surface census. Their disappearances connect the sky's fracture to the deep roads."},
   {"act3_ch3", "Echoing Chasm", "Voices cross the chasm in patterns that sound like a warning repeated too late. The echoes are real, but they do not claim to know who made the first breach."},
   {"act3_ch4", "Buried Choir", "In the Buried Choir, seven silent chambers surround an eighth without a door. The missing room implies someone deliberately removed the final note."},
   {"act3_ch5", "Gloamstone Vault", "Gloamstone tablets show the six regional seals arranged around an empty center. The map can guide the investigation without revealing what waits beneath that center."},
   {"act3_ch6", "Abyssal Watch", "The Abyssal Watch guarded a passage below the deepest known floor. Its last sentries left a single instruction: do not confuse a voice with its owner."},
   {"act3_ch7", "The Eighth Note", "With the Hollows cleared, the underground choir falls quiet long enough to hear a new pattern. Five notes answer five regions; a sixth remains incomplete."}
  },
  { // Act 4: Iron Foundry
   {"act4_ch1", "The Foundry's Debt", "Iron Foundry engineers promised that their machines would keep every city supplied. After the moon broke, the same machines began repairing weapons no one had ordered."},
   {"act4_ch2", "Rustglass Foundry", "Rustglass vats hold metal that reflects memories instead of faces. A worker's etched warning identifies a command that survived its original creator."},
   {"act4_ch3", "Brassfang Works", "Brassfang's gears still turn around an absent master key. The design was copied from a seal brought from the ashlands before anyone understood its purpose."},
   {"act4_ch4", "Broken Crucible", "The crucible shattered while forging a crown-shaped circuit. Its shards bear six symbols, but the diagram leaves no place for a seventh hand."},
   {"act4_ch5", "Furnaceheart Keep", "Deep within Furnaceheart, repair drones obey the last impossible instruction they received: prepare for a visitor who has not yet arrived."},
   {"act4_ch6", "Ironwake Vault", "The Ironwake plans reveal that machines once moved messages between shrine networks. A forged message could have carried the false summons that divided the regions."},
   {"act4_ch7", "The Unmade Key", "With the Foundry cores quiet, an unfinished key can finally be understood. It may open a path to the truth, but the wanderer must decide which witness to trust."}
  },
  { // Act 5: Frostbound Reach
   {"act5_ch1", "A Promise in Ice", "Frostbound elders kept the last clear star charts beneath glacial stone. Their descendants guard them from anyone who promises a quick answer to the moon's ruin."},
   {"act5_ch2", "Shiverpeak Cairn", "A mountain cairn holds the instruments of a cartographer who mapped light as though it were a road. His final route ends in an unmarked sky."},
   {"act5_ch3", "Winterglass Tomb", "Winterglass preserves a treaty signed by travelers from every region. Frost crept across the ink on the day the signatories ceased speaking."},
   {"act5_ch4", "Frostmourn Hollows", "The hollows' ice remembers a battle never recorded in the histories. Its echoes warn that victory over a guardian is not the same as saving a land."},
   {"act5_ch5", "Glacierwatch Ruins", "From Glacierwatch, the fractured moon appears whole for a heartbeat each winter. The phenomenon hints at a missing piece rather than a destroyed world."},
   {"act5_ch6", "Coldstar Bastion", "Coldstar's gates were closed to keep something inside, not to keep invaders out. The last captain recorded a plea for patience rather than a call to arms."},
   {"act5_ch7", "The Last Clear Night", "When the frost wardens are defeated, the old star chart becomes readable. Its final coordinate points beyond the six roads toward an eclipse no one can yet prevent."}
  },
  { // Act 6: Eclipse Wastes
   {"act6_ch1", "At the Dark Horizon", "The Eclipse Wastes are where the six regional stories begin to disagree most sharply. Here the sky refuses a single shape, and every prophecy has an author."},
   {"act6_ch2", "Umbral Scar", "The scar marks the place where ground and shadow stopped obeying the same horizon. A surviving inscription asks who benefits when the world fears the dark."},
   {"act6_ch3", "Nightfall Reliquary", "In Nightfall's vaults, histories have been rewritten and sealed again. The corrections suggest that more than one faction attempted to claim the moon's fall."},
   {"act6_ch4", "Duskspire Ruins", "Duskspire's ruined observatory follows two divine trajectories through the fractured sky. The names Z and Nyra appear as gods in old warnings, not mortal companions."},
   {"act6_ch5", "Black Sun Sanctuary", "The Black Sun sanctuary guarded the final witness to an ancient bargain. What remains is testimony, not permission to rewrite any of the gods' identities."},
   {"act6_ch6", "Eclipse Throne", "At the Eclipse Throne, the six threads of evidence converge on a chamber still waiting for its true scene and encounter. Clearing its guardian opens the last written record."},
   {"act6_ch7", "The Unwritten Dawn", "With the six roads explored and the Eclipse Wastes quiet, the chronicle ends at a choice still unwritten. The gods remain themselves; the next confrontation deserves its own authored quest."}
  }
};
constexpr int PopCount5(std::uint32_t Bits) {
 int Count = 0;
 for (int Bit = 0; Bit < DungeonsPerAct; ++Bit)
  Count += (Bits & (1u << Bit)) != 0u ? 1 : 0;
 return Count;
}
constexpr bool ValidProgress(std::uint32_t Discovered, std::uint32_t Defeated) {
 return (Discovered & ~ValidDungeonBits) == 0u
  && (Defeated & ~ValidDungeonBits) == 0u
  && (Defeated & ~Discovered) == 0u;
}
constexpr std::uint32_t ZoneBits(std::uint32_t Mask, int Zone) {
 return Zone >= 0 && Zone < ActCount
   ? (Mask >> (Zone * DungeonsPerAct)) & 31u : 0u;
}
constexpr EBeatState BeatState(int Zone, int Beat,
 std::uint32_t Discovered, std::uint32_t Defeated) {
 if (Zone < 0 || Zone >= ActCount || Beat < 0 || Beat >= BeatsPerAct
     || !ValidProgress(Discovered, Defeated))
  return EBeatState::Hidden;
 if (Beat == 0) return EBeatState::Uncovered; // regional prelude
 const std::uint32_t Found = ZoneBits(Discovered, Zone);
 const std::uint32_t Won = ZoneBits(Defeated, Zone);
 if (Beat == 6)
  return Won == 31u ? EBeatState::Uncovered : EBeatState::Hidden;
 const std::uint32_t Bit = 1u << (Beat - 1);
 return (Won & Bit) != 0u ? EBeatState::Cleared
  : (Found & Bit) != 0u ? EBeatState::Uncovered : EBeatState::Hidden;
}
constexpr FProgress Summarize(int Zone, std::uint32_t Discovered,
 std::uint32_t Defeated) {
 if (Zone < 0 || Zone >= ActCount || !ValidProgress(Discovered, Defeated))
  return {false, 0, 0, 0, false};
 const std::uint32_t Found = ZoneBits(Discovered, Zone);
 const std::uint32_t Won = ZoneBits(Defeated, Zone);
 const int Bosses = PopCount5(Won);
 const int Revealed = 1 + PopCount5(Found) + (Bosses == 5 ? 1 : 0);
 // Nonlinear dungeon order is supported. Feature a discovered, unbeaten
 // chapter first, then the epilogue or the regional prelude.
 int Featured = Bosses == 5 ? 6 : 0;
 if (Bosses < 5) {
  for (int D = 0; D < DungeonsPerAct; ++D)
   if ((Found & (1u << D)) != 0u && (Won & (1u << D)) == 0u) {
    Featured = D + 1;
    break;
   }
 }
 return {true, Revealed, Bosses, Featured, Bosses == 5};
}
// Read-only overview for future world-map/quest-journal progress displays.
// Invalid masks fail closed and report no fabricated progress or rewards.
struct FWorldProgress {
 bool Valid;
 int DungeonsDiscovered;
 int BossesDefeated;
 int ActsCompleted;
 int ChaptersRevealed;
 bool ChronicleComplete;
};
constexpr FWorldProgress SummarizeWorld(std::uint32_t Discovered,
 std::uint32_t Defeated) {
 if (!ValidProgress(Discovered, Defeated))
  return {false, 0, 0, 0, 0, false};
 int Dungeons = 0, Bosses = 0, Completed = 0, Chapters = 0;
 for (int Zone = 0; Zone < ActCount; ++Zone) {
  const FProgress Progress = Summarize(Zone, Discovered, Defeated);
  Dungeons += PopCount5(ZoneBits(Discovered, Zone));
  Bosses += Progress.BossesDefeated;
  Completed += Progress.Complete ? 1 : 0;
  Chapters += Progress.Revealed;
 }
 return {true, Dungeons, Bosses, Completed, Chapters,
  Completed == ActCount};
}
constexpr const FBeat* RevealedBeat(int Zone, int Beat,
 std::uint32_t Discovered, std::uint32_t Defeated) {
 return BeatState(Zone, Beat, Discovered, Defeated) == EBeatState::Hidden
     ? nullptr : &Acts[Zone][Beat];
}
static_assert(TotalBeats == 42, "Six seven-chapter acts required");
static_assert(ValidProgress(0u, 0u), "Fresh save must be valid");
static_assert(!ValidProgress(0u, 1u), "Boss kills require discovered dungeons");
static_assert(Summarize(0, 0u, 0u).Revealed == 1,
 "Never reveal undiscovered dungeon lore on a fresh save");
static_assert(Summarize(0, 31u, 31u).Revealed == 7,
 "Clear a region to reveal its epilogue");
static_assert(Summarize(0, 31u, 31u).Complete, "Completed act should finish");
static_assert(RevealedBeat(0, 1, 0u, 0u) == nullptr,
 "Locked dungeon lore must remain hidden");
static_assert(SummarizeWorld(0u, 0u).ChaptersRevealed == 6,
 "Fresh world reveals only six regional preludes");
static_assert(SummarizeWorld(ValidDungeonBits, ValidDungeonBits).ChaptersRevealed == 42,
 "Clearing all 30 dungeons reveals the full chronicle");
static_assert(SummarizeWorld(ValidDungeonBits, ValidDungeonBits).ChronicleComplete,
 "All six completed acts complete the chronicle");
static_assert(SummarizeWorld(31u, 31u).ActsCompleted == 1,
 "One cleared region must not complete other acts");
static_assert(!SummarizeWorld(0u, 1u).Valid,
 "Invalid progress must fail closed in world overview");
} // namespace MoonfallChronicle
