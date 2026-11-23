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
 // The first Arcana damage rank is gated by its Spirit Well prerequisite.
 // It must become 4% more *offensive* magic damage, not free healing,
 // and spending it must use the existing finite 2-points/level budget.
 assert(Spend(5,R,FindNode("spell_focus"))==Result::Ok);
 assert(Evaluate(5,R,S)==Result::Ok && S.Available==5 && S.Spent==3);
 const double ArcanaDamage=100.0 * (1.0+S.Percent[static_cast<int>(Stat::MagicDamage)]);
 assert(std::abs(ArcanaDamage-104.0)<1e-8);
 // A rank cannot apply when the prerequisite is missing.
 Ranks Disconnected{};
 Disconnected[FindNode("spell_focus")]=1;
 assert(Evaluate(5,Disconnected,S)==Result::MissingPrerequisite);
 R[FindNode("swift_step")]=-1;
 assert(Evaluate(5,R,S)==Result::InvalidRank);
 R.fill(0);
 assert(Spend(100,R,-1)==Result::InvalidRank);
 assert(Evaluate(100,R,S)==Result::Ok && S.Available==198);
 return 0;
}
