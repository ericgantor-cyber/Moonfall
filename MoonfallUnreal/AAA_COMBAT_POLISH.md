# Moonfall UE5: AAA combat-feel pass 1

## Confirmed-hit camera response

This pass reuses the existing spring-arm camera impulse, previously used only
when the hero took damage. Successful **enemy HP-damaging spells** now create
a small, short-lived, bounded camera response that builds with actual damage,
combo stage and important strike milestones. Bosses and lethal finishes have
slightly more weight. Misses, blocked/no-HP-damage hits and menu interactions
never create offensive camera feedback.

- **Safe feedback, not a combat buff:** no change to actual damage, combo
  rewards, enemy reactions, movement speeds, global time dilation, save fields,
  target acquisition or camera zoom controls.
- **Accessibility first:** the existing Reduce Motion setting completely
  suppresses the new impulse. It remains selectable through Esc settings.
  The system never creates warning text, character-attached damage numbers,
  additional camera actors or repeated dynamic Niagara components.
- **Bounded CPU/GPU cost:** constant-time scalar calculations, no allocations,
  additional Ticks or new rendering loops. Hits within 90ms cannot stack a new
  impulse. The offensive impulse is clamped to 24 units, matching the existing
  spring-arm interpolation limit. The response decays through the current
  frame-rate-independent spring-arm interpolation.
- **Combat-readability:** combo stages 4/8/12 have a small distinct impulse.
  Boss and kill cues are additive but a single hit contributes at most
  10 camera-arm units. The player retains control during the entire response.
- **Robustness:** invalid timestamps, damage values and camera state are
  rejected/sanitized; backwards world clock changes cannot permanently
  disable new impact response.

## Spell impact presentation, ready for authored effects

All six offensive schools now expose a **BlueprintImplementableEvent**
`OnSpellImpact(School, bLeftHand, ImpactPoint, AppliedDamage, bCritical,
bKillingBlow)` on `AMoonfallHeroPawn`. It only fires for actual hostile HP
damage with a valid impact position. The original `OnSpellCast` event is
unchanged; healing, misses and invulnerable/fully absorbed impacts do not
produce a false success cue.

The previous `DrawDebugSphere` and `DrawDebugLine` spell-trace artifacts
have been removed from runtime casting. A derived hero Blueprint can now
author the appropriate VFX and audio by school, including different critical
and finishing hit variants, without changing C++ damage math or source
artwork.

**Important:** This is an authoring API, not a claim that an existing
Niagara or sound asset has been connected. An unimplemented Blueprint
event deliberately draws no effect. Hook real authored assets to the
event in the UE5 Editor and test all six offensive schools before release.
No combat text, countdown, unrequested camera actors or global time
dilation has been added.

## Verification

Automated GitHub regression:
- C++17 policy tests compile with `-Wall -Wextra -Werror -pedantic`.
- Checks include misses, zero/nonfinite damage, repeated dual casts, capped
  sustained hits, combo milestones, boss lethal hits, reduced motion, modal
  menus, clock resets and invalid current camera state.
- Python source integration ensures the call site is under confirmed HP damage
  and the preexisting camera decay, Reduce Motion and combo notification remain.

**Native UE5 Windows compilation and actual gameplay testing are outstanding.**
Before merging, use a UE5 Windows 10/11 development build and verify:
1. Normal hits versus misses and invulnerable enemies, then multiple dual casts.
2. 4th, 8th and 12th hit chain milestones, lethal kills and bosses.
3. Camera smoothing at 30, 60 and 120+ FPS with mouse wheel zoom.
4. Reduced Motion toggled during a hit and while finishing an animation.
5. Inventory, atlas, character and Esc settings during combat.
6. No motion sickness symptoms, clipping, FOV changes, gameplay stun,
   above-hero damage numbers or combat countdown text.
7. Z and Nyra's existing art and all original assets remain unchanged.
8. Implement and visually test `OnSpellImpact` in derived hero Blueprints:
   confirm school colors, left/right hand, critical and killing-blow variants,
   no world-space debug spheres/lines, no false cues for misses, healing,
   absorbed hits or corrupted impact positions.

This is one concrete polish improvement, **not** a claim that the overall
Moonfall game has reached full AAA content or production quality. Full AAA
readiness also requires native build QA, visual assets, animation direction,
audio, level design, accessibility playtests, performance and packaging.
