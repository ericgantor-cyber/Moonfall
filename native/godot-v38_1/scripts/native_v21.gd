extends "res://scripts/native_v20.gd"
## v21 - biome art extravaganza; pure decoration; keeps hero and beast source textures intact.
## 6 regional monuments + 72 sculptural setpieces + 1,440 batched microprops.
const V21_PIECES_PER_REGION := 12
const V21_SCATTER_PER_LAYER := 80
const V21_SCATTER_LAYERS := 3
var v21_region_roots: Array[Node3D] = []
var v21_landmarks: Array[Node3D] = []
var v21_crests: Array[Sprite3D] = []
var v21_setpiece_count := 0
var v21_scatter_count := 0
var v21_scatter_batches := 0

func _ready() -> void:
 super._ready()
 _show_message("Moonfall v21: Grand Biome Art expansion", 4.0)

func _build_region(zone:int) -> void:
 super._build_region(zone)
 var region: Node3D = decorative_groups[zone]
 var root := Node3D.new()
 root.name = "V21_GrandScenery_%d" % zone
 region.add_child(root)
 v21_region_roots.append(root)
 var center: Vector2 = REGIONS[zone]["at"]
 var accent := Color(str(REGIONS[zone]["light"]))
 var earthy := Color(str(REGIONS[zone]["ground"]))
 # Landmark is always visible in Full graphics, hidden alongside all v14+ detail in Smooth.
 var monument := Node3D.new()
 monument.name = "V21_RegionMonument_%d" % zone
 monument.position = Vector3(center.x + 16.0,0.1,center.y - 17.0)
 root.add_child(monument)
 v21_landmarks.append(monument)
 _make_monument(monument,zone,accent,earthy)
 # One unique PNG illustration physically visible on each region monument.
 var crest := Sprite3D.new()
 crest.name = "V21_RegionCrest_%d" % zone
 crest.texture = load("res://assets/v21/crest_%d.png" % zone)
 crest.pixel_size = 0.0105
 crest.billboard = BaseMaterial3D.BILLBOARD_ENABLED
 crest.position = Vector3(0,3.35,0)
 crest.shaded = true
 monument.add_child(crest)
 v21_crests.append(crest)
 # Twelve sculptural dioramas per region, deterministic and collision-free.
 for n in range(V21_PIECES_PER_REGION):
  var phase:float = TAU * float(n) / float(V21_PIECES_PER_REGION)
  var radius:float = 18.0 + float(n % 4) * 2.25
  var setpiece := Node3D.new()
  setpiece.name = "V21_Diorama_%02d" % n
  setpiece.position = Vector3(center.x + cos(phase)*radius,0.04,center.y + sin(phase)*radius)
  setpiece.rotation.y = phase
  root.add_child(setpiece)
  _make_diorama(setpiece,zone,n,accent,earthy)
  v21_setpiece_count += 1
 # Scatter hundreds of tiny props efficiently in three draw batches per region.
 _make_scatter(root,zone,center,accent,earthy)

func _make_monument(root:Node3D,zone:int,accent:Color,earthy:Color) -> void:
 _form(root,"cylinder",Vector3(0,0.24,0),Vector3(5.0,0.45,5.0),earthy.lightened(0.18))
 _form(root,"ring",Vector3(0,0.48,0),Vector3(3.1,0.12,3.1),accent,true)
 match zone:
  0: # Hearthstone's watchtower, bell and sentry lanterns.
   for dx in [-1.25,1.25]:
    for dz in [-1.25,1.25]:_form(root,"cylinder",Vector3(dx,2.1,dz),Vector3(0.44,4.2,0.44),Color("7c5740"))
   _form(root,"box",Vector3(0,4.35,0),Vector3(3.9,0.4,3.9),Color("a88053"))
   _form(root,"cone",Vector3(0,5.25,0),Vector3(4.3,1.75,4.3),Color("663c36"))
   _form(root,"sphere",Vector3(0,4.0,0),Vector3(0.6,0.75,0.6),accent,true)
  1: # Witchwood's spirit tree with glowing eyes and reaching boughs.
   _form(root,"cylinder",Vector3(0,2.65,0),Vector3(1.7,5.3,1.7),Color("463044"))
   for j in range(6):
    var ang:float = TAU*float(j)/6.0
    _form(root,"cone",Vector3(cos(ang)*1.55,4.95,sin(ang)*1.55),Vector3(2.5,3.8,2.5),Color("284d49"),false,Vector3(0,j*60.0,17))
    _form(root,"sphere",Vector3(cos(ang)*2.9,5.45,sin(ang)*2.9),Vector3(0.21,0.3,0.21),accent,true)
  2: # Moonmist's moon-gate over stacked water bowls.
   for dx in [-1.55,1.55]:_form(root,"cylinder",Vector3(dx,2.4,0),Vector3(0.65,4.8,0.65),Color("536f75"))
   _form(root,"box",Vector3(0,4.9,0),Vector3(4.25,0.5,1.1),Color("607989"))
   _form(root,"ring",Vector3(0,3.2,0),Vector3(1.9,0.3,1.9),accent,true,Vector3(90,0,0))
   _form(root,"sphere",Vector3(0,1.1,0),Vector3(0.55,0.36,0.55),Color("b5eff6"),true)
  3: # Highcrest's gigantic crystalline drill-spire.
   _form(root,"cone",Vector3(0,2.8,0),Vector3(2.8,5.5,2.8),Color("665b55"))
   for j in range(5):
    var ang:float=TAU*float(j)/5.0
    _form(root,"cone",Vector3(cos(ang)*1.9,2.4,sin(ang)*1.9),Vector3(0.72,4.2,0.72),accent,true,Vector3(16,0,8))
   _form(root,"ring",Vector3(0,5.5,0),Vector3(1.8,0.18,1.8),accent,true)
  4: # Shattered arch surrounding an astral core.
   for dx in [-2.0,2.0]:
    _form(root,"cylinder",Vector3(dx,2.3,0),Vector3(0.8,4.6,0.8),Color("655570"))
    _form(root,"cone",Vector3(dx,5.05,0),Vector3(1.2,1.1,1.2),Color("86718f"))
   _form(root,"box",Vector3(0,4.8,0),Vector3(5.0,0.65,1.2),Color("756483"))
   _form(root,"sphere",Vector3(0,2.8,0),Vector3(0.95,0.95,0.95),accent,true)
  5: # The Eclipse's dark throne and bifurcated crystal horns.
   _form(root,"box",Vector3(0,1.2,0),Vector3(3.6,2.4,2.4),Color("3b2335"))
   _form(root,"box",Vector3(0,2.55,0.85),Vector3(3.7,3.5,0.45),Color("582c48"))
   for dx in [-1.8,1.8]:
    _form(root,"cone",Vector3(dx,4.6,0.85),Vector3(0.8,4.8,0.8),Color("8e4267"))
    _form(root,"sphere",Vector3(dx,6.8,0.85),Vector3(0.24,0.24,0.24),accent,true)
   _form(root,"ring",Vector3(0,3.5,-0.25),Vector3(2.8,0.12,2.8),accent,true,Vector3(75,0,0))

func _make_diorama(root:Node3D,zone:int,index:int,accent:Color,earthy:Color) -> void:
 var variation:float = float(index % 4)*0.18
 match zone:
  0: # Campfires, provision stacks, and old palisades.
   _form(root,"box",Vector3(0,0.35,0),Vector3(1.7,0.7,1.3),Color("6a5040"))
   _form(root,"cone",Vector3(0,1.2,0),Vector3(2.3,1.5,2.3),Color("9d6650"))
   _form(root,"sphere",Vector3(0.75,1.65,0.15),Vector3(0.18,0.25,0.18),accent,true)
  1: # Soul mushrooms, roots and ghost bulbs.
   for side in [-0.55,0.6]:
    _form(root,"cylinder",Vector3(side,0.75,0),Vector3(0.24,1.5,0.24),Color("6a5970"))
    _form(root,"cone",Vector3(side,1.7,0),Vector3(1.25+variation,0.7,1.25+variation),Color("84548c"))
   _form(root,"sphere",Vector3(0,0.65,0.9),Vector3(0.34,0.48,0.34),accent,true)
  2: # Reed pools, waterlilies and lantern pods.
   _form(root,"cylinder",Vector3(0,0.05,0),Vector3(2.0,0.11,2.0),Color("245c6a"))
   for n in range(4):
    var a:float=TAU*float(n)/4.0
    _form(root,"cone",Vector3(cos(a)*0.7,0.6,sin(a)*0.7),Vector3(0.2,1.2,0.2),Color("568878"))
   _form(root,"sphere",Vector3(0,0.39,0),Vector3(0.43,0.29,0.43),accent,true)
  3: # Mined stone, pick shafts, angled ore shards.
   _form(root,"box",Vector3(0,0.42,0),Vector3(1.8,0.84,1.25),Color("80766a"),false,Vector3(0,17,12))
   for n in range(3):
    _form(root,"cone",Vector3(float(n-1)*0.54,1.35,0),Vector3(0.52,1.65+variation,0.52),accent,true,Vector3(10,n*21,13))
  4: # Tumbled columns, astral runes and relic globes.
   _form(root,"cylinder",Vector3(-0.4,0.6,0),Vector3(0.65,1.2,0.65),Color("7e6e88"),false,Vector3(0,0,62))
   _form(root,"ring",Vector3(0,0.2,0),Vector3(1.5,0.13,1.5),accent,true)
   _form(root,"sphere",Vector3(0.7,0.9,0.2),Vector3(0.35,0.35,0.35),Color("bba3db"),true)
  5: # Blood-braziers and obsidian spires.
   _form(root,"cone",Vector3(-0.5,1.4,0),Vector3(0.9,2.8,0.9),Color("332334"))
   _form(root,"cone",Vector3(0.5,1.0,0),Vector3(0.65,2.0,0.65),Color("71314b"))
   _form(root,"ring",Vector3(0,1.65,0),Vector3(1.4,0.12,1.4),accent,true)
   _form(root,"sphere",Vector3(0,2.0,0),Vector3(0.33,0.42,0.33),accent,true)

func _make_scatter(root:Node3D,zone:int,center:Vector2,accent:Color,earthy:Color) -> void:
 var seed_gen:=RandomNumberGenerator.new()
 seed_gen.seed=212121+zone*713
 for layer in range(V21_SCATTER_LAYERS):
  var scatter:=MultiMeshInstance3D.new()
  scatter.name="V21_BatchedScatter_%d_%d" % [zone,layer]
  var mesh_batch:=MultiMesh.new()
  mesh_batch.transform_format=MultiMesh.TRANSFORM_3D
  mesh_batch.mesh=_mesh_for("cone" if layer==0 else "sphere" if layer==1 else "box")
  mesh_batch.instance_count=V21_SCATTER_PER_LAYER
  scatter.multimesh=mesh_batch
  var swatch:=earthy.lightened(0.12 + float(layer)*0.12)
  if layer==1:swatch=accent.darkened(0.48)
  scatter.material_override=_vmat(swatch,false)
  root.add_child(scatter)
  for j in range(V21_SCATTER_PER_LAYER):
   var angle:float=seed_gen.randf_range(0.0,TAU)
   var rad:float=seed_gen.randf_range(9.0,27.5)
   var local_pos:=Vector3(center.x+cos(angle)*rad,0.08,center.y+sin(angle)*rad)
   # Keep the cardinal main roads and central play paths visually uncluttered.
   if absf(sin(angle)*rad)<2.2:local_pos.z+=3.0
   if absf(cos(angle)*rad)<2.2:local_pos.x+=3.0
   var scale := Vector3(
    seed_gen.randf_range(0.12,0.40),
    seed_gen.randf_range(0.18,0.95) if layer==0 else seed_gen.randf_range(0.08,0.45),
    seed_gen.randf_range(0.12,0.40)
   )
   var turn:=Basis(Vector3.UP,seed_gen.randf_range(0.0,TAU))
   mesh_batch.set_instance_transform(j,Transform3D(turn.scaled(scale),local_pos))
   v21_scatter_count+=1
  v21_scatter_batches+=1
