#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

// Portable bounded CPU-side frame-interval diagnostics. Not GPU timing.
namespace MoonfallPerformance {
struct FSnapshot {
 double AverageFPS=0, MeanMs=0, P95Ms=0, WorstMs=0, OnePercentLowFPS=0;
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
  // Average of the slowest ceil(1%) frame intervals, expressed as FPS.
  const std::size_t TailCount=(Count+99)/100;
  double TailSum=0;
  for (std::size_t I=Count-TailCount;I<Count;++I) TailSum+=Sorted[I];
  Snapshot={1000.0/Mean,Mean,Sorted[P95Index],Sorted[Count-1],
            1000.0*static_cast<double>(TailCount)/TailSum,
            static_cast<unsigned>(Count),Slow,Hitch,true};
 }
};
// Engine-provided stage times are delayed estimates, not synchronized GPU
// profiler captures. Require eight valid consecutive samples for advice.
struct FStageAudit {
 std::array<std::array<double,3>,8> Samples{};
 std::array<double,3> Mean{};
 unsigned Count=0, Next=0;
 void Reset() { Count=Next=0; Mean={}; }
 bool Ready() const { return Count==Samples.size(); }
 void Observe(double Game, double Render, double GPU) {
  for (double V:{Game,Render,GPU}) {
   if (!std::isfinite(V) || V<=0 || V>2000) { Reset(); return; }
  }
  Samples[Next]={Game,Render,GPU};
  Next=(Next+1)%Samples.size();
  Count=std::min(Count+1,static_cast<unsigned>(Samples.size()));
  Mean={};
  for (unsigned I=0;I<Count;++I)
   for (unsigned J=0;J<3;++J) Mean[J]+=Samples[I][J]/Count;
 }
};
enum class EStage { Unknown, Balanced, Game, Render, GPU };
inline EStage LikelyStage(const FStageAudit& S, const FSnapshot& Frame) {
 if (!S.Ready() || !Frame.Ready || !std::isfinite(Frame.MeanMs) || Frame.MeanMs<=0)
  return EStage::Unknown;
 const auto& M=S.Mean;
 for (double V:M) if (!std::isfinite(V) || V<=0) return EStage::Unknown;
 unsigned Largest=0;
 for (unsigned I=1;I<3;++I) if (M[I]>M[Largest]) Largest=I;
 if (M[Largest]<Frame.MeanMs*.75 || M[Largest]>Frame.MeanMs*1.5)
  return EStage::Balanced; // caps, waits, mismatched samples or other work
 for (unsigned I=0;I<3;++I)
  if (I!=Largest && M[Largest]<M[I]*1.2) return EStage::Balanced;
 return Largest==0?EStage::Game:Largest==1?EStage::Render:EStage::GPU;
}
enum class EAdvice { Collecting, ReduceQuality, Smooth, Monitor };
inline EAdvice Advice(const FSnapshot& S) {
 if (!S.Ready || S.Samples<30 || !std::isfinite(S.AverageFPS) ||
     !std::isfinite(S.P95Ms) || S.AverageFPS<=0 || S.P95Ms<=0)
  return EAdvice::Collecting;
 if (S.AverageFPS<30 || S.P95Ms>50) return EAdvice::ReduceQuality;
 if (S.AverageFPS>=55 && S.P95Ms<=25 && S.Hitches==0) return EAdvice::Smooth;
 return EAdvice::Monitor;
}
enum class EAction { Collect, LowerPreset, LowerRenderScale, ReviewHitches, AtMinimum, Keep };
inline EAction Recommend(const FSnapshot& S, int Quality, double Scale, double MinimumScale) {
 const auto State=Advice(S);
 if (State==EAdvice::Collecting || Quality<0 || Quality>3 ||
     !std::isfinite(Scale) || !std::isfinite(MinimumScale) ||
     MinimumScale<=0 || Scale<MinimumScale || Scale>100) return EAction::Collect;
 // CPU intervals alone cannot identify a GPU bottleneck. Try a single
 // reversible change and measure again rather than promising a speedup.
 if (State==EAdvice::ReduceQuality) {
  if (Quality>0) return EAction::LowerPreset;
  if (Scale>MinimumScale+0.5) return EAction::LowerRenderScale;
  return EAction::AtMinimum;
 }
 if (S.Hitches>0 || S.P95Ms>33.3334 ||
     (std::isfinite(S.OnePercentLowFPS) && S.OnePercentLowFPS>0 && S.OnePercentLowFPS<30))
  return EAction::ReviewHitches;
 return EAction::Keep;
}
}
