# Moonfall's UE6 migration track

**Status as of 2026-11-22: design and portable data contract only.** Unreal Engine 6 is not released, no UE6 project has been compiled, and there is **no claim of a working UE6 Moonfall executable**.

## Official engine boundary

Epic's [November 20, 2026 announcement](https://www.unrealengine.com/news/an-update-on-blueprints-and-the-path-to-ue6) changes the migration premise: UE6 Early Access is expected to use **Verse and Scene Graph without Blueprints or the traditional Actor Gameplay Framework**. The earlier June roadmap targeted UE6 Early Access near the end of 2027, with a full release later: [Road to UE6](https://www.unrealengine.com/news/the-road-to-ue-6). Keep the existing Unreal 5.4 C++ project buildable; **do not set `.uproject` to 6 or blindly copy Actor/Blueprint classes**.

## 141-node classless talents, engine-neutral boundary

- **Canonical catalog:** `godot-v38/data/moonfall_talents.json` (exactly 141 Moonfall-original nodes, Might/Agility/Arcana, 14 tiers, prerequisite links, rank caps, two points earned per level through level 100).
- **UE5 runtime adapter:** `MoonfallUnreal/Source/MoonfallUnreal/MoonfallTalentRules.h` mirrors those 141 nodes. The hero maintains validated ranks in a backward-compatible V13 save; `MoonfallTalentWebLayout.h` handles viewport-independent star positions and hit tests.
- **UE6 state exchange contract:** `talent_state.schema.json` describes only `schema_version`, `catalog_id`, `level`, and sparse `allocations`. It deliberately does **not** serialize earned/unspent points or bonus totals: these must be deterministically derived from the catalog.
- **Transaction invariants:** Validate every rank, talent ID, prerequisite, level gate, and two-points-per-level budget **before committing**. On failed save, retain old ranks/vitals. Never silently truncate invalid or newer saves.
- **UI and input:** In UE6, rebuild the web with Scene Graph and Verse from catalog/layout data, not UE5 Canvas/Blueprint classes. Preserve mouse wheel zoom, right-drag pan, clickable star selection, controller navigation, and confirm-before-respec behavior.
- **Gameplay:** Apply flat health/mana/stamina and percent melee/ranged/magic effects only through validated server-authoritative or single-player state, without stacking bonuses on repeated loads. Preserve existing damage/cooldown and HUD behavior.

## Migration acceptance gates

1. Wait for a released/supported UE6 SDK and official Epic conversion guidance, then create an **isolated** new UE6 implementation branch.
2. Build a Verse+Scene Graph talent adapter that loads the exact same 141-node catalog and passes source comparison plus invalid-state, prerequisite, level-gating, and respec rollback tests.
3. Verify **native UE6 compile and Windows 10/11 gameplay** with mouse and controller, including save upgrades and assets.
4. Preserve **Z, Nyra, and Morvella's original art unchanged** and never merge a conversion just because static tests pass.
5. Review all UE5 draft PR dependencies before migration. Do not merge automatically or delete Godot until game-wide parity is verified.

**This directory is an intentional future-compatibility seam, not an Unreal Engine 6 build.**
