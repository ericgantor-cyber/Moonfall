# Moonfall loot table editor (Unreal Engine 5.4)

This is a **mouse-editable Unreal Data Table** workflow, not yet an in-game
loot tuning panel. It has been added to **draft PR #1051**. Source checks
do not prove native UE5.4 compilation, CSV import success, packaged Windows
behavior, or actual in-game drop distributions.

## Easier option: standalone visual editor, no Unreal required

Open `MoonfallUnreal/Data/Moonfall_LootEditor.html` locally in a modern
browser (Edge, Chrome, Firefox or Safari). It is a single offline page with
large mouse-accessible controls and keyboard-focus indicators.

- **Load default 41 rows** starts from the shipped weighted presets.
- **Import CSV** opens an existing tuning file.
- **Add loot row**, **Duplicate**, and **Delete** change the table without code.
- **Undo / Redo** reverses up to 40 in-session editing actions, including
  row changes, imports, reset and bulk weight updates.
- **Bulk adjust** scales weights by 50%, 75%, 125%, 150% or 200% across
  all rows matching the source, region and text filters, even across pages.
  Invalid edits (including values above 10000 or missing required base pools)
  are rejected atomically without discarding the original values.
- **Filter + paginate** tables up to 2048 rows, with 40, 80 or 160
  editable rows per page; all matching rows participate in bulk edits.
- Edit source, zone, minimum level, rarity, weight, slot mask or trait in the
  grid; filter by source or name to find specific rows.
- Select reward source, region and hero level to preview **exact theoretical
  rarity, equipment-slot and trait percentages**, plus the 12 highest-weight
  eligible rows. Slots are uniformly selected among a row's allowed slot
  bits, while traits follow the existing Unreal rarity/forced-trait rule.
  These are **conditional-on-an-item** odds, not an overall drop rate.
  The preview is not a simulated Unreal gameplay test.
- **Export CSV for Unreal** saves a new `Moonfall_CustomLootTable.csv` on
  your computer. Invalid rows or missing generic pools block export.

The page does **not** contact a server, install software or modify the
repository or the live game. Changes and undo history exist only in the
current browser tab until you export a CSV; closing with unexported edits
shows a browser warning when supported. Imported files are limited to 1 MB.
Import validation is atomic, so invalid files preserve the current table.
The exported CSV is still the source to import into Unreal, not a live
gameplay configuration file.

## Create the visual loot editor without writing C++

1. Open the Moonfall Unreal project in UE5.4 after its C++ targets compile.
2. In the Content Browser, import
   `MoonfallUnreal/Data/Moonfall_DefaultLootTable.csv`.
   Select **FMoonfallLootTableEntry** as the row structure and name the
   new Data Table asset `DT_MoonfallLoot`.
3. Double-click the asset. Unreal's built-in **Data Table Editor** displays
   rows and columns that you can edit with a mouse. Save the asset.
4. Open the Blueprint class derived from **AMoonfallHeroPawn**. In **Class
   Defaults → Moonfall → Loot → Tables**, assign `DT_MoonfallLoot` to
   **Loot Data Table**. A complete, valid imported table replaces the
   built-in loot weights for that hero; no CSV is read directly at runtime.
5. Press Play In Editor. The pawn imports rows in **alphabetical row-name
   order** at BeginPlay. `bLootDataTableActive` reports whether the
   import succeeded. If invalid, the game logs a warning and preserves the
   native fallback pool.
6. Reimport the CSV / edit and save the Data Table asset to iterate. During
   Play In Editor, call the Blueprint function `ReloadLootDataTable` to
   refresh the already loaded asset without restarting. This does **not**
   save changes into the packaged game or the player's save slot.

UE5's visual Data Table Editor is separate from Moonfall's gameplay
menus. There is **no custom mouse-accessible in-game loot editor yet**.

## What each column controls

| Column | Accepted values | Meaning |
| --- | --- | --- |
| Name | 1–80 ASCII letters, digits, `_`, `-` | Unique ignoring case (Unreal `FName` semantics); deterministic sort order |
| Source | Regular, Elite, Boss, Treasure | Source-specific reward pool |
| ZoneIndex | -1 or 0..5 | -1 generic world/any dungeon; 0..5 an additional regional pool |
| MinimumLevel | 1..100 | Character level required before the row participates |
| Rarity | 0..5 | Common, Uncommon, Rare, Epic, Emerald, Legendary |
| Weight | 0..10000 | Relative chance among eligible rows; zero disables the row |
| SlotMask | 1..7 | 1 weapon, 2 armor, 4 accessory; combine bits, 7 means all |
| ForcedTrait | 0..3 | 0 random, 1 Force, 2 Ward, 3 Focus |

**Guarantees:** Elite rewards must be Rare or better; boss rewards must
be Epic or better; dungeon treasure must be Rare or better. Common gear
cannot have a forced trait. All reward sources require at least one
enabled generic, level-one row. Unsafe or incomplete assets fail back to
the built-in table instead of deleting loot.

The example contains **41 rows**: 17 generic rarity rows and 24 regional
bonus rows (six regions × four sources). Region rows change both equipment
slot and trait preferences using existing encoded item IDs. Each source
still has its own weighted rarity distribution, independent of others.

## See the real odds before importing

From the repository directory:

```powershell
python tools/moonfall_loot_table.py
python tools/moonfall_loot_table.py --source Boss --zone 5 --level 60 --details
python tools/moonfall_loot_table.py --source Treasure --zone 2 --level 20
python tools/moonfall_loot_table.py --file C:\MyLoot\Balance.csv --source Elite --zone -1 --level 30
```

The tool validates the table and prints the **effective percentage by
rarity**. With `--details`, it also prints effective slot and trait
probabilities and the 12 most likely eligible rows. Changing one row's weight changes the denominator too. It uses
the same one-million cumulative-weight cap and alphabetical row order
as the Unreal C++ selection code. The helper reports *theoretical odds*;
actual randomness and equipment drops still require gameplay testing.

Source smoke checks now run through GitHub Actions for the browser editor
and the Python CLI; they cover default data, 1,000,000-weight capping,
case-insensitive duplicate names, invalid bulk edits, undo/redo, pagination
and CSV round trips. These are **not** native UE5 tests.

In Blueprint you can read:
- `GetLootPoolTotalWeight(Source, ZoneIndex, HeroLevel)`
- `GetLootRarityChancePercent(Source, ZoneIndex, HeroLevel, Rarity)`
- `bLootDataTableActive`

These are **read-only preview** APIs; they do not spend currency, grant
items, change a saved treasure, or consume a random number. You can connect
them to a UI widget when the actual in-game loot editing UI is built.

## Save and gameplay rules

- **No save migration:** Equipment codes remain trait/slot/rarity/tier.
- **No loot duplication:** Ground drops still use the existing persistent
  GUID and transactional collection logic.
- **No treasure reroll:** Previously pending or claimed dungeon treasures
  are read from the V12 ledger before trying a new weighted roll.
- **No retroactive rebalancing:** Reweighting a Data Table changes only
  future generated rewards, not items already owned or saved.
- **Runtime edits:** The assigned UDataTable is a cooked game asset;
  packaged Windows users cannot permanently modify it through this
  editor-only workflow. An actual in-game UI plus safe persistence needs
  separate implementation.

## Required verification before merging

On a trusted UE5.4 Windows runner: compile C++ and generated headers,
import this CSV into a real Data Table asset, assign it to the hero,
and test all 4 sources × 6 regions at levels 1, 20, 40, 60 and 100.
Verify invalid assets use defaults, preview odds agree with measured
drops, deterministic treasure remains stable after reload, existing
V9/V12 saves open safely, and Windows packaging cooks the assigned asset.
Do not merge or call the Windows installer complete from source CI alone.
