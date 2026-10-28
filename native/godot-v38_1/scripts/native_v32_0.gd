extends "res://scripts/native_v31_6.gd"
## World Reborn v32.0: connected-world decorative stream, world wardens,
## boss cues and pooled impacts. No save schema, hero art or collision changes.
const V320_RULES:=preload("res://scripts/systems/world_reborn_rules.gd")
const V320_STREAM_TICK:float=0.30
const V320_IMPACT_POOL:int=8
var v320_decor:Array[MultiMeshInstance3D]=[]
var v320_wardens:Array[Dictionary]=[]
var v320_boss_cues:Array[Dictionary]=[]
var v320_impacts:Array[Dictionary]=[]
var v320_stream_clock:float=0.0
var v320_stream_visible:int=0
var v320_stream_changes:int=0
var v320_stream_usec:int=0
var v320_stream_peak_usec:int=0
var v320_impacts_triggered:int=0
var v320_warden_interactions:int=0
var v320_boss_warnings:int=0
var v320_impact_index:int=0

func _release_label()->String:return "v32.0"

func _build_scene()->void:
 super._build_scene()
 _v320_create_streamed_scenery()
 _v320_create_wardens()
 _v320_create_impact_pool()

func _v320_create_streamed_scenery()->void:
 # Six instanced batches. Only decoration is distance-hidden, never colliders,
 # creature AI, road meshes, or the continuous ground. No separate world modes.
 var mesh:=CylinderMesh.new()
 mesh.top_radius=0.05
 mesh.bottom_radius=0.19
 mesh.height=0.62
 mesh.radial_segments=4
 for zone in range(REGIONS.size()):
  var mm:=MultiMesh.new()
  mm.transform_format=MultiMesh.TRANSFORM_3D
  mm.mesh=mesh
  mm.instance_count=42
  var generator:=RandomNumberGenerator.new()
  generator.seed=320000+zone*317
  for i in range(mm.instance_count):
   var x:=generator.randf_range(-26.5,26.5)
   var z:=generator.randf_range(-25.0,25.0)
   # Leave central roads and interactable shrines clear.
   if absf(x)<4.5:x+=6.0 if x>=0.0 else -6.0
   if absf(z)<3.5:z+=5.0 if z>=0.0 else -5.0
   var scale_value:=generator.randf_range(0.55,1.45)
   var basis:=Basis().rotated(Vector3.UP,generator.randf_range(0.0,TAU)).scaled(Vector3(scale_value,scale_value,scale_value))
   mm.set_instance_transform(i,Transform3D(basis,Vector3(x,0.31*scale_value,z)))
  var inst:=MultiMeshInstance3D.new()
  inst.name="V320_DecorBatch_%d"%zone
  inst.multimesh=mm
  inst.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
  var mat:=StandardMaterial3D.new()
  mat.albedo_color=Color(str(REGIONS[zone]["ground"])).lightened(0.15)
  mat.roughness=1.0
  inst.material_override=mat
  var center:Vector2=REGIONS[zone]["at"]
  inst.position=Vector3(center.x,0.0,center.y)
  inst.visible=false
  world.add_child(inst)
  v320_decor.append(inst)

func _v320_create_wardens()->void:
 # NPC quest-givers use the already persisted six-region contract system.
 # No new progression fields: reputation derives from saved contracts/rifts.
 var body_mesh:=CapsuleMesh.new()
 body_mesh.radius=0.37
 body_mesh.height=1.6
 var head_mesh:=SphereMesh.new()
 head_mesh.radius=0.32
 head_mesh.height=0.64
 for zone in range(REGIONS.size()):
  var center:Vector2=REGIONS[zone]["at"]
  var holder:=Node3D.new()
  holder.name="V320_RegionalWarden_%d"%zone
  holder.position=Vector3(center.x-4.0,0.0,center.y+4.0)
  world.add_child(holder)
  var figure:=MeshInstance3D.new()
  figure.mesh=body_mesh
  figure.position.y=0.9
  var robe:=StandardMaterial3D.new()
  robe.albedo_color=Color(str(REGIONS[zone]["light"])).darkened(0.54)
  robe.roughness=0.93
  figure.material_override=robe
  figure.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
  holder.add_child(figure)
  var head:=MeshInstance3D.new()
  head.mesh=head_mesh
  head.position.y=1.93
  var helm:=StandardMaterial3D.new()
  helm.albedo_color=Color("c5b5a2")
  helm.roughness=0.8
  head.material_override=helm
  holder.add_child(head)
  var sign:=Label3D.new()
  sign.text="WARDEN • G TO TALK"
  sign.position.y=2.8
  sign.font_size=34
  sign.pixel_size=0.007
  sign.billboard=BaseMaterial3D.BILLBOARD_ENABLED
  sign.modulate=Color(str(REGIONS[zone]["light"]))
  holder.add_child(sign)
  v320_wardens.append({"node":holder,"zone":zone,"label":sign})

func _v320_create_impact_pool()->void:
 var orb:=SphereMesh.new()
 orb.radius=0.26
 orb.height=0.52
 var effect:=StandardMaterial3D.new()
 effect.shading_mode=BaseMaterial3D.SHADING_MODE_UNSHADED
 effect.albedo_color=Color("ffd19a")
 effect.emission_enabled=true
 effect.emission=Color("ff9f52")
 for i in range(V320_IMPACT_POOL):
  var pulse:=MeshInstance3D.new()
  pulse.name="V320_Impact_%d"%i
  pulse.mesh=orb
  pulse.material_override=effect
  pulse.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
  pulse.visible=false
  world.add_child(pulse)
  v320_impacts.append({"node":pulse,"expires":-1.0})

func _spawn_beast(at:Vector3,kind:String,zone:int,variant:String="normal",boss:bool=false)->void:
 var before:int=beasts.size()
 super._spawn_beast(at,kind,zone,variant,boss)
 if not boss or beasts.size()<=before:return
 var creature:Dictionary=beasts[-1]
 var host:CharacterBody3D=creature["body"]
 var indicator:=MeshInstance3D.new()
 indicator.name="V320_BossWindupHalo"
 var ring:=CylinderMesh.new()
 ring.top_radius=1.35
 ring.bottom_radius=1.35
 ring.height=0.035
 ring.radial_segments=20
 indicator.mesh=ring
 var mat:=StandardMaterial3D.new()
 mat.albedo_color=Color("9c3359")
 mat.emission_enabled=true
 mat.emission=Color("a8244c")
 mat.shading_mode=BaseMaterial3D.SHADING_MODE_UNSHADED
 indicator.material_override=mat
 indicator.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
 indicator.position.y=0.16
 indicator.visible=false
 host.add_child(indicator)
 v320_boss_cues.append({"creature":creature,"node":indicator,"was_warning":false})

func _apply_player_hit(target:Dictionary,base_damage:float,stagger_strength:float=0.0)->void:
 var prior:float=float(target.get("hp",0.0))
 super._apply_player_hit(target,base_damage,stagger_strength)
 if float(target.get("hp",0.0))>=prior or v320_impacts.is_empty():return
 var entry:Dictionary=v320_impacts[v320_impact_index]
 v320_impact_index=(v320_impact_index+1)%V320_IMPACT_POOL
 var actor:Node3D=target.get("body")
 var pulse:MeshInstance3D=entry["node"]
 if is_instance_valid(actor):pulse.global_position=actor.global_position+Vector3(0.0,1.1,0.0)
 pulse.visible=true
 pulse.scale=Vector3.ONE
 entry["expires"]=map_time+0.23
 v320_impacts_triggered+=1

func _interact()->void:
 # Existing chests, rifts, drops and shrines retain precedence whenever nearby.
 if hero_playing and not mounted and is_instance_valid(player):
  var closest:Dictionary={}
  var best:float=V320_RULES.WARDEN_REACH_SQ
  for warden in v320_wardens:
   var distance_sq:float=player.global_position.distance_squared_to((warden["node"] as Node3D).global_position)
   if distance_sq<best:
    best=distance_sq
    closest=warden
  if not closest.is_empty():
   var zone:int=int(closest["zone"])
   v320_warden_interactions+=1
   if _combat_threat():
    _show_message("The warden will talk after combat.",2)
   elif V320_RULES.contract_available(zone,contracts_done,contract_zone):
    _take_contract(zone)
   elif zone in contracts_done:
    _show_message("Warden: Our region is safer. Your standing: "+V320_RULES.standing(contracts_done,rifts_cleared.size()),3)
   else:
    _show_message("Complete your current contract first.",2)
   return
 super._interact()

func _v320_stream_update()->void:
 if not is_instance_valid(player):return
 var begin:int=Time.get_ticks_usec()
 var here:Vector3=player.global_position
 v320_stream_visible=0
 var low:bool=graphics_quality=="Smooth" or str(gfx_settings.get("ground_clutter","High"))=="Off" or str(gfx_settings.get("foliage_quality","High"))=="Off" or int(gfx_settings.get("environment_detail","6"))<=2
 for i in range(v320_decor.size()):
  var center:Vector2=REGIONS[i]["at"]
  var distance_sq:float=Vector2(here.x-center.x,here.z-center.y).length_squared()
  var batch:MultiMeshInstance3D=v320_decor[i]
  var show:bool=hero_playing and V320_RULES.visible_at(distance_sq,batch.visible,low)
  if show!=batch.visible:
   batch.visible=show
   v320_stream_changes+=1
  if show:v320_stream_visible+=1
 for warden in v320_wardens:
  var holder:Node3D=warden["node"]
  holder.visible=hero_playing and holder.global_position.distance_squared_to(here)<50.0*50.0
 for cue in v320_boss_cues:
  var creature:Dictionary=cue["creature"]
  var sign:MeshInstance3D=cue["node"]
  var actor:Node3D=creature["body"]
  var active:bool=hero_playing and bool(creature.get("alive",false)) and is_instance_valid(actor) and actor.visible and float(creature.get("windup",0.0))>0.0
  sign.visible=active
  if active and not bool(cue["was_warning"]):v320_boss_warnings+=1
  cue["was_warning"]=active
 for item in v320_impacts:
  var hit:MeshInstance3D=item["node"]
  hit.visible=hero_playing and map_time<float(item["expires"])
  if hit.visible:
   var t:float=clampf((float(item["expires"])-map_time)/0.23,0.0,1.0)
   hit.scale=Vector3.ONE*maxf(0.1,1.0-t*0.55)
 v320_stream_usec=Time.get_ticks_usec()-begin
 v320_stream_peak_usec=maxi(v320_stream_peak_usec,v320_stream_usec)

func _process(delta:float)->void:
 super._process(delta)
 v320_stream_clock+=clampf(delta,0.0,0.2)
 if v320_stream_clock>=V320_STREAM_TICK:
  v320_stream_clock=fmod(v320_stream_clock,V320_STREAM_TICK)
  _v320_stream_update()

func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if which=="World" and is_instance_valid(menu_body):
  menu_body.text+="\n\n[b]WORLD REBORN • WARDEN NETWORK[/b]\nMeet a warden near each regional center and press G to accept that region's existing hunting contract.\nYour standing: "+V320_RULES.standing(contracts_done,rifts_cleared.size())+".\nContracts completed: "+str(contracts_done.size())+"/6. Rift seals: "+str(rifts_cleared.size())+"/3.\nScenery streams visually as you explore. Roads, terrain, collision and encounters remain connected."

func _reset_new_adventure()->void:
 super._reset_new_adventure()
 v320_stream_clock=V320_STREAM_TICK
 for item in v320_impacts:
  (item["node"] as MeshInstance3D).visible=false
  item["expires"]=-1.0
 for cue in v320_boss_cues:
  (cue["node"] as MeshInstance3D).visible=false
  cue["was_warning"]=false

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["version"]="v32.0"
 report["world_streamed_decor_batches"]=v320_decor.size()
 report["world_streamed_decor_visible"]=v320_stream_visible
 report["world_stream_visibility_switches"]=v320_stream_changes
 report["world_stream_update_usec"]=v320_stream_usec
 report["world_stream_peak_usec"]=v320_stream_peak_usec
 report["regional_npc_wardens"]=v320_wardens.size()
 report["warden_interactions"]=v320_warden_interactions
 report["boss_windup_warning_transitions"]=v320_boss_warnings
 report["pooled_impact_effects"]=v320_impacts.size()
 report["impact_effects_triggered"]=v320_impacts_triggered
 return report
