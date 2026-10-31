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
