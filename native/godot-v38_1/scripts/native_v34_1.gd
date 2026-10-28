extends "res://scripts/native_v34_0.gd"
## v34.1 RIFT-inspired invasion ecology integrated into Moonfall's one world.
## Event progression is driven by enemy defeats, NEVER a survival timer.
## Three dormant, preallocated squads share one global active-invasion budget.
const V341_RULES:=preload("res://scripts/systems/lunar_invasions_v341.gd")
var v341_sites:Array[Dictionary]=[]
var v341_pressure:Dictionary={}
var v341_active_site:int=-1
var v341_wave:int=0
var v341_invasions_completed:int=0
var v341_commanders_defeated:int=0
var v341_marker_clock:float=0.0

func _release_label()->String:return "v34.1 • Living Lunar Invasions"

func _build_scene()->void:
 super._build_scene()
 _v341_prepare_invasions()

func _v341_prepare_invasions()->void:
 # Prebuild actors once and reuse them across all waves. Only three can be
 # active at once, and only one invasion can be active in the whole world.
 var portal_mesh:=CylinderMesh.new()
 portal_mesh.top_radius=2.0
 portal_mesh.bottom_radius=2.0
 portal_mesh.height=0.05
 portal_mesh.radial_segments=28
 var portal_mat:=StandardMaterial3D.new()
 portal_mat.shading_mode=BaseMaterial3D.SHADING_MODE_UNSHADED
 portal_mat.emission_enabled=true
 portal_mat.emission=Color(0.55,0.08,0.87)
 portal_mat.albedo_color=Color(0.47,0.12,0.78,0.82)
 for site in range(V341_RULES.REGIONS.size()):
  var zone:int=V341_RULES.REGIONS[site]
  var anchor:Vector3=V341_RULES.invasion_anchor(rifts[site]["anchor"],site)
  var portal:=Node3D.new()
  portal.name="LunarInvasionPortal_%d"%site
  portal.position=anchor
  world.add_child(portal)
  var disk:=MeshInstance3D.new()
  disk.mesh=portal_mesh
  disk.material_override=portal_mat
  disk.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
  disk.position.y=0.07
  portal.add_child(disk)
  var sign:=Label3D.new()
  sign.font_size=35
  sign.pixel_size=0.006
  sign.billboard=BaseMaterial3D.BILLBOARD_ENABLED
  sign.position.y=1.95
  sign.modulate=Color("f4c1ff")
  sign.text=V341_RULES.event_name(site)
  portal.add_child(sign)
  portal.visible=false
  var roster:Array[int]=[]
  var kind:String=str(REGIONS[zone]["monster"])
  for slot in range(V341_RULES.SOLDIER_SLOTS+1):
   var angle:float=float(slot)*TAU/4.0
   var at:Vector3=anchor+Vector3(cos(angle)*5.2,0,sin(angle)*5.2)
   var leader:bool=slot==V341_RULES.SOLDIER_SLOTS
   var before:int=beasts.size()
   _spawn_beast(at,kind,zone,"elder" if leader else "dusk" if site==1 else "moon",leader)
   if beasts.size()==before:continue
   var creature:Dictionary=beasts[-1]
   creature["invasion_site"]=site
   creature["invasion_slot"]=slot
   creature["invasion_commander"]=leader
   # Use dungeon reservation to prevent normal wildlife respawn and dormant
   # wildlife serialization. A negative id is never a real dungeon slot.
   creature["dungeon_site"]=-2
   _v341_dormant(creature)
   roster.append(int(creature["boss_id"]))
  v341_sites.append({"zone":zone,"anchor":anchor,"portal":portal,"sign":sign,"roster":roster})
  v341_pressure[site]=0

func _v341_dormant(creature:Dictionary)->void:
 _despawn_native_beast(creature,0.0)
 creature["respawn_at"]=-1.0

func _v341_roster_valid(site:int)->bool:
 # Fail closed if monster allocation changes or a spawn failed. Otherwise an
 # incomplete invasion can be unfinishable or can award loot with no commander.
 if site<0 or site>=v341_sites.size():return false
 var ids:Array=v341_sites[site]["roster"]
 if ids.size()!=V341_RULES.SOLDIER_SLOTS+1:return false
 var seen:Dictionary={}
 var commanders:int=0
 for raw_id in ids:
  var id:int=int(raw_id)
  if id<0 or id>=beasts.size() or seen.has(id):return false
  seen[id]=true
  var beast:Dictionary=beasts[id]
  if int(beast.get("invasion_site",-1))!=site:return false
  if bool(beast.get("invasion_commander",false)):commanders+=1
  var body:CharacterBody3D=beast.get("body")
  if not is_instance_valid(body):return false
 return commanders==1

func _v341_start(site:int)->void:
 if site<0 or site>=v341_sites.size() or v341_active_site>=0:return
 if not hero_playing or death_active or active_rift>=0 or active_world_event>=0:return
 if not _v341_roster_valid(site):
  v341_pressure[site]=0
  _show_message("Lunar invasion postponed: squad unavailable.",2.0)
  return
 v341_active_site=site
 v341_wave=1
 _v341_activate_wave()
 _show_message("THE SKY HAS FRACTURED! "+V341_RULES.event_name(site)+" invasion. Defeat its commander to seal the portal.",4.5)

func _v341_activate_wave()->void:
 if v341_active_site<0:return
 if not _v341_roster_valid(v341_active_site):
  _v341_reset()
  return
 var site:Dictionary=v341_sites[v341_active_site]
 var ids:Array=site["roster"]
 var count:int=V341_RULES.wave_size(v341_wave)
 for slot in range(ids.size()):
  var beast:Dictionary=beasts[int(ids[slot])]
  if (v341_wave<3 and slot>=count) or (v341_wave==3 and not bool(beast.get("invasion_commander",false))):
   _v341_dormant(beast)
   continue
  if v341_wave<3 and bool(beast.get("invasion_commander",false)):
   _v341_dormant(beast)
   continue
  _reset_native_beast(beast)
  var commander:bool=bool(beast.get("invasion_commander",false))
  var strength:float=V341_RULES.wave_health(v341_wave,commander)
  beast["max"]=float(beast["base_hp"])*strength
  beast["hp"]=float(beast["max"])
  beast["alert_until"]=map_time+60.0
  beast["respawn_at"]=-1.0
  if commander:
   var tag:Label3D=beast["nameplate"]
   tag.text=V341_RULES.commander_name(v341_active_site)
 _v341_refresh_portals()
 _show_message(V341_RULES.event_name(v341_active_site)+" • "+("COMMANDER" if v341_wave==3 else "WAVE "+str(v341_wave)+" / 2"),3.0)

func _v341_refresh_portals()->void:
 for site in range(v341_sites.size()):
  var entry:Dictionary=v341_sites[site]
  var portal:Node3D=entry["portal"]
  var visible_now:bool=hero_playing and site==v341_active_site
  if visible_now and is_instance_valid(player):
   visible_now=player.global_position.distance_squared_to(portal.global_position)<58.0*58.0
  portal.visible=visible_now
  if visible_now:
   var sign:Label3D=entry["sign"]
   sign.text=V341_RULES.event_name(site)+" • "+("COMMANDER" if v341_wave==3 else "WAVE "+str(v341_wave))

func _v341_advance_if_cleared()->void:
 if v341_active_site<0:return
 if not _v341_roster_valid(v341_active_site):
  _v341_reset()
  return
 var entry:Dictionary=v341_sites[v341_active_site]
 for id in entry["roster"]:
  if bool(beasts[int(id)].get("alive",false)):return
 if v341_wave<V341_RULES.WAVES:
  v341_wave+=1
  _v341_activate_wave()
  return
 # Completing the commander phase awards existing materials, never a currency.
 var zone:int=int(entry["zone"])
 var prior_site:int=v341_active_site
 v341_active_site=-1
 v341_wave=0
 v341_pressure[prior_site]=0
 v341_invasions_completed+=1
 v341_commanders_defeated+=1
 gold+=80+zone*20
 _gain_material("Moon Sigil",2)
 _gain_material("Dusk Cinder",1)
 _award_experience(zone,"elder",true)
 _v341_refresh_portals()
 _show_message("INVASION SEALED! +2 Moon Sigils, 1 Dusk Cinder, gold and XP.",4.0)
 suppress_autosave_message=true
 _save_game()
 suppress_autosave_message=false

func _v341_add_pressure(site:int)->void:
 if site<0 or site>=v341_sites.size() or v341_active_site>=0:return
 var next:int=V341_RULES.add_pressure(int(v341_pressure.get(site,0)),true)
 v341_pressure[site]=next
 if next>=V341_RULES.PRESSURE_TRIGGER and active_rift<0 and hero_playing and not death_active:
  _v341_start(site)
 elif next==V341_RULES.PRESSURE_TRIGGER-1:
  _show_message("LUNAR UNREST: dimensional energy is gathering nearby.",2.3)

func _defeat_beast(beast:Dictionary,pet_kill:bool)->void:
 var alive_before:bool=bool(beast.get("alive",false))
 var was_hostile:bool=bool(beast.get("hostile",false))
 var zone:int=int(beast.get("zone",-1))
 var invasion_site:int=int(beast.get("invasion_site",-1))
 var dungeon_site:int=int(beast.get("dungeon_site",-1))
 var is_boss:bool=bool(beast.get("boss",false))
 super._defeat_beast(beast,pet_kill)
 if not alive_before:return
 if invasion_site>=0:
  # Inactive invasion creatures must never respawn through ecology.
  beast["respawn_at"]=-1.0
  if invasion_site==v341_active_site:_v341_advance_if_cleared()
 elif was_hostile and not is_boss and dungeon_site==-1 and not pet_kill and v341_active_site<0 and region_index==zone:
  var site:int=V341_RULES.REGIONS.find(zone)
  if site>=0:_v341_add_pressure(site)

func _v341_reset()->void:
 for entry in v341_sites:
  for id in entry["roster"]:
   var slot:int=int(id)
   if slot<0 or slot>=beasts.size():continue
   var creature:Dictionary=beasts[slot]
   if is_instance_valid(creature.get("body")):_v341_dormant(creature)
 v341_active_site=-1
 v341_wave=0
 v341_pressure.clear()
 for site in range(v341_sites.size()):v341_pressure[site]=0
 _v341_refresh_portals()

func _start_game(selected:String)->void:
 super._start_game(selected)
 _v341_reset()

func _restore_data(data:Dictionary)->void:
 super._restore_data(data)
 # An interrupted invasion is transient; permanent player progress persists.
 _v341_reset()

func _respawn_player()->void:
 var was_dying:bool=hp<=0.0
 super._respawn_player()
 # Phoenix recovery is not death. Only reset invasions on actual death.
 if death_active and was_dying:_v341_reset()

func _death_return_to_sanctuary()->void:
 super._death_return_to_sanctuary()
 _v341_reset()

func _death_return_to_title()->void:
 super._death_return_to_title()
 _v341_reset()

func _process(delta:float)->void:
 super._process(delta)
 v341_marker_clock+=clampf(delta,0.0,0.2)
 if v341_marker_clock>=0.4:
  v341_marker_clock=0.0
  _v341_refresh_portals()

func _menu_tab(which:String)->void:
 super._menu_tab(which)
 if which=="World" and is_instance_valid(menu_body):
  var status:String="No invasion active"
  if v341_active_site>=0:status=V341_RULES.event_name(v341_active_site)+" • "+("Commander" if v341_wave==3 else "Wave "+str(v341_wave))
  menu_body.text+="\n\n[b]LUNAR INVASIONS[/b]\n"+status+"\nDefeat hostile creatures in Witchwood, Highcrest, or Eclipse Citadel to stir the rift. Survive two waves and defeat a commander to earn Moon Sigils, Dusk Cinder and gold. No survival timer or building."

func _perf_report()->Dictionary:
 var stats:Dictionary=super._perf_report()
 stats["version"]="v34.1"
 stats["invasion_active_site"]=v341_active_site
 stats["invasion_stage"]=v341_wave
 stats["invasions_completed"]=v341_invasions_completed
 stats["invasion_preallocated_actors"]=v341_sites.size()*(V341_RULES.SOLDIER_SLOTS+1)
 return stats
