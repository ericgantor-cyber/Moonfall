#pragma once
// Lightweight six-region living-world policy. Pure engine-free C++17.
#include <cmath>
namespace MoonfallLivingSimulation {
constexpr int RegionCount=6;
// Up to 80 weak resident references; select at most 20 nearby residents
// per world pulse, with rotating fairness when a settlement is crowded.
// Distant residents cost only lightweight eligibility checks, not movement.
constexpr int ResidentBudget=20;
constexpr int ResidentRegistryBudget=80;
constexpr int ResidentWindowCount(int registered) {
 return registered<=0?0:(registered<ResidentBudget?registered:ResidentBudget);
}
constexpr int ResidentWindowIndex(int cursor,int step,int registered) {
 return registered<=0 || cursor<0 || step<0 ? -1 : (cursor+step)%registered;
}
// This engine-free selector evaluates at most 80 cheap eligibility predicates
// and executes at most 20 expensive resident routines. When <=20 are nearby
// they are all updated on every 4-second pulse, regardless of registry slot.
// With >20 nearby, continuation begins immediately after the last inspected
// resident, so a crowded settlement gets fair round-robin service.
template <typename IsNearby, typename UpdateResident>
int ProcessNearbyResidents(int cursor,int registered,
                           IsNearby isNearby,UpdateResident updateResident) {
 if(registered<=0)return 0;
 const int start=cursor<0?0:cursor%registered;
 int inspected=0;
 int processed=0;
 while(inspected<registered && processed<ResidentBudget) {
  const int index=ResidentWindowIndex(start,inspected,registered);
  ++inspected;
  if(!isNearby(index))continue;
  updateResident(index);
  ++processed;
 }
 return (start+inspected)%registered;
}
constexpr float PulseSeconds=4.f;
// Two real hours per celestial day, beginning at noon for a fresh session.
// Sample the existing world clock; do not introduce another ticking actor.
constexpr double DaySeconds=7200.0;
// V17 was introduced by the stacked equipment PR and is not released.
// -1 means a legacy save with no celestial state.
inline bool ValidSavedClock(double Clock){return Clock==-1.0||(std::isfinite(Clock)&&Clock>=0.0&&Clock<DaySeconds);}
inline double SavedClock(double Clock){return ValidSavedClock(Clock)&&Clock>=0.0?Clock:0.0;}
inline double AdvanceClock(double Saved,double Anchor,double Now){
 const double Base=SavedClock(Saved);
 if(!std::isfinite(Anchor)||!std::isfinite(Now)||Anchor<0.0||Now<Anchor)return Base;
 return std::fmod(Base+std::fmod(Now-Anchor,DaySeconds),DaySeconds);
}
inline int CelestialMinute(double Elapsed){
 if(!std::isfinite(Elapsed)||Elapsed<0.0)return 720;
 const double Wrapped=std::fmod(Elapsed,DaySeconds);
 return (720+static_cast<int>(Wrapped/5.0))%1440;
}
enum class EDayPhase { Night, Dawn, Day, Dusk };
constexpr EDayPhase DayPhase(int Minute){
 return Minute>=288&&Minute<504?EDayPhase::Dawn:
  Minute>=504&&Minute<936?EDayPhase::Day:
  Minute>=936&&Minute<1152?EDayPhase::Dusk:EDayPhase::Night;
}
constexpr const char* DayPhaseName(EDayPhase Phase){
 switch(Phase){
 case EDayPhase::Dawn:return "DAWN";
 case EDayPhase::Day:return "DAY";
 case EDayPhase::Dusk:return "DUSK";
 default:return "NIGHT";
 }
}
constexpr int LightingHour(int Minute){return Minute>=0&&Minute<1440?Minute/60:12;}
// Ten in-game minute buckets prevent per-frame changes to global light.
constexpr int CelestialLightBucket(int Minute){return Minute>=0&&Minute<1440?Minute/10:72;}
constexpr float SunlightAmount(int Bucket){
 if(Bucket<0||Bucket>=144)return 1.f;
 const int Distance=Bucket>=72?Bucket-72:72-Bucket;
 if(Distance>=48)return 0.f;
 const float T=1.f-static_cast<float>(Distance)/48.f;
 return T*T*(3.f-2.f*T);
}
constexpr float GlobalLightScale(int Bucket){return 0.28f+0.72f*SunlightAmount(Bucket);}
// Never extinguish weather lighting or alter authored sanctuary lighting.
constexpr float CelestialLightScale(int Hour){
 if(Hour<0||Hour>23)return 1.f;
 const int Distance=Hour>12?Hour-12:12-Hour;
 return 1.f-0.025f*static_cast<float>(Distance);
}
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
// Nearby civilians must still retreat during danger or severe weather,
// and guards should return to their posts. A conversation always pins the
// focused resident in place so mouse/gamepad dialogue is not interrupted.
constexpr bool PauseResidentMotion(bool inConversation,bool nearHero,
                                   bool sheltering,bool holdingPost) {
 return inConversation || (nearHero && !sheltering && !holdingPost);
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
static_assert(ResidentWindowCount(0)==0,"Empty resident registry");
static_assert(ResidentWindowCount(8)==8,"Small settlement pulse");
static_assert(ResidentWindowCount(80)==20,"Large settlement pulse");
static_assert(ResidentWindowIndex(60,19,80)==79,"Final resident in a pulse");
static_assert(ResidentWindowIndex(60,20,80)==0,"Registry wrap");
static_assert(ResidentWindowIndex(0,0,0)==-1,"No empty modulo");
static_assert(RegionFromWorldX(0.f)==3,"World region mapping");
static_assert(ActivityFor(ERole::Merchant,0,EWeather::Storm)==EActivity::Shelter,"Weather routines");
static_assert(!MayEncounter(2,true,true,false,false),"Sanctuary is safe");
} // namespace MoonfallLivingSimulation
