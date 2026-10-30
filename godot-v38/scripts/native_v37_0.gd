extends "res://scripts/native_v36_3.gd"
## v37 Living Realm. Reuses the six existing water basins, material inventory,
## saved gathering site-day registry, world rifts and invasion logic. No new mode.
const V37_RULES:=preload("res://scripts/systems/living_realm_v37.gd")
var v37_swim_zone:int=-1
var v37_swimming:bool=false
var v37_swim_elapsed:float=0.0
var v37_realm_elapsed:float=0.0
var v37_wildlife:Array[Dictionary]=[]
var v37_wakes:Array[MeshInstance3D]=[]
var v37_ward_lights:Array[MeshInstance3D]=[]
# -1 means the initial state must be applied; later only changes update the GPU.
var v37_ward_clean_states:Array[int]=[]
var v37_world_events_reacted:int=0
var v37_wildlife_visual_updates:int=0
var v37_profession_contracts:int=0

func _ready()->void:
 super._ready()
 _v37_build_realm()
 _v37_refresh_quality()

func _v37_build_realm()->void:
 if region_nodes.size()<V37_RULES.REGION_COUNT or not v362_water_ready:return
 var critter_mesh:=SphereMesh.new()
 critter_mesh.radius=0.42
 critter_mesh.height=0.78
 critter_mesh.radial_segments=8
 critter_mesh.rings=4
 var ears:=CylinderMesh.new()
 ears.top_radius=0.06
 ears.bottom_radius=0.14
 ears.height=0.52
 ears.radial_segments=5
 var tint_a:=_v361_material(Color("a3a58a"))
 var tint_b:=_v361_material(Color("7a9a9a"))
 var ear_mat:=_v361_material(Color("d9bb9b"))
 for zone in range(V37_RULES.REGION_COUNT):
  var at:Vector2=REGIONS[zone]["at"]
  var water_offset:Vector2=V362_JOBS.initial_water_offset(zone)
  var base:Vector3=Vector3(at.x+water_offset.x+10.0,0.0,at.y+water_offset.y+3.0)
  var wildlife_root:=Node3D.new()
  wildlife_root.name="V37_RegionWildlife_%d"%zone
  region_nodes[zone].add_child(wildlife_root)
  for slot in range(V37_RULES.WILDLIFE_PER_REGION):
   var creature:=Node3D.new()
   creature.name="Moonhare_%d_%d"%[zone,slot]
   wildlife_root.add_child(creature)
   var body:=_v361_primitive(creature,critter_mesh,tint_a if slot==0 else tint_b,Vector3(0.0,0.48,0.0),Vector3(1.35,0.85,0.86))
   body.name="V37_PassiveWildlifeMesh"
   _v361_primitive(creature,ears,ear_mat,Vector3(-0.24,1.02,0.0))
   _v361_primitive(creature,ears,ear_mat,Vector3(0.24,1.02,0.0))
   var origin:Vector3=base+Vector3(float(slot)*2.0,0.0,float(slot)*3.0)
   creature.position=origin
   v37_wildlife.append({"zone":zone,"node":creature,"origin":origin,"slot":slot,"fright":0.0})
  # Reuse GPU-friendly torus geometry for a small, non-colliding swim wake.
  var ring:=_v361_primitive(v362_water_roots[zone],v361_ring_mesh,_v361_material(Color("78d4f1"),true),Vector3.ZERO,Vector3(0.75,0.045,0.75))
  ring.name="V37_SurfaceSwimWake"
  ring.visible=false
  v37_wakes.append(ring)
  # An existing purified rift/invasion visibly affects its regional ward beacon.
  var beacon:=_v361_primitive(region_nodes[zone],v361_orb_mesh,_v361_material(Color("8fdec4"),true),Vector3(at.x+5.0,2.8,at.y+4.0),Vector3(0.55,0.7,0.55))
  beacon.name="V37_RealmWardBeacon_%d"%zone
  v37_ward_lights.append(beacon)
  v37_ward_clean_states.append(-1)

func _v37_refresh_quality()->void:
 var detailed:bool=V361_ART.details_enabled(gfx_settings,graphics_quality)
 for animal in v37_wildlife:
  var node:Node3D=animal["node"]
  if is_instance_valid(node):node.visible=detailed
 for beacon in v37_ward_lights:
  if is_instance_valid(beacon):beacon.visible=detailed
 # Water and interactive gathering markers remain visible on Smooth.

func _gfx_apply(apply_display:bool=true)->void:
 super._gfx_apply(apply_display)
 _v37_refresh_quality()

func _physics_process(delta:float)->void:
 v37_swim_zone=-1
 v37_swimming=false
 if hero_playing and not death_active and not mounted and is_instance_valid(player):
  v37_swim_zone=V37_RULES.swim_zone(player.global_position.x,player.global_position.z,REGIONS,player.global_position.y)
  v37_swimming=v37_swim_zone>=0
 super._physics_process(delta)
 # Swim-zone detection remains the source of truth for the optional v37.2 dive.
 # Existing terrain collision and hero artwork remain unchanged.

func _map_movement_to_camera(raw_direction:Vector3)->Vector3:
 var result:Vector3=super._map_movement_to_camera(raw_direction)
 return result*V37_RULES.SWIM_SPEED_MULTIPLIER if v37_swimming else result

func _process(delta:float)->void:
 super._process(delta)
 v37_realm_elapsed+=minf(delta,0.15)
 if v37_realm_elapsed<V37_RULES.TICK_INTERVAL:return
 var step:float=v37_realm_elapsed
 v37_realm_elapsed=0.0
 v37_swim_elapsed+=step
 _v37_tick_realm()

func _v37_tick_realm()->void:
 if not is_instance_valid(player):return
 var at:Vector3=player.global_position
 for index in range(v37_wakes.size()):
  var ring:MeshInstance3D=v37_wakes[index]
  if not is_instance_valid(ring):continue
  var active:bool=hero_playing and not death_active and not menu_open and v37_swimming and index==v37_swim_zone
  ring.visible=active
  if active:
   ring.global_position=Vector3(at.x,0.14,at.z)
   var pulse:float=0.83+0.08*sin(v37_swim_elapsed*6.5)
   ring.scale=Vector3(pulse,0.045,pulse)
 var detailed:bool=V361_ART.details_enabled(gfx_settings,graphics_quality)
 for item in v37_wildlife:
  if int(item["zone"])!=region_index or not detailed:continue
  var creature:Node3D=item["node"]
  if not is_instance_valid(creature):continue
  var zone:int=int(item["zone"])
  var available:bool=V37_RULES.wildlife_available(zone,world_day,water_site_days)
  creature.visible=available
  if not available:continue
  var origin:Vector3=item["origin"]
  var fright:float=2.1 if at.distance_squared_to(creature.global_position)<20.0 else 0.0
  item["fright"]=move_toward(float(item["fright"]),fright,0.17)
  creature.position=origin+V37_RULES.wildlife_offset(zone,int(item["slot"]),v37_swim_elapsed,float(item["fright"]))
  creature.rotation.y=v37_swim_elapsed*0.6
  v37_wildlife_visual_updates+=1
 for zone in range(v37_ward_lights.size()):
  var beacon:MeshInstance3D=v37_ward_lights[zone]
  if not is_instance_valid(beacon):continue
  var clean:bool=V37_RULES.purified(zone,rifts,v341_active_site,v341_sites)
  var state:int=1 if clean else 0
  if zone>=v37_ward_clean_states.size() or v37_ward_clean_states[zone]==state:continue
  v37_ward_clean_states[zone]=state
  var shade:Color=Color("8fdec4") if clean else Color("a34baf")
  beacon.material_override=_v361_material(shade,true)
  if not clean:v37_world_events_reacted+=1

func _v37_defended_here()->bool:
 if v341_active_site>=0 and v341_active_site<v341_sites.size():
  if int(v341_sites[v341_active_site].get("zone",-1))==region_index:return false
 for entry in rifts:
  if int(entry.get("zone",-1))==region_index and bool(entry.get("cleared",false)):return true
 return false

func _apply_incoming_damage(raw:float)->void:
 # Defending and purifying a real rift grants a small, persistent region bonus.
 super._apply_incoming_damage(V37_RULES.ward_damage(raw,_v37_defended_here()))

func _v37_near_animal()->Dictionary:
 if not is_instance_valid(player):return {}
 var best:float=10.0
 var nearby:Dictionary={}
 for animal in v37_wildlife:
  if int(animal["zone"])!=region_index:continue
  var node:Node3D=animal["node"]
  if not is_instance_valid(node) or not V37_RULES.wildlife_available(int(animal["zone"]),world_day,water_site_days):continue
  var distance:float=player.global_position.distance_squared_to(node.global_position)
  if distance<best:
   best=distance
   nearby=animal
 return nearby

func _interact()->void:
 # Never steal the interaction from nearby ore, fish, or herb sites.
 if _v362_nearest_site().is_empty() and hero_playing and not death_active and not mounted and not _combat_threat():
  var animal:Dictionary=_v37_near_animal()
  if not animal.is_empty():
   var zone:int=int(animal["zone"])
   water_site_days[str(V37_RULES.WILDLIFE_SLOT_START+zone)]=world_day
   _gain_material("Fine Hide",2)
   skinning_skill=mini(300,skinning_skill+1)
   _show_message("Tracked wildlife: +2 Fine Hide • Skinning skill improved. Returns tomorrow.",2.5)
   _save_game()
   return
 super._interact()

func _v37_contract(zone:int)->void:
 if not hero_playing or death_active or mounted or _combat_threat():
  _show_message("Complete gathering contracts outside combat.",2.0)
  return
 if zone<0 or zone>=V37_RULES.REGION_COUNT or V37_RULES.contract_complete(zone,water_site_days):return
 var cost:Dictionary=V37_RULES.contract_cost(zone)
 var missing:String=V37_RULES.missing(cost,materials)
 if missing!="":
  _show_message("Gather more "+missing+" to complete this contract.",2.2)
  return
 V37_RULES.pay(cost,materials)
 water_site_days[str(V37_RULES.CONTRACT_SLOT_START+zone)]=world_day
 _gain_material("Moon Sigil",1)
 gold+=80+zone*20
 water_professions["Herbalism"]=mini(300,int(water_professions.get("Herbalism",1))+1)
 v37_profession_contracts+=1
 _show_message("Regional gathering contract delivered: Moon Sigil + gold.",2.3)
 _save_game()

func _v37_craft(index:int)->void:
 if not hero_playing or death_active or mounted or _combat_threat():
  _show_message("Craft when out of combat.",1.8)
  return
 if index<0 or index>=V37_RULES.RECIPES.size():return
 var recipe:Dictionary=V37_RULES.RECIPES[index]
 var issue:String=V363_RULES.recipe_issue(recipe,materials,gear_items.size(),bag_capacity)
 if issue!="":
  _show_message(issue,1.8)
  return
 V37_RULES.pay(recipe["cost"],materials)
 if int(recipe["power"])>0:
  gear_items.append({"name":recipe["result"],"rarity":"Crafted","power":recipe["power"]})
 else:
  _gain_material(str(recipe["result"]),1)
 var job:String=str(recipe["job"])
 water_professions[job]=mini(300,int(water_professions.get(job,1))+1)
 _show_message("Crafted "+str(recipe["name"])+"!",2.1)
 _save_game()

func _v37_consume(item:String)->void:
 if not hero_playing or death_active or mounted or _combat_threat():return
 var fraction:float=0.40 if item=="Riverguard Salve" else (0.30 if item=="Marshside Provisions" else 0.0)
 if fraction<=0.0 or hp>=max_hp:return
 if not INVENTORY_RULES.consume_one(materials,item):
  _show_message("No "+item+" available.",1.6)
  return
 hp=minf(max_hp,hp+max_hp*fraction)
 _show_message("Restored health with "+item+".",1.8)
 _save_game()

func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if not is_instance_valid(menu_body):return
 if which=="World":
  menu_body.text+="\n\n[b]LIVING REALM v37[/b]\nThe six lake centers support surface swimming and hold-to-dive oxygen in v37.2. Wildlife roams, flees approaching heroes and returns after a world-day harvest. Existing rift purification and invasion state change the regional ward beacons."
  menu_body.text+="\nWater traversal: "+("SWIMMING" if v37_swimming else "On land")
  menu_body.text+="\nGathering contracts completed: "+str(_v37_completed_contract_count())+"/6"
  menu_body.text+="\nPurified regional ward: "+("DEFENDING (-5% incoming damage)" if _v37_defended_here() else "Inactive")
 if which=="Professions":
  menu_body.text+="\n\n[b]LIVING REALM RECIPES[/b]"
  for index in range(V37_RULES.RECIPES.size()):
   var recipe:Dictionary=V37_RULES.RECIPES[index]
   var issue:String=V363_RULES.recipe_issue(recipe,materials,gear_items.size(),bag_capacity)
   menu_body.text+="\n"+str(recipe["name"])+" • "+str(recipe["cost"])+(" [READY]" if issue=="" else " ["+issue+"]")
   _atlas_action("CRAFT: "+str(recipe["name"]),"v37:craft:"+str(index))
  _atlas_action("USE RIVERGUARD SALVE (+40% HP)","v37:use:Riverguard Salve")
  _atlas_action("EAT MARSHSIDE PROVISIONS (+30% HP)","v37:use:Marshside Provisions")
  menu_body.text+="\n\n[b]REGIONAL GATHERING CONTRACTS[/b]\nOne delivery per region, using your existing Fishing, Mining, and Herbalism materials. Rewards use existing Moon Sigils and gold."
  for zone in range(V37_RULES.REGION_COUNT):
   if V37_RULES.contract_complete(zone,water_site_days):continue
   _atlas_action("DELIVER: "+str(REGIONS[zone]["name"]),"v37:contract:"+str(zone),str(V37_RULES.contract_cost(zone)))

func _v37_completed_contract_count()->int:
 var count:int=0
 for zone in range(V37_RULES.REGION_COUNT):
  if V37_RULES.contract_complete(zone,water_site_days):count+=1
 return count

func _atlas_execute(action:String)->void:
 if action.begins_with("v37:craft:"):
  _v37_craft(int(action.trim_prefix("v37:craft:")))
  _menu_tab("Professions")
  return
 if action.begins_with("v37:contract:"):
  _v37_contract(int(action.trim_prefix("v37:contract:")))
  _menu_tab("Professions")
  return
 if action.begins_with("v37:use:"):
  _v37_consume(action.trim_prefix("v37:use:"))
  _menu_tab("Professions")
  return
 super._atlas_execute(action)

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["living_realm"]="v37 bounded surface swimming + wildlife + professions"
 report["surface_swimming"]=v37_swimming
 report["passive_wildlife"]=v37_wildlife.size()
 report["wildlife_updates"]=v37_wildlife_visual_updates
 report["completed_profession_contracts"]=_v37_completed_contract_count()
 report["regional_wards"]=v37_ward_lights.size()
 return report
