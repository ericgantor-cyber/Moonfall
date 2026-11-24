#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallTalentRules.h"
#include <cassert>
#include <cmath>
#include <cstring>
using namespace MoonfallTalents;

int main() {
 static_assert(NodeCount==141,"Godot constellation has exactly 141 nodes");
 static_assert(PointsPerLevel==2,"Two talent points per level");
 int Counts[3]={0,0,0};
 for (const Node& N:Nodes) {
  ++Counts[static_cast<int>(N.Tree)];
  assert(N.MaxRank>=1 && N.MinLevel>=1 && N.MinLevel<=100);
  if (N.RequiresA>=0) assert(N.RequiresA<NodeCount);
  if (N.RequiresB>=0) assert(N.RequiresB<NodeCount);
 }
 assert(Counts[0]==47 && Counts[1]==47 && Counts[2]==47);
 Ranks R{};
 Snapshot S{};
 assert(Evaluate(1,R,S)==Result::Ok && S.Earned==0 && S.Available==0);
 assert(Spend(1,R,FindNode("iron_will"))==Result::InsufficientPoints);
 assert(R[FindNode("iron_will")]==0);
 assert(Evaluate(0,R,S)==Result::InvalidLevel);
 assert(Evaluate(101,R,S)==Result::InvalidLevel);
 assert(FindNode("not_a_talent")==-1 && FindNode(nullptr)==-1);
 assert(Spend(2,R,FindNode("iron_will"))==Result::Ok);
 assert(Spend(2,R,FindNode("iron_will"))==Result::Ok);
 assert(Spend(2,R,FindNode("iron_will"))==Result::InsufficientPoints);
 assert(R[FindNode("iron_will")]==2);
 assert(Spend(4,R,FindNode("breaker"))==Result::Ok);
 assert(Evaluate(4,R,S)==Result::Ok && S.Earned==6 && S.Spent==3 && S.Available==3);
 assert(std::abs(S.Percent[static_cast<int>(Stat::MeleeDamage)]-(0.03*2+0.04))<1e-8);
 // Valid level gate cannot be bypassed by preallocating an upper-tier node.
 Ranks Gate{};
 Gate[FindNode("breaker")]=1;
 assert(Evaluate(2,Gate,S)==Result::InvalidRank);
 assert(Evaluate(10,Gate,S)==Result::MissingPrerequisite);
 // Incompatible parent ranks cannot silently survive respec or reload.
 assert(Respec(10,Gate)==Result::MissingPrerequisite);
 assert(Respec(10,R)==Result::Ok);
 assert(Evaluate(10,R,S)==Result::Ok && S.Available==18 && S.Spent==0);
 for(int Rank:R)assert(Rank==0);
 // Root of every branch can be spent without a character-class lock.
 assert(Spend(5,R,FindNode("swift_step"))==Result::Ok);
 assert(Spend(5,R,FindNode("spirit_well"))==Result::Ok);
 assert(Evaluate(5,R,S)==Result::Ok && S.Available==6);
 assert(std::abs(S.Flat[static_cast<int>(Stat::Stamina)]-5.0)<1e-8);
 assert(std::abs(S.Flat[static_cast<int>(Stat::Mana)]-8.0)<1e-8);
 R[FindNode("swift_step")]=-1;
 assert(Evaluate(5,R,S)==Result::InvalidRank);
 R.fill(0);
 assert(Spend(100,R,-1)==Result::InvalidRank);
 assert(Evaluate(100,R,S)==Result::Ok && S.Available==198);
 // Resource caps must change by the delta, and undo exactly on respec.
 Ranks Empty{}, BoostedRanks{};
 Snapshot EmptySummary{}, BoostedSummary{};
 assert(Evaluate(5,Empty,EmptySummary)==Result::Ok);
 assert(Spend(5,BoostedRanks,FindNode("iron_will"))==Result::Ok);
 assert(Spend(5,BoostedRanks,FindNode("guard"))==Result::Ok);
 assert(Spend(5,BoostedRanks,FindNode("swift_step"))==Result::Ok);
 assert(Spend(5,BoostedRanks,FindNode("spirit_well"))==Result::Ok);
 assert(Evaluate(5,BoostedRanks,BoostedSummary)==Result::Ok);
 Resources Live{100.0,70.0,80.0,35.0,60.0,18.0};
 assert(ApplyResourceDelta(Live,EmptySummary,BoostedSummary));
 assert(Live.MaxHealth==108.0 && Live.Health==78.0);
 assert(Live.MaxMana==88.0 && Live.Mana==43.0);
 assert(Live.MaxStamina==65.0 && Live.Stamina==23.0);
 const Resources Once=Live;
 assert(ApplyResourceDelta(Live,BoostedSummary,BoostedSummary));
 assert(Live.MaxHealth==Once.MaxHealth && Live.Health==Once.Health);
 assert(Live.MaxMana==Once.MaxMana && Live.Mana==Once.Mana);
 assert(Live.MaxStamina==Once.MaxStamina && Live.Stamina==Once.Stamina);
 assert(ApplyResourceDelta(Live,BoostedSummary,EmptySummary));
 assert(Live.MaxHealth==100.0 && Live.Health==70.0);
 assert(Live.MaxMana==80.0 && Live.Mana==35.0);
 assert(Live.MaxStamina==60.0 && Live.Stamina==18.0);
 // Never resurrect a defeated actor by adding a health talent.
 Resources Defeated{100.0,0.0,80.0,35.0,60.0,18.0};
 assert(ApplyResourceDelta(Defeated,EmptySummary,BoostedSummary));
 assert(Defeated.Health==0.0);
 // Invalid overflow must fail atomically without partially changing resources.
 Resources Overflow{99999.0,100.0,80.0,35.0,60.0,18.0};
 const Resources OverflowBefore=Overflow;
 assert(!ApplyResourceDelta(Overflow,EmptySummary,BoostedSummary));
 assert(Overflow.MaxHealth==OverflowBefore.MaxHealth && Overflow.Mana==OverflowBefore.Mana);
 // Real sword/spell talent scaling, with strictly validated ranks.
 Ranks MeleeAlloc{};
 assert(Spend(5,MeleeAlloc,FindNode("iron_will"))==Result::Ok);
 assert(std::abs(DamageMultiplier(5,MeleeAlloc,Stat::MeleeDamage)-1.03)<1e-10);
 assert(DamageMultiplier(5,MeleeAlloc,Stat::MagicDamage)==1.0);
 Ranks SpellAlloc{};
 assert(Spend(5,SpellAlloc,FindNode("spirit_well"))==Result::Ok);
 assert(Spend(5,SpellAlloc,FindNode("spell_focus"))==Result::Ok);
 assert(std::abs(DamageMultiplier(5,SpellAlloc,Stat::MagicDamage)-1.04)<1e-10);
 assert(DamageMultiplier(5,SpellAlloc,Stat::MeleeDamage)==1.0);
 assert(DamageMultiplier(5,SpellAlloc,Stat::Health)==1.0);
 SpellAlloc[FindNode("spirit_well")]=0; // Orphaned child is an invalid save.
 assert(DamageMultiplier(5,SpellAlloc,Stat::MagicDamage)==1.0);
 assert(DamageMultiplier(0,MeleeAlloc,Stat::MeleeDamage)==1.0);
 return 0;
}
