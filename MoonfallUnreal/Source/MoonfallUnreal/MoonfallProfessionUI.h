#pragma once
namespace MoonfallProfessionUI {
 constexpr int LineCount=19;
 constexpr int Rows(float PanelHeight) {
  if(!(PanelHeight>=280.f)) return 1;
  const int N=int((PanelHeight-206.f)/22.f);
  return N<1?1:N>LineCount?LineCount:N;
 }
 constexpr int Start(int Offset,int Visible) {
  if(Visible<1||Visible>=LineCount) return 0;
  return Offset<0?0:Offset>LineCount-Visible?LineCount-Visible:Offset;
 }
 constexpr int Step(int Offset,int Visible,int Direction) {
  const int Current=Start(Offset,Visible);
  return Direction==1?Start(Current+1,Visible):Direction==-1?Start(Current-1,Visible):Current;
 }
}
