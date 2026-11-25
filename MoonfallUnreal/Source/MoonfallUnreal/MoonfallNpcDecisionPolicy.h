#pragma once
// Engine-independent policy. Keep enum order aligned with the Blueprint enum.
namespace MoonfallNpcPolicy {
enum class Action { Idle, Work, Eat, Socialize, Sleep, Shelter, Investigate, Flee, Defend, SeekHelp, Talk };
struct Input {
 float hour=12, courage=.5f, sociability=.5f, aggression=.3f, diligence=.5f;
 float danger=0, curiosity=0, trust=0; // trust: -1..1
 bool storm=false, sheltered=false, playerNearby=false, allyNearby=false, canDefend=true;
};
inline float Clamp(float x,float low=0,float high=1) { return x<low?low:(x>high?high:x); }
inline Action Decide(const Input& i) {
 const float danger=Clamp(i.danger), courage=Clamp(i.courage), aggression=Clamp(i.aggression);
 // Threats always outrank ordinary routines; assistance is preferred by timid NPCs near allies.
 if(danger>=.35f) {
   if(i.canDefend && courage*.75f+aggression*.25f>=.55f+danger*.25f) return Action::Defend;
   if(i.allyNearby && danger<.85f && courage<.55f) return Action::SeekHelp;
   return Action::Flee;
 }
 if(i.storm && !i.sheltered) return Action::Shelter;
 if(i.curiosity>=.35f && courage>=.25f) return Action::Investigate;
 if(i.playerNearby && i.trust>=0 && Clamp(i.sociability)+Clamp(i.trust,0,1)*.4f>=.65f) return Action::Talk;
 if(i.hour<6 || i.hour>=22) return Action::Sleep;
 if((i.hour>=7 && i.hour<8) || (i.hour>=12 && i.hour<13) || (i.hour>=18 && i.hour<19)) return Action::Eat;
 if(i.hour>=8 && i.hour<18 && i.diligence>=.3f) return Action::Work;
 if(i.sociability>=.45f) return Action::Socialize;
 return Action::Idle;
}
}
