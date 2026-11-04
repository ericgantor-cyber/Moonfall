# Moonfall AI Art Studio

Generate reviewed PNG creature art for the **55 guardian types** and the **Americas wildlife catalog**, without changing Z or Nyra. This is an optional developer tool; there is no AI call during gameplay, and the game builds without an API key.

## Preview prompts without a network connection

Run from the godot-v38 directory:

    python tools/ai_art_studio.py --guardian 0
    python tools/ai_art_studio.py --guardian 54
    python tools/ai_art_studio.py --animal "Jaguar"

The 55 profile IDs run from 0 through 54. Animal names must occur in scripts/systems/americas_wildlife_catalog.gd. Preview only prints the prompt and file paths. It does not call any API or modify any files.

## Optional AI generation (can cost money)

Set OPENAI_API_KEY as an environment variable on your own computer. Do not commit, upload, or paste your secret key. Then run:

    python tools/ai_art_studio.py --guardian 0 --generate
    python tools/ai_art_studio.py --animal "Jaguar" --generate

The tool requests one 1024x1024 transparent PNG via the Images API, using gpt-image-2.5-flare by default, or gpt-image-2.5-sunburst with --model. The response is staged under ai_art_staging, which is excluded from git. A candidate cannot overwrite another staged candidate.

## Import artwork generated elsewhere without an API key

You can create artwork with an image tool you already use and import its PNG,
instead of paying for a new image API request. From godot-v38 on Windows:

    python tools/ai_art_studio.py --guardian 0 --import-file "C:\\Art\\moonfang.png"
    python tools/ai_art_studio.py --animal "Jaguar" --import-file "C:\\Art\\jaguar.png"

This validates the local PNG, records it as a **local-import** candidate and
stages it for human review. It doesn't generate anything, call a cloud provider,
require a login, or approve anything automatically. Files taken from
`assets/heroes` are rejected, keeping Z and Nyra out of the new asset pipeline.
No existing staged or approved image is overwritten.

## Consistent creature art direction

The studio now uses `tools/ai_art_direction.py` for a fixed, game-ready art
language. Every guardian family has its own palette and motifs, five boss ranks
get distinct silhouette instructions, and the four combat traits influence
their intended glow and spell markings. America's wildlife uses each species'
actual habitat and animal class to ask for recognizable anatomy.

The camera, perspective, canvas margins and painted 2.5D finish are consistent
across all 350 planned images. The prompts preserve the original hero artwork,
and each species still needs human art review rather than relying on prompts alone.

## Review and approve

Inspect the staged image for recognizable silhouette, quality, background transparency and consistent Moonfall style. Then explicitly approve an asset:

    python tools/ai_art_studio.py --guardian 0 --approve
    python tools/ai_art_studio.py --animal "Jaguar" --approve

The script verifies a SHA-256 record, refuses overwrites and puts approved art at assets/ai_generated/guardians/guardian_00.png or assets/ai_generated/animals/animal_jaguar.png. It also stores a review/provenance JSON record next to the approved image. It can write ONLY to these game-art locations, never to assets/heroes, so original Z and Nyra artwork is untouched.

## Artwork in the game

The existing Godot wildlife instance checks for an approved image with the matching species name and displays it on a noncolliding Sprite3D, replacing the original placeholder's visible silhouette while retaining its native collision, AI, nameplate and combat. If the approved image is missing, the normal animal model is automatically restored. Guardian art loads only if the later 55-profile system supplies the exact regional_guardian_profile ID, so old game versions remain compatible. Neither loading feature creates AI enemies or modifies attack collisions.

These are 2D transparent creature cutouts in the isometric 3D world, NOT automatically generated 3D skeletal models or animations. Game rendering does not contact the image API.

## Optional automatic 3D and native 4K PBR textures

This new workflow can turn **already approved 2D creature art into real 3D meshes**, or import a GLB model you already made in Blender, Meshy, or another service. There is **no 3D generation during gameplay**, no key in the Windows installer, and no requirement to buy a third-party service.

### Free offline import: existing 3D model

From the godot-v38 directory:

    python tools/ai_art_3d.py --guardian 0 --import-model "C:\\MyModels\\moonfang.glb"
    python tools/ai_art_3d.py --guardian 0 --approve

Other examples:

    python tools/ai_art_3d.py --animal "Jaguar" --import-model "C:\\MyModels\\jaguar.glb"
    python tools/ai_art_3d.py --game-art scenery:shrine --import-model "C:\\MyModels\\shrine.glb"
    python tools/ai_art_3d.py --game-art gear:boots --import-model "C:\\MyModels\\boots.glb"
    python tools/ai_art_3d.py --game-art chambers:14 --import-model "C:\\MyModels\\bossroom.glb"

Each command validates a real, self-contained GLB 2.0 mesh with triangle geometry and bounded scene complexity. The model is **staged first**, not approved automatically. Visually inspect its appearance, mesh topology, materials, scale, orientation and polycount before --approve. Approved GLBs are copied to `assets/ai_generated/models/<category>/<asset_id>.glb`, where Godot auto-imports and uses them without changing source code for each new model.

### Optional paid image-to-3D with Meshy

Meshy's documented Image to 3D API accepts approved images, creates genuine 3D models, and offers 4K (4096×4096) texture maps. Its documented model/texture combination uses **meshy-7.1** for geometry and **meshy-7** for 4K PBR texturing. See https://docs.meshy.ai/en/api/image-to-3d for current compatibility and pricing.

Supply your **own MESHY_API_KEY as a local environment variable**. API calls may consume credits and require third-party account access. Explicitly submit exactly one conversion:

    python tools/ai_art_3d.py --guardian 0 --submit

This requires an already-approved transparent PNG for guardian_00. The code requests one 3D model, asks for a 12,000-triangle remeshed geometry budget and 4K PBR base-color/metallic/roughness/normal textures, and records a task identifier **without storing the key**.

Use these separate commands to check and fetch a result:

    python tools/ai_art_3d.py --guardian 0 --status
    python tools/ai_art_3d.py --guardian 0 --fetch
    python tools/ai_art_3d.py --guardian 0 --approve

No continuous polling occurs, and nothing spends credits unless you deliberately run --submit. --fetch stages the model for inspection; --approve adds a reviewed model to the actual Godot project. Generating 350 models automatically in a paid batch would be expensive and is **not** turned on. A provider request for 4K textures does not guarantee the service delivered the full detail; visually inspect the downloaded GLB.

### How true 3D loads inside the game

Godot prioritizes a validated, approved **GLB 3D model** over an approved 2D PNG sprite, and a reviewed PNG over the native old placeholder. No approved asset means unchanged art and unchanged gameplay.

- Existing wild animals and any guardian with a valid profile ID can display **real 3D geometry**, while native CharacterBody3D movement, attacks, collision, XP, loot, health, and save mechanics remain unchanged.
- A reviewed 3D shrine or equipment piece can appear directly in existing world and equipment-drop locations.
- Dungeon rooms 7 and 14 can show a true 3D themed room ornament. The other 12 rooms retain lightweight artwork. This protects Windows FPS and avoids loading 420 high-poly 4K meshes into the world.
- HUD, UI glyphs, and spell icons remain **2D**, because UI doesn't need 3D geometry.
- The loader dynamically scales mesh models to game-world size and rejects missing/invalid model assets. A default mesh or sprite remains available as a fallback.

4K describes source texture resolution, **not** 4K monitor output or a 4K polygon count. High-resolution source textures can consume a lot of VRAM; use Godot import texture compression, mipmaps, reasonable LODs and performance tests before shipping many 4K models. Meshy output is neither automatically rigged nor guaranteed to include unique walk/attack animation; that is a separate task.

**Z and Nyra are completely excluded.** Original player art is neither generated, replaced, converted, nor remixed.

## Whole-game AI art: 350 approved artwork slots

The studio is not limited to creatures. Its offline plan includes **350 approved-art slots**: 55 named guardian types, 234 distinct Americas animal species, and 61 additional slots covering **6 region murals, 14 dungeon chamber murals, 9 existing glyphs including the HP/mana/XP globe HUD, 10 spell/action icons, 12 gear categories, and 10 world scenery props**.

Preview any specific game asset without calling an API:

    python tools/ai_art_studio.py --list-game-art
    python tools/ai_art_studio.py --game-art regions:00
    python tools/ai_art_studio.py --game-art chambers:14
    python tools/ai_art_studio.py --game-art glyphs:orb_hud
    python tools/ai_art_studio.py --game-art glyphs:rift_warning
    python tools/ai_art_studio.py --game-art spells:nova
    python tools/ai_art_studio.py --game-art gear:boots
    python tools/ai_art_studio.py --game-art scenery:shrine

All these categories support --import-file, --generate and --approve with the same explicit review workflow. Approved PNGs live only under `assets/ai_generated/<category>/`; existing hero artwork cannot be targeted.

To see the full review board, run:

    python tools/ai_art_review.py --category all --gallery
    python tools/ai_art_review.py --category glyphs --gallery
    python tools/ai_art_review.py --category chambers --gallery

The offline HTML gallery can search and filter every entry. It does not generate files or incur API usage.

**Actual game integration:** Existing rift-warning, treasure-cache, treasure-carrier, forge, dungeon-seal, boss-phase, loot and moon-surge glyphs, plus the original globe HUD ornament, can be replaced by reviewed matching PNGs without new controls or extra hitboxes. Three native Q/E/R spell icons show only when their approved images are available. Real equipment drops can display approved charm, band, cowl, boots, mantle, crest, relic or weapon cutouts. Six existing shrine sites can show a regional mural, and the existing 420 regional dungeon chambers can show their approved themed chamber mural. The dungeon renderer still controls room visibility and camera culling. Approved PNGs are 2D visual elements inside Godot, not new 3D terrain or procedural geometry.

**Performance/scope:** No approved PNG means no new decorations or physics bodies; native art remains. The maximum optional chamber art budget is one noncolliding billboard per existing chamber, up to 420 only if all corresponding artwork is approved. Extra scenery/gear/spell production-plan entries are not all automatically placed as objects; they are usable art slots ready for appropriate existing game hooks. There is no autonomous, paid, on-the-fly AI generation inside a Windows game build.

## Full artwork plan and review gallery

The offline planning board covers **55 named guardians, 234 Americas animals, and 61 additional UI, item, scenery, region and dungeon art slots (350 planned images)**. Generating the plan or gallery never contacts the image API, changes gameplay assets or charges anything.

From the godot-v38 directory:

    python tools/ai_art_review.py
    python tools/ai_art_review.py --category guardians --gallery
    python tools/ai_art_review.py --category all --gallery
    python tools/ai_art_review.py --audit

Open `ai_art_staging/review/index.html` in a browser to see image previews, counts of missing/staged/approved artworks, validation issues and individual generation prompts. The offline gallery now has keyboard-accessible species search, category and review-status filters. The machine-readable companion `ai_art_staging/review/art_plan.json` contains all planned image prompts and statuses. Both live in the gitignored staging folder. This is a **review page**, not a paid batch-generation button. Generating assets still requires a separate explicit one-image command, and approving art still requires visual inspection and a separate command.

The studio validates each approved PNG's real header, bounded RGBA dimensions, per-chunk CRC, decompressed image length, manifest checksum and asset ID. Invalid/truncated/opaque-RGB payloads do not enter approved art. None of these checks guarantees aesthetic quality: inspect anatomy, silhouette, style and transparency manually.

## Offline tests

    python -m unittest discover -s tests -p "test_ai_art_*.py" -v

Tests check 55 distinct guardian art directions, habitat-driven animal prompts, local file imports, artwork-preservation guards, real RGBA PNG integrity, image staging, no-overwrite, an accessible offline gallery, and that preview never makes a paid API request.

Generation uses the developer's own account and API billing. Review image rights and suitability before committing approved results to a release.
