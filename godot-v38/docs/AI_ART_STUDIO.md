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

## Review and approve

Inspect the staged image for recognizable silhouette, quality, background transparency and consistent Moonfall style. Then explicitly approve an asset:

    python tools/ai_art_studio.py --guardian 0 --approve
    python tools/ai_art_studio.py --animal "Jaguar" --approve

The script verifies a SHA-256 record, refuses overwrites and puts approved art at assets/ai_generated/guardians/guardian_00.png or assets/ai_generated/animals/animal_jaguar.png. It also stores a review/provenance JSON record next to the approved image. It can write ONLY to these game-art locations, never to assets/heroes, so original Z and Nyra artwork is untouched.

## Artwork in the game

The existing Godot wildlife instance checks for an approved image with the matching species name and displays it on a noncolliding Sprite3D, layered over the existing animal rig. If no approved image exists, the normal animal model is unchanged. Guardian art loads only if the later 55-profile system supplies the exact regional_guardian_profile ID, so old game versions remain compatible. Neither loading feature creates AI enemies or modifies attack collisions.

These are 2D transparent creature cutouts in the isometric 3D world, NOT automatically generated 3D skeletal models or animations. Game rendering does not contact the image API.

## Full artwork plan and review gallery

The offline planning board covers **55 named guardians and 234 distinct Americas animals (289 planned images)**. Generating the plan or gallery never contacts the image API, changes gameplay assets or charges anything.

From the godot-v38 directory:

    python tools/ai_art_review.py
    python tools/ai_art_review.py --category guardians --gallery
    python tools/ai_art_review.py --category all --gallery
    python tools/ai_art_review.py --audit

Open `ai_art_staging/review/index.html` in a browser to see image previews, counts of missing/staged/approved artworks, validation issues and individual generation prompts. The machine-readable companion `ai_art_staging/review/art_plan.json` contains all planned image prompts and statuses. Both live in the gitignored staging folder. This is a **review page**, not a paid batch-generation button. Generating assets still requires a separate explicit one-image command, and approving art still requires visual inspection and a separate command.

The studio validates each approved PNG's real header, bounded RGBA dimensions, per-chunk CRC, decompressed image length, manifest checksum and asset ID. Invalid/truncated/opaque-RGB payloads do not enter approved art. None of these checks guarantees aesthetic quality: inspect anatomy, silhouette, style and transparency manually.

## Offline tests

    python -m unittest discover -s tests -p "test_ai_art_*.py" -v

Tests check 55 unique guardian prompts, the wildlife catalog, image staging, no-overwrite, checksums, and that preview is offline and paid generation requires a key.

Generation uses the developer's own account and API billing. Review image rights and suitability before committing approved results to a release.
