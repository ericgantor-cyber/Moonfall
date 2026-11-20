# The Divine Family of Moonfall

The **Divine Family** is the in-world pantheon of powerful gods. The title describes a shared divine circle; it does **not** establish a biological relationship or a fixed hierarchy.

| God | Divine identity | Presence |
| --- | --- | --- |
| Z | Void Reaper | Existing divine world boss |
| Nyra | Eclipse Queen | Existing divine world boss |
| The Crimson Reaper | Goddess of Death | New optional death-goddess encounter being developed |

## The Crimson Reaper
An alluring, formidable goddess of death associated with the Moonlit Necropolis. She carries a violet-glowing scythe and commands grave and soul magic. The approved portrait from the conversation on November 19, 2026 is the canonical visual reference. An approved image asset still has to be imported; no new art is substituted for it.

Her presence adds a third god to Moonfall's lore without rewriting Z or Nyra. The relationship between the three is open for future story development. All three are world powers, **not selectable heroes**.

## Integration status (draft PR #1075)
- Godot new-hero lore panel names the three members; this does **not** add a playable Godot boss.
- Unreal optional arena preset `death_goddess_crimson_reaper` instantiates the Death Goddess boss identity.
- Unreal HP-driven phase progression is defined for four phases; the combat HUD names the divine boss and displays its correct phase count.
- Signature scythe and soul attacks, dedicated runtime artwork, actual full encounter tuning, packaged builds, and in-engine testing remain pending.
- Her encounter is opt-in. The original default arena setup, existing Z/Nyra boss identifiers, existing hero creation and save compatibility remain unchanged.
