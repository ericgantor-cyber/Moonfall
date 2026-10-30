# Moonfall Native v27: AI, combat, and encounter modularization

- `scripts/systems/spawn_rules.gd`: deterministic six-region encounter placement and scaled base health.
- `scripts/systems/ai_rules.gd`: wildlife perception, flanking/guardian approach and retreat decisions.
- `scripts/systems/combat_rules.gd`: outgoing hit calculations, elite-attack hit checks and boss recovery timing.
- Old `native_v11.gd` call sites now delegate pure math and spawn planning to these modules, with their existing behavior, scene objects, signals, and save schema retained.
- `tests/v27_system_modules.gd` tests each module and records a 45k-call headless CPU microbenchmark (not a graphical FPS measurement).
- Windows build scripts and CI moved forward to v27, corrected old `v25_legacy_modernization.gd` filename.
- No new external assets. All 29 existing artwork images preserved byte-for-byte.

Actual Windows gameplay and installer tests remain outstanding.
