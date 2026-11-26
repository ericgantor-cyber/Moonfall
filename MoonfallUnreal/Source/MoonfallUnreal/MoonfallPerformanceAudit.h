#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

// Portable bounded CPU-side frame-interval diagnostics. Not GPU timing.
namespace MoonfallPerformance {
struct FSnapshot {
 double AverageFPS=0, MeanMs=0, P95Ms=0, WorstMs=0;
 unsigned Samples=0, SlowFrames=0, Hitches=0;
 bool Ready=false;
};
class FAudit {
 static constexpr std::size_t Capacity=240;
 std::array<double,Capacity> Frames{};
 std::size_t Count=0, Next=0;
 double LastTime=-1, PublishElapsed=0;
public:
 FSnapshot Snapshot{};
 void BreakSequence() { LastTime=-1; }
 void Reset() { Count=Next=0; LastTime=-1; PublishElapsed=0; Snapshot={}; }
 void Observe(double Now, bool Gameplay) {
  if (!Gameplay || !std::isfinite(Now)) { BreakSequence(); return; }
  if (LastTime<0) { LastTime=Now; return; }
  const double Seconds=Now-LastTime;
  LastTime=Now;
  if (!std::isfinite(Seconds) || Seconds<=0 || Seconds>2.0) {
   BreakSequence(); return;
  }
  Frames[Next]=Seconds*1000.0;
  Next=(Next+1)%Capacity;
  Count=std::min(Count+1,Capacity);
  PublishElapsed+=Seconds;
  if (PublishElapsed>=1.0) {
   PublishElapsed=0;
   Publish();
  }
 }
private:
 void Publish() {
  if (Count<30) return;
  auto Sorted=Frames;
  std::sort(Sorted.begin(),Sorted.begin()+Count);
  double Sum=0;
  unsigned Slow=0,Hitch=0;
  for (std::size_t I=0;I<Count;++I) {
   Sum+=Sorted[I];
   if (Sorted[I]>33.3334) ++Slow;
   if (Sorted[I]>=100.0) ++Hitch;
  }
  const double Mean=Sum/static_cast<double>(Count);
  const auto P95Index=(95*Count+99)/100-1;
  Snapshot={1000.0/Mean,Mean,Sorted[P95Index],Sorted[Count-1],
            static_cast<unsigned>(Count),Slow,Hitch,true};
 }
};
enum class EAdvice { Collecting, ReduceQuality, Smooth, Monitor };
inline EAdvice Advice(const FSnapshot& S) {
 if (!S.Ready || S.Samples<30 || !std::isfinite(S.AverageFPS) ||
     !std::isfinite(S.P95Ms) || S.AverageFPS<=0 || S.P95Ms<=0)
  return EAdvice::Collecting;
 if (S.AverageFPS<30 || S.P95Ms>50) return EAdvice::ReduceQuality;
 if (S.AverageFPS>=55 && S.P95Ms<=25 && S.Hitches==0) return EAdvice::Smooth;
 return EAdvice::Monitor;
}
}
