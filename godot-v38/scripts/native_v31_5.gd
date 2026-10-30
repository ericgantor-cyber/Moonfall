extends "res://scripts/native_v31_4.gd"
## Moonfall v31.5: modular, actual 3D organic creature surfaces and animation.
## All added models are visual-only: no new actors, physics, save keys or gameplay modes.
const V315_MESH:=preload("res://scripts/systems/creature_mesh_v315.gd")
const V315_UPDATE:float=0.085
const V315_NEAR_SQ:float=26.0*26.0
const V315_MID_SQ:float=43.0*43.0
var v315_tracks:Array[Dictionary]=[]
var v315_tick_time:float=0.0
var v315_visible_models:int=0
var v315_lod_skips:int=0
var v315_animation_updates:int=0
var v315_boss_phase_fx:int=0
var v315_pack_signals:int=0
var v315_melee_reactions:int=0
var v315_update_usec:int=0
var v315_peak_usec:int=0

func _release_label()->String:return "v31.5"

func _spawn_beast(at:Vector3,kind:String,zone:int,variant:String="normal",boss:bool=false)->void:
 var previous:int=beasts.size()
 super._spawn_beast(at,kind,zone,variant,boss)
 if beasts.size()<=previous:return
 var rig:Node3D=visual_rigs[-1]
 var creature:Dictionary=beasts[-1]
 var legacy:Array[MeshInstance3D]=[]
 for child in rig.get_children():
  if child is MeshInstance3D:
   legacy.append(child as MeshInstance3D)
 var geometry:Dictionary=V315_MESH.create(rig,kind,boss)
 v315_tracks.append({"body":creature["body"],"creature":creature,"zone":zone,
  "kind":kind,"boss":boss,"geometry":geometry,"legacy":legacy,
  "hit_until":0.0,"phase":1,"stride":float(v315_tracks.size())*0.71})

func _v315_tick_visuals()->void:
 if not hero_playing or not is_instance_valid(player):return
 var start:int=Time.get_ticks_usec()
 var hero_at:Vector3=player.global_position
 var high_quality:bool=graphics_quality!="Smooth"
 v315_visible_models=0
 v315_lod_skips=0
 for track in v315_tracks:
  var model:Dictionary=track["geometry"]
  var root:Node3D=model["root"]
  var beast:Dictionary=track["creature"]
  var body:CharacterBody3D=track["body"]
  var active:bool=bool(beast.get("alive",false)) and int(track["zone"])==region_index and is_instance_valid(body) and body.visible
  var distance_sq:float=body.global_position.distance_squared_to(hero_at) if active else INF
  var show_detail:bool=active and high_quality and distance_sq<V315_NEAR_SQ*(2.25 if bool(track["boss"]) else 1.0)
  root.visible=show_detail
  # The inherited low-polygon body is a real LOD fallback, not an additional draw.
  # Keep its existing articulated leg pivots/AI and every original collision intact.
  for old_mesh in track["legacy"]:
   if is_instance_valid(old_mesh):old_mesh.visible=not show_detail
  if not show_detail:
   v315_lod_skips+=1
   continue
  v315_visible_models+=1
  var motion:float=clampf(body.velocity.length()/4.5,0.0,1.0)
  var phase:float=map_time*(5.5+motion*4.0)+float(track["stride"])
  var staggering:bool=map_time<float(beast.get("stagger_until",0.0)) or map_time<float(track["hit_until"])
  var attacking:bool=float(beast.get("windup",0.0))>0.0
  var head:Node3D=model["head"]
  var tail:Node3D=model["tail"]
  head.rotation.z=lerpf(head.rotation.z,(-0.34 if staggering else 0.16 if attacking else 0.06*sin(phase*0.48)),0.46)
  tail.rotation.y=0.31*sin(phase*0.53)
  tail.rotation.x=0.14*sin(phase*0.6)
  root.position.y=(0.04*sin(phase*1.2)*motion) if not staggering else -0.11
  root.rotation.z=lerpf(root.rotation.z,0.17 if staggering else 0.09 if attacking else 0.025*sin(phase),0.4)
  for i in range((model["wings"] as Array).size()):
   var wing:Node3D=model["wings"][i]
   wing.rotation.x=(0.42+0.24*motion)*sin(phase*1.4)*(-1.0 if i==0 else 1.0)
  if bool(track["boss"]):
   var current_phase:int=int(beast.get("boss_phase",1))
   var weakpoint:MeshInstance3D=model["weakpoint"]
   weakpoint.visible=current_phase>1
   if current_phase!=int(track["phase"]):
    track["phase"]=current_phase
    v315_boss_phase_fx+=1
   if weakpoint.visible:weakpoint.scale=Vector3.ONE*(0.9+0.17*sin(phase))
   for i in range((model["crowns"] as Array).size()):
    var additional:Node3D=model["crowns"][i]
    additional.rotation.z=0.16*sin(phase*0.6+float(i)*1.8)
    additional.rotation.y=0.19*sin(phase*0.45+float(i))
  v315_animation_updates+=1
 v315_update_usec=Time.get_ticks_usec()-start
 v315_peak_usec=maxi(v315_peak_usec,v315_update_usec)

func _process(delta:float)->void:
 super._process(delta)
 if not hero_playing:return
 v315_tick_time+=clampf(delta,0.0,0.18)
 if v315_tick_time<V315_UPDATE:return
 v315_tick_time=fmod(v315_tick_time,V315_UPDATE)
 _v315_tick_visuals()

func _apply_player_hit(target:Dictionary,base_damage:float,stagger_strength:float=0.0)->void:
 var prior:float=float(target.get("hp",0.0))
 super._apply_player_hit(target,base_damage,stagger_strength)
 if prior<=float(target.get("hp",0.0)):return
 var body:Node3D=target.get("body")
 for track in v315_tracks:
  if track["body"]==body:
   track["hit_until"]=map_time+0.20
   v315_melee_reactions+=1
   break

func _update_beast_step(beast:Dictionary,delta:float)->void:
 # Bounded pack cooperation: all existing attack windups, ranges, and dodge windows
 # are respected. These ephemeral awareness flags are not part of saved checkpoints.
 if hero_playing and bool(beast.get("alive",false)) and bool(beast.get("hostile",false)) and not bool(beast.get("boss",false)) and bool(beast.get("engaged",false)):
  if map_time>=float(beast.get("v315_next_signal",0.0)):
   beast["v315_next_signal"]=map_time+1.8
   var sender:CharacterBody3D=beast.get("body")
   if is_instance_valid(sender):
    var sent:int=0
    for ally in zone_beast_lists[int(beast.get("zone",0))]:
     if ally==beast or not bool(ally.get("alive",false)) or not bool(ally.get("hostile",false)) or bool(ally.get("boss",false)):continue
     var other:CharacterBody3D=ally.get("body")
     if not is_instance_valid(other) or sender.global_position.distance_squared_to(other.global_position)>81.0:continue
     ally["alert_until"]=maxf(float(ally.get("alert_until",0.0)),map_time+2.2)
     v315_pack_signals+=1
     sent+=1
     if sent>=3:break
 super._update_beast_step(beast,delta)

func _apply_graphics_quality(mode:String)->void:
 super._apply_graphics_quality(mode)
 # Next animation tick switches detailed models versus low-poly source rigs.
 v315_tick_time=V315_UPDATE

func _reset_new_adventure()->void:
 super._reset_new_adventure()
 v315_tick_time=0.0
 for track in v315_tracks:
  track["hit_until"]=0.0
  track["phase"]=1
  (track["geometry"]["root"] as Node3D).visible=false

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["version"]="v31.5"
 report["v315_real_3d_models"]=v315_tracks.size()
 report["v315_visible_high_detail"]=v315_visible_models
 report["v315_low_detail_fallbacks"]=v315_lod_skips
 report["v315_animation_updates"]=v315_animation_updates
 report["v315_boss_phase_visuals"]=v315_boss_phase_fx
 report["v315_pack_signals"]=v315_pack_signals
 report["v315_hit_reactions"]=v315_melee_reactions
 report["v315_visual_update_usec"]=v315_update_usec
 report["v315_visual_peak_usec"]=v315_peak_usec
 return report
