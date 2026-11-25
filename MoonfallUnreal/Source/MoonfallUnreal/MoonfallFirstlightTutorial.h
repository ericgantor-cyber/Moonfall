#pragma once
// Optional tutorial is event-driven and session-only. No save migration,
// modal progression lock, actor search, or keyboard/controller remapping.
namespace MoonfallFirstlightTutorial {
enum class EAction : unsigned char {
 DiscoverShrine = 0, SpeakToGuide = 1, SwingSword = 2,
 PerformDodge = 3, OpenSpellbook = 4, Complete = 5
};
inline constexpr unsigned Total = 5u;
struct FProgress {
 unsigned Mask = 0u;
 bool Mark(EAction Action) noexcept {
  const unsigned Index = static_cast<unsigned>(Action);
  if (Index >= Total) return false;
  const unsigned Bit = 1u << Index;
  const bool IsNew = (Mask & Bit) == 0u;
  Mask |= Bit;
  return IsNew;
 }
 bool Has(EAction Action) const noexcept {
  const unsigned Index = static_cast<unsigned>(Action);
  return Index < Total && (Mask & (1u << Index)) != 0u;
 }
 unsigned Count() const noexcept {
  unsigned Counted = 0u;
  for (unsigned i = 0u; i < Total; ++i)
   if ((Mask & (1u << i)) != 0u) ++Counted;
  return Counted;
 }
 EAction Next() const noexcept {
  for (unsigned i = 0u; i < Total; ++i)
   if ((Mask & (1u << i)) == 0u) return static_cast<EAction>(i);
  return EAction::Complete;
 }
 bool Finished() const noexcept { return Next() == EAction::Complete; }
};
} // namespace MoonfallFirstlightTutorial
