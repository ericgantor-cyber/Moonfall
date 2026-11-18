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
static_assert(Same(Select(0,5,2,16,true,true,true,true),
                   {EAtlas::Masterwork,(16+5+2)%3,0,3,3}));
static_assert(Same(Select(2,4,1,15,true,true,true,true),
                   {EAtlas::Relic,(15+4+1)%3,2,3,3}));
static_assert(ValidCell(Select(1,5,1,25,true,true,true,true)));
static_assert(Same(Select(0,0,0,1,true,true,true,true,true,true),
                   {EAtlas::Wayfarer,1,0,3,3}));
static_assert(Same(Select(2,2,3,7,true,true,true,true,true,true),
                   {EAtlas::Wayfarer,(7+2+3)%3,2,3,3}));
static_assert(Same(Select(0,2,2,8,true,true,true,true,true,true),
                   {EAtlas::Trait,1,0,3,3}));
static_assert(Same(Select(0,5,1,22,true,true,true,true,true),
                   {EAtlas::Ascendant,(22+1+0)%3,0,3,3}));
static_assert(Same(Select(0,5,1,21,true,true,true,true,true),
                   {EAtlas::Masterwork,(21+5+1)%3,0,3,3}));
static_assert(Same(Select(0,4,1,25,true,true,true,true,true),
                   {EAtlas::Masterwork,(25+4+1)%3,0,3,3}));
static_assert(ValidCell(Select(1,0,0,1,false,false,false,false)));
static_assert(Same(Select(-1,5,1,25,true,true,true),
                   {EAtlas::None,0,0,0,0}));
static_assert(Same(Select(1,0,2,1,true,true,true),
                   {EAtlas::None,0,0,0,0}));

int main() {
 int checked=0, relicCells[3][3]={}, masterworkCells[3][3]={}, ascendantCells[3][3]={}, wayfarerCells[3][3]={};
 for(int slot=0;slot<3;++slot)
  for(int rarity=0;rarity<6;++rarity)
   for(int trait=0;trait<4;++trait)
    for(int tier=1;tier<=25;++tier) {
     if(!ValidEquipment(slot,rarity,trait,tier))continue;
     for(int mask=0;mask<64;++mask){
      const bool base=(mask&1)!=0, variant=(mask&2)!=0, relic=(mask&4)!=0;
      const bool masterwork=(mask&8)!=0, ascendant=(mask&16)!=0;
      const bool wayfarer=(mask&32)!=0;
      const FCell choice=Select(slot,rarity,trait,tier,base,variant,relic,masterwork,ascendant,wayfarer);
      assert(Same(choice,Select(slot,rarity,trait,tier,base,variant,relic,masterwork,ascendant,wayfarer)));
      assert(ValidCell(choice));
      if(choice.Atlas==EAtlas::None){
       assert(!base && !(variant&&trait>0) &&
              !(relic&&rarity>=3&&tier>=8) &&
              !(masterwork&&rarity>=4&&tier>=16) &&
              !(ascendant&&rarity==5&&tier>=22) &&
              !(wayfarer&&rarity<=2&&tier<=7));
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
       }else if(choice.Atlas==EAtlas::Relic) {
        assert(relic&&rarity>=3&&tier>=8);
        assert(!(masterwork&&rarity>=4&&tier>=16));
        assert(!(ascendant&&rarity==5&&tier>=22));
        assert(choice.Column==(tier+rarity+trait)%3);
        assert(choice.Row==slot);
        ++relicCells[choice.Row][choice.Column];
       }else if(choice.Atlas==EAtlas::Masterwork) {
        assert(masterwork&&rarity>=4&&tier>=16);
        assert(!(ascendant&&rarity==5&&tier>=22));
        assert(choice.Columns==3&&choice.Rows==3);
        assert(choice.Column==(tier+rarity+trait)%3);
        assert(choice.Row==slot);
        ++masterworkCells[choice.Row][choice.Column];
       }else if(choice.Atlas==EAtlas::Ascendant){
        assert(ascendant&&rarity==5&&tier>=22);
        assert(choice.Column==(tier+trait+slot)%3);
        assert(choice.Row==slot);
        assert(choice.Columns==3&&choice.Rows==3);
        ++ascendantCells[choice.Row][choice.Column];
       }else{
        assert(choice.Atlas==EAtlas::Wayfarer);
        assert(wayfarer&&rarity<=2&&tier<=7);
        assert(choice.Column==(tier+rarity+trait)%3);
        assert(choice.Row==slot);
        ++wayfarerCells[choice.Row][choice.Column];
       }
      }
      ++checked;
     }
    }
 for(auto& row:relicCells)for(int hits:row)assert(hits>0);
 for(auto& row:masterworkCells)for(int hits:row)assert(hits>0);
 for(auto& row:ascendantCells)for(int hits:row)assert(hits>0);
 for(auto& row:wayfarerCells)for(int hits:row)assert(hits>0);
 assert(checked==3*(25+5*4*25)*64);
 std::cout<<"Moonfall loot art: "<<checked
          <<" valid atlas/fallback combinations, all 36 upper-tier/wayfarer cells reachable OK\n";
}
