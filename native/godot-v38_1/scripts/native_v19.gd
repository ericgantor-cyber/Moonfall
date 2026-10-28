extends "res://scripts/native_v18_stable.gd"
## v19: low-overhead, biome-specific environmental set dressing.
## Decorative-only geometry, no navigation blocking, no changes to hero/bestiary textures.
const V19_SCENERY_PER_REGION := 9
var v19_scenery_roots: Array[Node3D] = []
var v19_scenery_count := 0
var v19_ambient_clock := 0.0
var v19_motion_updates := 0
var v19_crystals: Array[Dictionary] = []

func _ready() -> void:
 super._ready()
 _show_message("Moonfall v19: six-region environmental art pass",4.0)

func _build_region(zone:int) -> void:
 super._build_region(zone)
 # Reuse the v14 decorative container: it already obeys graphics settings and region culling.
 var root:Node3D=Node3D.new()
 root.name="V19_BiomeScenery_"+str(zone)
 decorative_groups[-1].add_child(root)
 v19_scenery_roots.append(root)
 var center:Vector2=REGIONS[zone]["at"]
 var accent:=Color(str(REGIONS[zone]["light"]))
 var dark:=Color(str(REGIONS[zone]["ground"])).lightened(0.16)
 for index in range(V19_SCENERY_PER_REGION):
  var angle:float=TAU*float(index)/float(V19_SCENERY_PER_REGION)
  var radius:float=13.0+float(index%3)*3.8
  var spot:=Vector3(center.x+cos(angle)*radius,0.17,center.y+sin(angle)*radius)
  var cluster:=Node3D.new()
  cluster.name="V19_SetPiece_"+str(index)
  cluster.position=spot
  root.add_child(cluster)
  match zone:
   0: # Outpost: sacks, lantern posts and stacked timber.
    _form(cluster,"box",Vector3(0,0.38,0),Vector3(1.3,0.7,0.75),Color("6d5843"))
    _form(cluster,"cylinder",Vector3(-0.42,1.05,0),Vector3(0.12,1.2,0.12),Color("8b7962"))
    _form(cluster,"sphere",Vector3(-0.42,1.7,0),Vector3(0.30,0.34,0.30),accent,true)
   1: # Witchwood: tall fungi and curled root silhouettes.
    _form(cluster,"cylinder",Vector3(0,0.61,0),Vector3(0.25,1.2,0.25),Color("6b6780"))
    _form(cluster,"cone",Vector3(0,1.29,0),Vector3(1.15,0.65,1.15),Color("775b9c"))
    _form(cluster,"sphere",Vector3(0.33,0.28,0.5),Vector3(0.3,0.45,0.30),accent,true)
   2: # Marsh: reeds, blue floating embers and pooled stones.
    for reed in range(3):
     _form(cluster,"cone",Vector3(float(reed-1)*0.37,0.75,0),Vector3(0.18,1.5+0.3*float(reed%2),0.18),Color("50796c"))
    _form(cluster,"sphere",Vector3(0,1.85,0),Vector3(0.19,0.19,0.19),accent,true)
   3: # Quarry: vein crystals on fractured rock.
    _form(cluster,"box",Vector3(0,0.30,0),Vector3(1.4,0.6,1.1),dark)
    var crystal:=_form(cluster,"cone",Vector3(0.1,1.3,0),Vector3(0.67,1.6,0.67),accent,true,Vector3(13,0,14))
    v19_crystals.append({"node":crystal,"origin":crystal.scale,"phase":angle,"zone":zone})
   4: # Ruins: fallen pillars, rune-crowned fragments.
    _form(cluster,"cylinder",Vector3(0,0.35,0),Vector3(1.2,0.65,1.2),Color("7b718b"),false,Vector3(0,23,78))
    _form(cluster,"ring",Vector3(0,0.23,0),Vector3(0.93,0.08,0.93),accent,true)
    _form(cluster,"box",Vector3(0.8,0.42,0.18),Vector3(0.4,0.65,0.55),dark)
   5: # Citadel: obsidian braziers and crimson sparks.
    _form(cluster,"cone",Vector3(0,0.64,0),Vector3(1.0,1.35,1.0),Color("2b2334"))
    _form(cluster,"ring",Vector3(0,1.3,0),Vector3(0.62,0.11,0.62),accent,true)
    _form(cluster,"sphere",Vector3(0,1.64,0),Vector3(0.32,0.47,0.32),accent,true)
  v19_scenery_count+=1

func _process(delta:float) -> void:
 super._process(delta)
 # Cosmetic movement at 8 Hz and only inside the visible active region.
 v19_ambient_clock+=minf(maxf(delta,0.0),0.2)
 if v19_ambient_clock<0.125:return
 v19_ambient_clock=fmod(v19_ambient_clock,0.125)
 if graphics_quality=="Smooth" or not hero_playing:return
 for crystal in v19_crystals:
  if int(crystal["zone"])!=region_index:continue
  var node:MeshInstance3D=crystal["node"]
  if not is_instance_valid(node) or not node.is_visible_in_tree():continue
  var amount:=1.0+0.04*sin(map_time*2.0+float(crystal["phase"]))
  node.scale=crystal["origin"]*amount
  v19_motion_updates+=1
