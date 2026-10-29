extends Node3D
## Reusable non-destructive hero rig for the existing flattened Sprite3D art.
## The armature and effect sockets are ready for future cutout layers; today only
## the parent art pivot moves so Z/Nyra's original pixels stay identical.
@export var hero_id:String="z"
var art_pivot:Node3D
var skeleton:Skeleton3D
var animation_player:AnimationPlayer
var sockets:Dictionary={}
var current_state:String=""
var gait_phase:float=0.0
# Layers are full-frame transparent cutouts, retaining exact original coordinates.
# Nothing auto-slices the original hero painting or its painted background.
const CUTOUT_BONES:Dictionary={
 "Backplate":"",
 "Pelvis":"Pelvis","Torso":"Torso","Head":"Head",
 "UpperArm_L":"Shoulder_L","LowerArm_L":"Elbow_L","Hand_L":"Hand_L",
 "UpperArm_R":"Shoulder_R","LowerArm_R":"Elbow_R","Hand_R":"Hand_R",
 "UpperLeg_L":"Hip_L","LowerLeg_L":"Knee_L","Foot_L":"Foot_L",
 "UpperLeg_R":"Hip_R","LowerLeg_R":"Knee_R","Foot_R":"Foot_R"
}
var cutout_source:Sprite3D
var cutout_sprites:Array[Sprite3D]=[]

func _ready()->void:
 art_pivot=Node3D.new()
 art_pivot.name="ArtMotionAnchor"
 add_child(art_pivot)
 skeleton=Skeleton3D.new()
 skeleton.name="HeroSkeleton"
 art_pivot.add_child(skeleton)
 _build_bones()
 _build_sockets()
 animation_player=AnimationPlayer.new()
 animation_player.name="AnimationPlayer"
 animation_player.root_node=NodePath("..")
 add_child(animation_player)
 _build_animations()
 play_state("idle")

func _build_bones()->void:
 # Relative positions approximate joints of a 2.65-unit hero; none change art.
 var definitions:Array=[
  ["Root",-1,Vector3.ZERO],
  ["Pelvis",0,Vector3(0.0,0.7,0.0)],
  ["Torso",1,Vector3(0.0,0.5,0.0)],
  ["Neck",2,Vector3(0.0,0.42,0.0)],
  ["Head",3,Vector3(0.0,0.25,0.0)],
  ["Shoulder_L",2,Vector3(-0.32,0.29,0.0)],
  ["Elbow_L",5,Vector3(-0.35,-0.30,0.0)],
  ["Hand_L",6,Vector3(-0.23,-0.30,0.0)],
  ["Shoulder_R",2,Vector3(0.32,0.29,0.0)],
  ["Elbow_R",8,Vector3(0.35,-0.30,0.0)],
  ["Hand_R",9,Vector3(0.23,-0.30,0.0)],
  ["Hip_L",1,Vector3(-0.20,-0.18,0.0)],
  ["Knee_L",11,Vector3(-0.06,-0.46,0.0)],
  ["Foot_L",12,Vector3(0.0,-0.41,0.12)],
  ["Hip_R",1,Vector3(0.20,-0.18,0.0)],
  ["Knee_R",14,Vector3(0.06,-0.46,0.0)],
  ["Foot_R",15,Vector3(0.0,-0.41,0.12)],
  ["HairFront",4,Vector3(0.0,0.22,-0.01)],
  ["HairBack",4,Vector3(0.0,0.20,0.02)],
  ["CapeTop",2,Vector3(0.0,0.24,0.06)],
  ["CapeEnd",19,Vector3(0.0,-0.59,0.05)],
  ["ClothFront",1,Vector3(0.0,-0.18,-0.03)],
  ["ClothBack",1,Vector3(0.0,-0.17,0.04)]
 ]
 for definition in definitions:
  skeleton.add_bone(str(definition[0]))
  var index:int=skeleton.get_bone_count()-1
  if int(definition[1])>=0:skeleton.set_bone_parent(index,int(definition[1]))
  skeleton.set_bone_rest(index,Transform3D(Basis.IDENTITY,definition[2]))

func _build_sockets()->void:
 var attachments:Array=[
  ["WeaponSocket","Hand_R"],
  ["CastSocket","Hand_L"],
  ["ProjectileSocket","Hand_L"],
  ["HitSocket","Hand_R"],
  ["HeadSocket","Head"],
  ["FootSocket","Foot_R"]
 ]
 for attachment in attachments:
  var socket:=BoneAttachment3D.new()
  socket.name=str(attachment[0])
  socket.bone_name=str(attachment[1])
  skeleton.add_child(socket)
  sockets[socket.name]=socket
 if hero_id=="nyra":
  var aura:=BoneAttachment3D.new()
  aura.name="AuraSocket"
  aura.bone_name="Torso"
  skeleton.add_child(aura)
  sockets[aura.name]=aura

func _add_motion(library:AnimationLibrary,label:String,duration:float,looping:bool,frames:Array)->void:
 var animation:=Animation.new()
 animation.length=duration
 animation.loop_mode=Animation.LOOP_LINEAR if looping else Animation.LOOP_NONE
 var lift:int=animation.add_track(Animation.TYPE_VALUE)
 animation.track_set_path(lift,NodePath("ArtMotionAnchor:position"))
 var tilt:int=animation.add_track(Animation.TYPE_VALUE)
 animation.track_set_path(tilt,NodePath("ArtMotionAnchor:rotation"))
 for frame in frames:
  animation.track_insert_key(lift,float(frame[0]),Vector3(0.0,float(frame[1]),0.0))
  animation.track_insert_key(tilt,float(frame[0]),Vector3(0.0,0.0,float(frame[2])))
 library.add_animation(label,animation)

func _build_animations()->void:
 var library:=AnimationLibrary.new()
 var power:float=0.8 if hero_id=="nyra" else 1.0
 _add_motion(library,"idle",1.4,true,[[0.0,0.0,0.0],[0.7,0.035*power,0.015*power],[1.4,0.0,0.0]])
 _add_motion(library,"walk",0.68,true,[[0.0,0.0,-0.035],[0.17,0.065*power,0.0],[0.34,0.0,0.035],[0.51,0.065*power,0.0],[0.68,0.0,-0.035]])
 _add_motion(library,"run",0.44,true,[[0.0,0.0,-0.065],[0.11,0.09*power,0.0],[0.22,0.0,0.065],[0.33,0.09*power,0.0],[0.44,0.0,-0.065]])
 _add_motion(library,"swim",0.95,true,[[0.0,0.0,-0.025],[0.48,0.04,0.025],[0.95,0.0,-0.025]])
 _add_motion(library,"attack",0.32,false,[[0.0,0.0,0.0],[0.06,0.025,-0.12*power],[0.14,0.075,0.17*power],[0.23,0.035,0.08*power],[0.32,0.0,0.0]])
 _add_motion(library,"cast",0.55,false,[[0.0,0.0,0.0],[0.14,0.025,0.045*power],[0.29,0.09,0.09*power],[0.38,0.065,-0.085*power],[0.55,0.0,0.0]])
 _add_motion(library,"dash",0.50,false,[[0.0,0.0,0.0],[0.12,-0.05,-0.11],[0.31,-0.05,0.08],[0.50,0.0,0.0]])
 _add_motion(library,"hurt",0.30,false,[[0.0,0.0,0.0],[0.10,-0.06,0.10],[0.30,0.0,0.0]])
 _add_motion(library,"death",0.55,false,[[0.0,0.0,0.0],[0.35,-0.14,0.18],[0.55,-0.22,0.22]])
 animation_player.add_animation_library("",library)

func get_art_pivot()->Node3D:
 return art_pivot

func get_socket(which:String)->Node3D:
 return sockets.get(which) as Node3D

func reset_pose()->void:
 if is_instance_valid(animation_player):animation_player.stop()
 for bone in range(skeleton.get_bone_count()):
  skeleton.set_bone_pose_rotation(bone,Quaternion.IDENTITY)
 art_pivot.position=Vector3.ZERO
 art_pivot.rotation=Vector3.ZERO
 art_pivot.scale=Vector3.ONE
 gait_phase=0.0
 current_state=""

func play_state(next_state:String)->void:
 if not animation_player.has_animation(next_state):return
 if current_state==next_state and animation_player.is_playing():return
 current_state=next_state
 # Fast combat transitions preserve response time; movement gets softer blends.
 var blend:float=0.045 if next_state in ["attack","cast","dash","hurt","death"] else 0.12
 animation_player.play(next_state,blend)

func step_bones(delta:float)->void:
 # Art sockets and approved cutouts move independently. The uncut source never changes.
 gait_phase+=minf(maxf(delta,0.0),0.08)
 var cycle:float=sin(gait_phase*(13.0 if current_state=="run" else 8.0))
 var locomotion:bool=current_state in ["walk","run"]
 var swing:float=cycle*(0.24 if hero_id=="z" else 0.18) if locomotion else 0.0
 if current_state=="attack":swing=-0.45
 elif current_state=="cast":swing=0.35 if hero_id=="nyra" else 0.24
 for side in ["L","R"]:
  var sign:float=-1.0 if side=="L" else 1.0
  _rotate_bone("Shoulder_"+side,swing*sign)
  _rotate_bone("Elbow_"+side,-0.28*sign if current_state=="attack" else 0.09*swing)
  _rotate_bone("Hip_"+side,-swing*sign if locomotion else 0.0)
  _rotate_bone("Knee_"+side,absf(swing)*0.42 if locomotion else 0.0)
 var wind:float=sin(gait_phase*(4.0 if hero_id=="nyra" else 3.1))
 _rotate_bone("HairFront",0.035*wind)
 _rotate_bone("HairBack",-0.06*wind)
 _rotate_bone("CapeTop",0.06*wind+(-0.08 if current_state=="run" else 0.0))
 _rotate_bone("CapeEnd",-0.10*wind+(0.15 if current_state=="run" else 0.0))
 _rotate_bone("ClothFront",0.055*wind)
 _rotate_bone("ClothBack",-0.07*wind)

func _rotate_bone(bone_name:String,angle:float)->void:
 var index:int=skeleton.find_bone(bone_name)
 if index>=0:
  var goal:=Quaternion(Vector3(0.0,0.0,1.0),angle)
  var current:Quaternion=skeleton.get_bone_pose_rotation(index)
  skeleton.set_bone_pose_rotation(index,current.slerp(goal,0.36))

func required_cutout_parts()->PackedStringArray:
 var parts:=PackedStringArray()
 for part in CUTOUT_BONES:parts.append(str(part))
 return parts

func is_cutout_active()->bool:
 return is_instance_valid(cutout_source) and not cutout_source.visible and cutout_sprites.size()==CUTOUT_BONES.size()

func install_cutouts(original:Sprite3D,layers:Dictionary,approved:bool=false)->bool:
 # Missing or unreviewed pieces are not allowed to hide the source artwork.
 if not approved or not is_instance_valid(original) or original.texture==null:return false
 var source_size:Vector2=original.texture.get_size()
 if source_size.x<=0 or source_size.y<=0:return false
 for part in CUTOUT_BONES:
  if not layers.has(part) or not (layers[part] is Texture2D):return false
  if (layers[part] as Texture2D).get_size()!=source_size:return false
 restore_original()
 cutout_source=original
 for part in CUTOUT_BONES:
  var target:Node3D=art_pivot
  var bone_name:String=str(CUTOUT_BONES[part])
  var rest_point:Vector3=Vector3.ZERO
  if bone_name!="":
   var bone:int=skeleton.find_bone(bone_name)
   if bone<0:
    restore_original()
    return false
   rest_point=skeleton.get_bone_global_rest(bone).origin
   var attachment:=BoneAttachment3D.new()
   attachment.name="LayerAttachment_"+str(part)
   attachment.bone_name=bone_name
   skeleton.add_child(attachment)
   target=attachment
  var layer:=Sprite3D.new()
  layer.name="Cutout_"+str(part)
  layer.texture=layers[part] as Texture2D
  layer.pixel_size=original.pixel_size
  layer.billboard=original.billboard
  layer.alpha_cut=original.alpha_cut
  layer.flip_h=original.flip_h
  layer.position=original.position-rest_point
  target.add_child(layer)
  cutout_sprites.append(layer)
 original.visible=false
 return true

func restore_original()->void:
 if is_instance_valid(cutout_source):cutout_source.visible=true
 for layer in cutout_sprites:
  if not is_instance_valid(layer):continue
  var parent:Node=layer.get_parent()
  layer.queue_free()
  if parent is BoneAttachment3D:parent.queue_free()
 cutout_sprites.clear()
 cutout_source=null
