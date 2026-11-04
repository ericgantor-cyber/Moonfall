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

## One-click reviewed-art approval for the entire game

The local HTML art gallery now lets you choose which **staged PNG and 3D GLB**
files are good enough to put in Moonfall. You can review all 350 catalog slots
and check only the files you've actually inspected. Approved files are never
overwritten, and no generated asset can replace the original Z or Nyra art.

1. Run `tools/Import-Moonfall-Art.cmd` on Windows. It stages recognized art
   and opens the local review gallery.
2. Inspect each staged PNG and open any GLB in your preferred 3D viewer.
   Check **Approve reviewed PNG** or **Approve reviewed 3D model** on individual
   cards only when their appearance and gameplay suitability are acceptable.
3. Choose **Export checked artwork for approval**. The browser downloads
   `moonfall_art_approval.json`, containing only stable asset IDs, type and
   SHA-256 content hashes. It does not add anything to the game on its own.
4. Drag that downloaded JSON onto `tools/Approve-Reviewed-Moonfall-Art.cmd`,
   or move it to `ai_art_staging/review/moonfall_art_approval.json` and
   double-click the same Windows script. The script first checks *every*
   selected asset, then requests an explicit Y/N confirmation.
5. The approved files are placed in `assets/ai_generated`; open the Godot
   editor or run the normal Windows build to import the newly approved assets.
   Previously released installers don't acquire new art without a rebuild.

The equivalent commands, useful for automation and accessibility tooling:

    python tools/ai_art_approve_selected.py --manifest "C:\\Users\\You\\Downloads\\moonfall_art_approval.json"
    python tools/ai_art_approve_selected.py --manifest "C:\\Users\\You\\Downloads\\moonfall_art_approval.json" --apply

The first command is a read-only preview. The second is an explicit approval.
Both are **fully offline** with no generation credits, network requests or
provider account. The validator confirms the catalog IDs, image or model
structure, provenance JSON, SHA-256 digest, review state, and absence of
already-approved files. It rejects unknown IDs, unsafe 3D categories, duplicate
entries, altered artwork, corrupted metadata and invalid files **before**
approving any file. If a disk write later fails, previously approved files
may remain, but the preflight prevents known invalid selections from producing
a partial batch.

Never approve artwork you haven't visually checked. Bulk approval is a tool
for speeding up a deliberate review, not an automatic aesthetic judgment.

## Creative limits removed: large rigs, animation libraries and art batches

Moonfall's **offline** import and review workflows no longer impose hard
caps of 128 bones per skin, 24 animation clips, 160 channels per clip,
6,000 keyframes per channel, or 45 seconds per clip. Rig data still has to
contain real skin joints, valid mesh bindings, correctly referenced animation
tracks, and plausible file-backed keyframe counts. The asset validator also
no longer imposes the old arbitrary 100,000-polygon, 120-mesh, 800-node
or 3,500-accessor caps on valid GLB geometry. Actual byte sizes, data
references, scene complexity and available hardware still matter.

The art inbox no longer refuses a batch after 900 entries or 512 MiB
aggregate data. Artwork is checked and staged **one file at a time**.
The selection-approval command no longer stops at 700 files. Its batch
content is restricted to actual registered core art slots and valid, reviewed
formats, which avoids approving unknown assets or original hero art.

You can now copy an already-rigged and animated creature model to:

    ai_art_inbox/rigged/guardian_00.glb
    ai_art_inbox/rigged/animal_jaguar.glb

Then run `tools/Import-Moonfall-Art.cmd`, inspect the rigging and actual
animation clips in the gallery, and approve selected rigged files through
`tools/Approve-Reviewed-Moonfall-Art.cmd`.

**What remains deliberately protected:** each individual file must still
pass GLB/PNG container and reference integrity checks, no arbitrary
external paths or unreviewed art can be imported, and the Windows game
automatically reduces detail when actual frame time deteriorates. The
350-slot roster describes current *integrated game asset locations*, not
a limit on animation clips, bone count or how many separate source files
you can review. Additional distinct gameplay asset locations still need
registration and Godot integration. 4K texture-service resolutions,
Meshy's per-request maximum 10 animation action IDs, and provider prices
are external service limits; this code cannot waive them. **Nothing
automatically triggers paid generation.**

## Rigged and animated AI artwork (new)

Approved 3D creature art now supports **real glTF skeletons** (skinned meshes
with bone joints) and animation clips imported into Godot as **Skeleton3D**
and **AnimationPlayer**. This is a visual layer over existing native
CharacterBody3D controls, AI, attack timing, hit detection, loot and health.
No new modes, AI actors or collision shapes are added. Original Z and Nyra
artwork remains completely unchanged.

### Rigged GLB import: no service or credits needed

For wolves, quadrupeds, birds, hydras, serpents and other non-humanoids,
prepare a compatible rig and animation in Blender or another animation tool
and export a **single skinned GLB** containing named clips. Standard Moonfall
clip names are `idle`, `move`, `attack`, `hurt`, `death`; optional
`spawn`, `telegraph`, `special_1`, `special_2`, `victory`.
Common `Walk`, `Run`, `Bite`, `Slash`, `Hit`, `Die`, and namespaced
Blender-style names are mapped to their closest state automatically.

From `godot-v38`:

    python tools/ai_art_autorig.py --guardian 0 --import-rig "C:\\MyModels\\rigged_boss.glb"
    python tools/ai_art_autorig.py --guardian 0 --approve
    python tools/ai_art_autorig.py --animal "Jaguar" --import-rig "C:\\MyModels\\rigged_jaguar.glb"
    python tools/ai_art_autorig.py --animal "Jaguar" --approve

A new rig is staged until reviewed. Its GLB is validated for actual bone
joints, skin-to-mesh binding, animation sampler indices, node references,
clip durations, keyframe budgets and names. **A static mesh is not claimed
to be animated.** Approval is explicit, separately stored under
`assets/ai_generated/rigged/<category>/<asset_id>.glb`, and does not
overwrite your existing approved 3D model or 2D sprite. Godot prefers the
new reviewed animated rig when it exists. If it does not, static approved
models and old native visuals remain available as fallbacks.

Alternatively, you can check a staged or approved normal GLB with:

    python tools/ai_art_3d.py --guardian 0 --audit
    python tools/ai_art_3d.py --guardian 0 --require-rig

The audit reports real skeleton bone counts, skinned meshes, animation
tracks, canonical action names, missing preferred clips, triangles and
embedded 4K material maps. A static non-animated GLB remains a valid
ordinary item if `--require-rig` is not set.

The offline review gallery includes rig metrics, imported clip names,
missing animations and a dedicated **Approve reviewed animated rig**
checkbox for valid combined clips. Export the approved selection and use
the existing `Approve-Reviewed-Moonfall-Art.cmd` Windows flow.
The read-only release auditor includes all approved rig overlays and
supports optional `--require-animated-creatures` to demand the complete
five-clip contract during specialized testing. The default strict release
audit allows old static art, so current Windows releases still build.

### Paid AI humanoid rigging and preset animation (optional)

Meshy's current automatic rigging endpoint works reliably only on
**clearly humanoid bipeds**. It does **not** promise to rig Moonfall's
wolves, dragons, hydras, jaguars, serpents or other four-legged creatures.
Use the free Blender rigged-GLB import above for non-humanoids.

If a model is an actual humanoid, first approve its textured, unrigged GLB
using the existing Moonfall art tools. With your own **MESHY_API_KEY**
environment variable set locally (never committed), intentionally submit
**one potentially paid job**:

    python tools/ai_art_autorig.py --guardian 0 --submit --confirm-humanoid
    python tools/ai_art_autorig.py --guardian 0 --status
    python tools/ai_art_autorig.py --guardian 0 --fetch --variant walking
    python tools/ai_art_autorig.py --guardian 0 --approve --variant walking

The example `guardian 0` is just a slot. **Do not submit an animal-shaped
guardian to Meshy**: humanoid confirmation is required, and the service
may fail or consume credits on non-humanoid inputs. The tool sends an
approved self-contained GLB as an in-memory data URI, checks source
provenance, refuses duplicate tasks, and keeps the response staged for
visual review. It never automatically pays, polls or approves.

To add several *actual* AI motion clips to a completed Meshy humanoid rig,
look up official action IDs and choose your own suitable moves:

    python tools/ai_art_autorig.py --guardian 0 --list-actions
    python tools/ai_art_autorig.py --guardian 0 --animate --action-ids "17,92"
    python tools/ai_art_autorig.py --guardian 0 --animation-status
    python tools/ai_art_autorig.py --guardian 0 --animation-fetch
    python tools/ai_art_autorig.py --guardian 0 --approve

The numeric IDs above demonstrate command syntax only. **Verify selected
IDs from the current Animation Library**; their meanings may change.
A task requests a merged GLB with the chosen motion clips. Only a valid
downloaded rig with recognizable action names will pass Moonfall review.
These are separate explicit paid requests, not a background batch.

Reference documents:
- https://docs.meshy.ai/en/api/rigging
- https://docs.meshy.ai/en/api/animation
- https://docs.godotengine.org/en/stable/tutorials/animation/animation_tree.html

### In-game animation and performance

Only the **nearby, already-loaded** 3D wildlife models and approved guardian
rigs update their animations. Every 80 ms, the visual wrapper reads the
native actor's velocity, existing melee-attack swing timing, hit-reaction
timing, boss windup and defeat state and selects a matching imported clip.
Existing AI and damage logic remain authoritative. Idle/movement clips can
repeat while one-shot hurt/attack/death clips are not restarted each tick.

If the imported GLB is missing a skeleton, AnimationPlayer or usable clips,
the game retains the original visuals and gameplay. Unlike a properly
authored game-specific death animation, the current native despawn hides
defeated actors immediately, so an imported death clip may not be visible
without an additional dedicated death-presentation upgrade. Automatic AI
rigging cannot create missing attack, hurt or death animations on its own.
Run the scene on Windows to check clipping, orientation, animation transitions,
GPU memory, and camera scale before shipping large animated model packs.

## Adaptive 3D art quality and verified release builds

Moonfall now treats approved 4K PBR art as an **optional visual upgrade**,
not an excuse to overload Windows GPUs or change the actual game.

The game starts at High detail, now allowing **all eligible nearby approved
wildlife and selected dungeon monument meshes** instead of rigid counts.
A lightweight frame-time governor
automatically steps down quality after **3 sustained seconds** of slow frames
(around 27 ms average) and only steps up after **16 sustained seconds** of
comfortable frames (around 17.5 ms average). It uses three purely visual
detail tiers:

- **Eco:** Use approved PNGs and original native creatures.
- **Balanced:** Display half of the eligible nearby 3D creature and monument models.
- **High:** Display **all** eligible nearby approved 3D creature and monument models.
No hard three-creature or one-room quota applies. New models are still admitted
gradually rather than all spawning in one frame.

A creature's high-detail model remains in place slightly beyond its initial
loading distance to avoid visual popping. New high-detail GLBs are admitted
at most once every 900 ms, avoiding a burst of GPU memory allocations when
entering a busy area. The existing 12-model scene-reference cache remains
bounded. These settings never alter creature AI, combat difficulty, dungeon
geometry or the original Z and Nyra artwork.

These are *conservative automatic limits*, not a measured hardware benchmark:
frame time includes CPU work, and texture memory use depends on the graphics
driver, Godot import options and the model itself. True fps, render latency,
VRAM usage and smooth transitions still need on-device Windows gameplay tests.

### Release audit: don't ship damaged or unexpectedly heavy artwork

From `godot-v38`:

    python tools/ai_art_release_audit.py
    python tools/ai_art_release_audit.py --strict
    python tools/ai_art_release_audit.py --strict --report "ai_art_staging/review/release_audit.json"

The audit only reads approved assets. It checks PNG/GLB structural validity,
the approved review record, SHA-256 integrity, model triangle counts,
embedded texture dimensions, and detected PBR materials. It shows estimated
uncompressed RGBA8 texture memory **including mipmaps** and warns about
large meshes, excessive total source size, duplicate artwork, missing
4K maps and absent PBR material maps. These estimates are not actual
GPU-resident VRAM, and PNG header size alone is not proof of image quality.

**Strict mode fails on damaged, altered or unreviewed approved artwork**.
Missing artwork is allowed because the game deliberately retains its built-in
visuals. Quality warnings remain warnings, not errors, so original assets,
low-memory devices, and intentionally lower-resolution artwork are supported.
No image-generation provider is contacted and no API keys are required.

The dedicated Moonfall AI Art Studio GitHub workflow now runs this audit
whenever art code or approved assets change, in addition to the offline
tests. This helps prevent corrupt approved files from entering a release.

## Automatic artwork intake and actual 4K verification

For Windows users, double-click `tools/Import-Moonfall-Art.cmd`. This creates
`ai_art_inbox` folders for guardians, animals, regions, chambers, glyphs,
spells, gear and scenery. Put existing approved-to-review images and models in
those folders using the stable names from `--list-game-art`, the 55 guardian
IDs, or the Americas wildlife IDs.

For example:

    ai_art_inbox/guardians/guardian_00.glb
    ai_art_inbox/animals/animal_jaguar.png
    ai_art_inbox/chambers/chamber_14.glb
    ai_art_inbox/glyphs/glyph_orb_hud.png

4K reference art is supported: reviewed PNG files can be up to
**4096×4096 pixels** and 32 MiB, with real RGBA transparency. Meshy's
separate image-to-3D upload limit is **20 MiB**, so larger 4K PNGs may need
to be recompressed for an AI conversion request. Importing an existing
GLB offline doesn't use that upload limit.

The Windows launcher stages matching files and opens a local searchable review
gallery. To run the same workflow directly:

    python tools/ai_art_intake.py --inbox ai_art_inbox
    python tools/ai_art_intake.py --inbox ai_art_inbox --stage
    python tools/ai_art_review.py --gallery

Only valid, exact names from the **350 registered slots** are accepted.
No symlinks, unexpected file types, oversized folders or hero artwork names
are accepted. It never contacts a paid API, never overwrites existing approved
art and **never silently approves new art**. Check the staged images and models
before approving a specific PNG or GLB using the appropriate studio command.
Godot imports the resulting reviewed asset on the next editor import/game
build. Existing compiled Windows installers don't gain new art until rebuilt.

If a provider claims a model has 4K textures, verify the **actual GLB**:

    python tools/ai_art_3d.py --guardian 0 --audit
    python tools/ai_art_3d.py --animal "Jaguar" --audit

This offline audit reports triangle count, mesh count, embedded texture image
dimensions, maximum texture size, and whether the GLB references 4096-by-4096
or larger texture maps with material PBR assignments. It does *not* infer 4K
quality merely from the generation request. As with any format metadata audit,
it checks declared embedded image dimensions and material links, not whether
textures are aesthetically sharp or correctly rigged.

### Keep high-resolution assets playable

The native 3D display uses performance-adaptive selection instead of the
old fixed three-wildlife / one-chamber quotas. At High detail all eligible
nearby models may be rendered, while faraway actors use original 2D/native
visuals. The resource cache retains only twelve recent scene references to
avoid unnecessary memory growth; this is a cache policy, not a creative limit.
Combat collisions, enemies, room geometry and Z/Nyra art are unchanged. High-quality 4K PBR material maps remain optional and can use
substantial GPU memory: monitor actual game performance and exported builds.

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

The review gallery now displays **both PNG and GLB** states. Staged
GLBs show actual triangle counts, maximum embedded texture dimensions
and measured PBR material links even when no 2D PNG exists. Its audit
mode also rejects invalid GLBs, so imported models cannot hide behind a
valid 2D image.

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
