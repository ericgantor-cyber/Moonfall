extends "res://scripts/native_v31_2.gd"
## v31.3: combat chains, tactical enemy behavior, connected rift traps,
## elite rewards, timed world surge, bounded FX, and richer F9 profiling.
## Existing six-region world, schema-2 checkpoints, Z and Nyra bytes untouched.
const V313_RULES:=preload("res://scripts/systems/gameplay_v313.gd")
const V313_TRAP_RADIUS_SQ:float=2.1*2.1
var v313_attack_active:bool=false
var v313_combo:int=0
var v313_combo_expires:float=-1.0
var v313_traps:Array[Dictionary]=[]
var v313_trap_clock:float=0.0
var v313_surge_clock:float=0.0
var v313_surge_zone:int=-1
var v313_surge_bonus:int=0
var v313_elite_bonus:int=0
var v313_tactical_retreats:int=0
var v313_trap_hits:int=0
var v313_update_usec:int=0
var v313_update_peak:int=0
var v313_sample_count:int=0
var v313_sample_total:int=0
var v313_profile_frame:int=0

func _release_label()->String:return "v31.3"

func _build_scene()->void:
 super._build_scene()
 _v313_create_rift_traps()

func _v313_create_rift_traps()->void:
 # Traps are a visual extension of the three existing connected-world rifts.
 # No new collision shapes or world instances are spawned during play.
 var shared_mesh:=CylinderMesh.new()
 shared_mesh.top_radius=1.45
 shared_mesh.bottom_radius=1.45
 shared_mesh.height=0.025
 shared_mesh.radial_segments=20
 var warning_mat:=StandardMaterial3D.new()
 warning_mat.albedo_color=Color(0.90,0.20,0.35,0.58)
 warning_mat.shading_mode=BaseMaterial3D.SHADING_MODE_UNSHADED
 warning_mat.transparency=BaseMaterial3D.TRANSPARENCY_ALPHA
 warning_mat.emission_enabled=true
 warning_mat.emission=Color(0.85,0.18,0.32)
 for site in range(rifts.size()):
  var anchor:Vector3=rifts[site]["anchor"]
  var rune:=MeshInstance3D.new()
  rune.name="V313_RiftHazard_%d"%site
  rune.mesh=shared_mesh
  rune.material_override=warning_mat
  rune.cast_shadow=GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
  rune.position=anchor+Vector3(3.0,0.07,2.0)
  rune.visible=false
  world.add_child(rune)
  v313_traps.append({"node":rune,"site":site,"warning":0.0,"cooldown":0.0,"armed":false})

func _attack()->void:
 # Every valid attack earns a short timing window. The inherited attack
 # routine still owns range checks, equipment damage, and cooldowns.
 if not hero_playing or mounted or attack_cooldown>0.0:
  super._attack()
  return
 var in_reach:Dictionary=_nearest_beast(3.1,false,true)
 if in_reach.is_empty():
  v313_combo=0
  super._attack()
  return
 v313_combo=V313_RULES.next_combo(map_time,v313_combo_expires,v313_combo)
 v313_combo_expires=map_time+V313_RULES.COMBO_WINDOW
 v313_attack_active=true
 super._attack()
 v313_attack_active=false

func _apply_player_hit(target:Dictionary,base_damage:float,stagger_strength:float=0.0)->void:
 var bonus:float=V313_RULES.combo_multiplier(v313_combo) if v313_attack_active and map_time<=v313_combo_expires else 1.0
 super._apply_player_hit(target,base_damage*bonus,stagger_strength)

func _update_beast_step(beast:Dictionary,delta:float)->void:
 # Keep boss windup/telegraph simulation in the original AI: only change
 # the tactical pattern selection at a phase boundary, never the attack timing.
 if bool(beast.get("alive",false)):
  if bool(beast.get("boss",false)) and int(beast.get("boss_phase",1))>=2 and float(beast.get("windup",0.0))<=0.0:
   if float(beast.get("attack",0.0))<=delta:
    beast["boss_pattern"]=V313_RULES.choose_boss_pattern(int(beast.get("boss_phase",1)),hp,max_hp,int(beast.get("boss_pattern",0)))
  elif V313_RULES.hurt_retreat(float(beast.get("hp",0.0)),float(beast.get("max",1.0)),bool(beast.get("boss",false)),bool(beast.get("engaged",false)),map_time>=float(beast.get("retreat_cooldown",0.0))):
   beast["retreat_until"]=map_time+1.35
   beast["retreat_cooldown"]=map_time+13.0
   v313_tactical_retreats+=1
 super._update_beast_step(beast,delta)

func _v313_tick_traps(seconds:float)->void:
 for trap in v313_traps:
  var site:int=int(trap["site"])
  var active:bool=hero_playing and active_rift==site and bool(rifts[site].get("active",false))
  var rune:MeshInstance3D=trap["node"]
  if not active:
   rune.visible=false
   trap["warning"]=0.0
   trap["armed"]=false
   trap["cooldown"]=0.0
   continue
  trap["cooldown"]=maxf(0.0,float(trap["cooldown"])-seconds)
  if not bool(trap["armed"]) and float(trap["cooldown"])<=0.0:
   trap["warning"]=V313_RULES.TRAP_WARNING
   trap["armed"]=true
  if bool(trap["armed"]):
   rune.visible=true # warning is visible even at minimum graphics quality
   trap["warning"]=maxf(0.0,float(trap["warning"])-seconds)
   if float(trap["warning"])<=0.0:
    trap["armed"]=false
    trap["cooldown"]=V313_RULES.TRAP_COOLDOWN
    rune.visible=false
    if is_instance_valid(player) and player.global_position.distance_squared_to(rune.global_position)<=V313_TRAP_RADIUS_SQ and dodge_time<=0.0:
     v313_trap_hits+=1
     _apply_incoming_damage(9.0) # intentionally below ordinary enemy strike
  else:rune.visible=false

func _v313_tick_surge()->void:
 var active:bool=hero_playing and V313_RULES.surge_active(map_time)
 var zone:int=V313_RULES.surge_zone(map_time,REGIONS.size()) if active else -1
 if zone!=v313_surge_zone:
  v313_surge_zone=zone
  if zone>=0:_show_message("WORLD EVENT: Moon Surge in "+str(REGIONS[zone]["name"])+"! Elite loot and bonus coins.",4.0)

func _defeat_beast(beast:Dictionary,pet_kill:bool)->void:
 if not bool(beast.get("alive",false)):return
 var variant:String=str(beast.get("variant","normal"))
 var zone:int=int(beast.get("zone",0))
 var boss:bool=bool(beast.get("boss",false))
 var hostile:bool=bool(beast.get("hostile",false))
 var at:Vector3=(beast["body"] as CharacterBody3D).global_position
 super._defeat_beast(beast,pet_kill)
 var extra:String=V313_RULES.elite_material(zone,variant,boss)
 if extra!="" and hostile:
  _gain_material(extra,1)
  v313_elite_bonus+=1
 var bonus_created:bool=extra!="" and hostile
 if v313_surge_zone==zone and hostile and not boss:
  _create_coin_drop(at,250+zone*60)
  v313_surge_bonus+=1
  bonus_created=true
 if bonus_created and hero_playing:
  _save_game()
 # Additive drops are guarded by the base alive->dead transition.
 # No save-format change; inherited inventory and ground-drop codecs persist them.

func _process(delta:float)->void:
 super._process(delta)
 if not hero_playing or menu_open:return
 var started:int=Time.get_ticks_usec() if (v313_profile_frame%10)==0 else 0
 v313_profile_frame+=1
 v313_trap_clock+=minf(maxf(delta,0.0),0.2)
 if v313_trap_clock>=0.10:
  var step:float=v313_trap_clock
  v313_trap_clock=0.0
  _v313_tick_traps(step)
 v313_surge_clock+=delta
 if v313_surge_clock>=0.6:
  v313_surge_clock=0.0
  _v313_tick_surge()
 if v313_combo>0 and map_time>v313_combo_expires:v313_combo=0
 if started>0:
  v313_update_usec=Time.get_ticks_usec()-started
  v313_sample_total+=v313_update_usec
  v313_sample_count+=1
  v313_update_peak=maxi(v313_update_peak,v313_update_usec)

func _perf_report()->Dictionary:
 var report:Dictionary=super._perf_report()
 report["version"]="v31.3"
 report["combo_chain"]=v313_combo
 report["rift_hazards"]=v313_traps.size()
 report["surge_bonus_kills"]=v313_surge_bonus
 report["elite_bonus_kills"]=v313_elite_bonus
 report["tactical_retreats"]=v313_tactical_retreats
 report["v313_average_update_usec"]=float(v313_sample_total)/maxi(1,v313_sample_count)
 report["v313_peak_update_usec"]=v313_update_peak
 report["render_scale"]=get_viewport().scaling_3d_scale
 report["graphics_preset"]=str(gfx_settings.get("quality_preset","Custom"))
 return report

func _reset_new_adventure()->void:
 super._reset_new_adventure()
 v313_combo=0
 v313_attack_active=false
 v313_combo_expires=-1.0
 v313_trap_hits=0
 v313_surge_zone=-1
 v313_surge_bonus=0
 v313_elite_bonus=0
 v313_tactical_retreats=0
 for trap in v313_traps:
  (trap["node"] as MeshInstance3D).visible=false
  trap["warning"]=0.0
  trap["cooldown"]=0.0
  trap["armed"]=false
