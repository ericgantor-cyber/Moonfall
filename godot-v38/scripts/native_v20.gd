extends "res://scripts/native_v19.gd"
## v20 cinematic boss presentation and lightweight creature hit reactions.
## Reuses shared meshes and materials; cosmetic only, no art / save edits.
const V20_BOSS_TICK := 0.09
const V20_BOSS_ANIMATE_RANGE := 40.0 * 40.0
var v20_guardian_sigils: Dictionary = {}
var v20_boss_timer := 0.0
var v20_animated_guardians := 0
var v20_impact_pulses := 0
var v20_phase_flashes := 0

func _ready() -> void:
 super._ready()
 _show_message("Moonfall v20: guardian spectacles and responsive combat",4.0)

func _spawn_beast(at:Vector3, kind:String, zone:int, variant:String="normal", boss:bool=false) -> void:
 super._spawn_beast(at,kind,zone,variant,boss)
 if not boss:return
 var creature:Dictionary=beasts[-1]
 var body:CharacterBody3D=creature["body"]
 var rig:=Node3D.new()
 rig.name="V20_GuardianSigil"
 body.add_child(rig)
 # A unique three-ring spellwheel for every roaming guardian.
 var region_color:=Color(str(REGIONS[zone]["light"]))
 for index in range(3):
  var ring:=_form(rig,"ring",Vector3(0,-0.50+0.2*float(index),0),Vector3(2.8-0.4*float(index),0.08,2.8-0.4*float(index)),region_color.lightened(float(index)*0.08),true,Vector3(float(index)*11.0,0,float(index)*-7.0))
  ring.name="GuardianSpellwheel_"+str(index)
 var beam:=_form(rig,"cone",Vector3(0,1.55,0),Vector3(0.50,2.7,0.50),region_color,true)
 beam.name="GuardianEnergyCrown"
 rig.visible=false
 v20_guardian_sigils[int(creature["boss_id"])]= {"root":rig,"zone":zone,"beam":beam,"phase":1,"tick":float(zone)*0.55}

func _refresh_boss_phase(beast:Dictionary) -> void:
 var old:int=int(beast.get("boss_phase",1))
 super._refresh_boss_phase(beast)
 var current:int=int(beast.get("boss_phase",1))
 if current<=old:return
 var id:int=int(beast.get("boss_id",-1))
 if not v20_guardian_sigils.has(id):return
 var effect:Dictionary=v20_guardian_sigils[id]
 var root:Node3D=effect["root"]
 if not is_instance_valid(root):return
 effect["phase"]=current
 root.visible=true
 root.scale=Vector3.ONE*(1.0 if current==2 else 1.25)
 v20_phase_flashes+=1

func _apply_player_hit(target:Dictionary,base_damage:float,stagger_strength:float=0.0) -> void:
 var old_hp:float=float(target.get("hp",0.0))
 super._apply_player_hit(target,base_damage,stagger_strength)
 if old_hp<=float(target.get("hp",0.0)) or not is_instance_valid(player):return
 var body:CharacterBody3D=target.get("body")
 if not is_instance_valid(body):return
 if body.global_position.distance_squared_to(player.global_position)>V20_BOSS_ANIMATE_RANGE:return
 # Existing v16 pooled / lifetime-capped effects avoid lingering on-hit nodes.
 if combat_cues.size()<MAX_COMBAT_CUES:
  _cue(body.global_position+Vector3(0,0.1,0),"attack",Color("ffe2a7") if not bool(target.get("boss",false)) else Color("ff6c9b"))
  v20_impact_pulses+=1

func _process(delta:float) -> void:
 super._process(delta)
 v20_boss_timer+=minf(maxf(delta,0.0),0.2)
 if v20_boss_timer<V20_BOSS_TICK:return
 v20_boss_timer=fmod(v20_boss_timer,V20_BOSS_TICK)
 if not hero_playing or not is_instance_valid(player):return
 for id in v20_guardian_sigils:
  var effect:Dictionary=v20_guardian_sigils[id]
  var root:Node3D=effect["root"]
  if not is_instance_valid(root):continue
  var zone:int=int(effect["zone"])
  if zone!=region_index or graphics_quality=="Smooth":
   root.visible=false
   continue
  var creature:Dictionary=beasts[int(id)]
  var body:CharacterBody3D=creature["body"]
  var phase:int=int(creature.get("boss_phase",1))
  if not bool(creature.get("alive",false)) or not is_instance_valid(body) or body.global_position.distance_squared_to(player.global_position)>V20_BOSS_ANIMATE_RANGE:
   root.visible=false
   continue
  root.visible=phase>=2
  if not root.visible:continue
  effect["tick"]=float(effect["tick"])+V20_BOSS_TICK
  root.rotation.y=fposmod(root.rotation.y+0.14*float(phase),TAU)
  var pulse:=1.0+0.08*sin(float(effect["tick"])*3.0)
  var base:=1.25 if phase==3 else 1.0
  root.scale=Vector3.ONE*base*pulse
  v20_animated_guardians+=1
