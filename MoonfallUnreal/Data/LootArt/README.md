# Moonfall loot art: six painted icons + live UE5 fallback

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

The **top three** are used by the Unreal inventory once the asset is
imported. The remaining three are supplied artwork, not new obtainable item
types or claims of implemented currency/crystal/chest gameplay.

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
as `/Game/Moonfall/LootArt/T_MoonfallLootAtlas`, checking the result
and saving a real `Texture2D` asset. Close and reopen the editor after
the first import so the native HUD constructor can resolve the asset.
Include that saved imported texture in the cooked Windows game.

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
- Inventory icon rendering uses one atlas with UV subdivisions; the
  texture is loaded only from a native-class default asset reference.
- Ground pickup shapes are configured during `InitializeLoot`, after the
  original canonical item-code validation, with no tick and no collision.
  This does not affect drop spawn identity, V12 save schema or pickups.
- No remote resources are fetched during gameplay, and there is a fallback
  when an artist's texture has not yet been imported.
- Verify that source CI passes, then **separately** compile UE5.4 on Windows,
  import and cook the asset, inspect icons at 720p/1080p/4K, test all
  weapon/armor/accessory slot codes and all rarities, collect items, reopen
  saves, and confirm no inventory pointer-row hit-test changes.
- Do not merge or advertise a working Windows installer until native
  validation is complete.
