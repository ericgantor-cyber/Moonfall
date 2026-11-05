# Godot-to-Unity/Unreal parity ledger (experimental, not a merge into Godot)

Canonical implementation: `godot-v38/` on `main`. Never alter Godot saves, scenes, Z/Nyra original artwork, or production launcher for this comparison. This ledger records actual source-level migration boundaries; a design checklist is not a playable port.

## Verified Godot sources inspected
- `godot-v38/project.godot`: native Forward+ 3D rendering, D3D12 with Vulkan/OpenGL fallbacks, 1280×800 default window, `res://scenes/main.tscn`.
- `godot-v38/scripts/systems/inventory_rules.gd`: bounded bag capacity (100–500; upgrades by 50), 1000 vault cap, 999 materials per stack, 99999 total per material, equipment power clamp (0–250), craft validation before consumption, and rarity-aware sorting.
- `godot-v38/README.md`: continuous world and in-game Raven Atlas, oxygen/status info, original artwork and save-protection requirements.
- `godot-v38/MAJOR_OVERHAUL_ROADMAP.md`: planned features are **not automatically implemented**. Distinguish them from actual production source.

## Port status
| System | Existing Godot | Unity 6 experiment | Unreal 5 experiment |
|---|---|---|---|
| Continuous six-region world, traversal and dungeons | Source exists, extent requires audit | Six adjacent test regions with 18 persistent enemies and 30 connected side chambers; actual Godot dungeon layouts not ported | Six adjacent connected test regions with 18 persistent enemies; actual Godot dungeons not ported |
| Combat and boss warnings | Source exists | Primitive test encounter only | Primitive test encounter only |
| Permanent progress and save migrations | Existing Godot format | Experimental PlayerPrefs for HP victories and relic inventory, not compatible | Isolated SaveGame for HP victories and relic inventory, not compatible |
| Inventory/crafting capacity & cost rules | `inventory_rules.gd` | `MoonfallInventoryRules.cs` source parity port added | `MoonfallInventoryRules.h` source parity port added |
| Items, rarity, loot and equipment | Existing subsystem | Rarity/pity and gear rule source ports only | Rarity/pity and gear rule source ports only |
| Atlas, quests, progression, professions | Existing features vary | Godot XP/level/Hero/Tamer point rules wired to prototype boss; Atlas/quests/professions pending | Godot XP/level/Hero/Tamer point rules wired to prototype boss; Atlas/quests/professions pending |
| Prototype minimap and HP/mana/XP orb HUD | Godot has Atlas and existing HUD | Prototype arena radar + red HP/blue placeholder mana/purple XP rings in OnGUI | Prototype arena radar + red HP/blue placeholder mana/purple XP rings in AHUD |
| Windows packaging, controller, accessibility | Godot source / CI | Untested | Untested |
| Z and Nyra original art | Preserve byte-identically | No import | No import |

## Inventory behavior contract
- Default 100 slots, max 500, upgrades of 50; upgrade price 100, 250, 400, 550, 700, 850, 1000, 1150; at max capacity cost 0.
- Material stack split into chunks of at most 999; clamp total to 99999.
- Reject any malformed material cost (zero, negative, over 99999); refuse new gear if bag full; do not consume materials on failed craft.
- Preserve deep-copy semantics if transferring mutable items; original Godot gear structures must not be aliased.
- Source ports added for gear transfer, power/name/rarity searches and stable sorting, and matching discard behavior. Still pending: engine compilation and runtime testing, richer gear data/deep-copy contracts, equipment copy, reward distribution, save schemas and UI.

## Next gates
1. Run equivalent Unity and Unreal inventory/crafting tests with shared test vectors, and connect these rules to the experimental gameplay UI.
2. Source-parity tests for invalid recipe costs, overflowing materials, full bags and capacity upgrades.
3. Audit actual Godot world/quest/loot/spell paths before expanding game content. Do not infer implementation from wishlists.
4. Build both engines in real editors on Windows; verify original art import byte identity and replay critical gameplay.
5. Only consider a migration after measured perf, graphics, input and save compatibility testing.

## Loot rule portability (new)
Godot `scripts/systems/equipment_rng.gd` supplies five rarity thresholds, clamped luck 0–40, pity guarantees at 30 and 80, at most three distinct affixes, and pity reset after Rare or better. `MoonfallLootRules.cs` and `MoonfallLootRules.h` implement deterministic **roll-value-to-rarity** and pity logic plus an affix-selection helper. They do NOT implement Godot's exact seed-to-roll random stream, affix value/tier calculations, full item catalog or loot integration. `tests/test_loot_parity_vectors.py` checks reference math vectors and source markers only. It does not compile or execute the Unity/Unreal implementations.

## Boss victory loot integration
Both experimental boss handlers now award **one session-only placeholder boss relic** on first victory, selecting its rarity using the ported Godot thresholds and an engine-native random integer in [0, 999]. This connects real combat events to the loot rules but is NOT a full Godot loot port. Loot now enters bounded experimental relic inventories (up to 100 entries) persisted through engine-local prototype saves. It does not yet use item tiers, affix values, luck/pity progression, Godot's seeded RNG, or production inventory/save schema. The +5 HP boss victory reward remains separate and saved by each experimental engine's own system. Engine compilation and visual gameplay testing remain pending.

## Inventory integration scope
The boss-victory handler now adds a single rarity-labeled relic to a 100-entry experimental bag, saved between sessions. Unity uses bounded indexed PlayerPrefs keys, Unreal adds a bounded FString array to its isolated SaveGame. Both restore these relics when starting, without touching Godot's original saves. These are NOT the full 100-500 slot Godot equipment bags, crafting UI or complete item models; no platform compilation or restart test has occurred yet.

## RPG progression added
From `godot-v38/scripts/native_v11.gd::_award_experience`, both experiments now port the actual XP gain rules: 15 base + 8 per zone + 12 for special variant + 100 for boss, up to +20 for kill combos within 7 seconds, and XP threshold `75 + (level-1)*40`. Level cap 100; level-up grants one Hero and one Tamer point and a bounded 20 HP heal. The single experimental boss is treated as a special variant in zone 0 and awards XP once on victory. Level, XP and points persist only in separate engine-specific prototype save formats. This is **not** yet the full Godot RPG: quest and dialogue systems, the six connected regions, companion AI, class-specific spell trees, player customization, gear equip bonuses and actual skill spending remain unported. No engine compilation or gameplay test has been completed.

Note: Godot also already has a separate `MoonfallUnreal/` codebase on main. It should be audited for reuse before recreating its character, spells, settings and combat work. Do not conflate that older project with this isolated comparison prototype.

## In-game minimap and orbs
The experimental **gameplay screen**, not just a menu, now draws a small arena minimap at the upper-right (hero and boss markers) and concentric red HP, blue mana and purple XP rings at the lower-left. XP and HP reflect gameplay state; mana remains a **full-value placeholder** because mana/spells are not integrated in these combat slices yet. This small arena radar does not contain the complete six-region Godot Raven Atlas and does not replace it. Unreal attaches its AHUD to the game mode; Unity draws in OnGUI. Visual layout, accessibility scaling, Windows compatibility and engine compilation are unverified.

## Minimal gameplay HUD requirement
User-approved gameplay overlay is strictly the upper-right minimap and lower-left red HP, blue mana, purple XP orbs. No experience counters, boss bars, loot text, level number, control legend, diagnostic text or floating status panels. Game mechanics, loot and progression continue underneath without extra permanent HUD widgets. Unreal on-screen debug messages have been removed; save errors still log via UE_LOG. The blue mana ring remains a placeholder until spells are implemented. Visual testing in actual engines is pending.

## Mana-based RPG combat (new)
The Unity and Unreal experimental combat slices now implement a **Q arcane bolt** (30 mana, 1.1-second cooldown, 25 + 2×level damage, 10 Unity units / 1000 Unreal cm range) and an **E heal** (40 mana, 5-second cooldown, 30 + level HP restored up to the existing max). Both regenerate 8 mana/sec capped at 180. Left mouse remains the **only basic attack input**. Dodge uses 45 stamina and regenerates at 30/sec. Spells work behind the user-approved gameplay UI and add no additional HUD widgets. The blue mana orb now reflects a resource that can change rather than a permanently full placeholder. These are **new experimental mechanics**, not an exact port of Z/Nyra class spells; original hero art is still unchanged. Unreal/Unity editor compilation and gameplay are unverified.

## Original artwork hookups
Unity now loads the original imported Z and Nyra hero images as camera-facing sprite visuals (keys 1 and 2), and the imported boss_0 art for the experimental boss. Missing assets retain the old placeholders. Unreal now uses original hero and boss textures as billboards once the separately staged artwork is imported in Unreal Editor. This connects art to prototype entities without changing the source pixels. Neither version has animated 3D character meshes or a complete character-select system yet. Editor compilation and visual tests are pending.

## Connected-world combat foundation (2026-11-05)
Both prototypes now build **one contiguous six-region test strip** at startup with slightly overlapping ground sections, **three fixed beast enemies per region (18 total)**, and a boss stationed at the far end. There are no wave spawns, region selection modes, or scene transitions. Fixed monsters have region-scaled health and contact damage, accept left-click and Q spell attacks, award zone-based XP once killed, and persist a saved 18-bit defeated-enemy mask in their isolated prototype saves. The compact minimap follows the hero and marks nearby beasts and the boss. Original bestiary artwork is used where engine import permits, with safe geometric fallbacks. This demonstrates a single-world RPG system but **does not reproduce the original Godot region maps, five dungeons per zone, quests, terrain artistry, navigable obstacles, loot tables or world streaming**. Real engine compilation, stability, performance and Windows installs are unverified. No production Godot assets, hero appearances or save files were modified.

## Walk-in chamber exploration (2026-11-05)
Thirty physical **side chamber prototypes** now connect to the continuous six-region test world, **five per region**. Each chamber has a floor that overlaps its parent region, an initially unclaimed cache, and proximity auto-pickup with no new buttons or UI panels. Claimed caches grant region-scaled XP and 25 mana (capped), then save a 30-bit discovery mask in each prototype's isolated save. The camera and compact minimap continue to follow the character; visible green markers show unclaimed caches only. Returning to a chamber after restarting does not duplicate its reward. These are one-room exploration alcoves, **not** the five complete multi-room Godot dungeons per zone, bosses, puzzles or quest content. Godot gameplay, art files, and saves remain unchanged. Engine/editor compilation and gameplay tests remain pending.

## Exploration safety and anti-farming fixes
Both new-engine experiments now restrict the final boss's attack telegraph and damage to a 16-unit / 1600-cm local radius, preventing unavoidable map-wide hits while exploring the other five regions or the side chambers. Defeating the boss saves a one-time cleared flag. Reloading hides that boss and cannot award another permanent HP boost from it. Remaining regular enemies, chambers and exploration are still playable. This is a deliberately small one-boss prototype; repeatable boss trials, save migrations and full dungeon boss progression are future work. Engine compilation and Windows gameplay have not yet been verified.

## Guarded chambers and local monster AI (2026-11-05)
Both engine experiments now place **30 fixed chamber guardians**, one per cache. The preexisting 18 regional beasts remain, giving 48 fixed overworld and chamber encounters in the experimental six-region strip. Caches **cannot be collected until their guardian has been defeated**; both defeated-guardian and claimed-cache states have separate 30-bit saved masks. Guards and regular monsters award XP on defeat. Standard enemies and guardians pursue nearby heroes using a limited leash to prevent map-wide aggro; no waves, spawning scheduler, new on-screen panels, or scene transitions were added. The guardian look is provisional: Unity uses an existing imported bestiary image; Unreal uses a basic mesh until the corresponding artwork is bound. Original Godot project, Z/Nyra artwork and production saves are unchanged. These are experimental connected-room fights, **not full multi-room dungeons**. Source-only static checks do not establish Unreal or Unity runtime performance, navigation quality, or Windows build success.

## Z and Nyra class identity and sustain (November 5, 2026)
Both experimental engines now give Z a stronger basic melee attack (20 + level base) and Nyra stronger magic (34 + 3×level) and healing (40 + level), with alternative values for their weaker specialization. Combat kills and level-ups no longer restore HP automatically. Only the manually cast healing spell restores HP; HP is clamped to the existing maximum. Guardian state still saves independently, so reloading does not enable duplicate kill rewards. Hero identity is saved in both prototypes and works even before artwork textures are imported, using geometric fallbacks when necessary. This is *provisional class balance* rather than a complete port of Z's Dragon Transformation or Nyra's unique spell tree. The original images are unchanged and the gameplay overlay remains orbs plus minimap only. Source checks cannot establish in-engine compilation or playability.

## Healing-on-kill removed (November 5, 2026)
As requested, both experimental engines now remove ALL passive HP restoration triggered by beast kills, chamber guardian kills, boss kills, or level-ups. Boss victories can still increase maximum HP capacity, but do not refill current health. XP, rewards, and the manual E-key healing spell remain. Static regression checks specifically reject the former heal-on-kill code. This does not modify the separate production Godot project.

## Imported world artwork connected (2026-11-05)
Both experimental engine worlds now display already-imported Moonfall illustrations as **six regional landmarks** and **treasure-cache artwork** at each unclaimed chamber. Unity loads Resources/MoonfallArt/v31_4_bestiary/region_0 through region_5 plus v31_3_art/treasure_cache using camera-facing sprites. Unreal loads the corresponding Content Browser textures after the optional Unreal Editor import step and uses non-colliding billboards; if assets are absent, geometry remains. The loot visuals disappear when collected. This reuses the existing original image files **without generating or modifying pixels**; it is not an in-game text-to-image generator, full terrain-texture conversion, animated VFX, or proof of editor compatibility. The original Z and Nyra artwork and Godot project remain untouched. No automatic HP healing from kills or level-ups was reintroduced.

## New AI-generated environment artwork hookup (2026-11-05)
Two new original environmental JPG crops, Moonfall moonlit gate and waterfall ruins, are distributed as a separate downloadable art ZIP in the conversation. The crops deliberately exclude any newly imagined hero art or numeric HUD so the original Z/Nyra imagery is never replaced. Unity's MoonfallPrototype automatically loads MoonfallGeneratedArt/moonfall_ai_waterfall_ruins in region 0 and moonfall_ai_moonlit_gate in region 4 if the user copies the files into its Assets/Resources/MoonfallGeneratedArt directory. Unreal's MoonfallPawn loads the same two optional assets at /Game/MoonfallGeneratedArt after JPG import into the Unreal Content Browser. Missing art is handled safely. IMPORTANT: the actual newly generated JPG binaries are in the downloadable pack, **not present in this GitHub branch yet**. Source static tests only validate hooks; engine compilation, art placement, and Windows builds remain unverified. Combat logic, original art, and no-passive-kill-healing rule stay unchanged.

## Gameplay checkpoint continuity (2026-11-05)
Both isolated engine experiments now periodically save the hero's position and actual remaining HP every eight seconds, and save both fields along with normal progression events. Reload restores an in-bounds position and valid nonzero HP. Older Unreal and Unity saves with no checkpoint continue at their original start positions and default health. Dead-state checkpoints are deliberately not written: restarting from the last living checkpoint is an experimental recovery policy, not a kill-healing mechanic. This avoids repeatedly resetting the character to the first region after completing dozens of chambers. Checkpoints remain **source-level, unverified in Unity and Unreal runtimes**, and should be tested for save migration and interrupted writes before any Windows release. Production Godot and Z/Nyra artwork remain untouched. No kill-based or level-based HP restoration was reintroduced.

### Checkpoint recovery bug fix (2026-11-05)
Unreal `SaveProgress` now attempts to load and reuse the existing experimental save object before creating a new one. Previously, a save triggered while HP was zero could write a brand-new object with `bHasCheckpoint=false`, erasing the last living checkpoint. The new code preserves the previous location and HP on death-time saves and updates it only while alive. Added source-level regression coverage; actual editor save/reload testing remains required.

### Clean exit and suspend persistence (2026-11-05)
Unity now saves experimental progression and the current living checkpoint when the application pauses or quits. Unreal now saves when the game exits or changes levels, but intentionally avoids autosaving during normal editor shutdown or arbitrary actor removal. Both keep the eight-second checkpoint timer, all existing rewards, and the no-passive-healing-on-kills rule. If the hero is dead, the previous living checkpoint is preserved. These hooks are covered by source tests but still require actual Unity/Unreal shutdown and reload playtesting, including force-quit scenarios.
