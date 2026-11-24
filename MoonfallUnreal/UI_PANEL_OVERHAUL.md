# Moonfall UE5: consistent native Canvas panels

This pass improves the **existing Unreal Engine 5 UI**, without rewriting
Godot, creating another UI framework, changing saved data, or touching the
original Z/Nyra art.

## Implemented in the PR branch

- A single fixed-cost `DrawModalChrome` painter gives **Talent Web, NPC dialogue,
  Atlas, Options, Inventory/Character, and Spell Book** matching gold perimeter
  trim, top accent, and header separation. Colors vary by panel, but geometry
  and hit testing remain unchanged. The inventory stays docked at the right
  with the world visible on the left.
- One `DrawSelectionRail` painter gives a visibly focused item in **talent
  branches, NPC choices, Options rows, bag items, weapon/spell favorites, and
  Spell Book rows**. It complements existing hover, selected, and equipped
  state colors without creating new input bindings.
- The character sheet retains its four read-only, mouse-clickable pages:
  **Attributes, Ancestry & Passives, Equipment, Reputation**. Faction values
  and equipment displays continue to read saved gameplay data, not previews.
- When a window becomes too small for its authored minimum panel, the NPC,
  Options and Inventory/Character surfaces show a readable enlargement and
  close/back instruction instead of painting a rectangle past the viewport.
  Atlas and Talent Web already had their own small-window fallback.
- Every panel continues to draw the existing **real-damage red menu cue**,
  including fallback states. UI navigation does not pause hostile combat.
- The shared painters use a constant number of Canvas rectangles. They do not
  query actors or iterate game-world entities and do not require a game save.

## Existing controls preserved

- **C**: Character. Repeated C cycles its four pages; mouse tabs and controller
  D-pad navigation remain supported.
- **Tab**: Atlas Chronicle focus, not a character sheet page.
- **B / I / Select**: Inventory. **P**: Spell Book. **J**: Atlas.
- **Esc / close X**: existing panel back/close behavior.
- **WASD and arrows**: movement only, not substituted for character tabs.
- Pointer selection and controller actions continue to use the current
  authoritative per-panel input geometry.

## Verification and limitations

- `python3 -m unittest -v tools.test_unreal_modal_panels` checks all six
  shared panel calls, focus markers, compact fallbacks and existing controls.
  It runs from `.github/workflows/unreal-source-checks.yml`.
- The existing character, talent, NPC, Atlas, inventory, combat and save
  source regression suites remain the broader safety net.
- Automated source checks **do not verify native UE5 C++ compilation, mouse
  coordinate accuracy, controller hardware, 4K visual quality, actual frame
  time, or a Windows executable**. Those still require a Windows UE5 build and
  live playtesting before merging.
- The upgrade is a shared visual and accessibility pass across the six major
  native menus. It does **not** claim that every gameplay HUD widget, original
  3D asset, equipment system or back-end feature was redesigned.
