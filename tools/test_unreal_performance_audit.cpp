#include "../MoonfallUnreal/Source/MoonfallUnreal/MoonfallPerformanceAudit.h"
#include <cassert>
#include <limits>
using namespace MoonfallPerformance;
int main() {
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
}
