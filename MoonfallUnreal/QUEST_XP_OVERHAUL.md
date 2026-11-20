# Moonfall UE5: resident quest progression and XP

This is a **source implementation** for Unreal Engine 5. Native UE5 Windows
compilation, gameplay, saving/reloading, UX readability and accessibility
remain unverified until an actual Editor/package test.

The game combines **25 unique resident salvage contracts** with **12 regional
dungeon discovery/boss progression quests**, while tracking up to **seven**
current objectives at once.

## Quest rewards

Completed resident salvage contracts now award XP **once** alongside the
existing **+6 resident bond** and **+3 faction reputation**. Their existing
salvage costs are unchanged. The reward uses Moonfall's existing
`AwardExperience` and level-100 cap, never a separate leveling counter.

For a valid contract (salvage cost 1–30), and level 1–99:

```text
XP = min(300, 20 + 2 × salvage cost + 2 × player level)
```

The validated existing resident contracts normally cost 10, 15, 20 or 25.
These get player-facing difficulty descriptions without changing quest RNG,
spawn, balance or saved IDs:

| Salvage cost | Name | Level-1 XP |
|---|---|---|
| 10 | APPRENTICE | 42 |
| 15 | ADVENTURER | 52 |
| 20 | VETERAN | 62 |
| 25 | MASTER | 72 |

Rewards are determined **at completion**, so leveling up between acceptance
and turn-in increases the award. At level 100, the award is 0 XP.

## Before and after turn-in

The NPC menu now displays the **same unique quest title** as the seven-slot
world tracker, resolved through a single cached, stable resident ID catalog.
The title appears below the NPC's name without replacing their ancestry,
social status, faction, trust or the existing choices. The NPC menu also
previews XP and whether the reward is expected to cause a level-up, along
with contract rank, available salvage, and existing bond and reputation
rewards. The starter Guide objective shows its XP reward when the contract
is ready. The confirmation message shows actual awarded XP and LEVEL UP
when applicable.

The preview uses a pure C++17 projection matching Moonfall's existing
100 + 55 × (level − 1) XP thresholds, including rollover and the
level-100 XP reset. Short windows omit the salvage footer that previously
overlapped the last selectable NPC dialogue row, but show a compact XP and
salvage line above the actions. Existing mouse/controller hit boxes are
unchanged. Reward text stays inside the NPC panel, never above the hero.

## Atomic save and replay safety

A contract only finishes from its persisted accepted state (1); completed
state (2) cannot be turned in again. The existing save includes the quest
state, XP, level, salvage, bond and faction reputation. Before saving,
the code snapshots all level-dependent attributes changed by XP. A
failed write rolls them all back alongside salvage and relationship progress.
Health-change Blueprint UI events are deferred until a successful save.

This change also whitelists the **12 separately authored waystation resident
IDs** in four existing resident allowlists: quest/bond eligibility, save
validation, save-load validation and load restoration. This is compatible
with the current save schema, does not spawn the NPCs, and does not merge
separate regional waystation PR #1070.

## Persistent world contract journal

The HUD's existing world quest tracker now considers **all 25 named
residents** whenever a saved contract is accepted. It reads only
the hero's existing saved `ResidentQuestStates`, not an expensive
world-wide NPC scan, and uses the pure C++17 `MoonfallQuestJournal.h`
catalog to identify each resident's expected contract cost and home.

- **Accepted** contracts (state 1) are candidates. The tracker now supports
  **up to seven quests simultaneously**, in a WoW Forever-style stacked list.
  Ready-to-turn-in contracts take precedence over those needing salvage.
  The first seven eligible contracts are tracked in stable order;
  more accepted quests remain saved and replace completed contracts.
- Every row displays a **unique named quest** (25 authored salvage
  contracts), **TURN IN** or **ACTIVE**, the resident's name and
  location, available/required salvage, projected XP/level-up, and a
  thin static salvage progress bar. The compact tracker is organized into
  three short lines per quest: **title + status**, **NPC + destination**,
  **salvage + XP**, with fixed right-hand status/XP columns to prevent
  a long combined label from overflowing a small window. The normal
  layout uses **70px** per quest row with more readable font scales,
  21–22px line spacing and a 3px progress bar. In short windows,
  **60px** rows preserve three text lines with 18px line spacing.
  Empty slots are not shown, and completed quests cannot reappear as active.
  The tracker selects no more than **seven** objectives even when more
  than seven NPC contracts or regional goals are eligible.
- At **720p or above, all seven tracked objectives** fit with 60px rows;
  at 850px or taller, spacious 70px rows display the full seven.
  Compact views safely show a subset (three rows at 600x420,
  four at 640x480), with a **1-3/7 MMB NEXT**-style range hint.
  **Middle mouse button (MMB) cycles between nonoverlapping pages** of
  tracked quests even when the gameplay cursor is hidden; the last page
  can be shorter so it doesn't leave empty decorative rows. Every quest
  remains tracked and can be viewed in a compact window without resizing.
  When a contract becomes ready to turn in, a quest is completed or
  accepted, a regional objective takes a free slot, or a different hero
  is loaded, the HUD automatically returns to page one to surface the
  newly prioritized objectives. Otherwise, manual MMB page selection is
  stable while progress counts and XP forecasts update in place. The HUD
  uses an exact, allocation-free signature of the seven ordered quest IDs
  plus each contract's ready/not-ready state, not save fields or world scans.
  The full seven appear at 720p+ without page navigation. The original
  discovery and boss bars stay beneath the currently visible rows.
  These limits come from pure C++17 viewport geometry
  (`MoonfallQuestLayout.h`). If no contract is accepted, the original
  sanctuary tutorial or world exploration tracker remains available.
  Accepted contracts stay visible when returning to the sanctuary.
- Quest titles are short, stable UI metadata, not new quest categories:
  each still completes by spending the existing NPC's required salvage.
  IDs, rewards, save fields and NPC identities are unchanged. Unique
  title lengths are verified in the C++17 and Python regression suites.
- The quest list uses the same live XP and level-up forecast as the NPC
  menu. Level-100 characters see MAX LEVEL rather than +0 XP. The retained
  summary line shows total active/completed counts, including quests beyond
  the seven tracked slots.
- A nonflashing **green** tracker edge marks a contract ready to turn in;
  **amber** marks one still gathering salvage. Exploration keeps its
  original cyan edge. The text also states TURN IN or ACTIVE, so readiness
  is not communicated by color alone. No new screen-space text is placed
  above the hero or enemies.
- Completed contracts (state 2) are counted but never tracked or
  replayed. When no contract is active the existing dungeon exploration
  and boss tracker resumes automatically.
- The starting-sanctuary tutorial remains available as the fallback
  when no contract is accepted, including inside the sanctuary. Once
  accepted, the Wandering Guide quest can occupy one of the seven slots
  and does not disappear while near the sanctuary. Original dungeon
  discovery and boss progress bars remain unchanged.
- For performance, the catalog resolves its `FName` lookup IDs only once,
  then performs a fixed **25 map lookups** on visible frames. The seven-slot
  renderer forecasts XP only for displayed rows, rather than redundantly
  building an unused first-quest summary. Dungeon target scanning and
  exploration string formatting are skipped while contracts are tracked.
  It does not iterate world actors, spawn content, allocate per-frame NPC
  metadata, introduce another Tick, or write save data.

The catalog's default costs mirror the currently authored NPC races
(`10 + race % 4 × 5` salvage; the Wandering Guide costs 10).
If a designer changes an individual NPC's race or contract cost, they
must update this UI-only metadata as well to keep the tracker accurate.
Regional NPC authoring remains in separate draft PR #1070.

## Twelve additional regional quests

Moonfall now offers **two original world quests per region**, 12 in all:
one to discover that region's five dungeon entrances, and one to defeat
its five dungeon bosses. They have individual story titles for all six
regions and update automatically from the **existing persistent 30-bit
discovery and boss-victory masks**. Completed objectives leave the tracker;
visiting a new zone or defeating a boss advances the relevant goals.

- Accepted NPC contracts remain first in the WoW Forever-style
  **seven-quest tracker**. If fewer than seven contracts are tracked,
  eligible regional objectives fill the remaining slots; the logical
  list never exceeds seven, and more eligible goals appear as completed
  objectives free up slots.
- In-progress regions are prioritized before unvisited ones. Boss goals
  are shown only after at least one entrance has been discovered. All
  completed regional goals disappear, and the existing endgame exploration
  fallback remains.
- Each regional row shows its unique title, region name, **0–5 progress**,
  an EXPLORE/DEFEAT indicator, and a progress bar. The status is
  **WORLD GOAL**, not a fictitious "+XP" turn-in reward.
- These regional quests are **progression objectives**, not additional
  reward claims. Existing dungeon bosses, monsters and exploration
  continue providing their original rewards; this patch introduces **no
  duplicate XP**, no bonus currency, no new save format, and no unlock
  of undiscovered dungeon markers.
- While the hero is at the starting sanctuary with no accepted contracts,
  the Firstlight tutorial remains the priority. Existing window geometry
  and adaptive seven-slot height/spacing rules are reused.
- No additional per-frame world actor scans: at most six per-zone saved
  discovery counts and six boss-victory counts are read when free quest
  slots are available. The pure C++17 `MoonfallWorldQuest.h` selection
  policy and automated tests validate all 12 goals, invalid state,
  completion filtering, and seven-slot prioritization.
- Keyboard **Q remains Dodge**. Other existing controls, especially
  J = atlas, B = inventory, and C = character sheet, are untouched.

## Native UE5 verification needed

1. Open a saved game before the change. Confirm existing quest states,
   XP, level, gear and previously discovered shrines remain intact.
2. Accept and turn in a cost-10 Guide contract. Verify the displayed and
   actual XP, level-up health/spell strength, bond and faction rewards.
3. Reopen the completed contract, reload, and retry: no duplicate XP or
   salvage spending.
4. Force a save failure and confirm that all numerical progression, UI
   notification, quest state, salvage and relationship values are unchanged.
5. Verify both compact (e.g. 720p window) and large (1080p/4K) NPC menus
   with mouse/controller. Check that the named quest matches the world
   tracker, does not cover race/trust/rank/rewards, and that the last
   selectable action remains clickable and accessible.
6. Accept eight or more contracts. Confirm **at most seven tracked
   quest entries**, with salvage-ready contracts first; the eighth
   accepted quest should appear when a tracked contract is completed.
   Test with 1 through 8 active contracts, including all seven visible
   at 720p/1080p. Verify XP, turn-in priority, and dungeon bars.
7. Return to the Shattered Sanctuary with seven accepted contracts.
   All seven should remain tracked. With no accepted contracts, the
   original Firstlight introduction should return. Test 600x420,
   640x480, 720p, 1080p and 4K. Compact views must show a truthful
   SHOWING X/7 count, keep rows above the health orb, and retain readable
   title/NPC/reward lines and 3px salvage bars. Verify actual visuals
   and mouse accessibility in native Unreal gameplay.
8. Complete all tracked contracts. Confirm the original exploration tracker
   returns, and a completed quest cannot claim rewards again. Inspect
   green/amber and text cues at 720p, 1080p, 4K and reduced motion.
9. If regional waystations are later merged, validate all 12 residents
   individually: accept, complete, bond, save, reload, and forbid duplicates.
10. With 0 through 7 accepted NPC contracts, verify the remaining
    seven-slot tracker positions are filled by the correct regional
    discovery/boss goals, and regional progress survives save/reload.
    At all 30 discovered and 30 defeated, verify regional goals vanish.
    At 600x420 and 640x480, press MMB to cycle through all seven
    objectives, verifying absolute row numbers and a shorter last page.
    From page two, make another contract ready to turn in or complete
    a quest and verify the tracker automatically returns to page one
    with the new ready-first ordering. Check that ordinary incremental
    progress does not needlessly reset the page. Changing hero must
    reset to page one. Mouse-wheel camera zoom, Q dodge, and J/B/C
    menu controls must retain
    their existing behavior. Test MMB while a modal menu or NPC dialogue
    is open; it must not change quest pages.
11. Profile menu/journal rendering and level-up tests in a real packaged
    Windows build. GitHub source tests alone do not prove an Unreal build.

No change to Z/Nyra original art, attack warning HUD, or combat balance.
