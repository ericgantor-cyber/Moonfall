# Morvella Velmora, Goddess of Death

**Title:** The Crimson Reaper. **Internal encounter ID:** `death_goddess_crimson_reaper` (kept stable for compatibility).

Status: **partial Unreal C++ implementation on this draft PR**. Arena preset, boss identity, four HP-based phases, and phase presentation are coded. Four **distinct named and shaped telegraphed attacks** are now coded in the shared Unreal combat pipeline, but dedicated animations/art, the remaining encounter mechanics, native compile and packaged gameplay are unverified/not implemented.

## Visual identity
Use the approved portrait of Morvella Velmora, the Death Goddess, in the November 19, 2026 conversation as the **canonical appearance reference**. Keep its face, silver-and-black hair, crimson eyes, ornate black skull armor, hood, crown, dramatic silhouette, and violet scythe consistent. Do not modify existing Z or Nyra artwork. The chat-generated image needs a separately committed, authorized art asset before runtime integration; this document is not that asset.

## Role and encounter
- Type: optional high-tier divine boss; no replacement of existing bosses.
- Arena: moonlit necropolis with clear, navigable floor and sparse grave markers.
- Four phases in a single continuous fight, each with a distinct mechanic. Phase transitions do not reset health.
- Keep boss warnings readable and accessible; use text, silhouettes, and audio-neutral visual cues. Do not require color alone.

### Phase I: Grave Waltz (100%–76% HP)
- Scythe Sweep: wide frontal arc with a visible windup, safe behind boss.
- Soul Needle: delayed single-target line strike with a marked ground lane.
- Reaper Step: short teleport, followed by a predictable half-circle cut.

### Phase II: Veil of Souls (75%–51%)
- Add spectral orbs that drift slowly and can be destroyed.
- Soul Harvest: pulls nearby orbs inward, then explodes in a clearly marked ring.
- The center remains navigable, with a deliberately visible escape gap.

### Phase III: Crimson Eclipse (50%–26%)
- Moon eclipse darkens the background **without reducing combat readability**.
- Triple Crescent: chained three-part scythe combo with timing cues.
- Blood Moon: slow ground zones that expire, never trapping the whole arena.

### Phase IV: Death's Embrace (25%–0%)
- Combines one mechanic from each previous phase, with a limited active hazard budget.
- Final Verdict: a large, heavily telegraphed strike with a clear dodge/parry opportunity.
- No guaranteed instant-kill moves. Defeat awards a unique cosmetic/title and appropriately balanced loot.

## Technical integration checklist
1. Add licensed/approved portrait and runtime sprite or 3D model as distinct assets; preserve source artwork.
2. Implement a data-driven four-phase state machine in UE5 (long-term engine) while leaving Godot intact.
3. Hook into existing attack telegraph and defensive input paths rather than adding duplicate combat systems.
4. Add explicit spawn rules and an arena entry trigger; do not introduce an unbounded enemy cap.
5. Test phase boundaries at 75%, 50%, and 25%, invulnerability transitions, death cleanup, checkpoint respawn, controller/mouse controls, save/load, performance, and Windows packaged builds.
6. Add automated tests where feasible and verify native gameplay before considering a merge.

**Acceptance:** boss spawns, phase changes occur exactly once per threshold, player has readable response windows, encounter finishes cleanly, and character reference visuals remain unchanged. No playable-build claim until compiled and verified.

## Implemented in draft PR #1075
- Opt-in `death_goddess_crimson_reaper` arena preset (does not change default showcase).
- Boss title and region and dedicated health phase thresholds at 75/50/25 percent.
- Phase-entered Blueprint callback and phase-number readout reuse existing native hooks.
- Four opt-in signature attacks are wired to the shared, real damage/defense/tell/cancel path; generic melee moves remain in use between signatures.
- Phase I: **Grave Needle**, a 220 cm-wide locked forward lane reaching 150 cm beyond its target, 1.55 s tell, 1.3x base damage.
- Phase II: **Soul Harvest**, caster-centered ring with 345 cm outer radius and 138 cm inner safe radius, 1.7 s tell, 1.35x base damage. Ranged casting is intentionally disabled for this move.
- Phase III: **Crimson Eclipse**, hero-locked cross (310 cm arm reach; 62 cm half-width), with safe diagonal quadrants, 1.35 s tell, 1.55x base damage.
- Phase IV: **Final Verdict**, hero-locked 400 cm circular marker, 1.85 s tell, 1.9x base damage.
- A new standalone C++ test verifies all four hit footprints including safe zones and invalid inputs; no Unreal packaged-runtime test has run yet.
- This **does not yet** implement teleporting, spawned spectral orbs, chained scythe animations, zone hazards or unique victory loot. The approved artwork also needs to be imported and checked against its reference. The complete encounter is not yet verified.
