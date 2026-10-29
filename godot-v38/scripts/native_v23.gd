extends "res://scripts/native_v22.gd"
## v23 creature animation pass. Supplementary procedural pivots only.
## Not a skeletal/IK animation system. Unmodified collisions, art and save data.
const V23_VISUAL_RANGE_SQ := 34.0 * 34.0
const V23_ANIMATION_TICK := 0.075
var v23_tracks: Array[Dictionary] = []
var v23_effect_clock := 0.0
var v23_near_animated := 0
var v23_culled := 0
var v23_hit_reactions := 0
var v23_boss_breaths := 0
var v23_procedural_joints := 0

func _ready() -> void:
 super._ready()
 _show_message("Moonfall v23: articulated fauna and guardian animation",3.5)

func _spawn_beast(at:Vector3,kind:String,zone:int,variant:String="normal",boss:bool=false) -> void:
 super._spawn_beast(at,kind,zone,variant,boss)
 var creature:Dictionary=beasts[-1]
 var rig:Node3D=visual_rigs[-1]
 var track:Dictionary={"root":rig,"kind":kind,"zone":zone,"body":creature["body"],"legs":[],"head":null,"wings":[],"segments":[],"boss_aura":null,"hit_until":-1.0,"offset":float(v23_tracks.size())*0.57,"boss":boss}
 # All fine geometry is additive and purely cosmetic; the body and collision capsule are unchanged.
 var warm:=Color("bba994")
 var shadow:=Color("423b43")
 var zone_tint:=Color(str(REGIONS[zone]["light"]))
 if kind in ["wolf","stag","boar","fox","panther","elk"]:
  for fore in [false,true]:
   for side in [-1.0,1.0]:
    var hip:=Node3D.new()
    hip.name="V23_Hip"
    hip.position=Vector3(0.36 if fore else -0.47,-0.20,side*0.26)
    rig.add_child(hip)
    _form(hip,"sphere",Vector3(0,-0.13,0),Vector3(0.18,0.33,0.17),shadow)
    var knee:=Node3D.new()
    knee.name="V23_Knee"
    knee.position=Vector3(0,-0.29,0)
    hip.add_child(knee)
    _form(knee,"cylinder",Vector3(0,-0.15,0),Vector3(0.13,0.31,0.13),warm.darkened(0.32))
    _form(knee,"sphere",Vector3(0.09,-0.32,0),Vector3(0.25,0.11,0.18),shadow)
    (track["legs"] as Array).append({"hip":hip,"knee":knee,"fore":fore,"side":side})
    v23_procedural_joints+=2
  var head:=Node3D.new()
  head.name="V23_HeadPivot"
  head.position=Vector3(0.55,0.19,0)
  rig.add_child(head)
  _form(head,"sphere",Vector3(0.26,-0.07,0),Vector3(0.53,0.30,0.37),shadow.lightened(0.2))
  for side in [-1.0,1.0]:
   _form(head,"sphere",Vector3(0.45,0.12,side*0.27),Vector3(0.10,0.08,0.09),zone_tint,true)
  if kind in ["fox","wolf","panther"]:
   _form(head,"cone",Vector3(0.65,-0.18,0),Vector3(0.22,0.43,0.22),warm,false,Vector3(0,0,-70))
  track["head"]=head
 elif kind=="owl":
  for side in [-1.0,1.0]:
   var pivot:=Node3D.new()
   pivot.name="V23_WingShoulder"
   pivot.position=Vector3(-0.13,0.06,side*0.48)
   rig.add_child(pivot)
   for i in range(3):
    _form(pivot,"sphere",Vector3(-0.12-float(i)*0.12,-0.02,side*(0.22+float(i)*0.22)),Vector3(0.48-float(i)*0.06,0.16,0.49),warm.darkened(0.2+float(i)*0.06))
   (track["wings"] as Array).append({"node":pivot,"side":side})
   v23_procedural_joints+=1
 else: # A segmented serpent with lateral rather than quadruped movement.
  for i in range(6):
   var segment:=Node3D.new()
   segment.name="V23_SpineSegment_%d" % i
   segment.position=Vector3(-0.73+float(i)*0.23,-0.16,0)
   rig.add_child(segment)
   _form(segment,"sphere",Vector3.ZERO,Vector3(0.28-float(i)*0.014,0.18,0.28-float(i)*0.014),warm.darkened(0.23))
   (track["segments"] as Array).append(segment)
   v23_procedural_joints+=1
 if boss:
  var aura:=Node3D.new()
  aura.name="V23_GuardianBreath"
  aura.position.y=0.18
  rig.add_child(aura)
  # Upper and lower rings pulse on windup and on phase transformations.
  _form(aura,"ring",Vector3(0,-0.60,0),Vector3(1.75,0.08,1.75),zone_tint,true)
  _form(aura,"ring",Vector3(0,1.24,0),Vector3(0.88,0.06,0.88),zone_tint.lightened(0.12),true,Vector3(24,0,0))
  aura.visible=false
  track["boss_aura"]=aura
 v23_tracks.append(track)

func _apply_player_hit(target:Dictionary,base_damage:float,stagger_strength:float=0.0) -> void:
 var before:=float(target.get("hp",0.0))
 super._apply_player_hit(target,base_damage,stagger_strength)
 if before<=float(target.get("hp",0.0)):return
 var body:CharacterBody3D=target.get("body")
 if not is_instance_valid(body):return
 for t in v23_tracks:
  if t["body"]==body:
   t["hit_until"]=map_time+0.28
   v23_hit_reactions+=1
   break

func _process(delta:float) -> void:
 super._process(delta)
 v23_effect_clock+=clampf(delta,0.0,0.2)
 if v23_effect_clock<V23_ANIMATION_TICK:return
 var seconds:=v23_effect_clock
 v23_effect_clock=fmod(v23_effect_clock,V23_ANIMATION_TICK)
 v23_near_animated=0
 if not hero_playing or not is_instance_valid(player):return
 var near:Vector3=player.global_position
 for i in range(v23_tracks.size()):
  var t:Dictionary=v23_tracks[i]
  var creature:Dictionary=beasts[i]
  if int(t["zone"])!=region_index or not bool(creature.get("alive",false)):continue
  var body:CharacterBody3D=t["body"]
  if not is_instance_valid(body) or not body.visible:continue
  if body.global_position.distance_squared_to(near)>V23_VISUAL_RANGE_SQ:
   v23_culled+=1
   continue
  if graphics_quality=="Smooth":continue
  v23_near_animated+=1
  var phase:float=map_time*8.3+float(t["offset"])
  var motion:float=clampf(body.velocity.length()/4.0,0.0,1.0)
  var hit:bool=map_time<float(t["hit_until"])
  var intensity:float=0.52*motion
  for j in range((t["legs"] as Array).size()):
   var leg:Dictionary=t["legs"][j]
   var side:float=float(leg["side"])
   var step:float=phase+(PI if bool(leg["fore"]) else 0.0)+(PI if side<0.0 else 0.0)
   var hip:Node3D=leg["hip"]
   var knee:Node3D=leg["knee"]
   hip.rotation.z=lerp_angle(hip.rotation.z,sin(step)*intensity,0.5)
   knee.rotation.z=lerp_angle(knee.rotation.z,-maxf(0.0,sin(step))*0.48*motion,0.5)
  var head:Node3D=t["head"]
  if is_instance_valid(head):
   head.rotation.z=lerpf(head.rotation.z,(0.24 if hit else -0.035+0.04*sin(phase*0.27)),0.35)
   head.rotation.y=0.055*sin(phase*0.20)
  for wing in t["wings"]:
   var pivot:Node3D=wing["node"]
   pivot.rotation.x=sin(phase*1.25)*0.46*float(wing["side"])
  for j in range((t["segments"] as Array).size()):
   var segment:Node3D=t["segments"][j]
   segment.position.z=0.18*sin(phase*0.85-float(j)*0.6)*(0.5+motion)
  var aura:Node3D=t["boss_aura"]
  if is_instance_valid(aura):
   var windup:float=float(creature.get("windup",0.0))
   var active:bool=int(creature.get("boss_phase",1))>1 or windup>0.0
   aura.visible=active
   if active:
    v23_boss_breaths+=1
    aura.rotation.y=fposmod(aura.rotation.y+seconds*0.6,TAU)
    var pulse:float=1.0+0.16*sin(phase*0.5)
    if windup>0.0:pulse+=0.15*sin(phase*2.1)
    aura.scale=Vector3.ONE*pulse

func _apply_graphics_quality(mode:String) -> void:
 super._apply_graphics_quality(mode)
 for t in v23_tracks:
  for leg in t["legs"]: (leg["hip"] as Node3D).visible=(graphics_quality!="Smooth")
  for wing in t["wings"]: (wing["node"] as Node3D).visible=(graphics_quality!="Smooth")
  for segment in t["segments"]: (segment as Node3D).visible=(graphics_quality!="Smooth")
  var head:Node3D=t["head"]
  if is_instance_valid(head):head.visible=(graphics_quality!="Smooth")
  var aura:Node3D=t["boss_aura"]
  if is_instance_valid(aura) and graphics_quality=="Smooth":aura.visible=false
