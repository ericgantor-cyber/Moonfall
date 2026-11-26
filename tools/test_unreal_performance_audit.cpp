#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallPerformanceAudit.h"
#include <cassert>
#include <limits>
using namespace MoonfallPerformance;
#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallBugBreadcrumbs.h"
int main() {
 using MoonfallDiagnostics::FBreadcrumbs;
 using MoonfallDiagnostics::EEvent;
 FBreadcrumbs events;
 events.Record(EEvent::SpellCast,1.0);
 events.Record(EEvent::SpellCast,1.1);
 assert(events.Size()==1);
 events.Record(EEvent::DamageTaken,1.2);
 assert(events.Size()==2 && events.At(1).Kind==EEvent::DamageTaken);
 events.Record(EEvent::OptionsOpened,std::numeric_limits<double>::quiet_NaN());
 assert(events.Size()==2);
 for(int i=0;i<40;++i) events.Record(EEvent::GraphicsChanged,2.0+i);
 assert(events.Size()==32 && events.At(0).At==10.0 && events.At(31).At==41.0);
 events.Record(EEvent::OptionsClosed,0.5);
 assert(events.Size()==1 && events.At(0).Kind==EEvent::OptionsClosed);

 FAudit audit;
 double now=0;
 audit.Observe(now,true);
 assert(Advice(audit.Snapshot)==EAdvice::Collecting);
 for(int i=0;i<300;++i) { now+=1.0/60; audit.Observe(now,true); }
 assert(audit.Snapshot.Ready && audit.Snapshot.Samples==240);
 assert(std::abs(audit.Snapshot.AverageFPS-60)<.001);
 assert(std::abs(audit.Snapshot.P95Ms-1000.0/60)<.001);
 assert(audit.Snapshot.SlowFrames==0 && audit.Snapshot.Hitches==0);
 assert(Advice(audit.Snapshot)==EAdvice::Smooth);
 assert(std::abs(audit.Snapshot.OnePercentLowFPS-60)<.001);
 assert(Recommend(audit.Snapshot,3,100,50)==EAction::Keep);
 assert(Recommend(audit.Snapshot,-1,100,50)==EAction::Collect);
 // After a menu or long gap, retain the old snapshot only until a fresh
 // consecutive gameplay window is ready, not a blend of two environments.
 FAudit resumed;
 double resumeNow=0;
 resumed.Observe(resumeNow,true);
 for(int i=0;i<240;++i) { resumeNow+=1.0/60; resumed.Observe(resumeNow,true); }
 assert(resumed.Snapshot.Ready);
 resumed.Observe(resumeNow+10,false);
 assert(std::abs(resumed.Snapshot.AverageFPS-60)<.001);
 resumeNow+=20;
 resumed.Observe(resumeNow,true);
 for(int i=0;i<40;++i) { resumeNow+=.05; resumed.Observe(resumeNow,true); }
 assert(resumed.Snapshot.Ready && resumed.Snapshot.Samples>=30 && resumed.Snapshot.Samples<=40);
 assert(std::abs(resumed.Snapshot.AverageFPS-20)<.001);
 assert(std::abs(resumed.Snapshot.OnePercentLowFPS-20)<.001);
 resumed.Observe(resumeNow+10,true);
 assert(std::abs(resumed.Snapshot.AverageFPS-20)<.001);
 resumeNow+=11;
 resumed.Observe(resumeNow,true);
 for(int i=0;i<65;++i) { resumeNow+=1.0/60; resumed.Observe(resumeNow,true); }
 assert(resumed.Snapshot.Ready && resumed.Snapshot.Samples<=65);
 assert(std::abs(resumed.Snapshot.AverageFPS-60)<.001);
 auto previous=audit.Snapshot;
 audit.Observe(now+100,false);
 audit.Observe(now+200,true);
 assert(audit.Snapshot.AverageFPS==previous.AverageFPS);
 audit.Observe(std::numeric_limits<double>::quiet_NaN(),true);
 audit.Reset();
 assert(!audit.Snapshot.Ready && audit.Snapshot.Samples==0);
 now=0;
 audit.Observe(now,true);
 for(int i=0;i<300;++i) { now+=.05; audit.Observe(now,true); }
 assert(audit.Snapshot.Ready && audit.Snapshot.SlowFrames==240);
 assert(std::abs(audit.Snapshot.AverageFPS-20)<.001);
 assert(Advice(audit.Snapshot)==EAdvice::ReduceQuality);
 assert(Recommend(audit.Snapshot,2,100,50)==EAction::LowerPreset);
 assert(Recommend(audit.Snapshot,0,80,50)==EAction::LowerRenderScale);
 assert(Recommend(audit.Snapshot,0,50,50)==EAction::AtMinimum);
 assert(Recommend(audit.Snapshot,0,40,50)==EAction::Collect);
 assert(Recommend(audit.Snapshot,0,std::numeric_limits<double>::quiet_NaN(),50)==EAction::Collect);
 for(int i=0;i<300;++i) { now+=.12; audit.Observe(now,true); }
 assert(audit.Snapshot.Hitches==240);
 assert(audit.Snapshot.WorstMs>=119);
 audit.Observe(now+10,true);
 assert(audit.Snapshot.WorstMs<121);
 audit.Reset();
 now=0;
 audit.Observe(now,true);
 for(int i=0;i<300;++i) { now+=(i%20==0?.12:1.0/60); audit.Observe(now,true); }
 assert(audit.Snapshot.Ready && audit.Snapshot.Hitches>0);
 assert(audit.Snapshot.P95Ms<=audit.Snapshot.WorstMs);
 assert(Advice(audit.Snapshot)!=EAdvice::Smooth);
 assert(audit.Snapshot.OnePercentLowFPS<10);
 assert(Recommend(audit.Snapshot,2,100,50)==EAction::ReviewHitches);
 FStageAudit stages;
 FSnapshot frame{};
 frame.Ready=true; frame.MeanMs=20;
 assert(LikelyStage(stages,frame)==EStage::Unknown);
 for(int i=0;i<8;++i) stages.Observe(9,8,19);
 assert(LikelyStage(stages,frame)==EStage::GPU);
 for(int i=0;i<8;++i) stages.Observe(20,8,9);
 assert(LikelyStage(stages,frame)==EStage::Game);
 for(int i=0;i<8;++i) stages.Observe(9,19,8);
 assert(LikelyStage(stages,frame)==EStage::Render);
 for(int i=0;i<8;++i) stages.Observe(18,19,20);
 assert(LikelyStage(stages,frame)==EStage::Balanced);
 for(int i=0;i<8;++i) stages.Observe(2,3,4);
 assert(LikelyStage(stages,frame)==EStage::Balanced);
 stages.Observe(0,9,15);
 assert(LikelyStage(stages,frame)==EStage::Unknown);
 stages.Reset();
 assert(!stages.Ready());
}
