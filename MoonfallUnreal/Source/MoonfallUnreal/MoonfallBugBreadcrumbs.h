#pragma once
#include <array>
#include <cmath>
#include <cstddef>

namespace MoonfallDiagnostics {
// Fixed event vocabulary: no arbitrary text, player identities or save data.
enum class EEvent { OptionsOpened, OptionsClosed, GraphicsChanged, DamageTaken, SpellCast };
struct FEvent { EEvent Kind=EEvent::OptionsOpened; double At=0; };
class FBreadcrumbs {
 static constexpr std::size_t Capacity=32;
 std::array<FEvent,Capacity> Events{};
 std::size_t Count=0,Next=0;
public:
 void Record(EEvent Kind,double Now) {
  if (!std::isfinite(Now) || Now<0) return;
  if (Count) {
   const auto& Last=Events[(Next+Capacity-1)%Capacity];
   if (Now<Last.At) { Count=Next=0; }
   else if (Last.Kind==Kind && Now-Last.At<0.5) return;
  }
  Events[Next]={Kind,Now};
  Next=(Next+1)%Capacity;
  if (Count<Capacity) ++Count;
 }
 std::size_t Size() const { return Count; }
 FEvent At(std::size_t I) const {
  return I<Count?Events[(Next+Capacity-Count+I)%Capacity]:FEvent{};
 }
};
}
