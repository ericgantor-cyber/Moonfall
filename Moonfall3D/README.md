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
