extends "res://scripts/native_v32_0.gd"
## World Reborn Stage 2: three visual chambers per original world-connected
## rift, chamber-specific reused enemies, final champion and themed relics.
## Never move terrain, change collision, modify hero art, or alter save schema.
const V321_RULES:=preload("res://scripts/systems/dungeon_stage2_rules.gd")
const V321_VISUAL_INTERVAL:float=0.50
var v321_chambers:Array[Dictionary]=[]
var v321_chamber_clock:float=0.0
var v321_wave_starts:int=0
var v321_champion_spawns:int=0
var v321_bonus_relics:int=0
var v321_chamber_checks:int=0
var v321_champion_halo:Dictionary={}

func _release_label()->String:return "v32.1"

func _build_scene()->void:
 super._build_scene()
 _v321_build_chambers()

func _v321_build_chambers()->void:
 var shared_floor:=CylinderMesh.new()
 shared_floor.top_radius=4.1
 shared_floor.bottom_radius=4.1
 shared_floor.height=0.055
 shared_floor.radial_segments=18
 var shared_pillar:=CylinderMesh.new()
 shared_pillar.top_radius=0.24
 shared_pillar.bottom_radius=0.38
 shared_pillar.height=2.2
 shared_pillar.radial_segments=6
 var shared_cap:=SphereMesh.new()
 shared_cap.radius=0.28
 shared_cap.height=0.56
 for site_id in range(rifts.size()):
  var entry:Dictionary=rifts[site_id]
  var zone:int=int(entry["zone"])
  var color:Color=Color(str(REGIONS[zone]["light"]))
  var floor_material:=StandardMaterial3D.new()
  floor_material.albedo_color=color.darkened(0.72)
  floor_material.roughness=0.95
  var stone_material:=StandardMaterial3D.new()
  stone_material.albedo_color=color.darkened(0.63)
  stone_material.roughness=0.90
  var glow_material:=StandardMaterial3D.new()
  glow_material.shading_mode=BaseMaterial3D.SHADING_MODE_UNSHADED
  glow_material.albedo_color=color
  glow_material.emission_enabled=true
  glow_material.emission=color
  for stage in range(1,4):
   var root:=Node3D.new()
   root.name="V321_Rift_%d_Chamber_%d"%[site_id,stage]
   root.position=V321_RULES.room_position(entry["anchor"],stage)
   world.add_child(root)
   var floor_mesh:=MeshInstance3D.new()
   floor_mesh.name="RiftChamberFloor"
   floor_mesh.mesh=shared_floor
   floor_mesh.material_override=floor_material
   floor_mesh.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
   floor_mesh.position.y=0.04
   root.add_child(floor_mesh)
   for p in range(3):
    var angle:float=TAU*float(p)/3.0
    var pos:=Vector3(cos(angle)*4.0,1.10,sin(angle)*4.0)
    var post:=MeshInstance3D.new()
    post.name="RiftPillar_%d"%p
    post.mesh=shared_pillar
    post.material_override=stone_material
    post.position=pos
    post.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
    root.add_child(post)
    var tip:=MeshInstance3D.new()
    tip.mesh=shared_cap
    tip.material_override=glow_material
    tip.position=pos+Vector3(0,1.1,0)
    tip.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
    root.add_child(tip)
   var sign:=Label3D.new()
   sign.name="RiftChamberStage"
   sign.text="CHAMBER %d • %s"%[stage,V321_RULES.chamber_title(stage)]
   sign.font_size=29
   sign.pixel_size=0.005
   sign.position.y=2.8
   sign.billboard=BaseMaterial3D.BILLBOARD_ENABLED
   sign.modulate=color
   root.add_child(sign)
   root.visible=false
   v321_chambers.append({"node":root,"site":site_id,"stage":stage,"label":sign})

func _activate_rift_wave(site_id:int)->void:
 super._activate_rift_wave(site_id)
 if site_id<0 or site_id>=rifts.size():return
 var entry:Dictionary=rifts[site_id]
 var stage:int=clampi(int(entry["stage"]),1,3)
 var slots:Array=entry["slots"]
 for i in range(slots.size()):
  var enemy:Dictionary=beasts[int(slots[i])]
  var body:CharacterBody3D=enemy["body"]
  var home:Vector3=V321_RULES.enemy_position(entry["anchor"],stage,i)
  enemy["home"]=home
  body.position=home+Vector3(0,0.62,0)
  if stage==3 and i==2:
   enemy["max"]=V321_RULES.final_champion_health(float(enemy["max"]),stage,i)
   enemy["hp"]=enemy["max"]
   enemy["dungeon_champion"]=true
   var nameplate:Label3D=enemy["nameplate"]
   if is_instance_valid(nameplate):
    nameplate.text="RIFT CHAMPION • "+str(entry["name"])
    nameplate.modulate=Color("ffcb76")
   v321_champion_spawns+=1
  else:enemy["dungeon_champion"]=false
 v321_wave_starts+=1
 _v321_update_chambers()
 _show_message(str(entry["name"])+" • "+V321_RULES.chamber_title(stage),2.5)

func _on_rift_enemy_defeated(beast:Dictionary)->void:
 var site_id:int=int(beast.get("dungeon_site",-1))
 var valid:bool=site_id>=0 and site_id<rifts.size()
 var already_cleared:bool=bool(rifts[site_id].get("cleared",false)) if valid else true
 super._on_rift_enemy_defeated(beast)
 if not valid:return
 var entry:Dictionary=rifts[site_id]
 if not already_cleared and bool(entry["cleared"]):
  # Exactly one bonus set-themed legendary per rift, claimed via the base
  # persistent 'rifts_cleared' flag. The existing save writer owns persistence.
  var zone:int=int(entry["zone"])
  var reward:Dictionary=V321_RULES.named_relic(site_id,zone,AFFIXES[site_id%AFFIXES.size()])
  _create_gear_drop(entry["anchor"],"Rift Champion","elder",zone,reward)
  v321_bonus_relics+=1
 _v321_update_chambers()

func _v321_update_chambers()->void:
 if not is_instance_valid(player):return
 var here:Vector3=player.global_position
 var quality_low:bool=graphics_quality=="Smooth" or int(gfx_settings.get("environment_detail",6))<=2
 for chamber in v321_chambers:
  var site:int=int(chamber["site"])
  var stage:int=int(chamber["stage"])
  var entry:Dictionary=rifts[site]
  var root:Node3D=chamber["node"]
  var close:bool=V321_RULES.room_visible(root.global_position.distance_squared_to(here),quality_low)
  root.visible=hero_playing and close
  var label:Label3D=chamber["label"]
  if bool(entry.get("cleared",false)):
   label.text="✓ "+V321_RULES.chamber_title(stage)+" • SEALED"
  elif bool(entry.get("active",false)):
   label.text=("▶ " if int(entry.get("stage",0))==stage else "◦ ")+V321_RULES.chamber_title(stage)
  else:label.text="CHAMBER %d • %s"%[stage,V321_RULES.chamber_title(stage)]
  v321_chamber_checks+=1

func _process(delta:float)->void:
 super._process(delta)
 v321_chamber_clock+=clampf(delta,0.0,0.2)
 if v321_chamber_clock>=V321_VISUAL_INTERVAL:
  v321_chamber_clock=fmod(v321_chamber_clock,V321_VISUAL_INTERVAL)
  _v321_update_chambers()

func _restore_data(data:Dictionary)->void:
 super._restore_data(data)
 _v321_update_chambers()

func _reset_new_adventure()->void:
 super._reset_new_adventure()
 v321_chamber_clock=V321_VISUAL_INTERVAL
 _v321_update_chambers()

func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if which=="World" and is_instance_valid(menu_body):
  menu_body.text+="\n\n[b]WORLD REBORN • THREE-CHAMBER RIFTS[/b]\nEach of the three existing world rifts now contains an Outer Sanctum, Moonbound Passage and Guardian Heart. Clear enemies to advance chambers, then defeat the Rift Champion for an additional named legendary. Rift completion is saved using the existing checkpoint."

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["version"]="v32.1"
 report["dungeon_chambers"]=v321_chambers.size()
 report["dungeon_waves_started"]=v321_wave_starts
 report["dungeon_champions_spawned"]=v321_champion_spawns
 report["dungeon_bonus_relics"]=v321_bonus_relics
 report["dungeon_culling_checks"]=v321_chamber_checks
 return report
