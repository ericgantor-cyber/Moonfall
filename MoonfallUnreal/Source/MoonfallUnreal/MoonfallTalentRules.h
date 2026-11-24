#pragma once
// Generated from godot-v38/data/moonfall_talents.json (schema 1).
// Moonfall's original 141-node constellation rules, compiled without extra
// Unreal assets or runtime JSON parsing. See tools/test_unreal_talent_parity.py.
// This is a pure gameplay rule module; it does not modify character artwork.
#include <array>
#include <cstring>
#include <algorithm>
#include <cmath>

namespace MoonfallTalents {
constexpr int NodeCount=141;
constexpr int PointsPerLevel=2;
constexpr int MaximumLevel=100;
enum class Branch {Might,Agility,Arcana};
enum class Stat {Health,Mana,Stamina,MeleeDamage,RangedDamage,MagicDamage,Count};
struct Node {
 const char* Id;
 const char* Title;
 Branch Tree;
 int Tier;
 int MinLevel;
 int RequiresA;
 int RequiresB;
 int MaxRank;
 Stat AffectedStat;
 double AmountPerRank;
 bool IsPercent;
};
inline constexpr std::array<Node,NodeCount> Nodes = {{
 {"iron_will","Iron Will",Branch::Might,1,1,-1,-1,5,Stat::MeleeDamage,0.0300000000000,true},
 {"breaker","Breaker",Branch::Might,2,4,0,-1,3,Stat::MeleeDamage,0.0400000000000,true},
 {"guard","Guard",Branch::Might,2,4,0,-1,3,Stat::Health,8.00000000000,false},
 {"titan_strike","Titan Strike",Branch::Might,3,10,1,2,1,Stat::MeleeDamage,0.0800000000000,true},
 {"swift_step","Swift Step",Branch::Agility,1,1,-1,-1,5,Stat::Stamina,5.00000000000,false},
 {"hawk_eye","Hawk Eye",Branch::Agility,2,4,4,-1,3,Stat::RangedDamage,0.0400000000000,true},
 {"wind_runner","Wind Runner",Branch::Agility,2,4,4,-1,3,Stat::Stamina,8.00000000000,false},
 {"eagle_strike","Eagle Strike",Branch::Agility,3,10,5,6,1,Stat::RangedDamage,0.0800000000000,true},
 {"spirit_well","Spirit Well",Branch::Arcana,1,1,-1,-1,5,Stat::Mana,8.00000000000,false},
 {"spell_focus","Spell Focus",Branch::Arcana,2,4,8,-1,3,Stat::MagicDamage,0.0400000000000,true},
 {"soul_ward","Soul Ward",Branch::Arcana,2,4,8,-1,3,Stat::Health,8.00000000000,false},
 {"starfall","Starfall",Branch::Arcana,3,10,9,10,1,Stat::MagicDamage,0.0800000000000,true},
 {"might_ashen_grip","Ashen Grip",Branch::Might,4,16,3,-1,2,Stat::MeleeDamage,0.0150000000000,true},
 {"might_crimson_guard","Crimson Guard",Branch::Might,4,16,3,-1,2,Stat::Health,7.00000000000,false},
 {"might_steel_momentum","Steel Momentum",Branch::Might,4,16,3,-1,2,Stat::MeleeDamage,0.0120000000000,true},
 {"might_battle_rhythm","Battle Rhythm",Branch::Might,4,16,3,-1,2,Stat::Health,6.00000000000,false},
 {"might_rift_breaker","Rift Breaker",Branch::Might,5,24,12,-1,2,Stat::MeleeDamage,0.0165000000000,true},
 {"might_stoneheart","Stoneheart",Branch::Might,5,24,13,-1,2,Stat::Health,9.00000000000,false},
 {"might_heavy_hand","Heavy Hand",Branch::Might,5,24,14,-1,2,Stat::MeleeDamage,0.0135000000000,true},
 {"might_resolute_march","Resolute March",Branch::Might,5,24,15,-1,2,Stat::Health,8.00000000000,false},
 {"might_siege_instinct","Siege Instinct",Branch::Might,6,32,16,-1,2,Stat::MeleeDamage,0.0180000000000,true},
 {"might_blood_oath","Blood Oath",Branch::Might,6,32,17,-1,2,Stat::Health,11.0000000000,false},
 {"might_hammerfall","Hammerfall",Branch::Might,6,32,18,-1,2,Stat::MeleeDamage,0.0150000000000,true},
 {"might_bulwark","Bulwark",Branch::Might,6,32,19,16,2,Stat::Health,10.0000000000,false},
 {"might_oath_of_iron","Oath of Iron",Branch::Might,7,42,20,-1,2,Stat::MeleeDamage,0.0195000000000,true},
 {"might_ironhide","Ironhide",Branch::Might,7,42,21,-1,2,Stat::Health,13.0000000000,false},
 {"might_executioner","Executioner",Branch::Might,7,42,22,-1,2,Stat::MeleeDamage,0.0165000000000,true},
 {"might_unshaken","Unshaken",Branch::Might,7,42,23,-1,2,Stat::Health,12.0000000000,false},
 {"might_warborn","Warborn",Branch::Might,8,52,24,-1,2,Stat::MeleeDamage,0.0210000000000,true},
 {"might_cragwall","Cragwall",Branch::Might,8,52,25,-1,2,Stat::Health,15.0000000000,false},
 {"might_tyrant_s_edge","Tyrant's Edge",Branch::Might,8,52,26,-1,2,Stat::MeleeDamage,0.0180000000000,true},
 {"might_heart_of_the_mountain","Heart of the Mountain",Branch::Might,8,52,27,-1,2,Stat::Health,14.0000000000,false},
 {"might_moonforged_blade","Moonforged Blade",Branch::Might,9,62,28,-1,2,Stat::MeleeDamage,0.0225000000000,true},
 {"might_aegis_pulse","Aegis Pulse",Branch::Might,9,62,29,-1,2,Stat::Health,17.0000000000,false},
 {"might_breaker_of_chains","Breaker of Chains",Branch::Might,9,62,30,-1,2,Stat::MeleeDamage,0.0195000000000,true},
 {"might_last_stand","Last Stand",Branch::Might,9,62,31,28,2,Stat::Health,16.0000000000,false},
 {"might_worldsplitter","Worldsplitter",Branch::Might,10,72,32,-1,2,Stat::MeleeDamage,0.0240000000000,true},
 {"might_fortress_soul","Fortress Soul",Branch::Might,10,72,33,-1,2,Stat::Health,19.0000000000,false},
 {"might_sundering_fury","Sundering Fury",Branch::Might,10,72,34,-1,2,Stat::MeleeDamage,0.0210000000000,true},
 {"might_immortal_march","Immortal March",Branch::Might,10,72,35,-1,2,Stat::Health,18.0000000000,false},
 {"might_titan_s_legacy","Titan's Legacy",Branch::Might,11,82,36,-1,1,Stat::MeleeDamage,0.0255000000000,true},
 {"might_eclipse_bastion","Eclipse Bastion",Branch::Might,11,82,37,-1,1,Stat::Health,21.0000000000,false},
 {"might_judgment_of_steel","Judgment of Steel",Branch::Might,11,82,38,-1,1,Stat::MeleeDamage,0.0225000000000,true},
 {"might_might_ascendant","Might Ascendant",Branch::Might,11,82,39,-1,1,Stat::Health,20.0000000000,false},
 {"agility_gale_footwork","Gale Footwork",Branch::Agility,4,16,7,-1,2,Stat::Stamina,5.00000000000,false},
 {"agility_silent_bow","Silent Bow",Branch::Agility,4,16,7,-1,2,Stat::RangedDamage,0.0150000000000,true},
 {"agility_springstep","Springstep",Branch::Agility,4,16,7,-1,2,Stat::Stamina,6.00000000000,false},
 {"agility_featherlight","Featherlight",Branch::Agility,4,16,7,-1,2,Stat::RangedDamage,0.0120000000000,true},
 {"agility_shadowstride","Shadowstride",Branch::Agility,5,24,44,-1,2,Stat::Stamina,7.00000000000,false},
 {"agility_piercing_sight","Piercing Sight",Branch::Agility,5,24,45,-1,2,Stat::RangedDamage,0.0165000000000,true},
 {"agility_quick_reflex","Quick Reflex",Branch::Agility,5,24,46,-1,2,Stat::Stamina,8.00000000000,false},
 {"agility_wind_dance","Wind Dance",Branch::Agility,5,24,47,-1,2,Stat::RangedDamage,0.0135000000000,true},
 {"agility_phantom_pace","Phantom Pace",Branch::Agility,6,32,48,-1,2,Stat::Stamina,9.00000000000,false},
 {"agility_deadeye_focus","Deadeye Focus",Branch::Agility,6,32,49,-1,2,Stat::RangedDamage,0.0180000000000,true},
 {"agility_aerial_balance","Aerial Balance",Branch::Agility,6,32,50,-1,2,Stat::Stamina,10.0000000000,false},
 {"agility_trailblazer","Trailblazer",Branch::Agility,6,32,51,48,2,Stat::RangedDamage,0.0150000000000,true},
 {"agility_mistwalker","Mistwalker",Branch::Agility,7,42,52,-1,2,Stat::Stamina,11.0000000000,false},
 {"agility_marked_shot","Marked Shot",Branch::Agility,7,42,53,-1,2,Stat::RangedDamage,0.0195000000000,true},
 {"agility_fleet_recovery","Fleet Recovery",Branch::Agility,7,42,54,-1,2,Stat::Stamina,12.0000000000,false},
 {"agility_storm_runner","Storm Runner",Branch::Agility,7,42,55,-1,2,Stat::RangedDamage,0.0165000000000,true},
 {"agility_eclipse_step","Eclipse Step",Branch::Agility,8,52,56,-1,2,Stat::Stamina,13.0000000000,false},
 {"agility_longshot","Longshot",Branch::Agility,8,52,57,-1,2,Stat::RangedDamage,0.0210000000000,true},
 {"agility_dancer_s_guard","Dancer's Guard",Branch::Agility,8,52,58,-1,2,Stat::Stamina,14.0000000000,false},
 {"agility_untethered","Untethered",Branch::Agility,8,52,59,-1,2,Stat::RangedDamage,0.0180000000000,true},
 {"agility_dusk_flight","Dusk Flight",Branch::Agility,9,62,60,-1,2,Stat::Stamina,15.0000000000,false},
 {"agility_falcon_s_reach","Falcon's Reach",Branch::Agility,9,62,61,-1,2,Stat::RangedDamage,0.0225000000000,true},
 {"agility_flow_state","Flow State",Branch::Agility,9,62,62,-1,2,Stat::Stamina,16.0000000000,false},
 {"agility_horizon_walker","Horizon Walker",Branch::Agility,9,62,63,60,2,Stat::RangedDamage,0.0195000000000,true},
 {"agility_ghostwind","Ghostwind",Branch::Agility,10,72,64,-1,2,Stat::Stamina,17.0000000000,false},
 {"agility_starlit_arrow","Starlit Arrow",Branch::Agility,10,72,65,-1,2,Stat::RangedDamage,0.0240000000000,true},
 {"agility_endless_motion","Endless Motion",Branch::Agility,10,72,66,-1,2,Stat::Stamina,18.0000000000,false},
 {"agility_tempest_path","Tempest Path",Branch::Agility,10,72,67,-1,2,Stat::RangedDamage,0.0210000000000,true},
 {"agility_moonshadow","Moonshadow",Branch::Agility,11,82,68,-1,1,Stat::Stamina,19.0000000000,false},
 {"agility_skyhunter","Skyhunter",Branch::Agility,11,82,69,-1,1,Stat::RangedDamage,0.0255000000000,true},
 {"agility_astral_stride","Astral Stride",Branch::Agility,11,82,70,-1,1,Stat::Stamina,20.0000000000,false},
 {"agility_agility_ascendant","Agility Ascendant",Branch::Agility,11,82,71,-1,1,Stat::RangedDamage,0.0225000000000,true},
 {"arcana_ember_sigil","Ember Sigil",Branch::Arcana,4,16,11,-1,2,Stat::MagicDamage,0.0150000000000,true},
 {"arcana_moonwell","Moonwell",Branch::Arcana,4,16,11,-1,2,Stat::Mana,6.00000000000,false},
 {"arcana_woven_ward","Woven Ward",Branch::Arcana,4,16,11,-1,2,Stat::Health,6.00000000000,false},
 {"arcana_star_current","Star Current",Branch::Arcana,4,16,11,-1,2,Stat::MagicDamage,0.0120000000000,true},
 {"arcana_spellweave","Spellweave",Branch::Arcana,5,24,76,-1,2,Stat::MagicDamage,0.0165000000000,true},
 {"arcana_azure_reservoir","Azure Reservoir",Branch::Arcana,5,24,77,-1,2,Stat::Mana,8.00000000000,false},
 {"arcana_spirit_mantle","Spirit Mantle",Branch::Arcana,5,24,78,-1,2,Stat::Health,8.00000000000,false},
 {"arcana_ethereal_flow","Ethereal Flow",Branch::Arcana,5,24,79,-1,2,Stat::MagicDamage,0.0135000000000,true},
 {"arcana_comet_lore","Comet Lore",Branch::Arcana,6,32,80,-1,2,Stat::MagicDamage,0.0180000000000,true},
 {"arcana_deep_focus","Deep Focus",Branch::Arcana,6,32,81,-1,2,Stat::Mana,10.0000000000,false},
 {"arcana_astral_shield","Astral Shield",Branch::Arcana,6,32,82,-1,2,Stat::Health,10.0000000000,false},
 {"arcana_runic_echo","Runic Echo",Branch::Arcana,6,32,83,80,2,Stat::MagicDamage,0.0150000000000,true},
 {"arcana_void_spark","Void Spark",Branch::Arcana,7,42,84,-1,2,Stat::MagicDamage,0.0195000000000,true},
 {"arcana_soul_channel","Soul Channel",Branch::Arcana,7,42,85,-1,2,Stat::Mana,12.0000000000,false},
 {"arcana_celestial_shelter","Celestial Shelter",Branch::Arcana,7,42,86,-1,2,Stat::Health,12.0000000000,false},
 {"arcana_arc_pulse","Arc Pulse",Branch::Arcana,7,42,87,-1,2,Stat::MagicDamage,0.0165000000000,true},
 {"arcana_twilight_flame","Twilight Flame",Branch::Arcana,8,52,88,-1,2,Stat::MagicDamage,0.0210000000000,true},
 {"arcana_arcane_spring","Arcane Spring",Branch::Arcana,8,52,89,-1,2,Stat::Mana,14.0000000000,false},
 {"arcana_ward_of_ages","Ward of Ages",Branch::Arcana,8,52,90,-1,2,Stat::Health,14.0000000000,false},
 {"arcana_mana_cascade","Mana Cascade",Branch::Arcana,8,52,91,-1,2,Stat::MagicDamage,0.0180000000000,true},
 {"arcana_eclipse_invocation","Eclipse Invocation",Branch::Arcana,9,62,92,-1,2,Stat::MagicDamage,0.0225000000000,true},
 {"arcana_starlight_font","Starlight Font",Branch::Arcana,9,62,93,-1,2,Stat::Mana,16.0000000000,false},
 {"arcana_spectral_bastion","Spectral Bastion",Branch::Arcana,9,62,94,-1,2,Stat::Health,16.0000000000,false},
 {"arcana_spellstorm","Spellstorm",Branch::Arcana,9,62,95,92,2,Stat::MagicDamage,0.0195000000000,true},
 {"arcana_aether_nova","Aether Nova",Branch::Arcana,10,72,96,-1,2,Stat::MagicDamage,0.0240000000000,true},
 {"arcana_infinite_wellspring","Infinite Wellspring",Branch::Arcana,10,72,97,-1,2,Stat::Mana,18.0000000000,false},
 {"arcana_runebound_heart","Runebound Heart",Branch::Arcana,10,72,98,-1,2,Stat::Health,18.0000000000,false},
 {"arcana_lunar_convergence","Lunar Convergence",Branch::Arcana,10,72,99,-1,2,Stat::MagicDamage,0.0210000000000,true},
 {"arcana_moonfall_arcanum","Moonfall Arcanum",Branch::Arcana,11,82,100,-1,1,Stat::MagicDamage,0.0255000000000,true},
 {"arcana_starborn_reservoir","Starborn Reservoir",Branch::Arcana,11,82,101,-1,1,Stat::Mana,20.0000000000,false},
 {"arcana_eternal_ward","Eternal Ward",Branch::Arcana,11,82,102,-1,1,Stat::Health,20.0000000000,false},
 {"arcana_arcana_ascendant","Arcana Ascendant",Branch::Arcana,11,82,103,-1,1,Stat::MagicDamage,0.0225000000000,true},
 {"might_cinder_vanguard","Cinder Vanguard",Branch::Might,12,86,40,-1,1,Stat::MeleeDamage,0.0260000000000,true},
 {"might_anvil_heart","Anvil Heart",Branch::Might,12,86,41,-1,1,Stat::Health,22.0000000000,false},
 {"might_ruin_cleaver","Ruin Cleaver",Branch::Might,12,86,42,-1,1,Stat::MeleeDamage,0.0230000000000,true},
 {"might_dreadnought","Dreadnought",Branch::Might,12,86,43,-1,1,Stat::Health,18.0000000000,false},
 {"might_sovereign_hammer","Sovereign Hammer",Branch::Might,13,91,108,-1,1,Stat::MeleeDamage,0.0280000000000,true},
 {"might_colossus_ward","Colossus Ward",Branch::Might,13,91,109,-1,1,Stat::Health,26.0000000000,false},
 {"might_shattercrest","Shattercrest",Branch::Might,13,91,110,-1,1,Stat::MeleeDamage,0.0270000000000,true},
 {"might_iron_dominion","Iron Dominion",Branch::Might,13,91,111,-1,1,Stat::Health,24.0000000000,false},
 {"might_eclipse_champion","Eclipse Champion",Branch::Might,14,96,112,-1,1,Stat::MeleeDamage,0.0320000000000,true},
 {"might_living_citadel","Living Citadel",Branch::Might,14,96,113,-1,1,Stat::Health,30.0000000000,false},
 {"might_final_reckoning","Final Reckoning",Branch::Might,14,96,114,-1,1,Stat::MeleeDamage,0.0350000000000,true},
 {"agility_duskwind_step","Duskwind Step",Branch::Agility,12,86,72,-1,1,Stat::Stamina,22.0000000000,false},
 {"agility_falcon_claw","Falcon Claw",Branch::Agility,12,86,73,-1,1,Stat::RangedDamage,0.0250000000000,true},
 {"agility_windborne","Windborne",Branch::Agility,12,86,74,-1,1,Stat::Stamina,21.0000000000,false},
 {"agility_silent_horizon","Silent Horizon",Branch::Agility,12,86,75,-1,1,Stat::RangedDamage,0.0230000000000,true},
 {"agility_mirage_runner","Mirage Runner",Branch::Agility,13,91,119,-1,1,Stat::Stamina,26.0000000000,false},
 {"agility_celestial_hunt","Celestial Hunt",Branch::Agility,13,91,120,-1,1,Stat::RangedDamage,0.0280000000000,true},
 {"agility_tempest_flight","Tempest Flight",Branch::Agility,13,91,121,-1,1,Stat::Stamina,25.0000000000,false},
 {"agility_specter_arrow","Specter Arrow",Branch::Agility,13,91,122,-1,1,Stat::RangedDamage,0.0270000000000,true},
 {"agility_ghost_of_dawn","Ghost of Dawn",Branch::Agility,14,96,123,-1,1,Stat::Stamina,30.0000000000,false},
 {"agility_skybreaker","Skybreaker",Branch::Agility,14,96,124,-1,1,Stat::RangedDamage,0.0340000000000,true},
 {"agility_horizon_sovereign","Horizon Sovereign",Branch::Agility,14,96,125,-1,1,Stat::Stamina,0.0310000000000,false},
 {"arcana_astral_ember","Astral Ember",Branch::Arcana,12,86,104,-1,1,Stat::MagicDamage,0.0270000000000,true},
 {"arcana_lunar_reservoir","Lunar Reservoir",Branch::Arcana,12,86,105,-1,1,Stat::Mana,22.0000000000,false},
 {"arcana_warden_of_stars","Warden of Stars",Branch::Arcana,12,86,106,-1,1,Stat::Health,22.0000000000,false},
 {"arcana_veil_of_eternity","Veil of Eternity",Branch::Arcana,12,86,107,-1,1,Stat::MagicDamage,0.0240000000000,true},
 {"arcana_nebula_spear","Nebula Spear",Branch::Arcana,13,91,130,-1,1,Stat::MagicDamage,0.0290000000000,true},
 {"arcana_endless_font","Endless Font",Branch::Arcana,13,91,131,-1,1,Stat::Mana,27.0000000000,false},
 {"arcana_aegis_of_twilight","Aegis of Twilight",Branch::Arcana,13,91,132,-1,1,Stat::Health,27.0000000000,false},
 {"arcana_rift_conduit","Rift Conduit",Branch::Arcana,13,91,133,-1,1,Stat::MagicDamage,0.0280000000000,true},
 {"arcana_astral_coronation","Astral Coronation",Branch::Arcana,14,96,134,-1,1,Stat::MagicDamage,0.0340000000000,true},
 {"arcana_fountain_of_moons","Fountain of Moons",Branch::Arcana,14,96,135,-1,1,Stat::Mana,32.0000000000,false},
 {"arcana_final_starward","Final Starward",Branch::Arcana,14,96,136,-1,1,Stat::Health,32.0000000000,false}
}};
using Ranks=std::array<int,NodeCount>;
enum class Result {Ok,InvalidLevel,InvalidRank,MissingPrerequisite,InsufficientPoints};
struct Snapshot {
 int Earned=0;
 int Spent=0;
 int Available=0;
 std::array<double,static_cast<int>(Stat::Count)> Flat{};
 std::array<double,static_cast<int>(Stat::Count)> Percent{};
};
inline int FindNode(const char* Id) {
 if (!Id || !*Id) return -1;
 for (int i=0;i<NodeCount;++i)
  if (std::strcmp(Nodes[i].Id,Id)==0) return i;
 return -1;
}
inline Result Evaluate(int Level,const Ranks& Allocated,Snapshot& Out) {
 Out=Snapshot{};
 if (Level<1 || Level>MaximumLevel) return Result::InvalidLevel;
 Out.Earned=(Level-1)*PointsPerLevel;
 for (int i=0;i<NodeCount;++i) {
  const Node& N=Nodes[i];
  const int Rank=Allocated[i];
  if (Rank<0 || Rank>N.MaxRank || (Rank>0 && Level<N.MinLevel))
   return Result::InvalidRank;
  if (Rank==0) continue;
  if ((N.RequiresA>=0 && Allocated[N.RequiresA]<1)
   || (N.RequiresB>=0 && Allocated[N.RequiresB]<1))
   return Result::MissingPrerequisite;
  Out.Spent+=Rank;
  const int S=static_cast<int>(N.AffectedStat);
  (N.IsPercent ? Out.Percent[S] : Out.Flat[S])+=Rank*N.AmountPerRank;
 }
 if (Out.Spent>Out.Earned) return Result::InsufficientPoints;
 Out.Available=Out.Earned-Out.Spent;
 return Result::Ok;
}
inline Result Spend(int Level,Ranks& Allocated,int NodeIndex) {
 if (NodeIndex<0 || NodeIndex>=NodeCount) return Result::InvalidRank;
 Snapshot Before;
 const Result Old=Evaluate(Level,Allocated,Before);
 if (Old!=Result::Ok) return Old;
 Ranks Candidate=Allocated;
 if (Candidate[NodeIndex]>=Nodes[NodeIndex].MaxRank) return Result::InvalidRank;
 ++Candidate[NodeIndex];
 Snapshot After;
 const Result Next=Evaluate(Level,Candidate,After);
 if (Next==Result::Ok) Allocated=Candidate;
 return Next;
}
// Read combat bonuses from the authoritative, validated allocation.
// Called only for attacks, never on Tick or by the render loop.
// An invalid rank ledger must never grant free damage.
inline double DamageMultiplier(int Level,const Ranks& Allocated,Stat Kind) {
 if (Kind!=Stat::MeleeDamage && Kind!=Stat::RangedDamage && Kind!=Stat::MagicDamage)
  return 1.0;
 Snapshot Summary;
 if (Evaluate(Level,Allocated,Summary)!=Result::Ok)return 1.0;
 const double Bonus=Summary.Percent[static_cast<int>(Kind)];
 if (!std::isfinite(Bonus))return 1.0;
 return 1.0+std::clamp(Bonus,0.0,1.0);
}

// Reconcile the *difference* between old and new valid allocations, so
// spending, respec and repeat loads never stack temporary resource caps.
struct Resources {
 double MaxHealth=100.0, Health=100.0;
 double MaxMana=100.0, Mana=100.0;
 double MaxStamina=100.0, Stamina=100.0;
};
inline bool ApplyResourceDelta(Resources& Live,const Snapshot& Before,const Snapshot& After) {
 const double DH=After.Flat[static_cast<int>(Stat::Health)]-Before.Flat[static_cast<int>(Stat::Health)];
 const double DM=After.Flat[static_cast<int>(Stat::Mana)]-Before.Flat[static_cast<int>(Stat::Mana)];
 const double DS=After.Flat[static_cast<int>(Stat::Stamina)]-Before.Flat[static_cast<int>(Stat::Stamina)];
 const double NextHealth=Live.MaxHealth+DH;
 const double NextMana=Live.MaxMana+DM;
 const double NextStamina=Live.MaxStamina+DS;
 if (!std::isfinite(Live.MaxHealth) || !std::isfinite(Live.Health)
     || !std::isfinite(Live.MaxMana) || !std::isfinite(Live.Mana)
     || !std::isfinite(Live.MaxStamina) || !std::isfinite(Live.Stamina)
     || !std::isfinite(NextHealth) || !std::isfinite(NextMana)
     || !std::isfinite(NextStamina)
     || Live.MaxHealth<1.0 || Live.MaxMana<=0.0 || Live.MaxStamina<=0.0
     || NextHealth<1.0 || NextMana<=0.0 || NextStamina<=0.0
     || NextHealth>100000.0 || NextMana>100000.0 || NextStamina>100000.0)
  return false;
 Resources Candidate=Live;
 Candidate.MaxHealth=NextHealth;
 Candidate.MaxMana=NextMana;
 Candidate.MaxStamina=NextStamina;
 // Additional capacity restores only the newly purchased amount, not full HP.
 Candidate.Health=Live.Health<=0.0 ? 0.0 : std::clamp(Live.Health+DH,0.0,NextHealth);
 Candidate.Mana=std::clamp(Live.Mana+DM,0.0,NextMana);
 Candidate.Stamina=std::clamp(Live.Stamina+DS,0.0,NextStamina);
 Live=Candidate;
 return true;
}

inline Result Respec(int Level,Ranks& Allocated) {
 Snapshot Before;
 const Result Old=Evaluate(Level,Allocated,Before);
 if (Old!=Result::Ok)return Old;
 Allocated.fill(0);
 return Result::Ok;
}
}
