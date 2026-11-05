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
| Continuous six-region world, traversal and dungeons | Source exists, extent requires audit | Not ported | Not ported |
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
