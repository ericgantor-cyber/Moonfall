# Moonfall Unreal Engine 5 experiment

Tracks #685. This is an implementation specification, NOT a runnable Unreal project.

Create a blank UE5 third-person C++ project and pin an exact engine version. Build an isometric camera with mouse wheel zoom, WASD movement, left-click attack, Shift dodge. Build a single boss with 100 HP and aim-locked 2.5 m radius orange telegraph, warning for 1.15 s then dealing 20 HP damage. Hero has 100 HP, 15 damage within 4 m, 0.35 s attack cooldown, 0.32 s dodge immunity. Victory grants a persistent, capped +5 maximum HP using Unreal SaveGame. No new game modes or waves. UI: red HP, blue mana, purple XP.

Document packaged Windows build results including controls, physics, telegraph hit correctness, save persistence, GPU frame-time and scene load measurements. Use placeholders only: never modify original Z/Nyra artwork. Benchmark against the matching Unity scene, not editor frame rate.
