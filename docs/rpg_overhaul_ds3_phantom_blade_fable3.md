# Moonfall RPG overhaul: design reference and implementation tracker

Updated: 2026-11-07. Inspiration only; do not copy assets, characters, maps, dialogue, lore, animations or exact UI from other games.

## Core direction
One continuous interconnected fantasy RPG world, not waves or separate arena modes. Original art for Z and Nyra must remain unchanged. Windows 10/11, controller support, click-to-attack, accessible options and reliable resume/installer are requirements. No finisher shockwaves, no KO/shatter KO labels, no attack-countdown HUD.

## Reference strengths and Moonfall translation

| Reference | Verified design principle | Original Moonfall adaptation |
| --- | --- | --- |
| Dark Souls III | Stamina management, meaningful equipment and dodge timing, weapon skills, challenging exploration | Distinct weapon weight and movesets, readable telegraphs with *no attack countdown*, dodge/parry/guard tradeoffs, memorable shortcut-linked dungeons |
| Phantom Blade Zero | Hybrid chained attacks and defense, opponent guard pressure, weapons obtained from bosses, exploratory routes gated by earned abilities | Alternate attack strings, precise single-target posture payoffs, bespoke boss trophies/weapon traits, secret traversal routes reopened by earned world abilities |
| Fable III | Interactable NPCs, relationships, reputation/consequences, economic properties and jobs, civic choices and quest map | Faction/settlement reputation, branches with durable local changes, housing and workshops, profession income with bounded economy, quests that react to decisions |

Sources:
- Dark Souls III starter guide: https://d1vtv52f4vjbmu.cloudfront.net/manuals/darksouls3/goty/ps4/DarkSouls3Goty_PS4_StarterGuide_GBQS.pdf
- Dark Souls III survival guide: https://news.xbox.com/en-us/2016/04/12/the-dark-souls-iii-survival-guide/
- Phantom Blade Zero developer coverage (2026-10-28): https://blog.playstation.com/2026/10/28/phantom-blade-zero-how-boss-weapons-transform-combat-and-exploration/
- Fable III game manual: https://manualzz.com/doc/36151307/microsoft-fable-iii-video-game-user-manual

## Development tracks and completion criteria

1. **Combat foundation (in progress).** Preserve working hit detection, four-stage same-target combos, guard/parry/dodge, enemy posture, queued death reactions, and five-style procedural finishers. Next: authored animation assets/AnimBP timing, input buffer that respects dodge/guard cancellation, distinct weapon move profiles, boss attack recovery. Test with Windows UE compilation and in-game fights.
2. **Equipment / progression (in progress).** Weighted RNG rarity for ordinary loot, guaranteed epic boss drops, at-least-rare elites is in source on this branch. Next: actually equipped item objects with persistable IDs, meaningful RNG affixes, boss-learned weapon abilities, reforge/salvage, explicit drop tables and tuning tests. Rarity alone must not be described as working affix RNG.
3. **World / traversal (planned).** One connected world with organically connected biomes, at least five dungeons per zone, fog-of-war minimap + terrain, quest markers for discovered objectives, fast travel after discovery, secrets/shortcuts and ability-gated optional routes. No waves or artificial extra game modes.
4. **Fable-like living settlements (planned).** NPC dialogue/relationship/reputation state, meaningful branching quests, local faction reactions, buyable property/workshops, crafting professions and jobs; persistent impact on routes, vendor prices, quests and environment. Build data-driven tables with stable IDs and explicit save-versioning before UI complexity.
5. **Boss identities (planned).** Each boss earns its own combat rhythm and world purpose. Boss unlocks could grant unique traits/skills/access to traversal secrets; never reuse copyrighted boss identities or artwork.
6. **Performance and accessibility (ongoing).** Cap dynamic effects, pool where justified, avoid per-frame world scans, maintain reduced-motion option, color-independent telegraphs, remappable controller/mouse controls, UI scaling, readable quests, options and autosave.
7. **Windows release gate (not yet verified).** Real Unreal Editor/BuildTool compile, cooked/pakaged Windows 10/11 binary, executable launch test, controller and mouse smoke tests, save/resume, performance pass, reproducible signed/unsigned installer and desktop shortcut. GitHub source and script checks are not proof of a working Unreal build.

## Verification snapshot
- **Implemented in source:** 4-hit same-target offensive spell combo, posture mechanic, guard/parry and dodge code, finisher queue, timed enemy death reactions, finisher momentum, weighted item rarity.
- **Not verified through Unreal compilation or playtesting:** all gameplay changes in this PR.
- **Planned only:** true cinematic animation assets, meaningful item affixes, social simulation, properties, full interconnected overworld, boss-earned traversals and complete Windows installer.
- **Never reintroduce:** finisher shockwaves, KO labels, extra arena/wave game modes, modified original Z/Nyra art.
