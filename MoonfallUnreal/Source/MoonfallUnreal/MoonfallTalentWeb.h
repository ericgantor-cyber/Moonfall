#pragma once
// One deterministic layout and navigation policy for UE5's Canvas renderer,
// mouse hit tests, keyboard and controller. Pure C++17, testable without UE.
#include "MoonfallTalentRules.h"
#include <algorithm>
#include <cmath>
namespace MoonfallTalentWeb {
constexpr int MaxTier=14;
struct Rect {
 double X=0, Y=0, W=0, H=0;
 bool Hit(double px,double py) const {
  return std::isfinite(px) && std::isfinite(py) && W>0 && H>0
      && px>=X && px<X+W && py>=Y && py<Y+H;
 }
};
struct Layout {
 Rect Panel;
 int VisibleTiers=0;
 int FirstTier=1;
 double TileWidth=0, TileHeight=0;
 bool Valid=false;
};
inline Layout ForViewport(double width,double height,int firstTier) {
 Layout p;
 if (!std::isfinite(width) || !std::isfinite(height)
  || width<520.0 || height<430.0)return p;
 p.Panel.W=std::min(1020.0,width-24.0);
 p.Panel.H=std::min(690.0,height-24.0);
 p.Panel.X=(width-p.Panel.W)*0.5;
 p.Panel.Y=(height-p.Panel.H)*0.5;
 p.VisibleTiers=p.Panel.W<700.0 ? 3 : p.Panel.W<890.0 ? 4 : 5;
 p.FirstTier=std::clamp(firstTier,1,MaxTier-p.VisibleTiers+1);
 p.TileWidth=(p.Panel.W-44.0-12.0*p.VisibleTiers)/p.VisibleTiers;
 p.TileHeight=std::min(70.0,(p.Panel.H-270.0)/4.0);
 p.Valid=p.TileWidth>=100.0 && p.TileHeight>=36.0;
 return p;
}
inline int BranchOf(int nodeIndex) {
 return nodeIndex>=0 && nodeIndex<MoonfallTalents::NodeCount
    ? static_cast<int>(MoonfallTalents::Nodes[nodeIndex].Tree) : -1;
}
inline int Root(int branch) {
 if (branch<0 || branch>2)return -1;
 for(int i=0;i<MoonfallTalents::NodeCount;++i)
  if (BranchOf(i)==branch)return i;
 return -1;
}
inline int Row(int nodeIndex) {
 if (nodeIndex<0 || nodeIndex>=MoonfallTalents::NodeCount)return -1;
 const auto& target=MoonfallTalents::Nodes[nodeIndex];
 int row=0;
 for(int i=0;i<nodeIndex;++i) {
  const auto& candidate=MoonfallTalents::Nodes[i];
  if (candidate.Tree==target.Tree && candidate.Tier==target.Tier)++row;
 }
 return row;
}
inline Rect NodeRect(const Layout& p,int nodeIndex,int branch) {
 Rect empty;
 if (!p.Valid || BranchOf(nodeIndex)!=branch)return empty;
 const auto& n=MoonfallTalents::Nodes[nodeIndex];
 const int column=n.Tier-p.FirstTier;
 const int row=Row(nodeIndex);
 if (column<0 || column>=p.VisibleTiers || row<0 || row>=4)return empty;
 return {p.Panel.X+22.0+column*(p.TileWidth+12.0),
   p.Panel.Y+127.0+row*(p.TileHeight+8.0),
   p.TileWidth,p.TileHeight};
}
inline Rect BranchTab(const Layout& p,int branch) {
 if(!p.Valid || branch<0 || branch>2)return {};
 return {p.Panel.X+20.0+branch*std::min(170.0,(p.Panel.W-100.0)/3.0),
  p.Panel.Y+48.0,std::min(155.0,(p.Panel.W-120.0)/3.0),31.0};
}
inline Rect CloseRect(const Layout& p) {
 return p.Valid ? Rect{p.Panel.X+p.Panel.W-43.0,p.Panel.Y+12.0,28.0,28.0}:Rect{};
}
inline Rect SpendRect(const Layout& p) {
 return p.Valid ? Rect{p.Panel.X+p.Panel.W-368.0,p.Panel.Y+p.Panel.H-49.0,164.0,30.0}:Rect{};
}
inline Rect RespecRect(const Layout& p) {
 return p.Valid ? Rect{p.Panel.X+p.Panel.W-186.0,p.Panel.Y+p.Panel.H-49.0,164.0,30.0}:Rect{};
}
inline int HitNode(const Layout& p,int branch,double x,double y) {
 if(!p.Valid)return -1;
 for(int i=0;i<MoonfallTalents::NodeCount;++i)
  if(NodeRect(p,i,branch).Hit(x,y))return i;
 return -1;
}
inline int Move(int nodeIndex,int branch,int tierStep,int rowStep) {
 if (branch<0 || branch>2)return -1;
 if (BranchOf(nodeIndex)!=branch)nodeIndex=Root(branch);
 if(nodeIndex<0)return -1;
 const auto& current=MoonfallTalents::Nodes[nodeIndex];
 const int currentRow=Row(nodeIndex);
 int best=nodeIndex, distance=100000;
 const int targetTier=std::clamp(current.Tier+tierStep,1,MaxTier);
 const int targetRow=std::clamp(currentRow+rowStep,0,3);
 for(int i=0;i<MoonfallTalents::NodeCount;++i) {
  const auto& n=MoonfallTalents::Nodes[i];
  if(BranchOf(i)!=branch || n.Tier!=targetTier)continue;
  const int row=Row(i);
  const int diff=std::abs(row-targetRow);
  if(diff<distance) {distance=diff;best=i;}
 }
 return best;
}
inline int FirstTierForSelection(int nodeIndex,int current,int visibleTiers) {
 if(nodeIndex<0 || nodeIndex>=MoonfallTalents::NodeCount || visibleTiers<1)return current;
 const int tier=MoonfallTalents::Nodes[nodeIndex].Tier;
 const int first=std::clamp(current,1,MaxTier-visibleTiers+1);
 if(tier<first)return tier;
 if(tier>=first+visibleTiers)return tier-visibleTiers+1;
 return first;
}
}