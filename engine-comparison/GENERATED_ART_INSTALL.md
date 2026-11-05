# Install the new AI environment paintings

The two original generated environment JPGs, `moonfall_ai_moonlit_gate.jpg` and `moonfall_ai_waterfall_ruins.jpg`, were delivered in a separate downloadable ZIP, **Moonfall_AI_Art_Ready_To_Import.zip**. The binary JPGs are not committed in this PR.

## Exact installation

Extract that ZIP at the **repository root**. It creates these four files:

- `engine-comparison/unity/Assets/Resources/MoonfallGeneratedArt/moonfall_ai_moonlit_gate.jpg`
- `engine-comparison/unity/Assets/Resources/MoonfallGeneratedArt/moonfall_ai_waterfall_ruins.jpg`
- `engine-comparison/unreal/MoonfallPrototype/SourceArt/MoonfallGeneratedArt/moonfall_ai_moonlit_gate.jpg`
- `engine-comparison/unreal/MoonfallPrototype/SourceArt/MoonfallGeneratedArt/moonfall_ai_waterfall_ruins.jpg`

The ZIP also includes `AI_ART_SHA256.json` for verifying the original bytes and `README_INSTALL.txt`.

## Engine setup

Unity loads the paintings automatically through `Resources.Load<Texture2D>("MoonfallGeneratedArt/...")` as non-colliding, camera-facing world art in regions 0 and 4.

Unreal: Open `MoonfallPrototype.uproject`, enable the **Python Editor Script Plugin**, then execute `Content/Python/import_moonfall_art.py` in Unreal Editor. The importer now discovers the two generated JPGs under `SourceArt/MoonfallGeneratedArt` and imports them to `/Game/MoonfallGeneratedArt`, matching the code's asset names.

After reviewing the result in-engine, commit the newly added JPGs on this experimental branch. The code gracefully skips missing resources.

**Preservation:** Do not overwrite Z_Original.webp, Nyra_Original.png or production Godot art. These are standalone environmental images. There is no passive HP healing on kills, and there are no waves.

**Unverified:** Real editor builds and Windows playtests are outstanding. Adding art loader code does not establish that images have been committed to GitHub.
