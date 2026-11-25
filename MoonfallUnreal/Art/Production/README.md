# Moonfall UE5 production art

This is the first original, standalone UV-mapped mesh pack, not a claim of AAA fidelity. The six OBJ source assets have explicit triangle budgets and material slots; the geometry is authored by procedural profile construction, not a hand-sculpted high-poly character. No existing content, Z, or Nyra artwork is replaced.

## Import
Open **MoonfallUnreal/MoonfallUnreal.uproject** in Unreal Engine 5.4 with editor scripting enabled, then run **Content/Python/import_production_art.py** via Execute Python Script. It only imports the six allowlisted meshes into /Game/MoonfallProduction/Meshes. Existing assets are skipped. No gameplay collision is generated. The script checks returned assets are StaticMesh objects. Verify axis orientation, scale, normals, UV seams, material slots, appearance and collision separately in the editor. The UVs are per-shell and need proper unique layout before baked texture production. The MTL contains diffuse/specular placeholders, not PBR texture maps. Commit reviewed Unreal packages through the native asset workflow; until that happens the meshes are not automatically used by gameplay or included in a packaged Windows build.

## Production gates
| Area | Required next deliverable | Current status |
|---|---|---|
| Environment | Modular ruins, terrain, foliage, rock and settlement kits; LOD/Nanite review | Column source mesh only |
| Gear | Original weapon, armor and accessory meshes; PBR maps | Sword, shield, cuirass and amulet source meshes; cuirass is a solid display prop, not wearable or rigged armor |
| Shrines | Authored modular reliquary and emissive material | Reliquary source mesh only |
| Enemies and bosses | Unique sculpt, retopology, rig, skin weights, animation sets | Not delivered |
| Combat | Retargeted directional strikes, blocking, dodging, reactions | Not delivered |
| Spells | Authored Niagara systems and scalable budgets | Not delivered |
| Materials | Base color, normal, roughness, metallic, AO; material instance library | Not delivered |
| Lighting | Lumen/other lighting profile, scalable non-Lumen fallback and GPU timing | Not delivered |
| UI | Production icon/texture atlas, readability, controller focus | Earlier Canvas art only |

Use engine/editor validation, asset provenance and license review, gameplay integration, visual accessibility review, and Windows performance measurements before labeling any category production-ready. Preserve original hero art and existing save/input contracts. Keep rendering changes opt-in until tested.
