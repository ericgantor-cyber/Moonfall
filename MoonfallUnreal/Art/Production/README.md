# Moonfall UE5 production art

This is the first original, standalone UV-mapped mesh pack, not a claim of AAA fidelity. The six OBJ source assets have explicit triangle budgets and material slots; the geometry is authored by procedural profile construction, not a hand-sculpted high-poly character. No existing content, Z, or Nyra artwork is replaced.

## Import
Open **MoonfallUnreal/MoonfallUnreal.uproject** in Unreal Engine 5.4 with editor scripting enabled, then run **Content/Python/import_production_art.py** via Execute Python Script. It only imports the six allowlisted meshes into /Game/MoonfallProduction/Meshes. Existing assets are skipped. No gameplay collision is generated. The script checks returned assets are StaticMesh objects. Verify axis orientation, scale, normals, UV seams, material slots, appearance and collision separately in the editor. The UVs are per-shell and need proper unique layout before baked texture production. The MTL contains diffuse/specular placeholders, not PBR texture maps. Commit reviewed Unreal packages through the native asset workflow. The native ground-loot display now optionally discovers the sword, cuirass and amulet packages during construction, uses their valid mesh bounds to fit the existing display footprint, and falls back to primitive artwork when packages are absent or invalid. Restart the editor after importing to rebuild native class defaults; confirm the three packages are cooked by adding /Game/MoonfallProduction/Meshes to the editor packaging settings if needed. Check the cooked Windows build, material assignments, orientation, rarity cues and pickup behavior. The Lunar Reliquary optionally replaces shrine base/beacon visuals without altering checkpoint state or discovery. The Gothic Column optionally replaces tall decorative sanctuary pillars in a separate noncolliding instance batch, preserving their original footprints and cube fallbacks. The Firstlight Shield optionally appears on eight sanctuary pillars as display-only heraldry. Inspect the imported shrine and column orientation, materials and bounds in the native editor before shipping. No imported packages or native editor verification are included in this branch.

## Material installation
After importing the six meshes, execute **Content/Python/install_production_materials.py** in UE5. It validates source material slots, creates five physically based constant materials (bronze, leather, steel, stone, moonstone) in /Game/MoonfallProduction/Materials, and fills only default or empty material slots. Existing authored materials and packages are not overwritten. These are base-color, metallic and roughness values, not detailed PBR texture maps. Native material compilation, slot mapping and visual inspection are still required.

## Texture source generation
Run `python MoonfallUnreal/Art/Production/generate_texture_maps.py` from the repository root. It generates 15 deterministic 1024x1024 PNG maps in Art/Production/Textures: five surfaces, each with sRGB base color, tangent-space normal, and linear packed ambient occlusion/roughness/metallic (R/G/B). Existing differing files are never overwritten. These are original procedural tileable textures, not scanned or hand-painted assets. Use a separate output folder for alternative resolutions. Run **Content/Python/import_production_textures.py** in UE5 to import the files into /Game/MoonfallProduction/Textures. Existing texture packages are preserved. Material graph hookup, visual review and native compilation are still pending. CI generates and decodes 64-pixel test maps; the full 1024 source files are generated locally, not committed binary assets.

## Production gates
| Area | Required next deliverable | Current status |
|---|---|---|
| Environment | Modular ruins, terrain, foliage, rock and settlement kits; LOD/Nanite review | Column source mesh only |
| Gear | Original weapon, armor and accessory meshes; PBR maps | Sword, shield, cuirass and amulet source meshes; cuirass is a solid display prop, not wearable or rigged armor |
| Shrines | Authored modular reliquary and emissive material | Reliquary source mesh only |
| Enemies and bosses | Unique sculpt, retopology, rig, skin weights, animation sets | Not delivered |
| Combat | Retargeted directional strikes, blocking, dodging, reactions | Not delivered |
| Spells | Authored Niagara systems and scalable budgets | Not delivered |
| Materials | Base color, normal, roughness, metallic, AO; material instance library | Constant PBR material installer added; texture maps and native validation pending |
| Lighting | Lumen/other lighting profile, scalable non-Lumen fallback and GPU timing | Not delivered |
| UI | Production icon/texture atlas, readability, controller focus | Earlier Canvas art only |

Use engine/editor validation, asset provenance and license review, gameplay integration, visual accessibility review, and Windows performance measurements before labeling any category production-ready. Preserve original hero art and existing save/input contracts. Keep rendering changes opt-in until tested.
