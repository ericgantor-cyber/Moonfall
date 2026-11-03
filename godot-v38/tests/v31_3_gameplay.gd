extends SceneTree
const CURRENT_RELEASE = preload("res://tests/current_release.gd")
const RULES:=preload("res://scripts/systems/gameplay_v313.gd")
var failures:int=0
func check(ok:bool,label:String)->void:
 if ok:print("PASS v31.3: "+label)
 else:
  failures+=1
  push_error("FAIL v31.3: "+label)
func _initialize()->void:call_deferred("_run")
func _run()->void:
 check(RULES.next_combo(3.0,4.0,2)==3,"Timed 3-hit combo")
 check(RULES.next_combo(6.0,4.0,2)==1,"Combo expiration")
 check(is_equal_approx(RULES.combo_multiplier(3),1.16),"Capped combo damage")
 check(RULES.hurt_retreat(20,100,false,true,true),"Wounded creature retreats")
 check(not RULES.hurt_retreat(20,100,true,true,true),"Bosses do not retreat")
 check(RULES.choose_boss_pattern(3,100,100,0)==1,"Boss tactics rotate")
 check(RULES.surge_zone(350,6)==2,"Surge cycles regions")
 check(RULES.surge_active(165),"Surge begins at interval")
 check(not RULES.surge_active(195),"Surge expires safely")
 check(RULES.elite_material(2,"moon",false)=="Moon Sigil","Elite rewards")
 var game:Node3D=load("res://scenes/main.tscn").instantiate()
 root.add_child(game)
 await process_frame
 check(CURRENT_RELEASE.matches_project(game),"v31.3 scene active")
 check((game.get("v313_traps") as Array).size()==30,"One reusable hazard per dungeon across 30 rifts")
 check((game.get("rifts") as Array).size()==30,"All 30 connected rifts retained")
 check((game.get("treasure_chests") as Array).size()==6,"Six treasure caches intact")
 check(game.get("v312_foliage").size()==6,"Existing graphics effects intact")
 game.call("_start_game","z")
 await process_frame
 check(game.get("hero_playing"),"Hero starts normally")
 var traps:Array=game.get("v313_traps")
 var active:Array=game.get("rifts")
 (active[0] as Dictionary)["active"]=true
 game.set("active_rift",0)
 game.call("_v313_tick_traps",0.2)
 check((traps[0]["node"] as MeshInstance3D).visible,"Warning visible before hazard")
 var stable_warning:float=float(traps[0]["warning"])
 for broken_seconds in [NAN,INF,-INF,-1.0,0.0]:
  game.call("_v313_tick_traps",broken_seconds)
 check(is_equal_approx(float(traps[0]["warning"]),stable_warning),"Invalid hazard clocks cannot advance trap warnings")
 var dormant_trap:Dictionary=traps[1]
 var old_site:int=int(dormant_trap["site"])
 dormant_trap["site"]=-1
 game.call("_v313_tick_traps",0.1)
 dormant_trap["site"]=old_site
 check(not (dormant_trap["node"] as MeshInstance3D).visible,"Invalid trap site is ignored safely")

 game.call("_v313_tick_traps",1.05)
 check(not (traps[0]["node"] as MeshInstance3D).visible,"Previously shown warning resolves after its countdown")
 check(is_equal_approx(RULES.TRAP_COOLDOWN,20.0),"Rift hazard cooldown is twenty seconds")
 check(is_equal_approx(float(traps[0]["cooldown"]),20.0),"Hazard starts a full twenty-second cooldown after strike")
 game.call("_v313_tick_traps",19.0)
 check(not bool(traps[0]["armed"]) and is_equal_approx(float(traps[0]["cooldown"]),1.0),"Trap stays disarmed until its twenty-second cooldown expires")
 game.call("_v313_tick_traps",0.9)
 check(not bool(traps[0]["armed"]) and not (traps[0]["node"] as MeshInstance3D).visible,"Trap does not rearm before twenty seconds")
 game.call("_v313_tick_traps",0.1)
 check(bool(traps[0]["armed"]) and (traps[0]["node"] as MeshInstance3D).visible,"Trap rearms after the full twenty-second cooldown")
 # Leaving a rift must not wipe an unfinished cooldown and allow rapid rearming.
 game.call("_v313_tick_traps",1.2)
 check(is_equal_approx(float(traps[0]["cooldown"]),20.0),"Rearmed trap begins a fresh twenty-second cooldown after firing")
 game.set("active_rift",-1)
 (active[0] as Dictionary)["active"]=false
 game.call("_v313_tick_traps",0.2)
 check(is_equal_approx(float(traps[0]["cooldown"]),19.8) and not bool(traps[0]["armed"]),"Leaving a rift preserves the remaining trap cooldown")
 game.set("active_rift",0)
 (active[0] as Dictionary)["active"]=true
 game.call("_v313_tick_traps",0.2)
 check(is_equal_approx(float(traps[0]["cooldown"]),19.6) and not bool(traps[0]["armed"]) and not (traps[0]["node"] as MeshInstance3D).visible,"Reopening a rift cannot bypass the trap cooldown")
 # Interrupting an armed warning must start a fresh twenty-second recovery.
 traps[0]["cooldown"]=0.0
 traps[0]["warning"]=RULES.TRAP_WARNING
 traps[0]["armed"]=true
 (traps[0]["node"] as MeshInstance3D).visible=true
 var hits_before_cancel:int=int(game.get("v313_trap_hits"))
 game.set("active_rift",-1)
 (active[0] as Dictionary)["active"]=false
 game.call("_v313_tick_traps",0.2)
 check(not bool(traps[0]["armed"]) and not (traps[0]["node"] as MeshInstance3D).visible and is_equal_approx(float(traps[0]["cooldown"]),20.0),"Interrupting armed rift warning starts twenty-second cooldown")
 game.set("active_rift",0)
 (active[0] as Dictionary)["active"]=true
 game.call("_v313_tick_traps",0.2)
 check(not bool(traps[0]["armed"]) and is_equal_approx(float(traps[0]["cooldown"]),19.8),"Reentering after warning cancellation cannot instantly rearm")
 check(int(game.get("v313_trap_hits"))==hits_before_cancel,"Cancelled trap warning causes no damage")

 # Even a very long frame must show the full warning before any damage.
 traps[0]["cooldown"]=0.0
 traps[0]["armed"]=false
 var hits_before_long_frame:int=int(game.get("v313_trap_hits"))
 game.call("_v313_tick_traps",5.0)
 check(bool(traps[0]["armed"]) and (traps[0]["node"] as MeshInstance3D).visible and is_equal_approx(float(traps[0]["warning"]),RULES.TRAP_WARNING),"Long frame arms visible trap with full warning time")
 check(int(game.get("v313_trap_hits"))==hits_before_long_frame,"Long frame cannot deal an instant untelegraphed trap hit")
 game.call("_v313_tick_traps",RULES.TRAP_WARNING)
 check(not bool(traps[0]["armed"]) and is_equal_approx(float(traps[0]["cooldown"]),RULES.TRAP_COOLDOWN),"Trap resolves only on a later warning countdown")

 # A corrupted active trap must disarm without striking or immediately restarting.
 var saved_hits:int=int(game.get("v313_trap_hits"))
 for broken_clock in [NAN,INF,-INF,-1.0]:
  traps[0]["cooldown"]=broken_clock
  traps[0]["armed"]=false
  game.call("_v313_tick_traps",0.1)
  check(not bool(traps[0]["armed"]) and not (traps[0]["node"] as MeshInstance3D).visible and is_equal_approx(float(traps[0]["cooldown"]),RULES.TRAP_COOLDOWN),"Invalid trap cooldown resets safely "+str(broken_clock))
 for broken_clock in [NAN,INF,-INF,-1.0]:
  traps[0]["cooldown"]=0.0
  traps[0]["warning"]=broken_clock
  traps[0]["armed"]=true
  game.call("_v313_tick_traps",0.1)
  check(not bool(traps[0]["armed"]) and not (traps[0]["node"] as MeshInstance3D).visible and is_equal_approx(float(traps[0]["cooldown"]),RULES.TRAP_COOLDOWN),"Invalid armed trap warning disarms safely "+str(broken_clock))
 check(int(game.get("v313_trap_hits"))==saved_hits,"Invalid trap clocks cannot inflict damage")

 game.set("active_rift",-1)
 (active[0] as Dictionary)["active"]=false
 game.call("_v313_tick_traps",0.2)
 check(not (traps[0]["node"] as MeshInstance3D).visible,"Rift closing disables hazards")
 var expired_rune:MeshInstance3D=traps[1]["node"] as MeshInstance3D
 expired_rune.queue_free()
 await process_frame
 game.call("_v313_tick_traps",0.2)
 check(not is_instance_valid(traps[1]["node"]),"Freed trap visual is skipped without interrupting other hazards")

 game.set("map_time",166.0)
 game.call("_v313_tick_surge")
 check(int(game.get("v313_surge_zone"))>=0,"Moon surge becomes active")
 var before:Dictionary=game.call("_perf_report")
 check(str(before.get("version","")).begins_with("v3"),"F9 report identifies current v3x release")
 check(before.has("v313_average_update_usec"),"F9 includes additional profiler fields")
 game.call("_reset_new_adventure")
 check(int(game.get("v313_combo"))==0,"New game resets combo")
 check(int(game.get("v313_surge_zone"))==-1,"New game resets world event")
 game.queue_free()
 print("MOONFALL v31.3 GAMEPLAY PASS" if failures==0 else "MOONFALL v31.3 GAMEPLAY FAIL")
 quit(0 if failures==0 else 1)
