# Morvella Velmora: approved portrait and Unreal import

The exact approved Death Goddess portrait from November 19, 2026 is the canonical image. **No redraw, recolor, replacement or crop is authorized** as a substitute for her approved appearance.

- Original image filename: `a_detailed_highly_rendered_fantasy_illustration_o.png`
- Image: PNG, 1199 x 1312, RGB
- SHA-256: `b5553347b7f555fe646be4150eb9418816ad98d1ce5efb0d00f5cc0afb888af8`
- GitHub binary import: **still pending**. The original portrait exists as a conversation attachment, not yet as a committed Unreal asset.
- Original Z and Nyra images: **leave their existing source files unchanged**.

## How to bind the final art to the existing encounter

1. Copy the approved PNG unchanged into the Unreal project as a reference asset; compare SHA-256 before and after the copy.
2. In Unreal Editor, import the PNG as a Texture2D for portrait/UI use.
3. For the world encounter, build a 3D model or a billboard representing the **same approved appearance**; a portrait texture by itself is not a fully rigged 3D character. Do not mislabel a generic model as final art.
4. Create a Blueprint subclass of `AMoonfallEnemy` for Morvella and attach its approved visual model to the existing combat actor. Wire the existing `OnDivineAttackTelegraphed`, `OnDivineAttackResolved`, `OnDivineAttackInterrupted`, and `OnDivinePhaseEntered` events to VFX and animation assets.
5. In the persistent/default `AMoonfallArena` actor, set `MorvellaWorldActorClass` to that subclass. Default `bSpawnMorvellaInWorld = true` adds a walkable encounter at the sixth-region Moonlit Necropolis. Keep `FeaturedDivineBossId = NAME_None` for the default Z/Nyra showcase.
6. Validate UE5 compile, encounter visibility, art fidelity, warnings, four combat phases, boss victory save/load and Windows package before release.

Until steps 1-5 are completed, **the live encounter uses the base enemy visible mesh as a temporary gameplay proxy.** This is a functional world encounter prototype, not a finished visual boss.
