# Moonfall Native v13 | Gameplay & Accessibility Upgrade

- New circular real-time world map with six zone anchors, awakened shrines, three rifts, regional events and moving hero marker. Tab (keyboard) / D-pad down (controller) toggles.
- Gamepad menu focus: first playable hero and Atlas tab automatically receive keyboard/gamepad focus when those menus open.
- Gamepad: left stick movement, X attack, A dodge, B interact, Y tame, shoulder spells, Start atlas, D-pad actions. Keyboard and mouse remain supported.
- Companion stances: Guard, Follow (targets actively engaged hostiles only), Passive (no attacks). Change with H or through the Tamer atlas. Safe fallback on older save games.
- Bounded regional power balancing: hero level plus equipped item power versus recommended zone power affects dealt and received damage without gating exploration.
- Shrine light shimmer throttled to 10Hz to reduce repeated property writes.
- Save schema 2 retained and compatible with v11/v12; a pet stance field is optional.
- No changes to Z or Nyra original art; all six existing regions and progression systems retained.

## Testing and limitations

Godot 4.7.2 Linux headless engine smoke, live and additional v13 regression tests. Windows executable is bundled using the user-provided Windows Godot 4.7.2 editor runtime with an embedded PCK, **not** an optimized export-template build. Windows visual gameplay and installer still require Windows testing.

## Artwork proof (SHA-256)
Z_Original.webp: 81b8ac6bae0830ab8d8bf53d2cdcbc8bae07256a8b7a9d4398ce4591bafd0666
Nyra_Original.png: 64aa23e2de7ea0e07347171c3ebe4beae13166359b08855d1076db9dd25871a9
