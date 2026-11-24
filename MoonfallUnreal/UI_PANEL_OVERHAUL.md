# Moonfall UE5: consistent native Canvas panels

This pass improves the **existing Unreal Engine 5 UI**, without rewriting
Godot, creating another UI framework, changing saved data, or touching the
original Z/Nyra art.

## Implemented in the PR branch

- A single fixed-cost `DrawModalChrome` painter gives **Talent Web, NPC dialogue,
  Atlas, Options, Inventory/Character, and Spell Book** matching gold perimeter
  trim, top accent, and header separation. Colors vary by panel, but geometry
  and hit testing remain unchanged. The inventory stays docked at the right
  with the world visible on the left.
- One `DrawSelectionRail` painter gives a visibly focused item in **talent
  branches, NPC choices, Options rows, bag items, weapon/spell favorites, and
  Spell Book rows**. It complements existing hover, selected, and equipped
  state colors without creating new input bindings.
- The character sheet retains its three read-only, mouse-clickable pages:
  **Stats & Gear, Ancestry & Passives, Reputation**. Equipment and stats share one page. Faction values
  and equipment displays continue to read saved gameplay data, not previews.
- When a window becomes too small for its authored minimum panel, the NPC,
  Options and Inventory/Character surfaces show a readable enlargement and
  close/back instruction instead of painting a rectangle past the viewport.
  Atlas and Talent Web already had their own small-window fallback.
- Every panel continues to draw the existing **real-damage red menu cue**,
  including fallback states. UI navigation does not pause hostile combat.
- The shared painters use a constant number of Canvas rectangles. They do not
  query actors or iterate game-world entities and do not require a game save.

## Second pass: consistent hover and world HUD visibility

- An input-neutral `DrawHoverFrame` painter uses one finite, visible-pointer
  query per HUD pass to outline existing mouse hit rectangles. It covers:
  **five close buttons** (Talent, Resident, Atlas, Options, Inventory/Character);
  **Talent** branch tabs, point spending and reset confirmation;
  **Resident** choices; **Atlas** region rows; **Options** rows and +/-;
  **Character** tabs and Reputation rows; **Spell Book** tab and spells;
  **Inventory** sort, equipment action, favorite slots and bag rows.
  This never changes the underlying clickable area, input bindings, save
  schema, combat behavior or focus state. Hidden cursor means no hover.
- The **quest tracker** checks the actual on-screen horizontal bounds of the
  active nearby boss HUD. It shifts beneath the boss HUD only when they
  intersect, rather than covering the boss's health, posture and warning.
  It preserves saved quest progression and the current objective system.
- **Reduced Motion** now disables interpolation of the red HP, blue Mana,
  purple XP and green Stamina progress readouts. It also suppresses the
  critical-health pulsation, while still displaying the critical-health cue.
  No resource calculations or actual game combat values change.
- The quest tracker receives a clear static top/bottom border for legibility.
  No additional Canvas textures or world scans were introduced.

## Third pass: new original Moonfall interface artwork

- The shared **six-menu chrome** now paints engraved, symmetrical forged-metal
  corner ornaments, filigree accents and a small diamond knot. These are
  directly rendered by `MoonfallCombatHUD.cpp`, not a sketch or mock-up.
- A single, **fixed 12-segment** runtime vector-sigil painter provides four
  moonlit forms: **eclipse**, **wayfinder compass**, **ward shield**, and
  **four-point arcane talisman**. The six major menus receive distinct
  colored identities; talent branch badges, equipment rarity rows,
  reputation faction rows, the nearby boss panel, quest tracker and resource
  orb use the same shared artwork. Sigils are decorative only, with no
  clickable areas or gameplay statistics.
- `Art/UI/moonfall_eclipse_crest.svg` is the **original editable vector
  source illustration** for the Moonfall eclipse emblem. It is a design
  reference for future texture export, not yet an imported UE5 texture
  or proof of runtime asset packaging. The **runtime equivalents are drawn
  procedurally** by native Canvas code and require no missing textures.
- No external artwork, copyrighted franchise icons, original **Z/Nyra**
  drawings, gameplay meshes, shader assets, fonts, or save schemas were
  changed. The new decorations are fixed-cost and use only Canvas rectangles
  and line segments. They do not trigger actor scans, timers, effects or
  motion, and preserve original clickable geometry.
- New source regressions parse the SVG as XML, verify its trusted-local
  artwork structure, ensure the native art painter is finite/bounded, and
  check the art is actually hooked into the requested UI surfaces.
- **Native UE5 art acceptance still required:** inspect small windows,
  high DPI/4K, reduced motion, boss overlap, character panel, controller
  navigation and pointer targeting before merging.

## Fourth pass: original spell and equipment art across every panel

- The Grimoire now draws **six unique, directly usable Canvas rune icons**
  matching the real spell metadata order: Ember Bolt, Frost Shard, Eclipse
  Lance, Mending Light, Storm Spark and Thorn Dart. The icons are drawn only
  where row geometry safely permits; in compact windows, text stays readable.
- The Inventory and combined **Stats & Gear** page share **three original
  sword/armor/talisman glyphs** derived from validated saved equipment slots.
  Colors come from real item rarity and empty/unowned items cannot invent
  equipment. The glyphs do not equip or mutate anything.
- The Atlas has a static **compass rose and north label** within its map
  viewport. Its fog-of-war cell filtering remains authoritative; no hidden
  roads, shrines, or undiscovered dungeons are revealed.
- The Talent Web uses a restrained heraldic stamp for invested or focused
  talents; readable node titles and prerequisite wires remain intact.
- NPC dialogue gains a social heraldry emblem; wide Options rows and
  character stat sections get small category etchings. Existing click
  rectangles, selection styles, combat-live warning and text scales remain.
- A second original editable asset,
  `Art/UI/moonfall_grimoire_armory_atlas.svg`, provides a labeled
  **3x3 art sheet** of the six spell schools and three gear types. Its
  vector artwork is **source/reference art**, not yet a shipped UE5 texture
  or proof of asset import. The corresponding runtime art is procedural.
- The new painters are bounded, stateless, local to `DrawHUD` and never
  scan actors, allocate UI widgets, change gameplay actions or save data.
  Native compile, 4K/hardware and in-game hitbox/legibility testing are
  required before merge.

## Fifth pass: region and faction identity throughout the panels

- A dedicated six-way **region emblem** renderer now provides original
  illustrations of the six real biomes: **Ashen Wilds** (volcanic spire),
  **Moonveil Forest** (ancient tree), **Hollow Depths** (descending strata),
  **Iron Foundry** (anvil), **Frostbound Reach** (ice star), and
  **Eclipse Wastes** (split moon). Their unique seals render by the existing
  Atlas region names and in the current-region quest tracker.
- The six symbols identify *already named regions*. They never paint unseen
  map tiles, undiscovered dungeons, shrines, or roads. Atlas fog of war stays
  driven exclusively by exploration and discovery state.
- Five original **faction crests** are now tied to the saved faction IDs
  Wanderers, VerdantCircle, StoneHearth, MoonCoven, and AshenClans. Reputation
  rows use the unique crest for each faction, while nearby NPC dialogue
  uses the matching crest when its faction is recognized and a neutral
  symbol otherwise. No faction score changes.
- The shared panel chrome gained an extra inset iron border and mirrored
  side engravings across all six existing major menu families. All new
  decorations are noninteractive, have finite bounded coordinates, and
  render statically with no new world scans or actor components.
- Two original vector design sheets are committed:
  `Art/UI/moonfall_six_region_emblems.svg` (six zone insignia) and
  `Art/UI/moonfall_five_faction_crests.svg` (five faction crests).
  These SVGs are editable **source designs**, not UE5-imported textures.
  Matching **native Canvas artwork is already coded** into the HUD.
- Source regressions validate actual HUD call sites, five faction ID mappings,
  the names of all six zones, safe cardinality and input-free rendering, and
  the XML structure of both original art sheets. **Native UE5 compilation,
  legibility on tiny screens, controller navigation and visual checks in
  4K still require a Windows UE5 build** before merging.

## Existing controls preserved

- **C**: Character. Repeated C cycles its three pages; mouse tabs and controller
  D-pad navigation remain supported.
- **Tab**: Atlas Chronicle focus, not a character sheet page.
- **B / I / Select**: Inventory. **P**: Spell Book. **J**: Atlas.
- **Esc / close X**: existing panel back/close behavior.
- **WASD and arrows**: movement only, not substituted for character tabs.
- Pointer selection and controller actions continue to use the current
  authoritative per-panel input geometry.

## Verification and limitations

- `python3 -m unittest -v tools.test_unreal_modal_panels` checks all six
  shared panel calls, focus markers, compact fallbacks and existing controls.
  It runs from `.github/workflows/unreal-source-checks.yml`.
- The existing character, talent, NPC, Atlas, inventory, combat and save
  source regression suites remain the broader safety net.
- Automated source checks **do not verify native UE5 C++ compilation, mouse
  coordinate accuracy, controller hardware, 4K visual quality, actual frame
  time, or a Windows executable**. Those still require a Windows UE5 build and
  live playtesting before merging.
- The upgrade is a shared visual and accessibility pass across the six major
  native menus. It does **not** claim that every gameplay HUD widget, original
  3D asset, equipment system or back-end feature was redesigned.
