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
// Regional pressure is transient, bounded and updated on the existing pulse.
// Dungeon victories lower baseline danger; recent local combat raises pressure.
// The pressure is not evidence of crime and never authorizes NPC aggression.
constexpr int ClampPressure(int Value){return Value<0?0:Value>60?60:Value;}
constexpr int StepRegionalPressure(int Previous,bool LocalCombat){
 return ClampPressure(ClampPressure(Previous)+(LocalCombat?8:-2));
}
constexpr int RegionalDanger(int Pressure,int DefeatedBosses){
 if(DefeatedBosses<0||DefeatedBosses>5)return -1;
 return (5-DefeatedBosses)*8+ClampPressure(Pressure);
}
constexpr const char* RegionalDangerName(int Danger){
 return Danger<0||Danger>100?"UNKNOWN":Danger>=65?"HIGH":Danger>=35?"WATCH":"CALM";
}
// Reuse the existing one-event scheduler. A dangerous province gets more
// watch patrols; liberated provinces favour trade. Severe weather always wins.
constexpr EEvent ReactiveEncounter(EEvent Planned,EWeather Weather,int Danger,int DefeatedBosses){
 if(Danger<0||Danger>100||DefeatedBosses<0||DefeatedBosses>5)return Planned;
 if(Severe(Weather))return Planned==EEvent::Patrol?EEvent::Patrol:EEvent::Rescue;
 if(Danger>=65&&(Planned==EEvent::Merchant||Planned==EEvent::Dispute))return EEvent::Patrol;
 if(Danger<35&&DefeatedBosses==5&&Planned==EEvent::Monster)return EEvent::Merchant;
 return Planned;
}
// Regional concern is deliberately separate from firsthand danger memory.
enum class ERegionalRoutine { Normal, CautiousWork, GuardWatch, ReturnHome };
constexpr ERegionalRoutine RegionalRoutine(ERole Role,int Danger,bool Eligible){
 if(!Eligible||Danger<65||Danger>100)return ERegionalRoutine::Normal;
 switch(Role){
 case ERole::Guard:return ERegionalRoutine::GuardWatch;
 case ERole::Scout:case ERole::Merchant:return ERegionalRoutine::CautiousWork;
 default:return ERegionalRoutine::ReturnHome;
 }
}
constexpr bool RegionalRoutineAllowsGuide(ERegionalRoutine Routine){
 return Routine==ERegionalRoutine::Normal;
}
static_assert(RegionalRoutine(ERole::Guard,65,true)==ERegionalRoutine::GuardWatch,"Guard watch");
static_assert(RegionalRoutine(ERole::Merchant,90,true)==ERegionalRoutine::CautiousWork,"Merchant stays available");
static_assert(RegionalRoutine(ERole::Citizen,90,true)==ERegionalRoutine::ReturnHome,"Civilian returns home");
static_assert(RegionalRoutine(ERole::Citizen,90,false)==ERegionalRoutine::Normal,"Protected resident");
static_assert(RegionalRoutine(ERole::Scout,101,true)==ERegionalRoutine::Normal,"Invalid danger fails closed");
static_assert(StepRegionalPressure(58,true)==60,"Pressure cap");
static_assert(StepRegionalPressure(1,false)==0,"Pressure decay");
static_assert(RegionalDanger(60,0)==100&&RegionalDanger(0,5)==0,"Danger bounds");
static_assert(RegionalDanger(0,6)==-1,"Invalid dungeon count");
static_assert(ReactiveEncounter(EEvent::Merchant,EWeather::Clear,70,0)==EEvent::Patrol,"High pressure patrol");
static_assert(ReactiveEncounter(EEvent::Monster,EWeather::Clear,10,5)==EEvent::Merchant,"Liberated region trade");
static_assert(ReactiveEncounter(EEvent::Monster,EWeather::Storm,10,5)==EEvent::Rescue,"Weather priority");
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
