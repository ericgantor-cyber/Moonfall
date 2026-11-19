# Moonfall UE5: resident quest progression and XP

This is a **source implementation** for Unreal Engine 5. Native UE5 Windows
compilation, gameplay, saving/reloading, UX readability and accessibility
remain unverified until an actual Editor/package test.

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

The NPC menu previews XP and whether the reward is expected to cause a
level-up. It also shows contract rank, available salvage, and existing bond
and reputation rewards. The starter Guide objective shows its XP reward
when the contract is ready. The confirmation message shows actual awarded
XP and LEVEL UP when applicable.

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
residents** when no starting-sanctuary objective is active. It reads only
the hero's existing saved `ResidentQuestStates`, not an expensive
world-wide NPC scan, and uses the pure C++17 `MoonfallQuestJournal.h`
catalog to identify each resident's expected contract cost and home.

- **Accepted** contracts (state 1) are candidates. The tracker selects a
  quest that has enough available salvage first, giving the player a
  clear **TURN IN CONTRACT: [resident]** objective.
- If no accepted contract is ready, the first active contract is shown
  as **ACTIVE CONTRACT: [resident]**, with available/required salvage.
- The second line shows the NPC's location, salvage progress, active
  contract count and completed count.
- Completed contracts (state 2) are counted but never tracked or
  replayed. When no contract is active the existing dungeon exploration
  and boss tracker resumes automatically.
- The starting-sanctuary tutorial still takes precedence when the hero
  is within its area. Original world-tracker dimensions and dungeon
  discovery/boss progress bars are unchanged.
- For performance, the catalog resolves its `FName` lookup IDs only once,
  then performs a fixed **25 map lookups** on visible frames. It does not
  iterate world actors, spawn content, allocate per-frame NPC metadata,
  introduce another Tick, or write save data.

The catalog's default costs mirror the currently authored NPC races
(`10 + race % 4 × 5` salvage; the Wandering Guide costs 10).
If a designer changes an individual NPC's race or contract cost, they
must update this UI-only metadata as well to keep the tracker accurate.
Regional NPC authoring remains in separate draft PR #1070.

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
   with mouse/controller. The last button must not be covered by salvage text.
6. Accept two contracts at once. Confirm the world tracker prioritizes a
   salvage-ready contract, counts active/completed quests, and switches back
   to exploration when the last contract is turned in.
7. If regional waystations are later merged, validate all 12 residents
   individually: accept, complete, bond, save, reload, and forbid duplicates.
8. Profile menu/journal rendering and level-up tests in a real packaged
   Windows build. GitHub source tests alone do not prove an Unreal build.

No change to Z/Nyra original art, attack warning HUD, or combat balance.
