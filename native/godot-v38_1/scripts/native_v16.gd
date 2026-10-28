extends "res://scripts/native_v15.gd"
## Moonfall v16 visual polish: biome surface textures, combat sprite posing,
## slash/dodge/spell cues and bounded cosmetic updates. Hero images untouched.
const SURFACE_SIZE := 96
const MAX_COMBAT_CUES := 12
var zone_surface_textures: Dictionary = {}
var combat_cues: Array[Dictionary] = []
var combat_pose_age := 0.0
var combat_pose_kind := "idle"
var combat_cues_created := 0
var cosmetic_updates := 0
var sampled_frames := 0
var performance_seconds := 0.0
var simulation_frame_rate := 0.0

func _ready() -> void:
 super._ready()
 _show_message("Moonfall Native v16: combat motion and six biome surfaces.", 4.5)

func _surface_texture(zone:int, base:Color) -> ImageTexture:
 # Repeatable tile texture; no dependency on external image files or random seeds.
 var image := Image.create(SURFACE_SIZE,SURFACE_SIZE,false,Image.FORMAT_RGBA8)
 for y in range(SURFACE_SIZE):
  for x in range(SURFACE_SIZE):
   var a:=TAU*float(x)/float(SURFACE_SIZE)
   var b:=TAU*float(y)/float(SURFACE_SIZE)
   var big:=sin(a*3.0+0.7*float(zone))*cos(b*3.0-0.3*float(zone))
   var fine:=sin(a*13.0+b*7.0)*cos(b*11.0-a*5.0)
   var detail:=big*0.13+fine*0.062
   match zone:
    1:detail+=0.075*sin(a*6.0+b*4.0) # forest moss
    2:detail+=0.10*cos(a*2.0-b*5.0) # marsh pools
    3:detail+=0.13*absf(sin(a*5.0+b*7.0)) # quarry streaks
    4:detail+=0.10*sin(a*7.0)*sin(b*7.0) # stone seams
    5:detail+=0.09*cos(a*4.0+b*4.0) # citadel ash
   var color:=base.lightened(detail) if detail>0.0 else base.darkened(-detail)
   image.set_pixel(x,y,color)
 return ImageTexture.create_from_image(image)

func _build_region(zone:int) -> void:
 super._build_region(zone)
 var ground:Node3D=region_nodes[zone].get_node_or_null("RegionGround") as Node3D
 if ground==null:return
 var ground_mesh:MeshInstance3D=null
 for child in ground.get_children():
  if child is MeshInstance3D:ground_mesh=child;break
 if ground_mesh==null:return
 var base:=Color(str(REGIONS[zone]["ground"]))
 var texture:=_surface_texture(zone,base)
 var material:=StandardMaterial3D.new()
 material.albedo_texture=texture
 material.roughness=0.96
 material.uv1_scale=Vector3(12.0,12.0,12.0)
 material.texture_filter=BaseMaterial3D.TEXTURE_FILTER_LINEAR_WITH_MIPMAPS
 ground_mesh.material_override=material
 zone_surface_textures[zone]=texture

func _start_game(selected:String) -> void:
 super._start_game(selected)
 combat_pose_kind="idle"
 combat_pose_age=0.0
 if is_instance_valid(hero_sprite):
  hero_sprite.position=Vector3(0,0.85,0)
  hero_sprite.scale=Vector3.ONE
  hero_sprite.rotation=Vector3.ZERO

func _cue(at:Vector3, kind:String, color:Color) -> void:
 if combat_cues.size()>=MAX_COMBAT_CUES or not is_instance_valid(world):return
 var node:=Node3D.new()
 node.name="V16_"+kind.capitalize()+"Cue"
 world.add_child(node)
 node.global_position=at
 if kind=="attack":
  _form(node,"ring",Vector3(0,0.07,0),Vector3(1.25,0.10,1.25),color,true,Vector3(22,0,0))
  _form(node,"cone",Vector3(0.55,0.46,0),Vector3(0.21,0.90,0.21),color,true,Vector3(0,0,-66))
 elif kind=="dodge":
  _form(node,"ring",Vector3.ZERO,Vector3(0.8,0.08,0.8),color,true)
 else:
  _form(node,"sphere",Vector3(0,0.4,0),Vector3(0.46,0.46,0.46),color,true)
  _form(node,"ring",Vector3.ZERO,Vector3(1.10,0.09,1.10),color,true)
 combat_cues.append({"node":node,"age":0.0,"life":0.42 if kind=="attack" else 0.65,"kind":kind})
 combat_cues_created+=1

func _attack() -> void:
 var before:=attack_cooldown
 super._attack()
 if attack_cooldown>before and is_instance_valid(player):
  combat_pose_kind="attack"
  combat_pose_age=0.0
  _cue(player.global_position+Vector3(0.7 if not hero_sprite.flip_h else -0.7,-0.5,0),"attack",Color("ffca83"))

func _dodge() -> void:
 var before:=dodge_cooldown
 super._dodge()
 if dodge_cooldown>before and is_instance_valid(player):
  combat_pose_kind="dodge"
  combat_pose_age=0.0
  _cue(player.global_position+Vector3(0,-0.65,0),"dodge",Color("9ac7ff"))

func _spell(which:String) -> void:
 var before:=ability_cooldown
 super._spell(which)
 if ability_cooldown>before and is_instance_valid(player):
  combat_pose_kind="spell"
  combat_pose_age=0.0
  _cue(player.global_position+Vector3(0,-0.55,0),"spell",Color("b79aff") if hero=="nyra" else Color("ffa8c2"))

func _process(delta:float) -> void:
 super._process(delta)
 var step:=clampf(delta,0.0,0.1)
 sampled_frames+=1
 performance_seconds+=step
 if performance_seconds>=2.0:
  simulation_frame_rate=float(sampled_frames)/maxf(performance_seconds,0.001)
  sampled_frames=0
  performance_seconds=0.0
 if is_instance_valid(hero_sprite):
  combat_pose_age+=step
  var bob:=0.02*sin(map_time*5.5) if hero_playing and not menu_open else 0.0
  var tilt:=0.0
  var squash:=1.0
  var duration:=0.0
  match combat_pose_kind:
   "attack":duration=0.34
   "dodge":duration=0.52
   "spell":duration=0.62
  if duration>0.0 and combat_pose_age<duration:
   var phase:=combat_pose_age/duration
   var intensity:=sin(phase*PI)
   if combat_pose_kind=="attack":
    tilt=0.13*intensity*(-1.0 if hero_sprite.flip_h else 1.0)
    squash=1.0+0.09*intensity
   elif combat_pose_kind=="dodge":
    tilt=0.23*intensity*(-1.0 if hero_sprite.flip_h else 1.0)
    squash=1.0-0.14*intensity
   else:
    squash=1.0+0.06*intensity
    bob+=0.10*intensity
  elif duration>0.0:
   combat_pose_kind="idle"
  # Only transforms the Sprite3D node: original Z/Nyra texture bytes are never modified.
  hero_sprite.position=Vector3(0,0.85+bob,0)
  hero_sprite.rotation.z=tilt
  hero_sprite.scale=Vector3(squash,2.0-squash,1.0)
 for entry in combat_cues.duplicate():
  var node:Node3D=entry["node"]
  if not is_instance_valid(node):
   combat_cues.erase(entry)
   continue
  entry["age"]=float(entry["age"])+step
  var fraction:=float(entry["age"])/float(entry["life"])
  if fraction>=1.0:
   combat_cues.erase(entry)
   node.queue_free()
  else:
   var amount:=1.0+fraction*(1.3 if entry["kind"]=="attack" else 1.7)
   node.scale=Vector3.ONE*amount
   node.rotation.y+=step*3.5
 cosmetic_updates+=1
