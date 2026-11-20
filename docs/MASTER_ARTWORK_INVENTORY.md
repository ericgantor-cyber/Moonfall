# Moonfall master artwork inventory and preservation gate

Status: **repository manifest inventory, not a complete visual-art audit** (2026-11-20).

## Existing source library

The tracked `engine-comparison/ART_IMPORT_MANIFEST.json` declares **71 source images** under `godot-v38/assets/` and **142 converted copies** (one Unity and one Unreal *prototype SourceArt* output per source). Category breakdown:

| Folder | Declared images |
| --- | ---: |
| beasts | 9 |
| heroes | 2 |
| ui | 1 |
| v21 | 6 |
| v22 | 12 |
| v31_3_art | 8 |
| v31_4_bestiary | 18 |
| v31_realism | 14 |
| v36_world_art | 1 |
| **Total** | **71** |

The original Z and Nyra source files are:
- `godot-v38/assets/heroes/Z_Original.webp`, SHA-256 `81b8ac6bae0830ab8d8bf53d2cdcbc8bae07256a8b7a9d4398ce4591bafd0666`.
- `godot-v38/assets/heroes/Nyra_Original.png`, SHA-256 `64aa23e2de7ea0e07347171c3ebe4beae13166359b08855d1076db9dd25871a9`.

The same checksums appear in `godot-v38/docs/ART_ASSET_INTEGRITY.txt` and the later full-image manifest. The existing conversion manifest records **distinct** hashes for Z's PNG export and the original WEBP, as expected; do not demand byte identity across different encodings.

## What this confirms, and what it does not

- The **GitHub-committed** originals are version controlled; a local read-only SHA-256 comparison can establish whether the working tree still contains the expected bytes.
- An Unreal *SourceArt* copy is **not** proof that an image was imported into the actively playable `MoonfallUnreal` project as a `.uasset`, assigned to a material, or rendered by its characters.
- This list **does not certify every image the user has designed**, particularly new Morvella, Death God, bosses, NPCs, environments, ferry decorations, or art created only in a conversation, downloaded locally, or on unmerged branches. These require a separate original-file reconciliation before claiming they are saved.
- This inventory does not automatically upload, change, regenerate or back up any images.

## Verification

From the repository root, run:

```sh
python tools/verify_artwork_inventory.py
python tools/verify_artwork_inventory.py --check-exports
```

The first command compares all declared original files to the manifest and reports any missing images, altered bytes, additional unmanifested images, or malformed/duplicate inventory entries. The optional second command also validates all declared Unity and Unreal prototype exports. These checks are **read-only**. Treat any failure as a blocker before importing art into UE5 or merging a mass asset change.

## Non-destructive Unreal migration policy

1. Never edit or overwrite `godot-v38/assets/heroes/Z_Original.webp` or `Nyra_Original.png` to improve Unreal rendering; import copied assets instead.
2. Preserve the 71 baseline sources and record any new approved original in a separate additive manifest with name, source path, SHA-256 and intended Unreal destination.
3. Confirm missing new designs from their actual image files. A concept description or generated placeholder is **not** equivalent to a committed original asset.
4. For each new UE import, verify appearance against the source and check a real UE5 Editor run, packaged Windows rendering, and source-file checksums.
5. Do not merge art replacement or delete an original without explicit review. Keep an off-GitHub backup of approved source files as a second recovery location.

**Current gap:** This check has not been run against a native Windows UE5 installation, and the newly designed character/boss artwork has not been reconciled.

## Reconciliation of newer character and boss artwork (2026-11-20)

Passing integrity CI establishes that the **71 recorded source images and 142 recorded prototype exports** match their manifest. It does **not** prove that every recently designed character or environment image has been committed. Default-branch GitHub search is not an exhaustive binary-image or all-branch scan.

| Subject | Confirmed repository evidence | Artwork status | Safe next step |
| --- | --- | --- | --- |
| Z | Original WEBP and SHA-256 recorded in baseline | Verified in baseline | Preserve exact image bytes during Unreal import |
| Nyra | Original PNG and SHA-256 recorded in baseline | Verified in baseline | Preserve exact image bytes during Unreal import |
| Death Goddess | PR #1075 documents the four-phase boss and explicitly says no image was committed | Design documented; original art not verified | Locate the actual approved original image; record path and SHA-256 |
| Morvella | Referenced in recent Moonfall work; no confirmed original image in default-branch code search | Unverified, **not declared lost** | Reconcile approved art from source files, uploads, or other branches |
| Other new bosses, NPCs, zones and scenery | Some older assets are in the verified manifest; new designs may not be | Requires per-design reconciliation | Inventory exact original files without regenerating or replacing artwork |

Never silently replace an approved image with a placeholder. Import copies into UE5 after confirming the visual identity, and add newly approved originals to an **additive** manifest. Do not alter any of the original 71 assets.
