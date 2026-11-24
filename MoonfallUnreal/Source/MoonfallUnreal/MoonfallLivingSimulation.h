#pragma once
// Lightweight six-region living-world policy. Pure engine-free C++17.
#include <cmath>
namespace MoonfallLivingSimulation {
constexpr int RegionCount=6;
constexpr int ResidentBudget=20;
constexpr float PulseSeconds=4.f;
enum class EWeather { Clear, Fog, Rain, Storm, Snow, Ash };
enum class EEvent { Merchant, Patrol, Rescue, Dispute, Monster };
enum class EActivity { Work, Patrol, Travel, Rest, Shelter };
enum class ERole { Citizen, Guard, Merchant, Scout, Scholar, Tinker, Elder };
// Short lived, per-resident danger memory drives behaviour as well as dialogue.
// It never creates save fields, enemy targets or independent NPC ticking.
enum class EAlertReaction { Normal, HoldPost, SeekShelter };
inline EAlertReaction ReactionFor(ERole role,float secondsSinceDanger) {
 if(!std::isfinite(secondsSinceDanger) ||
     secondsSinceDanger<0.f || secondsSinceDanger>=85.f)
  return EAlertReaction::Normal;
 return role==ERole::Guard?EAlertReaction::HoldPost:EAlertReaction::SeekShelter;
}
inline int Phase(float time, float duration) {
 if(!std::isfinite(time)||time<0.f||!std::isfinite(duration)||duration<=0.f)return 0;
 return static_cast<int>(std::fmin(time/duration,10000000.f));
}
constexpr int RegionFromWorldX(float x) {
 if(!(x>=-20250.f&&x<20250.f))return -1;
 return static_cast<int>((x+20250.f)/6750.f);
}
// Road encounters should appear ahead of actual east/west travel, not
// permanently on the east. A candidate may never cross a province boundary:
// switch sides near the edge rather than silently skipping the event.
// Returning zero rejects invalid coordinates or movement, with no actor scan.
inline float RoadEncounterOffset(float worldX,float travelVelocityX) {
 if(!std::isfinite(worldX)||!std::isfinite(travelVelocityX))return 0.f;
 const int region=RegionFromWorldX(worldX);
 if(region<0)return 0.f;
 const float preferred=travelVelocityX<-25.f?-600.f:600.f;
 if(RegionFromWorldX(worldX+preferred)==region)return preferred;
 if(RegionFromWorldX(worldX-preferred)==region)return -preferred;
 return 0.f;
}
constexpr EWeather WeatherFor(int region,int phase) {
 if(region<0||region>=6||phase<0)return EWeather::Clear;
 const int cycle=(phase+region*3)%6;
 if(region==4)return cycle<3?EWeather::Snow:(cycle==3?EWeather::Storm:EWeather::Fog);
 if(region==0||region==5)return cycle<2?EWeather::Ash:
   (cycle<4?EWeather::Clear:EWeather::Storm);
 if(region==1)return cycle<3?EWeather::Fog:(cycle<5?EWeather::Rain:EWeather::Clear);
 if(region==2)return cycle<2?EWeather::Fog:(cycle<4?EWeather::Rain:EWeather::Clear);
 return cycle<2?EWeather::Rain:(cycle<4?EWeather::Storm:EWeather::Clear);
}
constexpr const char* WeatherName(EWeather w) {
 switch(w) {
 case EWeather::Clear:return "CLEAR";
 case EWeather::Fog:return "FOG";
 case EWeather::Rain:return "RAIN";
 case EWeather::Storm:return "STORM";
 case EWeather::Snow:return "SNOW";
 case EWeather::Ash:return "ASHFALL";
 }return "CLEAR";
}
constexpr bool Severe(EWeather w){return w==EWeather::Storm||w==EWeather::Snow;}
constexpr EActivity ActivityFor(ERole role,int phase,EWeather weather) {
 const int shift=phase<0?0:phase%4;
 if(Severe(weather)&&role!=ERole::Guard)return EActivity::Shelter;
 if(shift==3)return role==ERole::Guard?EActivity::Patrol:EActivity::Rest;
 if(role==ERole::Guard)return shift==1?EActivity::Patrol:EActivity::Work;
 if(role==ERole::Scout)return shift==0?EActivity::Travel:EActivity::Work;
 if(role==ERole::Merchant||role==ERole::Citizen)
  return shift==2?EActivity::Travel:EActivity::Work;
 return shift==2?EActivity::Rest:EActivity::Work;
}
constexpr EEvent EncounterFor(int region,int phase) {
 if(region<0||region>=6||phase<0)return EEvent::Patrol;
 const int slot=(region*7+phase*3)%5;
 return slot==0?EEvent::Merchant:slot==1?EEvent::Patrol:
  slot==2?EEvent::Rescue:slot==3?EEvent::Dispute:EEvent::Monster;
}
// Weather influences which story can start; avoid unexpected hostile spawns
// during severe, low-visibility conditions. Ordinary weather is unchanged.
constexpr EEvent EncounterForWeather(int region,int phase,EWeather weather) {
 const EEvent planned=EncounterFor(region,phase);
 if(Severe(weather))
  return planned==EEvent::Patrol?EEvent::Patrol:EEvent::Rescue;
 return planned;
}
// Encounter expiry is deliberately paused while a player is actively
// speaking with its visitor or fighting its nearby monster. No actor Tick,
// extra timers, quest state or immortal crowd of world events is required.
inline bool ShouldRetireAmbient(float now,float expiresAt,bool conversationActive,
                                bool monsterBattleActive) {
 return std::isfinite(now)&&std::isfinite(expiresAt)&&now>=expiresAt
      &&!conversationActive&&!monsterBattleActive;
}
constexpr bool MayEncounter(int region,bool sanctuary,bool nearRoad,bool inMenu,bool eventAlive){
 return region>=0&&region<6&&!sanctuary&&nearRoad&&!inMenu&&!eventAlive;
}
static_assert(RegionFromWorldX(0.f)==3,"World region mapping");
static_assert(ActivityFor(ERole::Merchant,0,EWeather::Storm)==EActivity::Shelter,"Weather routines");
static_assert(!MayEncounter(2,true,true,false,false),"Sanctuary is safe");
} // namespace MoonfallLivingSimulation
