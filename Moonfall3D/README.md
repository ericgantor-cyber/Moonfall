# Moonfall 3D experimental Godot project

This is a standalone **Godot 4.3+** prototype, kept separate from the existing Moonfall browser game. Open this folder's `project.godot` with Godot and press F5.

Controls: **WASD/arrows** move; **left mouse/J** attack; **space** dodge; **mouse wheel** zoom the third-person follow camera; **R** restart after defeat.

Included: Z/Nyra choice using **geometric placeholders**, enemy spawning/chasing, attack, HP, XP, leveling, healing pickups, a boss spawning approximately every 120 seconds (after the previous boss is defeated), and a third-person follow camera with zoom.

Excluded by request: switchable camera modes, camera switch on C, survival timer and visible boss countdown. Internal boss scheduling remains.

**Do not change Z or Nyra's original art.** No character models have been ported. The existing Moonfall game and its art remain unchanged.

**Status:** experimental integration source. Not yet launched or gameplay-tested in Godot. Not a replacement for the existing game or Windows installer.

## Experimental dungeon/combat pass

- Visual room themes and perimeter pillars rotate every 15 defeated enemies. These are decorative arena variations, not separate connected levels.
- Ordinary enemies have two basic steering patterns; full obstacle-aware pathfinding is not implemented.
- Z and Nyra produce distinct temporary colored spell impact spheres. These do not change original character art.
- Third-person follow camera and no survival timer remain unchanged.
- Requires testing in Godot; no runtime gameplay validation was performed.

Camera update: the overhead viewpoint was removed. Camera follows behind the facing direction of the player with mouse-wheel distance adjustment. No camera-mode toggle.

## WoW-style third-person camera

Hold **right mouse button** and drag to orbit camera and turn your character. Hold **middle mouse button** and drag to orbit without turning the character. Mouse wheel zooms. WASD/arrows move relative to the current camera direction. There is no overhead mode, camera-toggle key, or survival timer. Camera collision avoidance and full WoW movement bindings are not yet implemented. Godot playtesting is pending.

## Third-person combat and navigation pass

- Camera obstruction raycast pulls the camera forward when it intersects **physics colliders**; current decorative pillars do not have colliders, so practical collision avoidance is limited until solid dungeon geometry is added.
- Basic directional target selection prioritizes reachable nearby enemies in front of the character and displays a visible target ring; this is not mouse cursor picking or full lock-on.
- Procedural capsule placeholder bobs during movement. Original Z and Nyra artwork/models are not modified.
- Four room waypoints are tracked as navigation landmarks; actual connected rooms and NavMesh pathfinding are still future work.
- No overhead camera, camera switching, or survival timer.
- Source updated, **not tested in a Godot runtime**.

## Solid dungeon collision pass

- Added collision-enabled perimeter walls and four interior obstacles.
- Player now uses CharacterBody3D move_and_slide() with capsule collision against static walls.
- Basic ray-based enemy obstacle steering attempts to move around walls, but enemies are still Node3D agents without full navigation meshes or collision bodies. They may become stuck or pass through obstacles.
- Camera obstacle checks use world geometry collision layer 2, avoiding player collision interference.
- This remains **one arena** rather than a true multi-room dungeon. No changes to original Z/Nyra art, camera mode, or removed survival timer.
- GDScript changes merged only after source review; **Godot playtesting is still required**.

## Three connected dungeon rooms (experimental)

Two solid divider walls with center doorways divide the arena into three traversable rooms. Room label and visual theme now follow the character's Z position instead of kill count. Enemies in different rooms aim for divider doorways first; their navigation is heuristic and not NavMesh pathfinding. All rooms remain in one loaded scene. Boss timer remains internal, without a displayed survival timer. WoW-style third-person camera and original character art are unchanged. **Godot gameplay and pathfinding tests not yet run.**

## Randomized room obstacles and loot chests

Each new run seeds three interior obstacle positions while retaining the existing three-room floor plan and central doorways. Three golden chests appear, one per room. Press **E** within 2.5 units to open a chest for 10–35 gold and either +25 HP (capped at max HP) or +2 damage. Gold appears on the HUD. This is a small procedural-variation and loot prototype, **not** a full procedural dungeon generator, inventory system, or NavMesh pathfinding implementation. Godot runtime gameplay tests are still needed. Original Z/Nyra art and the WoW-style camera remain unchanged.

## Equipment and inventory prototype

Chests now give gold plus one weighted-rarity weapon: Common, Uncommon, Rare, Epic, or Legendary. Inventory is capped at 20 items; an overflow chest awards extra gold instead. Press **I** to toggle the bag listing. The strongest found weapon automatically equips and contributes its power differential to damage, without stacking duplicate bonuses. No manual equip, sell, persistence, armor slots, or full equipment UI yet. Existing doorway-steering AI is unchanged; sophisticated navigation is still pending. Godot runtime tests are not yet run.

## Equipment slots and enemy abilities (experimental)

Chests may now yield Weapon or Armor items. Weapon damage and armor damage-reduction are independent. Open inventory with I and press F to cycle through inventory items and equip the selected item; this is a rudimentary control, not a full equipment screen. Armor reduces each enemy hit by its power with a minimum of 1 damage. Bosses gain a close-range magic strike on cooldown and skirmishers can retreat in short bursts. These are simple prototypes, not full tactical AI. No changes to original Z/Nyra art, WoW camera or removed survival timer. Godot gameplay testing is still pending.

## WoW-inspired keybinds (experimental)

Default keys: WASD movement, Space dodge, 1 attack, 2 second attack trigger (currently uses same base attack, not a separate spell), Tab cycle nearby targets, B inventory, F interact, G equip next item with inventory open, F10 keybind settings. Left mouse attack and right/middle drag camera remain. In F10 keybind settings use Up/Down to select an action, Enter to capture a new key, Esc to cancel. Remaps save to user://moonfall_keybinds.json. Duplicate key choices are not assigned. This is a prototype, not a full WoW action-bar UI. Existing arrow-key movement and mouse zoom remain. Godot runtime testing has not yet been performed.

## Jump and initial 12-slot action bar

Space now jumps; Q dodges. Existing locally saved defaults with Space dodge are migrated to Space jump/Q dodge. The 12 action slots use keys 1–0, minus, and equals and show cooldowns. These are placeholder variants of the same attack, **not twelve unique spells**. F10 continues to support remapping movement/jump/dodge/attack/spell, while full remapping of all twelve slots remains future work. The three-room arena still lacks full floor physics and the jump implementation uses simple vertical velocity and a y=0 ground clamp. Godot runtime testing required.

## Action-bar remapping and grounded jumping

A solid physics floor now supports CharacterBody3D grounded jumping. **Space** jumps, **Q** dodges. F10 settings now list all twelve independently remappable action slots (1–0, minus, equals by default). Up/down selects, Enter captures a new key, Esc cancels. Saved bindings migrate the former Attack and Spell entries into slots 1 and 2; duplicate key assignments are rejected. The actions still use repeated prototype attack variants, not twelve unique spells. Godot execution/playtesting is pending. The WoW-style camera, removed survival timer and original Z/Nyra artwork remain unchanged.

## Skyrim-inspired dual-hand spell menu (experimental)

Press M to open the spell menu. Keys 1–4 assign Flame, Frost, Arcane or Restoration to the left hand; 5–8 assign the same spells to the right hand. Press Z to cast left, X to cast right, and C to dual-cast when both hands have the same spell. Dual cast consumes twice the mana and has enhanced effect/cooldown. Mana regenerates; hand spell cooldowns are independent. Restoration heals, and other spells use the existing target and combat/loot pipeline. The original twelve action slots now trigger spell types, but repeated slots and generic visuals remain, rather than twelve unique spells. No actual animated Skyrim hand assets or copied designs are used. Godot runtime testing remains pending. WoW camera, Space jump/Q dodge, and original Z/Nyra art remain unchanged.

## Expanded Moonfall spellbook

M opens an expanded six-spell spellbook. Up/Down browse; Z assigns selected spell to left hand and X to right hand. Existing 1–4 and 5–8 shortcuts still select the original four spells while the menu is open. New Storm and Venom damage schools have different power/cost/cooldown and colored magic flashes, alongside Flame, Frost, Arcane, and Restoration. Outside the menu Z/X cast equipped hands and C dual casts matching spells. The twelve action slots cycle through the six spells. This is a source-only prototype: no full hand animation, damage-over-time or slowing debuffs, or Godot gameplay verification yet. WoW camera, Space jump/Q dodge, no survival timer and original Z/Nyra assets stay unchanged.

## Elemental spell overhaul

Flame applies a 4-second burn (6 damage/second), Frost applies a 3.5-second slow (35% movement reduction), Venom applies a 6-second poison (3.5 damage/second), and Storm adds a 9-damage shock burst. Effects scale in duration or burst with dual cast. Arcane retains direct damage and Restoration heals. Damage-over-time defeats now use the existing XP, kill and loot routine. Spell schools have shared cooldowns across hands and the 12-slot bar; action-slot cooldown is only consumed after successful casts. Spell menu now displays each school effect, cost and cooldown. This is a substantial source-level prototype, **not** a verified Godot build, animated hands, persistence or a fully graphical spellbook. Original character artwork and WoW camera are untouched.

## Stabilization pass (Oct 2026)

Adds solid-wall line-of-sight checks for basic enemy melee and auto-target acquisition, prevents holding Q from repeatedly retriggering dodge after cooldown, and gives C dual casting input priority over Z/X hand casting. Godot binary was not available in the execution environment; these are source-reviewed changes rather than successful Godot playtest results. Other gameplay and performance regressions may still exist.

## GitHub automated Godot checks and experimental Windows build

Workflow `.github/workflows/moonfall3d-godot.yml` downloads Godot 4.3 and export templates, imports the Godot project, runs a short headless scene check and exports a Windows x86_64 executable. An `Moonfall3D-Windows-experimental` artifact is uploaded only if earlier stages succeed. Runs on Moonfall3D or workflow changes, and manually via Actions. This is a portable executable artifact, **not an installer** and has not yet passed CI or been tested on Windows. The original browser game and hero art are unaffected.

## Experimental Windows installer

After the Godot validation/export job succeeds, the Windows CI job runs Inno Setup and uploads `Moonfall3D-Windows-Installer-experimental`. The setup installs the game, adds a Start Menu shortcut, and offers an optional desktop shortcut plus launch-on-finish. Requires an actual successful Actions run and Windows 10/11 playtesting before release. This prototype installer does not contain original 3D Z/Nyra models, only existing temporary placeholders.

## Action bar combat feedback

The twelve action slots now display their assigned school names, the larger of their slot or school cooldowns, and insufficient-mana warnings. Spellbook shows mana and the dual-casting requirement. Cast failure due to insufficient mana displays a short message. This is source-level UI feedback and does not add twelve unique spells. Godot runtime test still required.

## Current controls: no WoW-style action bars

The twelve-slot WoW-style action bar and the 1–0/-/= ability-slot bindings have been **removed** by request. Magic uses the Skyrim-inspired **M** spellbook, **Z** left-hand cast, **X** right-hand cast, and **C** matching-spell dual cast. Space jumps, Q dodges, and F10 remains the movement/accessibility keybinding menu. Inside the spellbook, numeric keys 1–4 and 5–8 still provide spell-selection shortcuts (they no longer cast from a bar). Some earlier log sections describe the old bar and are historical only. The third-person camera stays; the original 2D/browser game and Z/Nyra art remain untouched.

## Graphical spellbook

The M spellbook now opens a Godot PanelContainer with readable spell buttons, mana/cooldown information and left/right hand summary. Select spells by clicking or with Up/Down, equip selected spell to left with Z or right with X, close with M. The older 12-slot WoW-style action bars stay removed. These are functional UI widgets, not new spell art; Godot CI and gameplay validation are required.

## Mouse-friendly spellbook controls

The graphical spellbook now displays six school symbols, a highlighted selected row, and separate LEFT/RIGHT buttons on each spell. Click either hand button to equip that spell, with a checkmark showing the current loadout. Keyboard assignment with Z/X remains available. The obsolete WoW 12-slot action bar stays removed. Characters' original artwork remains unchanged. This update has not been runtime-tested in Godot.

## Expanded spellbook school categories

The graphical spellbook groups Flame, Frost and Storm under Destruction; Arcane under Mysticism; Restoration under Restoration; and Venom under Alchemy. Spell rows now have larger symbols, full gameplay descriptions, costs, and cooldown values. Left and right equip buttons remain clickable. No twelve-slot WoW action bars have returned. Godot runtime validation of this change is pending.

## Scrollable spellbook and spell preview

The M spellbook now places its categorized list in a ScrollContainer for mouse-wheel browsing and adds a selected-spell details panel showing school effect, mana, power, and cooldown. Clicking spell rows changes the preview; LEFT/RIGHT equipping remains. These UI changes do not add a WoW action bar, modify Z/Nyra artwork, or change the camera. Godot runtime validation required.

## Animated spell feedback

The spellbook's selected spell preview has a subtle rotating symbol sequence even while the spellbook pauses gameplay; combat impact spheres also spin during their brief lifespan. No new character art or WoW action bars. These effects are lightweight prototypes, not hand-casting animations or cinematic effects. Build validation and Windows playtest remain necessary.

## Dual-hand casting visuals

Successful Z/X spells now emit independent left- or right-side elemental glows; matching-spell C dual casting emits both. Cast effects are short lived and have a 65-active-visual cap to guard against effect buildup. These are lightweight geometric spell effects attached to neither original hero art nor custom 3D hand rigs. No WoW action bars are restored. A running Godot build and gameplay frame-rate test are still needed.

## Hand casting arc animation

Each temporary left/right casting glow follows its respective side of the character and rises in a short arc over the 0.25-second effect lifetime. Effects continue to use the existing capped VFX pool; no additional art assets, WoW action bars, or hero redesigns. This is a procedural spell animation, **not** skeletal hand animation. Godot runtime verification required.

## Elemental impact silhouettes

The six magic schools now have slightly different impact-orb proportions in addition to their existing colors. These are lightweight meshes rather than particle simulations. The VFX cap and original artwork remain unchanged; no WoW action bars are reintroduced. Requires Godot build and gameplay verification.

## Headless gameplay smoke-test gate

GitHub Actions now runs `Moonfall3D/tests/smoke_test.gd` before exporting Windows builds. It checks default Space/Q bindings, absence of WoW action-bar slot bindings, hero selection, spellbook construction, independent hand equip, Restoration healing/mana cost, and school cooldown enforcement. A failing test blocks the installer build. These are automated smoke checks, **not** a substitute for mouse/keyboard, visual, performance, or Windows 10/11 testing.

## Controller overhaul (experimental)

Godot gamepad support now uses left stick for analog camera-relative movement (dead zone 0.18), right stick for orbital camera (dead zone 0.20), A for jump, B for dodge, left/right shoulder for left/right spellcasting and both shoulders for same-spell dual cast. Y opens or closes the M spellbook. While the spellbook is open, move the left stick up/down to select, X equips left hand, and A equips right hand. Back/View opens inventory; pressing the right stick cycles targets. Space/Q keyboard behavior remains, and no WoW-style action bars have been restored. Defaults currently target standard Xbox-style button names; controller-specific differences, input conflicts, and analog feel still require physical Windows gameplay testing. No controller remapping UI has been added yet. Original character art unchanged.

## Controller accessibility settings (F9)

Press **F9** to open a graphical controller settings panel. It provides sliders for movement stick dead zone, right-stick camera dead zone, and camera sensitivity. It also provides an **opt-in vibration** toggle (off by default). Successful spell casts briefly vibrate the first connected gamepad when enabled; switching vibration off stops the motors. Values currently apply for the active session only; saving controller preferences and remapping controller buttons still need development. No WoW action bars or changes to original Z/Nyra character artwork.

## Unified Options menu

Press **Esc** on the keyboard or **Start/Menu** on a connected Xbox-style controller to open the centralized Options menu. Choose Resume, Spellbook, Inventory, Keybindings, or Controller Settings using the focusable buttons. Esc or Start closes the main menu; Esc while in a submenu returns to the main Options menu; Start from a submenu opens the main menu. Gameplay pauses while Options, the spellbook, controller settings, keybindings, or inventory are open. Older direct shortcuts may still open their own panels, but Esc/Start is the universal entry point. No WoW action bars are added. Z and Nyra's original artwork remains untouched. Requires build and controller playtesting.

## Full-game overhaul, stabilization phase

October 2026 stability pass: identified a failing Godot CI run where six GDScript type inference parse errors prevented the main game script from loading and made the smoke test time out. Explicit types were added to navigation, distance, room tint and spell description values. CI now checks Godot import and startup output for script errors, failing early instead of allowing misleading green steps. This is **phase 1** of a larger full-game overhaul, not a claim that gameplay/graphics/Windows controller testing is complete. Keep Esc/Start options, Skyrim hand casting, third-person camera, Space jump, Q dodge, original Z and Nyra art, and no WoW action bars.

## Full game overhaul phase 2: combat AI

Boss ranged attacks now require an unobstructed line of sight. Enemy groups use short-range repulsion to limit overlap, skirmishers select independent clockwise/counterclockwise strafing directions, and ranged skirmisher retreat is frame-scaled to avoid abrupt teleport-like movement. Separation remains a basic capped-mob steering heuristic, not NavMesh pathfinding. Godot build and actual gameplay verification still required. Skyrim-style casting, Esc/Start Options, Space jump, Q dodge, no WoW action bars, and original Z/Nyra art remain unchanged.

## Phase 2: telegraphed boss attacks

Bosses choose between a larger delayed impact (1.1-second windup, 3.1-unit radius) and a smaller faster impact (0.75-second windup, 1.8-unit radius). A red translucent ground indicator marks the target area before impact, letting the player move or dodge out of it. Damage resolves against the marked spot, checks dodge invulnerability and wall line-of-sight, and obeys the visual-effect cap. This is an experimental first pair of boss attack patterns and does not yet constitute fully distinct bosses or animation rigs. Godot gameplay testing is still required. Original Z/Nyra art and the removal of WoW bars are preserved.
