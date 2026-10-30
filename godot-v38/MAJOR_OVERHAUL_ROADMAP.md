# Moonfall: integrated major-overhaul implementation plan

Status: **planned**, except items verified in merged PRs. This document does not claim implementation.

## Non-negotiable constraints
- Preserve the original Z and Nyra artwork bytes exactly. Never replace their existing textures with generated substitutes.
- One connected game world. Avoid new standalone modes.
- Preserve saved games through schema versioning and migration; test upgrades.
- Keep combat readable and controller/keyboard remapping usable.
- Profile first, optimize measured bottlenecks, and preserve performance at 20, 40, and 60 active enemies.
- Keep features modular, avoid duplicated mechanics, and require Godot and Windows build checks before merge.

## Delivery sequence

| Phase | Systems | Acceptance criteria |
|---|---|---|
| 1 | Combat responsiveness, hitboxes, stagger and knockback, boss cues, combat AI | Reliable attack/dodge input; no resource drain in menus; readable damage reactions; boss warnings have counterplay |
| 2 | Living world, biomes, dungeons, rooms, events, bosses | One continuous world; deterministic room generation by seed; no progression dead ends |
| 3 | Loot, item affixes, equipment, meaningful rarity, professions, crafting | Data-driven drops and recipes; no unbounded item IDs; balance checks |
| 4 | Z/Nyra talent trees, spells, progression, character builds | Distinct builds; no artwork changes; spell/resource and upgrade tests |
| 5 | HUD, settings, accessible/controller controls | Live HP/mana/XP/stamina, no sample-number conflicts, remappable keys and controller flows |
| 6 | Performance, save migration, QA, Windows 10/11 release | Profiling at 20/40/60 mobs; no severe frame spikes; verified checkpoints; repeatable installer build |

## Engineering milestones
1. Establish smoke tests and record initial frame-time, AI time, physics and draw-call baselines.
2. Consolidate combat feedback into existing systems; avoid duplicate stagger, knockback, floaters.
3. Implement world/dungeon content via data tables, procedural seeds and existing asset pipeline.
4. Add progression, loot, crafting and professions in bounded data-driven modules.
5. Run gameplay, save-load, controller and accessibility test matrix.
6. Publish a tagged Windows installer only after CI plus practical gameplay verification.

## Definition of done
A task is **done** only when implementation is committed, tests pass, gameplay behaves as specified, and any required artwork and installer assets are present. PR opened != merged; CI passing != visual gameplay validation.
