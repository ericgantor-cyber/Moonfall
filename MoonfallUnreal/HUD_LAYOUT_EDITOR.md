# Moonfall Unreal HUD Layout Editor (UE 5.4 source)

**Status:** Source implementation in a draft pull request. This has not been
verified by compiling or running Unreal Engine or a packaged Windows build.

## Use

1. During gameplay, open **Esc / Start → Options**.
2. Select **HUD layout editor** (ninth row) and click either +/- button;
   keyboard/controller left/right also opens it while the row is selected.
3. The editor outlines **ORBS**, **QUEST**, and **BOSS**. Boss placement is
   editable even when no boss is nearby.
4. Left-click and drag an outlined panel to move it. Scroll the wheel to
   resize the selected panel from **70% to 160%**. Arrow or D-pad up/down
   selects a panel, left/right changes its size.
5. Choose **RESET** to restore all three default placements/sizes. Choose
   **DONE**, or press Esc / Start, to save and close the editor.

Panel layout is an accessibility/user preference. It is saved to the local
`GameUserSettings.ini` under `[Moonfall.HUDLayout]` as normalized offsets
and individual sizes. Invalid/nonfinite settings are clamped. The geometry
is viewport-bounded so screen resizing or switching to 4K cannot put the
whole panel outside the screen. Game progress is still saved in the
independent existing V12 save slot; no save-format migration is needed.

## Preserved

- Four resource indicators (red HP, blue mana, purple XP, green stamina),
  combat events, boss attack cues, quest progression and gameplay bindings.
- No additional characters, art replacements, texture retouching or modifications
  to the original Z/Nyra assets.
- Godot is untouched. The Unreal branch is isolated from draft dungeon/loot PR #1051.

## Verification before merge

- Run `python tools/check_unreal_source.py` and available source/structure CI.
- Compile the UE 5.4 Editor and Win64 game targets on an authorized Windows runner.
- In native play, test pointer-down, drag, release, wheel, keyboard/controller,
  reset, Esc, loss of health, loading after restart, UI at 720p/1080p/4K and
  portrait-ish small windows. Test absent/active bosses and quest transitions.
- Verify combat input is blocked while editing, resumed after Done; monitor
  render cost and confirm the normal gameplay HUD has no extra actor scans.
- Validate the cooked game, Windows 10/11 launch and installer separately.

Passing the source tests alone does **not** prove engine compilation or
working UI interaction.
