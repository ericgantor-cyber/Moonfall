# Moonfall Unreal: AAA-quality gameplay polish slice (priorities 2–5)

This branch begins the **combat, world/dungeon presentation, RPG progression,
and engineering polish** priorities. It is a coherent **source implementation**,
not a complete AAA-quality game or a verified Windows build.

## Continued combat upgrade: truthful warnings for ordinary enemies

- Every ordinary/elite enemy now exposes read-only `GetAttackWarningRemaining`,
  `GetAttackWarningDuration` and `GetAttackWarningLabel` methods. The
  countdown reads existing committed attack timers and returns nothing for
  invalid numbers, dead actors, interrupted/expired windups or bosses.
- The **existing single world-enemy HUD scan** chooses the earliest
  approaching non-boss hit within 1050 units, breaking ties by distance.
  A compact warning tells the player the actual move label, tenths of a
  second remaining, and the supported **GUARD / PARRY / DODGE** choices.
  It includes a static `IMPACT SOON` indication during the final 0.3 s.
- When a boss is nearby, the existing boss telegraph takes priority.
  Regional boss dodge-only moves retain their red `DODGE ONLY` text and
  exact existing damage logic. We do not incorrectly claim that ordinary
  attacks are dodge-only.
- This is **presentation only**: no extra actor tick, navigation query,
  player damage, stagger, attack frequency, chance, quest, save field or
  controller binding was changed. Warnings do not flash and have text
  independent of color.
- On screens under 480×400 the compact warning stays hidden to avoid
  occlusion; boss alerts are unchanged. Test those window sizes natively.
- New six-test regression suite checks invalid/expired timing,
  no extra world-actor iterator, boss priority, menu cancellation, the
  text/visual budget and attack-type correctness.
- The warning UI is source-implemented and **not yet visually verified
  through a native UE5.4 build**.

## WoW F-style combat floats: enemy-only overhaul

- **Nothing floats above the player's character.** Both existing outgoing
  damage numbers and the five results of enemy attacks are anchored to
  their source **enemy**. Incoming damage says `HIT YOU -damage` to
  distinguish it from outgoing damage. Other labels are `PARRY!`,
  `DODGE!`, `BLOCKED!` and `MISS!`. Each keeps a visible text
  label and color palette, so color is never the only indicator.
- The original **hero-side eight-entry ring and recorder are removed**,
  not merely hidden. A four-entry, unsaved, fixed-size queue is now
  owned by each attacking enemy. It records a result only after actual
  hit arc, cover, dodge, parry, block and health changes have resolved.
  Blueprints still receive the existing result and damage events.
- The original **single HUD enemy scan** supplies both outgoing and
  enemy-attributed result labels. Candidate actors are capped at 64;
  drawing is limited to **16 recent outcome labels plus 32 outgoing
  damage numbers**, even during large battles. No extra actor scans,
  spawned effects, game ticks, RNG, save fields or damage calculations.
  Outgoing killing blows remain eligible for their final damage label.
- Results are shown for **1.1 seconds** using two deterministic
  two-row screen-space lanes per enemy with a dark text backing,
  bounded screen-space clipping and a gentle upward drift. When
  Reduced Motion is enabled, they remain stationary and quietly fade.
- **Player-safe crowd placement:** The HUD projects the player only
  once to establish a protected on-screen silhouette, **never as a
  source of combat labels**. Each result tries up to eleven fixed
  nearby positions and rejects boxes that overlap the player or any
  of the already accepted enemy results. If none fit, it skips that
  optional label rather than obscuring the player. At most sixteen
  rectangles are compared, using a stack-local fixed array with no
  extra actors, heap allocation, AI scans or timers.
- **Newest results first:** Each enemy's four-entry ring is inspected
  from the latest resolved hit backward. This prioritizes a fresh
  parry, dodge or damage result when the scene reaches its label cap.
- **Esc > Options** retains ten selectable rows, with `Floating
  results` On/Off, `Text palette` Classic/Contrast/Mono, and existing
  `Combat text` sizing at 75% to 200%. Mouse, keyboard and controller
  controls stay synchronized; UI preferences persist in
  `GameUserSettings.ini` rather than savegame data.
- Expanded source tests explicitly forbid an incoming hero label
  queue, hero-position projection or unbounded enemy scans. A
  C++17 test validates the animation-free lane, fade and reduced
  motion policy with thousands of deterministic cases.

**Native QA still required:** Compile UE5.4, check enemy-only label
placement in melee (including when an attacker stands next to the
hero), simultaneous 60+ mobs, incoming/ outgoing damage readability,
parry/dodge/block/miss truthfulness, cap limits, standard and 4K
resolutions, smallest window size, all three palettes, Reduced Motion,
mouse/controller settings persistence, FPS and valid final-hit
feedback. Also check attackers standing directly next to the player:
outcome labels must move aside or be suppressed, never overlap the
player silhouette. Verify label separation across adjacent enemies
and whether small viewports skip optional text without covering
gameplay. The C++17 layout test now checks screen exclusion,
inter-label collisions, expiry, bounds and Reduced Motion.
Source tests **do not** prove a working Windows build.

## Real impact outcomes and sound integration

- The timer event `OnAttackTelegraphResolved` still represents only the
  scheduled strike deadline, not a successful blow. A new event,
  `OnAttackImpactOutcome(MoveId, Outcome, HealthLost)`, fires **after**
  existing hit-arc, cover, dodge, parry and hero HP calculations for
  ordinary/elite melee attacks. Its explicit outcomes are **HIT, PARRY,
  DODGE, GUARD, MISS**. The move ID is captured before the timing event
  clears it, so authored impact effects can match their anticipation.
- Recorded HP loss comes from the player's actual pre/post-defense
  health values. Fully blocked strikes are GUARD; attacks outside
  range or blocked by cover are MISS. No success effect is claimed for
  misses, perfect dodges or successful parries.
- New optional `AttackHitSound` and `AttackParrySound` assets are
  **null by default**. If supplied, audio plays only for real matching
  outcomes, within 1000 units and with a per-enemy 0.4-second limit.
  A Blueprint callback that kills the enemy cannot trigger ghost audio.
- All this is feedback-only: it does not alter damage, enemy AI,
  attack cooldown, boss phase mechanics, stamina, XP, loot, save formats,
  hit collisions, Godot, Z or Nyra art. No new runtime actors or scanning.
- `MoonfallAttackOutcome.h` is engine-free and independently tests
  **80,008 combinations** of contact, defensive state, HP loss and
  audio gating, alongside focused Unreal source integration tests.

**Native QA:** attach authored impact sounds and montage/VFX to real
Blueprint enemy variants, verify misses behind walls, shield blocks,
perfect parries, invulnerable dodges, low HP/defeat, and saved state.
Check crowd audio mixing and win10/11 latency. No actual sound files or
native UE5.4 gameplay test are included in this source change.

## Built-in in-world attack visualization (no external asset required)

- Added a **procedural, visible 3D tell** to nonboss enemies and elites.
  A single optional `UStaticMeshComponent` child uses the engine's standard
  cube mesh to communicate an attack in-world. The existing sphere/collider
  root, collision shape, sight tests, AI/attack timing, and actual damage
  resolution are unchanged.
- Five deterministic profiles match the existing move classifications:
  quick narrow strike, taller heavy hit, wide sweep, broad crush, and
  parry-adaptive counter-sweep. A marker scales and turns gently over the
  **real authoritative attack windup**. No extra actors, physics overlap,
  shadows, hitboxes, navigation scans, gameplay randomization, or timers.
- The cue appears only while a live ordinary/elite attack is committed;
  it is hidden on impact or interruption, death, menus, retreat, posture
  break, or a canceled Blueprint callback. Bosses, including Z and Nyra,
  never display this new marker, preserving their original visuals.
- **Reduced Motion:** When the hero's existing `bReduceMotion` option is
  on, the marker appears at its full readable size without turning or
  moving. No flashing is used; the existing text countdown remains the
  primary accessible cue.
- `bEnableProceduralAttackTell` defaults on but is configurable in
  derived Blueprints so professional skeletal animations can replace this
  placeholder without changing the actual attack clock.
- The sizing/timing policy is in a tiny, pure C++17 header and has
  **50,500 profile/timing regression combinations**, plus six source
  integration tests checking no collision, real timing, animation
  interruption, Reduced Motion, zero new scans and original actor control.

**Native work still outstanding:** compile in UE5.4 on Windows, verify
the engine cube is visible and legible with shipped game materials, check
marker offsets on different body scales, verify no player/projectile
occlusion or camera obstruction, compare stable gameplay FPS across 60+
mobs, inspect 720p and 4K UI plus Reduced Motion settings, and replace
placeholder markers with authored animation montages and audio assets
as those become available. Static/source tests do not prove rendered
visual quality or a playable build.

## Combat animation life-cycle integration

The optional Blueprint **non-boss attack animation protocol** is now
complete: `OnAttackTelegraphCommitted(MoveId, WarningSeconds)` when the
real AI strike timer starts; `OnAttackTelegraphResolved(MoveId)` at the
authoritative impact deadline; and `OnAttackTelegraphInterrupted(MoveId)`
on canceled attacks. `Resolved` does **not** mean the hit landed;
ordinary collision, guard, parry and dodge logic still decides damage.

The move ID is captured at the start and cleared **before** a completion
event is emitted. Each attack therefore delivers at most one end event,
even when a nested Blueprint callback cancels or destroys its owner.
Interruption events cover posture breaks, wounded-elite retreat, death,
modal menus, player possession changes, invalid timestamps, stagger,
practice resets and disengagement. Elite adaptation and sound cooldown
remain separate from animation state.

If an authored Blueprint cancels its own telegraph during the commitment
event, the native code clears its timer and reports interruption before
considering the optional sound; this prevents a ghost attack sound.
All three hooks are inert unless a game designer implements them in a
Blueprint, and **no finished animation or sound assets have been added**.
Boss animation events, original Z/Nyra art and boss timing are untouched.

Native verification needed: wire three hooks to an Unreal montage with
anticipation, hit and interruption transitions; test missed attacks,
successfully parried attacks, postponement by attack-slot coordination,
menus, death, respawn, crowd audio, short and long windups, and
Blueprint callbacks that destroy their owning actor. Confirm no double
notify or audio after cancellation on a real packaged Windows build.

## Continued AI upgrade: earned elite counter-sweeps and authored action cues

- **Elite tactical response:** A *successful* player parry can arm one
  counter-sweep on an eligible, living elite. The enemy must not be a
  boss, sanctuary practice target, or in a posture-break window, and
  the response must be off its independent eight-second cooldown.
  The response expires after **4 seconds** if no legal attack starts.
- When the next valid attack is committed in time, it replaces that
  elite's scheduled pattern with the **existing wide sweep**. It gets
  a readable **0.82-second minimum windup** and a distinct
  **COUNTER SWEEP** HUD label. Its existing damage, sweep range, guard,
  parry, dodge and recovery rules remain unchanged. It is not an
  unavoidable follow-up, extra hit, guaranteed enemy counter, or an
  alteration to loot, difficulty settings or boss mechanics.
- Enemy AI does **not** run an additional actor query, trace, timer,
  pathfinding request, tick, or RNG call. Normal pack strike-slot
  coordination and the existing cancel-on-menu/interrupt/posture-break
  protections still apply. **Hardening:** menus, lost possession, invalid
  windup timers, retreat, death, posture break and leaving combat clear
  stale adaptation state. Ordinary hit-stagger clears only the committed
  label so the response newly earned by a successful parry remains
  possible after stagger recovery. A resolved sweep clears its label.
- A native **`OnAttackTelegraphCommitted(FName,float)`** Blueprint
  event fires once when an ordinary or elite attack really begins.
  Its move ID comes from the same source as the on-screen warning,
  so a designer can hook up an appropriate **attack montage, voice,
  VFX, or controller cue** without guessing a strike duration.
- The optional `EliteAttackTellSound` is **null by default**.
  If a designer assigns a real cooked `USoundBase`, it plays once
  at each eligible elite attack commitment at low volume (0.55),
  only within 1000 world units. It never fires for regular mobs,
  sanctuary targets or boss attacks. It has an independent two-second
  per-elite playback cooldown to prevent rapid repeated sounds. After
  the synchronous Blueprint animation hook, the audio path revalidates
  that the enemy is alive, not being destroyed, and still has a finite
  active windup before playing, preventing ghost warnings after
  interrupted attacks. **No audio or animation assets are included.**
- Added pure C++17 policies and tests for all eligible state flags,
  cooldown/memory boundaries, and normal attack patterns. Source
  regressions separately verify the actual parry branch, finite
  telemetry, sound gating, single-scan HUD and boss priority.

**Native UE5.4 follow-up:** check attack montage alignment with
the real `WindupEndsAt` clock and collision resolution. Parry an elite
twice in quick succession; verify no second adaptation is armed during
the eight-second cooldown. Check expiration, interruption, posture
breaks, modal menus, armor and combat saves; test optional authored
sound mixing with multiple elites in a dungeon. Verify controller
cues and reduce-motion UI readability. The pure C++/source tests do
not substitute for those Windows gameplay tests.

## 2. Combat feel

- Original **12-hit same-target combat, guard, parry, dodge and posture**
  rules are untouched. Successful combat now produces brief, readable
  textual cues: PERFECT DODGE, PERFECT PARRY / COUNTER READY, and new
  posture-chain milestones at hits 4, 8, and 12.
- A cue is recorded **only on an earned event**, not from opening an
  inventory, an invalid/missed spell, repeated dodge input or guard idle.
  Only damage to a real opponent advances a combo milestone.
- The Canvas overlay is static text without flashing, forced camera shake,
  transient widgets, audio dependencies, or extra actors. The existing
  combat text scaling applies. On defeat cues are cleared.
- Authored UE animation montages and quality impact sound effects still
  require actual asset creation/import and playtesting; the existing hooks
  remain. This patch does not claim new motion-captured animations.

## 3. Dungeon/world presentation

- Every 14-room Unreal dungeon now gets two visual rune pillars per
  chamber, giving the 30-dungeon connected world up to **840 additional
  instanced decorative primitives**, rather than 840 extra ticking actors.
- Pillars use the existing engine cube mesh, non-colliding, shadow-free,
  culled at distances 9–16k units. Height changes with room archetype;
  a deterministic visual seed varies height without touching the streams
  used for hazards, treasure, combat or bonus encounters.
- Props occupy chamber side alcoves, outside the central 360-unit route.
  They do not create traps, mobs, loot, cover collision or altered
  checkpoint triggers. The 30 dungeons and six regions stay connected.
- An optional, **unassigned by default** `ChamberEntrySound` property
  can be connected to a cooked `USoundBase` by an Unreal designer.
  A sound plays at most once on *successful first entry* to an interior
  room and never on a failed encounter-spawn attempt or re-entry. Without
  an assigned sound asset the game remains silent for this cue.
- The separate draft **PR #1054** adds six regional waystations with NPC
  contracts and shrines. This PR does not merge or copy that work.

## 4. RPG gear and crafting progression

- Added `MoonfallForgeForecast.h`, a **pure, deterministic C++17**
  pricing and canonical equipment validator for the existing five-tier
  salvage forge. No new item code, currency, tier, loot probability,
  equipment-slot encoding or saved property.
- A selected inventory item now shows a read-only **Forge 5**
  forecast with the number of affordable upgrades, projected final tier
  and exact total salvage. For invalid gear, display a clear error;
  at tier 25, display **MAX TIER**.
- The real `ForgeInventoryItemLevels` path consults the same forecast,
  then invokes existing validated, transactional one-tier upgrades.
  Equipment sync still uses existing equip logic; a failed craft stops
  without spending salvage on subsequent tiers.
- This is an affordable **upgrade path planner**, not a newly invented
  recipe tree. Broader professions, loot-set expansions and major talent
  systems require their own design and verified gameplay work.

## 5. Performance, accessibility and verification

- Source tests check earned feedback, early-exit guards, menu/defeat
  cleanup, lack of HUD VFX dependencies, optional sound gating and
  first-visit semantics, seed isolation, narrow collision-free side
  landmark placement, and existing forge rollback.
- **C++17 offline test** validates canonical item-code handling and
  forge prices against an independent per-step reference across all
  valid trait/slot/rarity/tier combinations, multiple salvage budgets
  and requested levels. It is compiled with `-O2 -Wall -Wextra -Werror`.
- Small viewports do not display the full cue banner; the underlying
  combat rules remain accessible. The existing reduce-motion setting
  is not bypassed by this text-only feature.
- No original Z or Nyra art, Godot implementation, XP save format,
  serialized loot, authored quest state or World NPC AI logic is changed.

## Native UE5.4 / Windows 10/11 verification still required

1. Compile **Unreal Engine 5.4** for Windows, then open the Entry map.
   Existing source/check workflows are *not* native UE builds.
2. Land successful parries, perfect dodges, 4/8/12-hit sequences.
   Check cues at 720p, 1080p, 4K and with reduced motion enabled;
   verify invalid/missed casts do not advertise a milestone.
3. Check generated and authored dungeons. Verify side decorations
   never obstruct enemy pursuit, line of sight, collision, spawn points,
   navigation, shrine triggers, pickups or room objectives.
4. Assign a valid sound cue to one authored dungeon, verify a single
   first-entry playback and silence on repeated or failed entries.
   With no sound configured, there should be no new audio.
5. Test Forge 5 with insufficient salvage, invalid/duplicate item
   codes, tier 24 and tier 25, equipped items, multiple budgets, the
   actual confirmation controls and save/reload.
6. Benchmark frame time and draw-call impact of 840 instances in
   the full continuous world, with 30 dungeons and six regions.
7. Native-test gameplay and Windows packaging before any merge.

**Merge gate:** draft PR only until those native checks are verified.
