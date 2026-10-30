extends "res://scripts/native_v31_5.gd"
## v31.6 Living Bestiary overhaul: articulated 3D limbs, species detail,
## boss-phase visuals, budgeted group alerts and distance-scaled animation.
## Cosmetic-only additions: no collision, save schema or map changes.
const V316_ANATOMY:=preload("res://scripts/systems/anatomy_v316.gd")
const V316_AI_ALERT_BUDGET:int=5
var v316_new_parts:int=0
var v316_limb_updates:int=0
var v316_far_skips:int=0
var v316_ai_deferred:int=0
var v316_boss_aura_visible:int=0
var v316_ai_remaining:int=V316_AI_ALERT_BUDGET
var v316_detail_culled:int=0
const V316_DETAIL_BUDGET:int=18

func _release_label()->String:return "v31.6"

func _spawn_beast(at:Vector3,kind:String,zone:int,variant:String="normal",boss:bool=false)->void:
 var prior_count:int=v315_tracks.size()
 super._spawn_beast(at,kind,zone,variant,boss)
 if v315_tracks.size()<=prior_count:return
 var track:Dictionary=v315_tracks[-1]
 var geometry:Dictionary=track["geometry"]
 var anatomy:Dictionary=V316_ANATOMY.decorate(geometry,kind,boss)
 track["v316"]=anatomy
 v316_new_parts+=(anatomy["legs"] as Array).size()

func _v315_tick_visuals()->void:
 # Retains original v31.5 actor visibility and LOD decisions.
 super._v315_tick_visuals()
 if not hero_playing or not is_instance_valid(player):return
 # Budget detailed models by actual hero proximity. Swap overflow actors to
 # their existing low-poly rig rather than hiding the actor or disabling AI.
 var hero_position:Vector3=player.global_position
 var nearby:Array[Dictionary]=[]
 for entry in v315_tracks:
  var geo:Dictionary=entry["geometry"]
  var root:Node3D=geo["root"]
  if not root.visible:continue
  var body:CharacterBody3D=entry["body"]
  if not is_instance_valid(body):continue
  nearby.append({"track":entry,"distance":body.global_position.distance_squared_to(hero_position)})
 if nearby.size()>V316_DETAIL_BUDGET:
  nearby.sort_custom(func(a:Dictionary,b:Dictionary)->bool:return float(a["distance"])<float(b["distance"]))
  for i in range(V316_DETAIL_BUDGET,nearby.size()):
   var culled:Dictionary=nearby[i]["track"]
   (culled["geometry"]["root"] as Node3D).visible=false
   for legacy in (culled["legacy"] as Array):
    if is_instance_valid(legacy):legacy.visible=true
   v315_visible_models=maxi(0,v315_visible_models-1)
   v315_lod_skips+=1
   v316_detail_culled+=1
 v316_boss_aura_visible=0
 for track in v315_tracks:
  var geo:Dictionary=track["geometry"]
  var root:Node3D=geo["root"]
  if not root.visible:
   v316_far_skips+=1
   continue
  var details:Dictionary=track["v316"]
  var creature:Dictionary=track["creature"]
  var body:CharacterBody3D=track["body"]
  var gait:float=clampf(body.velocity.length()/3.6,0.0,1.0)
  var activity:bool=map_time<float(creature.get("stagger_until",0.0)) or map_time<float(track.get("hit_until",0.0))
  var strike:bool=float(creature.get("windup",0.0))>0.0
  var rhythm:float=map_time*(5.0+gait*5.0)+float(track["stride"])
  var legs:Array=details["legs"]
  for i in range(legs.size()):
   var joint:Node3D=legs[i]
   var opposite:float=PI if i==1 or i==2 else 0.0
   joint.rotation.z=lerpf(joint.rotation.z,(-0.24 if activity else -0.13 if strike else 0.42*sin(rhythm+opposite)*gait),0.50)
   if str(track["kind"])=="owl":joint.rotation.x=0.15*sin(rhythm*0.5+float(i))*gait
   v316_limb_updates+=1
  if str(track["kind"])=="serpent":
   root.rotation.y=0.11*sin(rhythm*0.55)*maxf(0.25,gait)
  var aura:MeshInstance3D=details["aura"]
  if aura!=null:
   aura.visible=int(creature.get("boss_phase",1))>=2
   if aura.visible:
    aura.scale=Vector3(1.0+0.08*sin(rhythm*0.67),0.45,1.0+0.08*sin(rhythm*0.67))
    v316_boss_aura_visible+=1

func _update_beast_step(beast:Dictionary,delta:float)->void:
 # Prevent large packs scanning the entire region list every AI step.
 # Existing windups, damage and movement remain owned by the old AI.
 var defer_signal:bool=false
 var prior_signal:float=float(beast.get("v315_next_signal",0.0))
 if hero_playing and bool(beast.get("engaged",false)) and bool(beast.get("alive",false)) and bool(beast.get("hostile",false)) and not bool(beast.get("boss",false)) and map_time>=prior_signal:
  var sender:CharacterBody3D=beast.get("body")
  if is_instance_valid(sender):
   var distant:bool=not is_instance_valid(player) or sender.global_position.distance_squared_to(player.global_position)>22.0*22.0
   if distant or v316_ai_remaining<=0:
    # Prevent a costly same-frame pack scan, then restore the original timer.
    # Repeated deferrals must not postpone this creature indefinitely.
    defer_signal=true
    beast["v315_next_signal"]=map_time+1.0
    v316_ai_deferred+=1
   else:v316_ai_remaining-=1
 super._update_beast_step(beast,delta)
 if defer_signal and bool(beast.get("alive",false)):
  beast["v315_next_signal"]=prior_signal

func _process(delta:float)->void:
 v316_ai_remaining=V316_AI_ALERT_BUDGET
 super._process(delta)

func _perf_report()->Dictionary:
 var stats:Dictionary=super._perf_report()
 stats["version"]="v31.6"
 stats["v316_articulated_limb_nodes"]=v316_new_parts
 stats["v316_oversubscribed_detail_fallbacks"]=v316_detail_culled
 stats["v316_detail_actor_budget"]=V316_DETAIL_BUDGET
 stats["v316_limb_animation_updates"]=v316_limb_updates
 stats["v316_distant_animation_skips"]=v316_far_skips
 stats["v316_ai_broadcasts_deferred"]=v316_ai_deferred
 stats["v316_active_boss_phase_cores"]=v316_boss_aura_visible
 return stats
