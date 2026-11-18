// Pure C++17 checks for the shipping Moonfall loot-art selection function.
// No Unreal Engine runtime, packaged asset or render test is implied.
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallLootArtRules.h"
#include <array>
#include <cassert>
#include <iostream>
using namespace MoonfallLootArt;

constexpr bool Same(FCell A,FCell B) {
 return A.Atlas==B.Atlas && A.Column==B.Column && A.Row==B.Row &&
  A.Columns==B.Columns && A.Rows==B.Rows;
}
static_assert(Same(Select(0,0,0,1,false,false,false),
                   {EAtlas::None,0,0,0,0}));
static_assert(Same(Select(0,0,0,1,true,true,true),
                   {EAtlas::Base,0,0,3,2}));
static_assert(Same(Select(1,3,2,5,true,true,true),
                   {EAtlas::Trait,0,1,3,3}));
static_assert(Same(Select(2,4,3,12,true,true,true),
                   {EAtlas::Relic,(12+4+3)%3,2,3,3}));
static_assert(Same(Select(0,3,1,8,true,true,false),
                   {EAtlas::Trait,0,0,3,3}));
static_assert(Same(Select(-1,5,1,25,true,true,true),
                   {EAtlas::None,0,0,0,0}));
static_assert(Same(Select(1,0,2,1,true,true,true),
                   {EAtlas::None,0,0,0,0}));

int main() {
 int checked=0, relicCells[3][3]={};
 for(int slot=0;slot<3;++slot)
  for(int rarity=0;rarity<6;++rarity)
   for(int trait=0;trait<4;++trait)
    for(int tier=1;tier<=25;++tier) {
     if(!ValidEquipment(slot,rarity,trait,tier))continue;
     for(int mask=0;mask<8;++mask){
      const bool base=(mask&1)!=0, variant=(mask&2)!=0, relic=(mask&4)!=0;
      const FCell choice=Select(slot,rarity,trait,tier,base,variant,relic);
      assert(Same(choice,Select(slot,rarity,trait,tier,base,variant,relic)));
      if(choice.Atlas==EAtlas::None){
       assert(!base && !(variant&&trait>0) &&
              !(relic&&rarity>=3&&tier>=8));
       assert(choice.Columns==0&&choice.Rows==0);
      }else {
       assert(choice.Column>=0&&choice.Column<choice.Columns);
       assert(choice.Row>=0&&choice.Row<choice.Rows);
       if(choice.Atlas==EAtlas::Base){
        assert(base&&choice.Row==0&&choice.Column==slot);
        assert(choice.Columns==3&&choice.Rows==2);
       }else if(choice.Atlas==EAtlas::Trait){
        assert(variant&&trait>0&&choice.Row==slot);
        assert(choice.Columns==3&&choice.Rows==3);
       }else {
        assert(choice.Atlas==EAtlas::Relic);
        assert(relic&&rarity>=3&&tier>=8);
        assert(choice.Column==(tier+rarity+trait)%3);
        assert(choice.Row==slot);
        ++relicCells[choice.Row][choice.Column];
       }
      }
      ++checked;
     }
    }
 for(auto& row:relicCells)for(int hits:row)assert(hits>0);
 assert(checked==3*(25+5*4*25)*8);
 std::cout<<"Moonfall loot art: "<<checked
          <<" valid atlas/fallback combinations, all 9 relic cells reachable OK\n";
}
