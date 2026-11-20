# The Divine Family of Moonfall

The **Divine Family** is the in-world pantheon of powerful gods. The title describes a shared divine circle; it does **not** establish a biological relationship or a fixed hierarchy.

| God | Divine identity | Presence |
| --- | --- | --- |
| Z | Void Reaper | Existing divine world boss |
| Nyra | Eclipse Queen | Existing divine world boss |
| Morvella Velmora (The Crimson Reaper) | Goddess of Death | Default reachable Moonlit Necropolis world boss in the Unreal draft |

## Morvella Velmora, The Crimson Reaper
Morvella Velmora, known as **The Crimson Reaper**, is an alluring, formidable goddess of death associated with the Moonlit Necropolis. She carries a violet-glowing scythe and commands grave and soul magic. The approved portrait from the conversation on November 19, 2026 is the canonical visual reference. An approved image asset still has to be imported; no new art is substituted for it.

Her presence adds a third god to Moonfall's lore without rewriting Z or Nyra. The relationship between the three is open for future story development. All three are world powers, **not selectable heroes**.

## Integration status (draft PR #1075)
- Godot new-hero lore panel names the three members; this does **not** add a playable Godot boss.
- Unreal optional arena preset `death_goddess_crimson_reaper` instantiates the Death Goddess boss identity.
- Unreal HP-driven phase progression is defined for four phases; the combat HUD names the divine boss and displays its correct phase count.
- Four distinct telegraphed signature footprints (lane, safe-center ring, cross, circle) are wired to the shared Unreal damage/defense path; full scythe choreography, soul summons, dedicated runtime artwork, full encounter tuning, packaged builds, and in-engine testing remain pending.
- Her separate world encounter is enabled by default (editor-toggleable), while the original one-god arena preset remains optional. The sixth-zone road connects to her necropolis without a teleport. A dedicated saved-victory flag prevents respawn after defeat without changing the legacy 30-dungeon victory ledger. The original Z/Nyra boss IDs, appearances, hero creation, and earlier saves remain unchanged.
