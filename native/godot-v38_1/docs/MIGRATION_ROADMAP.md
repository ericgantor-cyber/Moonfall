# Moonfall HTML → native Godot migration map

**Reference:** Moonfall v114 All Regions Isometric 3D HTML, source art and architecture.

| Module | v1 status | Native expansion work |
|---|---|---|
| Direct3D 12 native renderer | Configured | Verify actual driver on Windows using `RenderingServer.get_current_rendering_driver_name()` |
| Z/Nyra original art | Original files retained, billboards + selection | Work out approved in-world presentation without changing source pixels |
| Witchwood terrain and obstacles | Native 3D demo implemented | Navigation, procedural LOD, dense flora, terrain elevation |
| Camera | Isometric orthographic follow | Camera obstruction avoidance, zoom, options |
| Combat | Basic proximity attacks and wolf damage | Abilities, parry, balanced damage, telegraphs, projectiles |
| Wolves | Five native wolf enemies | Full wildlife AI, prey, packs, species skins |
| Shrine | Activation, heal and native checkpoint | Real fast-travel network and seven zones/shrines |
| Ground loot | Rare Moonfang beacon and G pickup | Full itemization, colored drops, bag capacity |
| Hunter Senses | Basic clue visibility | Actual tracking, clue actions, contracts |
| Saving | Native v1 JSON (separate from HTML) | v114 checkpoint importer + robust validation |
| Remaining regions | Not migrated | Bring in other five regions as one connected world |
| Skinning | Not migrated | Timed carcasses, profession leveling, recipes |
| Tamer/companions | Not migrated | AI and 18+ skill perk logic |
| Raven Atlas and quests | Not migrated | UI port with accessibility and controller support |
| Mounts | Not migrated | Travel-only, no mounted combat, 20-second combat auto-dismount |
| Native Windows build | Export preset included | Build with matching export templates; test .exe / installer |

## No-redesign guarantee

The hero art is included at `assets/heroes/Z_Original.webp` and `assets/heroes/Nyra_Original.png` without edits. The source SHA-256 hashes are recorded in `ART_ASSET_INTEGRITY.txt`. The original Moonfall HTML file remains untouched.
