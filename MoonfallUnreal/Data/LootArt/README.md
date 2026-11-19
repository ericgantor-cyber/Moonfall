# Moonfall loot art: 51 original icons + five-level UE5 fallback

**Status:** Original art and source integration are present on a draft PR.
Native Unreal Engine 5.4 compilation, import, Win64 cooking and gameplay
have **not** been verified by source checks.

The new original Moonfall-painted 120×80 PNG atlas uses **six square 40×40 cells**:

| Atlas cell | Position | Gameplay |
| --- | --- | --- |
| Rune sword | row 0, column 0 | Weapon inventory icons |
| Lunar cuirass | row 0, column 1 | Armor inventory icons |
| Crescent amulet | row 0, column 2 | Accessory inventory icons |
| Treasure chest | row 1, column 0 | Source art for future treasure UI |
| Emerald shard | row 1, column 1 | Source art for future materials UI |
| Moon coins | row 1, column 2 | Source art for future currency UI |

The **top three** are used by the Unreal inventory once the base asset is
imported. The remaining three are supplied artwork, not new obtainable item
types or claims of implemented currency/crystal/chest gameplay.

## Expanded 9-icon atlas

`Moonfall_LootVariants_Atlas.png` is a transparent 120×120 PNG with
**nine square 40×40 cells**. An offline high-resolution edition and nine
separate original 256px illustrations are distributed in the expanded
Moonfall loot art pack. The checked-in atlas is optimized for the existing
16–30px inventory icon display.

| Cell (row, column) | Original artwork | Current integration |
| --- | --- | --- |
| (0,0) | Moon Axe | Force weapon illustration |
| (0,1) | Eclipse Spellblade | Ward weapon illustration |
| (0,2) | Moonstar Staff | Focus weapon illustration |
| (1,0) | Eclipse Shield | Ward armor illustration |
| (1,1) | Moonwarden Helm | Focus armor illustration |
| (1,2) | Starforged Gauntlets | Force armor illustration |
| (2,0) | Emerald Ring | Force or Ward accessory illustration |
| (2,1) | Crimson Potion | Source art only; consumables not added |
| (2,2) | Moon Grimoire | Focus accessory illustration |

The 3 canonical equipment slots are still weapon, armor and accessory.
**These are visual motifs selected by the existing encoded Force/Ward/Focus
trait**, not additional equipment slot classes, spells, consumables or
mechanics. No loot table weights, item IDs, equipment ownership, stats or
save structures changed.

If the expanded atlas is absent, the HUD uses the original six-cell atlas.
If neither atlas is available, native Canvas geometry still shows a sword,
breastplate or amulet, with rarity-colored borders and readable text labels.
The existing non-colliding ground-drop silhouettes remain unchanged.


## Third atlas: nine relic illustrations

`Moonfall_LootRelics_Atlas.png` is a compact, transparent 120×120 PNG
with nine 40×40 cells. The **high-resolution originals** are also delivered
in `Moonfall_Relic_Loot_Art_Pack.zip` as separate 256px images and enlarged
atlases. The committed compact PNG is optimized for actual inventory use.

| Cell | Original art | Current cosmetic mapping |
| --- | --- | --- |
| (0,0) | Twin Moon Daggers | Weapon relic illustration |
| (0,1) | Celestial Glaive | Weapon relic illustration |
| (0,2) | Meteor Warhammer | Weapon relic illustration |
| (1,0) | Nightveil Cloak | Armor relic illustration |
| (1,1) | Starforged Boots | Armor relic illustration |
| (1,2) | Eclipse Crown | Armor relic illustration |
| (2,0) | Moon Lantern | Accessory relic illustration |
| (2,1) | Astral Key | Accessory relic illustration |
| (2,2) | Starlit Locket | Accessory relic illustration |

**Art selection:** When the relic atlas is available, an **Epic, Emerald
or Legendary item of tier 8+** uses its existing equipment slot as the
atlas row, with column `(tier + rarity + trait) % 3`. That creates nine
additional deterministic motifs without new equipment slots, ID formats,
random number rolls, save fields, item drops or character stats. When the
relic atlas is missing, trait art is used if available; when the trait atlas
is missing, base art is used; if both are missing, geometric Canvas
silhouettes are used. The same item always renders the same image for a
given installed set of textures.

Every rendered icon retains a rarity-colored frame **and six white/gray
quality pips** (one through six bright marks), supporting low-color and
color-blind accessibility. Item names and rarity words remain readable.

## Fourth atlas: nine Eclipse Masterwork illustrations

`Moonfall_LootMasterworks_Atlas.png` adds a **fourth 120×120,
3×3, transparent, paletted PNG atlas** with nine named illustrations.
A separate downloadable high-resolution pack contains original 256px
artwork and a large preview sheet.

| Cell | Original illustration | Cosmetic equipment family |
| --- | --- | --- |
| (0,0) | Obsidian Greatsword | Weapon |
| (0,1) | Crescent Bow | Weapon |
| (0,2) | Aether Lance | Weapon |
| (1,0) | Eclipse Cuirass | Armor |
| (1,1) | Winged Moonhelm | Armor |
| (1,2) | Ember Greaves | Armor |
| (2,0) | Starglass Ring | Accessory |
| (2,1) | Sunless Pendant | Accessory |
| (2,2) | Astral Hourglass | Accessory |

When this atlas is imported and available, existing **Emerald and
Legendary equipment at tier 16+** displays a masterwork motif selected
deterministically by `(tier + rarity + trait) % 3` from its existing
slot row. **Masterwork is a cosmetic artwork layer, not a seventh rarity
or an added item property.** The item's gear code, loot tables, owner,
save data and combat stats remain identical.

With four available layers, the selection priority is **Masterwork
(eligible tier/rarity) → Relic (Epic+ tier 8+) → Trait → Original → Canvas
silhouette**. You can preview all combinations in the offline Atelier,
including missing-art fallbacks. Gold corner highlights indicate a
selected masterwork motif; rarity pips and text still identify the
actual item quality.

## Fifth atlas: nine Ascendant illustrations

`Moonfall_LootAscendants_Atlas.png` adds **nine more original art designs**
in a compact 120×120 transparent 3×3 PNG. The independent
`Moonfall_Ascendant_Loot_Art_Pack.zip` includes the larger original
artwork and individual transparent icon PNGs.

| Cell | Artwork | Existing equipment slot |
| --- | --- | --- |
| (0,0) | Solar Eclipse Scythe | Weapon |
| (0,1) | Starfall Longbow | Weapon |
| (0,2) | Twilight Twin Spears | Weapon |
| (1,0) | Eclipsed Starplate | Armor |
| (1,1) | Winged Astral Helm | Armor |
| (1,2) | Nightglass Greaves | Armor |
| (2,0) | Ouroboros Crescent Ring | Accessory |
| (2,1) | Voidheart Locket | Accessory |
| (2,2) | Celestial Astrolabe | Accessory |

**Selection rule:** Existing **Legendary tier-22 to tier-25** equipment,
when the optional fifth atlas is imported, uses the same equipment-slot
row and deterministic column `(tier + trait + slot) % 3`. This cosmetic
rule adds no new Legendary rarity tier, equipment slot, item code, loot
drop, attribute, world asset, animation, inventory operation or save
field. It creates no new named, obtainable item archetype.
If the fifth texture is unavailable the preview and runtime choose
Masterwork → Relic → Trait → Original → geometric fallback.

The offline Loot Atelier's 51-icon gallery and five availability
toggles preview this same selection logic; the default UI continues to
show text rarity labels and one-to-six bright rarity pips.

## Sixth atlas: nine Wayfarer illustrations for early-game loot

`Moonfall_LootWayfarers_Atlas.png` contains **nine square 40×40
sprites** in a 120×120 transparent PNG, giving the lower end of the
game's equipment progression greater visual variety.

| Cell | Illustration | Existing equipment slot |
| --- | --- | --- |
| (0,0) | Wayfarer Crescent Sword | Weapon |
| (0,1) | Verdant Moon Bow | Weapon |
| (0,2) | Sapphire Moon Spear | Weapon |
| (1,0) | Wayfarer Mantle | Armor |
| (1,1) | Moonstone Helm | Armor |
| (1,2) | Lunar Trail Boots | Armor |
| (2,0) | Emerald Wayfarer Ring | Accessory |
| (2,1) | Indigo Star Pendant | Accessory |
| (2,2) | Wayfarer Astrolabe | Accessory |

Existing **Common, Uncommon, and Rare equipment at tiers 1–7** displays
this alternate art when imported. Art comes from the existing equipment
slot (atlas row) and deterministic column
`(tier + rarity + trait) % 3`, not a new roll or drop-rate modifier.
For Common gear the trait is always None; the offline gallery now resets
an incompatible trait when switching to Common.

**Wayfarer is cosmetic**, not a new rarity, equipment slot, or item type.
If its atlas is missing, existing Trait → Base → geometric fallback art
remains. All item codes, loot tables, saved ownership, rarity marks and
equipped-item stats remain unchanged.

The Unreal import preflight additionally verifies **decompressed PNG
pixel integrity and non-interlaced scanline lengths**, beyond basic CRC
checks. All six source images are checked before importing any asset.

## No-code gallery and testing

Open `Moonfall_LootArt_Gallery.html` next to the six PNGs in a modern
browser. It requires no server, engine, account or build process. The
mouse- and keyboard-friendly gallery shows all 51 images, offers search,
collection filters and interactive item slot / quality / trait / tier
controls. Disable one or more of the six atlas layers to preview exactly which art
the native Unreal HUD should choose, including the fallback.

The shared `MoonfallLootArtRules.h` is dependency-free C++17 and
exhaustively tested in GitHub CI for all **100,800 valid combinations** of
three slots, six rarities, four traits, 25 tiers and atlas-presence flags
(with Common's invalid trait pairings excluded). Browser gallery logic
is tested against the same selection contracts. These checks do not
replace a native UE5.4 render test.

## Import the painted image into Unreal Engine 5.4

This is a source PNG, **not yet a .uasset**. Unreal must import and cook it.
With the `MoonfallUnreal` project open in UE5.4, open the Python console
(Tools → Debug → Python Console) and run:

```python
exec(open(r"<YOUR-MOONFALL-REPO>/MoonfallUnreal/Build/Import-Loot-Art.py", encoding="utf-8").read())
```

Replace the placeholder with your local checkout path, or run the script
from a controlled UnrealEditor-Cmd.exe Python session. The script uses the
existing PythonScriptPlugin and EditorScriptingUtilities to import the PNG
as **six** saved `Texture2D` assets:
`/Game/Moonfall/LootArt/T_MoonfallLootAtlas`,
`/Game/Moonfall/LootArt/T_MoonfallLootVariants`, and
`/Game/Moonfall/LootArt/T_MoonfallLootRelics`, and
`/Game/Moonfall/LootArt/T_MoonfallLootMasterworks`, and
`/Game/Moonfall/LootArt/T_MoonfallLootAscendants`, and
`/Game/Moonfall/LootArt/T_MoonfallLootWayfarers`. It validates **all six**
PNGs' dimensions, bit depths, chunk structure, CRC checksums, presence of
an image and valid terminators **before importing any file**. It also
rejects unrelated pre-existing Unreal assets occupying any of the art
paths, instead of overwriting them. Close and reopen the editor after the first import so the
native HUD constructor can resolve all available assets.
Include all six imported Texture2D assets in the cooked Windows game.
The imported content is a real `.uasset` only after this Unreal Editor
step; checking in a PNG source file alone does not package a texture.
A damaged or missing source file stops the batch before touching a
previously imported asset.

If the texture is missing, the inventory **still displays visible geometric
sword/armor/amulet fallback art**. No inventory rows, item slots, sorting,
equipping, or loot drops are lost. The drop actors use three distinct
**non-colliding 3D silhouettes** built from the engine's primitive meshes,
not additional ticking actors.

Rarity remains clearly identified by both **text** and **color** in the
existing six-rarity Moonfall scheme. The image is illustrative and contains
no embedded item stats or new player save fields.

## Performance and safety gates

- Source artwork is under `Data/LootArt`, not a change to Z or Nyra
  character designs. Godot art and maps are unchanged.
- Inventory icon rendering uses six compact optional atlases with UV
  subdivisions; textures load from native-class default asset references,
  not during per-frame gameplay. Each illustration remains identifiable
  with a rarity border and accompanying text.
- Ground pickup shapes are configured during `InitializeLoot`, after the
  original canonical item-code validation, with no tick and no collision.
  This does not affect drop spawn identity, V12 save schema or pickups.
- No remote resources are fetched during gameplay, and there is a fallback
  when an artist's texture has not yet been imported.
- Verify that source CI passes, then **separately** compile UE5.4 on Windows,
  import and cook all six assets, inspect icons at 720p/1080p/4K, test all
  weapon/armor/accessory slots × all four traits × all six rarities,
  test with zero to six textures present, collect items, reopen saves,
  and confirm no inventory pointer-row hit-test changes.
- Do not merge or advertise a working Windows installer until native
  validation is complete.
