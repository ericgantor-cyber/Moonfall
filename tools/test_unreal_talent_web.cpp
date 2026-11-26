#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallTalentWeb.h"
#include <cassert>
#include <cmath>
#include <limits>
using namespace MoonfallTalentWeb;
int main() {
 assert(ResetConfirmationSeconds==3.0);
 assert(!ResetConfirmationActive(false,10.0,11.0));
 assert(ResetConfirmationActive(true,10.0,10.0));
 assert(ResetConfirmationActive(true,10.0,13.0));
 assert(!ResetConfirmationActive(true,10.0,13.001));
 assert(!ResetConfirmationActive(true,10.0,9.9));
 assert(!ResetConfirmationActive(true,-1.0,0.0));
 assert(!ResetConfirmationActive(true,10.0,std::numeric_limits<double>::infinity()));
 assert(!ResetConfirmationActive(true,std::numeric_limits<double>::quiet_NaN(),10.0));
 const auto Normal=ForViewport(1280.0,800.0,1);
 assert(Normal.Valid && Normal.VisibleTiers==5 && Normal.FirstTier==1);
 assert(Normal.Panel.W<=1020 && Normal.Panel.H<=690);
 assert(!ForViewport(480.0,800.0,1).Valid);
 assert(!ForViewport(1280.0,320.0,1).Valid);
 assert(!ForViewport(std::numeric_limits<double>::quiet_NaN(),720.0,1).Valid);
 assert(ForViewport(600.0,480.0,80).FirstTier==12);
 for (int branch=0;branch<3;++branch) {
  const int root=Root(branch);
  assert(root>=0 && BranchOf(root)==branch);
  assert(Row(root)==0);
  int count=0,visible=0;
  for (int i=0;i<MoonfallTalents::NodeCount;++i) {
   if (BranchOf(i)!=branch) {
    assert(NodeRect(Normal,i,branch).W==0);
    continue;
   }
   ++count;
   const int row=Row(i);
   assert(row>=0 && row<4);
   const auto n=MoonfallTalents::Nodes[i];
   auto local=ForViewport(1280.0,800.0,n.Tier);
   const auto rect=NodeRect(local,i,branch);
   assert(rect.W>100 && rect.H>=36);
   assert(rect.X>=local.Panel.X && rect.X+rect.W<=local.Panel.X+local.Panel.W);
   assert(rect.Y>=local.Panel.Y && rect.Y+rect.H<local.Panel.Y+local.Panel.H-110);
   assert(HitNode(local,branch,rect.X+rect.W/2,rect.Y+rect.H/2)==i);
   if (n.Tier<=Normal.FirstTier+Normal.VisibleTiers-1)++visible;
   // Each axis navigation retains the selected branch and finds
   // the nearest authored row in the requested tier.
   const int right=Move(i,branch,1,0);
   const int down=Move(i,branch,0,1);
   assert(BranchOf(right)==branch && BranchOf(down)==branch);
   assert(MoonfallTalents::Nodes[right].Tier>=n.Tier);
   assert(MoonfallTalents::Nodes[down].Tier==n.Tier);
   // Every node can switch branches while retaining its tier.
   for (int target=0;target<3;++target) {
    const int next=SameTierInBranch(i,target);
    assert(next>=0 && BranchOf(next)==target);
    assert(MoonfallTalents::Nodes[next].Tier==n.Tier);
    if (target==branch) assert(next==i);
    for (int candidate=0;candidate<MoonfallTalents::NodeCount;++candidate) {
     if (BranchOf(candidate)==target && MoonfallTalents::Nodes[candidate].Tier==n.Tier)
      assert(std::abs(Row(next)-row)<=std::abs(Row(candidate)-row));
    }
   }
  }
  assert(count==47 && visible>0);
  const auto rootRect=NodeRect(Normal,root,branch);
  assert(HitNode(Normal,branch,rootRect.X+2,rootRect.Y+2)==root);
  assert(HitNode(Normal,branch,-10,-10)==-1);
  assert(HitNode(Normal,branch,std::numeric_limits<double>::infinity(),0)==-1);
  assert(FirstTierForSelection(root,11,Normal.VisibleTiers)==1);
 }
 assert(SameTierInBranch(-1,0)==Root(0));
 assert(SameTierInBranch(0,-1)==-1);
 assert(SameTierInBranch(0,3)==-1);
 const auto spend=SpendRect(Normal),respec=RespecRect(Normal);
 assert(spend.W>0 && respec.W>0 && spend.X+spend.W<respec.X);
 assert(CloseRect(Normal).Hit(Normal.Panel.X+Normal.Panel.W-25,
    Normal.Panel.Y+20));
 for(int branch=0;branch<3;++branch) {
  const auto tab=BranchTab(Normal,branch);
  assert(tab.W>0 && tab.H>0 && tab.Hit(tab.X+4,tab.Y+4));
 }
 return 0;
}
