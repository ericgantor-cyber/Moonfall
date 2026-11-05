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
| Permanent progress and save migrations | Existing Godot format | Experimental PlayerPrefs only, not compatible | Isolated SaveGame slot, not compatible |
| Inventory/crafting capacity & cost rules | `inventory_rules.gd` | `MoonfallInventoryRules.cs` source parity port added | `MoonfallInventoryRules.h` source parity port added |
| Items, rarity, loot and equipment | Existing subsystem | Rarity/pity and gear rule source ports only | Rarity/pity and gear rule source ports only |
| Atlas, quests, progression, professions | Existing features vary | Not ported | Not ported |
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
Both experimental boss handlers now award **one session-only placeholder boss relic** on first victory, selecting its rarity using the ported Godot thresholds and an engine-native random integer in [0, 999]. This connects real combat events to the loot rules but is NOT a full Godot loot port. Loot is not added to an inventory, does not persist, and does not yet use item tiers, affix values, luck/pity progression, or Godot's seeded RNG. The +5 HP boss victory reward remains separate and saved by each experimental engine's own system. Engine compilation and visual gameplay testing remain pending.
